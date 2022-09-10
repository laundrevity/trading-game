import socketio

class UnixSocketManager:
    def __init__(self, sio: socketio.AsyncServer, path='/app/sock'):
        self.sio = sio
        self.connected = False
        self.path = path
        self.reader = None
        self.writer = None
        self.bytes_sent = 0
