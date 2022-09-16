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
        self.gm = gm
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
        self.instrument = None
        self.precision = None

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
                        # let gm know that it was created successfully
                        if self.gm.current_market_game is not None:
                            # wait a second to allow the page to load in the browser
                            await asyncio.sleep(1)
                            print(f"emitting book", flush=True)
                            await self.sio.emit("book", self.gm.get_book_json(), broadcast=True)
                        else:
                            print(f"not emitting book cuz self.gm.current_market_game is None", flush=True)

                    case "TRADE":
                        trade = pb_msg.trade
                        instrument = trade.instrument
                        iid = instrument.id
                        precision = instrument.precision
                        print(f"TRADE: {iid=}, {precision=}, {trade.volume=}, {trade.price=}, {trade.buyer_account=}, {trade.seller_account=}")

                    case "LEVEL_UPDATE":
                        update = pb_msg.level_update
                        instrument = update.instrument
                        iid = instrument.id
                        precision = instrument.precision
                        print(f"LEVEL_UPDATE: {iid=}, {precision=}, {update.account_name=}, {update.volume=}, {update.price=}, {update.side=}")
                        
                        print(f"adding order to book", flush=True)
                        self.gm.current_market_game.add_order(update.account_name, update.price, update.volume, update.side)
                        
                        print(f"iterating over {self.gm.players=}")
                        for player in self.gm.players:
                            j = self.gm.get_book_json(player)
                            print(f"emitting book with {j=}", flush=True)
                            await self.sio.emit("book", j, broadcast=True)

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

    async def create_market(self, instrument_id, precision, user_names):
        instrument = pb2.Instrument(id=instrument_id, precision=precision)
        create_msg = pb2.CreateMarket(instrument=instrument)
        for user_name in user_names:
            create_msg.user_name.append(user_name)
        message = pb2.Message(type=['CREATE_MARKET'], create_market=create_msg)
        await self.write_proto_message(message)
        self.instrument = instrument
        self.precision = precision

    async def insert_order(self, data):
        order_px_int = int(float(data['row']) * 10**(self.precision))

        if "your" in data['column']:
            if "bid" in data['column']:
                print(f"found your_bid_qty", flush=True)
                insert_msg = pb2.InsertOrder(
                    request_id=self.request_id,
                    instrument=self.instrument,
                    account_name=data['user'],
                    volume=1,
                    price=order_px_int,
                    side=0)
            else:
                print(f"did not find your_bid_qty", flush=True)
                insert_msg = pb2.InsertOrder(
                    request_id=self.request_id,
                    instrument=self.instrument,
                    account_name=data['user'],
                    volume=1,
                    price=order_px_int,
                    side=1)
            print(f"side={insert_msg.side}", flush=True)
            message = pb2.Message(type=['INSERT_ORDER'], insert_order=insert_msg)
            await self.write_proto_message(message)
    
    async def cancel_order(self, data):
        order_px_int = int(float(data['row']) * 10**(self.precision))
        if "your" in data['column']:
            if "bid" in data["column"]:
                cancel_msg = pb2.CancelOrder(
                    request_id=self.request_id,
                    instrument=self.instrument,
                    account_name=data['user'],
                    price=order_px_int,
                    side=0)
            else:
                cancel_msg = pb2.CancelOrder(
                    request_id=self.request_id,
                    instrument=self.instrument,
                    account_name=data['user'],
                    price=order_px_int,
                    side = 1)
            message = pb2.Message(type=['CANCEL_ORDER'], cancel_order=cancel_msg)
            await self.write_proto_message(message)
