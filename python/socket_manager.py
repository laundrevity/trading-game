import socketio
import asyncio
import struct


class UnixSocketManager:
    def __init__(self, sio: socketio.AsyncServer, path='/app/sock'):
        self.sio = sio
        self.connected = False
        self.path = path
        self.reader = None
        self.writer = None
        self.bytes_sent = 0
        self.connected = False
        self.writer_lock = asyncio.Lock()
        self.request_id = 0

    async def connect(self):
        if not self.connected:
            self.reader, self.writer = await asyncio.open_unix_connection(self.path)
            self.connected = True
            print(f"[UnixSocketManager] connected!", flush=True)

    async def listen(self):
        while True:
            if self.connected:
                try:
                    message = await self.reader.read(1024)
                    if message:
                        print(f"[UnixSocketManager] got message: {message}", flush=True)
                except ConnectionResetError:
                    print(f"[UnixSocketManager] ConnectionResetError!", flush=True)
                    exit()
            else:
                print(f"[UnixSocketManager] disconnected during listen, trying to reconnect", flush=True)
                await self.connect()        
    
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
