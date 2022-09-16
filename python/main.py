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
import signal
from typing import Any

quart_app = Quart(__name__)
quart_app.secret_key = 'superdupersecret'
AuthManager(quart_app)
sio = socketio.AsyncServer(async_mode='asgi', cors_allowed_origins='*')
app = socketio.ASGIApp(sio, quart_app)
gm = GameManager(sio)
usm = UnixSocketManager(sio, gm)

shutdown_event = asyncio.Event()


def _signal_handler(*_: Any) -> None:
    shutdown_event.set()

async def tick():
    while True:
        data = {
            'dt': str(datetime.datetime.now())[:-7]
        }

        if gm.current_width_game is not None:
            await gm.current_width_game.handle_tick()
            data['width_time_left'] = gm.current_width_game.time_left

        if gm.current_market_game is not None:
            await gm.current_market_game.handle_tick()
            data['market_time_left'] = gm.current_market_game.time_left

        # instead, send book when we get MARKET_CREATED message from game server
        # if gm.current_market_game is not None:
        #     if not gm.current_market_game.unsent:
        #         await sio.emit("book", gm.get_book_json(), broadcast=True)

        await sio.emit('tick', json.dumps(data), broadcast=True)
        await asyncio.sleep(1)


@quart_app.before_serving
async def initialize():
    loop = asyncio.get_event_loop()
    loop.create_task(usm.start())
    loop.create_task(usm.process())
    loop.create_task(usm.consume())
    loop.create_task(tick())
    loop.add_signal_handler(signal.SIGTERM, _signal_handler)
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
    return await render_template('market.html', user=current_user.auth_id)


@quart_app.route('/open')
@login_required
async def open_route():
    return await render_template('open.html', user=current_user.auth_id)


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


@quart_app.route('/test_market')
@login_required
async def test_market():

    if gm.current_market_game is None:
        gm.initialize_market_game(current_user.auth_id, 100, 101)
        await usm.create_market(0, 2, 627, ["conor", "ronoc"])

    for player in gm.players:
        await sio.emit("book", gm.get_book_json(player), broadcast=True)
    
    return await render_template('market.html', user=current_user.auth_id)


@sio.on('connect')
async def connect(sid, environ):
    pass


@sio.event
async def handle_width_update(sid, msg):
    data = json.loads(msg)
    if 'user' not in data or data['user'] not in gm.players:
        raise Exception(f"user not in {data=} or user not in {gm.players=}")
    

    if gm.current_width_game is not None:
        if data['width'] != '':
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
        'initial_mm': data['user']
    }
    gm.initialize_market_game(data['user'], data['bid'], data['ask'])
    await usm.create_market(0, 2, 627, gm.players)
    # redirect to trading open (all but MM)
    await sio.emit("advance_to_trading_open", json.dumps(payload), broadcast=True)
    # wait a second for folks to get redirected
    await asyncio.sleep(1)
    # broadcast the initial book and prices
    await sio.emit("initial_prices", json.dumps(payload), broadcast=True)
    for player in gm.players:
        await sio.emit("book", gm.get_book_json(player), broadcast=True)


@sio.event
async def handle_open_side(sid, msg):
    print(f"handle_open_side for {msg=} from {sid=}", flush=True)
    data = json.loads(msg)

    # send both MM and this crossing order to matcher, to simulate market-at-open orders


    
    await sio.emit("advance_to_market", json.dumps({}))
    await asyncio.sleep(1)
    await usm.handle_open_side(data)
    # broadcast the initial book
    for player in gm.players:
        await sio.emit("book", gm.get_book_json(player), broadcast=True)


@sio.event
async def handle_click(sid, msg):
    data = json.loads(msg)
    print(f"handle_click: {sid=}, {data=}", flush=True)
    await usm.insert_order(data)


@sio.event
async def handle_right_click(sid, msg):
    data = json.loads(msg)
    print(f"handle_right_click: {sid=}, {data=}", flush=True)
    await usm.cancel_order(data)
