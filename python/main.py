from quart import Quart, render_template, url_for, request, redirect, jsonify
from game_manager import GameManager
from socket_manager import UnixSocketManager
from quart_auth import (
    AuthUser, AuthManager, current_user, login_required, login_user, logout_user
)
from utils import load_credentials
import socketio
import asyncio
import datetime
import json


quart_app = Quart(__name__)
quart_app.secret_key = 'superdupersecret'
AuthManager(quart_app)
sio = socketio.AsyncServer(async_mode='asgi', cors_allowed_origins='*')
app = socketio.ASGIApp(sio, quart_app)
usm = UnixSocketManager(sio)
gm = GameManager(sio)

async def tick():
    while True:
        data = {
            'dt': str(datetime.datetime.now())[:-7]
        }

        if gm.current_width_game is not None:
            await gm.current_width_game.handle_tick(gm.browser_ids)
            data['width_time_left'] = gm.current_width_game.time_left

        await sio.emit('tick', json.dumps(data), broadcast=True)
        await asyncio.sleep(1)


@quart_app.before_serving
async def initialize():
    loop = asyncio.get_event_loop()
    loop.create_task(tick())
    gm.credentials = load_credentials()
    print(f"loaded creds: {gm.credentials}")


@quart_app.route('/')
@login_required
async def index():
    return await render_template('index.html', user=current_user.auth_id)


@quart_app.route('/width')
@login_required
async def width():
    if gm.current_width_game is None:
        # start a new active width game (here can pass the value desc)
        gm.initialize_width_game()
    # here is where the opening width is established, and initial bid/ask is established
    return await render_template('width.html', user=current_user.auth_id)


@quart_app.route('/market')
@login_required
async def market():
    return await render_template('market.html')


@quart_app.route('/open')
@login_required
async def open_route():
    return await render_template('open.html')


@quart_app.route('/login')
async def login():
    return await render_template('login.html')


@quart_app.route('/handle_login', methods=['POST'])
async def handle_login():
    body = await request.get_json()
    creds = (body['username'], body['password'])
    if creds in gm.credentials:
        print(f"Login successful! for user={creds[0]}")
        login_user(AuthUser(creds[0]))
        if creds[0] not in gm.players:
            gm.register_player(creds[0])
        return jsonify({'success': 'TRUE'})
    else:
        print(f"Login failed with {creds=}")
        return jsonify({'success': 'FALSE'})


@sio.on('connect')
async def connect(sid, environ):
    pass


@sio.event
async def handle_width_update(sid, msg):
    data = json.loads(msg)
    if 'user' not in data or data['user'] not in gm.players:
        raise Exception(f"user not in {data=} or user not in {gm.players=}")
    

    if gm.current_width_game is not None:
        await gm.current_width_game.update(
            float(data['width']),
            data['user']
        )
    else:
        print(f"Not updating width because gm.current_width_game is none", flush=True)


@sio.event
async def handle_bid_submission(sid, msg):
    print(f"handle_bid_submission for {msg=} from {sid=}", flush=True)
    data = json.loads(msg)
    payload = {
        'initial_bid': data['bid'],
        'initial_ask': data['ask'],
        'initial_mm': gm.browser_ids[sid]
    }
    gm.initialize_market_game(sid, data['bid'], data['ask'])
    # redirect to trading open (all but MM)
    await sio.emit("advance_to_trading_open", json.dumps(payload), broadcast=True)
    # wait a second for folks to get redirected
    await asyncio.sleep(1)
    # broadcast the initial book and prices
    await sio.emit("initial_prices", json.dumps(payload), broadcast=True)
    await sio.emit("book", gm.get_book_json(), broadcast=True)


@sio.event
async def handle_open_side(sid, msg):
    print(f"handle_open_side for {msg=} from {sid=}", flush=True)
    data = json.loads(msg)
    await sio.emit("advance_to_market", json.dumps({}))
    await asyncio.sleep(1)
    # broadcast the initial book
    await sio.emit("book", gm.get_book_json(), broadcast=True)
