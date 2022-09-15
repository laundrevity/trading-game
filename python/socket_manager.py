from game_manager import GameManager
import socketio
import asyncio
import struct
import common_pb2 as pb2


class UnixSocketManager:
    def __init__(
        self, 
        sio: socketio.AsyncServer, 
        gm: GameManager,        # so we can pass events directly to gm
        path='/app/sock'
    ):
        self.sio = sio
        self.connected = False
        self.path = path
        self.reader = None
        self.writer = None
        self.bytes_sent = 0
        self.connected = False
        self.writer_lock = asyncio.Lock()
        self.raw_message_queue = asyncio.Queue()
        self.message_queue = asyncio.Queue()
        self.request_id = 0

    async def connect(self):
        if not self.connected:
            self.reader, self.writer = await asyncio.open_unix_connection(self.path)
            self.connected = True
            print(f"[UnixSocketManager] connected!", flush=True)

    async def listen(self):
        print(f"listen", flush=True)
        while True:
            if self.connected:
                try:
                    message = await self.reader.read(1024)
                    if self.reader.at_eof():
                        print(f"reader eof!")
                        exit()

                    if isinstance(message, bytes):
                        await self.raw_message_queue.put(message)
                    
                    # if message:
                    #     print(f"[UnixSocketManager] got message: {message}", flush=True)
                
                except ConnectionResetError:
                    print(f"[UnixSocketManager] ConnectionResetError!", flush=True)
                    exit()
            else:
                print(f"[UnixSocketManager] disconnected during listen, trying to reconnect", flush=True)
                await self.connect()        
    
    async def process(self):
        print(f"process", flush=True)
        incomplete_msg = bytes(b'')
        while True:
            msg = None
            try:
                msg = await self.raw_message_queue.get()
            except RuntimeError:
                print(f"runtime error in process!")
                exit()
            
            if msg:
                msg = incomplete_msg + msg
            incomplete_msg = bytes(b'')

            while msg:
                try:
                    header_0 = struct.unpack('<L', msg[:4])[0]
                    header_1 = struct.unpack('<L', msg[4:8])[0]
                    if len(msg[8:8+header_0]) < header_0:
                        incomplete_msg = msg[:8 + header_0]
                    elif header_1 == 0:
                        await self.message_queue.put(msg[8:8+header_0])
                except struct.error:
                    incomplete_msg = msg
                
                msg = msg[8+header_0:]

    async def consume(self):
        print(f"consume", flush=True)
        while True:
            try:
                msg = await self.message_queue.get()
            except RuntimeError:
                raise asyncio.exceptions.CancelledError
            
            pb_msg = pb2.Message()
            pb_msg.ParseFromString(msg)

            for msg_type in set(pb_msg.type):
                name = pb2._MESSAGETYPE.values_by_number[msg_type].name
                match name:
                    case "CREATE_MARKET_REPLY":
                        print(f"CREATE_MARKET_REPLY", flush=True)
                    case _:
                        print("UNRECOGNIZED MSG TYPE IN consume", flush=True)

    async def start(self):
        await self.connect()
        await self.listen()

    async def write_proto_message(self, proto):
        async with self.writer_lock:
            header = struct.pack('<L', proto.ByteSize()) + b'\x00'*4
            msg = header + proto.SerializeToString()
            self.writer.write(msg)
            await self.writer.drain()
            self.request_id += 1
