import socketio
import asyncio

class UnixSocketManager:
    def __init__(self, sio: socketio.AsyncServer, path='/app/sock'):
        self.sio = sio
        self.connected = False
        self.path = path
        self.reader = None
        self.writer = None
        self.bytes_sent = 0
        self.connected = False

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
