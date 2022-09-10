from quart import Quart, render_template, url_for
from socket_manager import UnixSocketManager
import socketio
import asyncio
import datetime
import json


quart_app = Quart(__name__)
sio = socketio.AsyncServer(async_mode='asgi', cors_allowed_origins='*')
app = socketio.ASGIApp(sio, quart_app)
usm = UnixSocketManager(sio)

async def tick():
    while True:
        data = {
            'dt': str(datetime.datetime.now())[:-7]
        }
        await sio.emit('tick', json.dumps(data), broadcast=True)
        await asyncio.sleep(1)

@quart_app.before_serving
async def initialize():
    loop = asyncio.get_event_loop()
    loop.create_task(tick())


@quart_app.route('/')
async def index():
    return await render_template('index.html')

@quart_app.route('/width')
async def width():
    # here is where the opening width is established, and initial bid/ask is established
    return await render_template('width.html')

@quart_app.route('/market')
async def market():
    return await render_template('market.html')

@sio.on('connect')
async def connect(sid, environ):
    print(f"@sio.on(connect): {sid=}, {environ=}")
    # add to the list of current participants
