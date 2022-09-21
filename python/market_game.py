from plistlib import InvalidFileException
from xml.dom import InvalidModificationErr
from socketio import AsyncServer
from enum import Enum
from typing import List, Dict
import asyncio
import json

class MarketGameState(Enum):
    OPEN = 1
    ACTIVE = 2
    OVER = 3

class MarketGame:
    def __init__(self, sio: AsyncServer, players: List[str], book: Dict, open_player, open_bid, open_ask, settlement, precision=2):
        self.sio = sio
        self.players = players
        self.state = MarketGameState.OPEN
        self.book = book
        self.tick_size = 1 / (10**precision)
        self.columns = ['your bid qty', 'market bid qty', 'price', 'market ask qty', 'your ask qty']
        self.unsent = True
        self.precision = precision
        self.positions = {p: 0 for p in self.players}
        self.open_mm = open_player
        self.open_bid = int(float(open_bid) * 10**self.precision)
        self.open_ask = int(float(open_ask) * 10**self.precision)
        self.settlement = settlement
        self.time_left = 60
        self.seconds_per_update = 30
        self.trade_id = 0
        self.trades = []

    async def handle_tick(self):
        print(f"MarketGame.handle_tick", flush=True)
        if self.state != MarketGameState.OVER:
            if self.time_left == 1:
                self.state = MarketGameState.OVER
                await self.resolve()
            else:
                self.time_left -= 1
                print(f"{self.time_left=}", flush=True)
    
    async def resolve(self):
        payload = {}
        await self.sio.emit("end_market_game", json.dumps({}), broadcast=True)

        rows = [t['id'] for t in self.trades]
        columns = ['passive user', 'passive side', 'trade qty', 'trade px', 'trade pnl']
        grid = {}
        settle_px = self.settlement / (10**self.precision)
        for t in self.trades:
            if t['passive_side'] == "BUY":
                pnl = t['qty'] * (settle_px - t['price'])
            else:
                pnl = t['qty'] * (t['price'] - settle_px)

            grid[t['id']] = {
                'passive user': t['passive_player'],
                'passive side': t['passive_side'],
                'trade qty': t['qty'],
                'trade px': t['price'],
                'trade pnl': pnl
            }
        
        payload = {
            'rows': rows,
            'columns': columns,
            'grid': grid
        }

        await asyncio.sleep(1)

        await self.sio.emit('results', json.dumps(payload), broadcast=True)

    def get_levels_grid(self, player):
        # first populate rows based on min/max price in book
        min_bid = min(list(self.book['bids'].keys()))
        max_ask = max(list(self.book['asks'].keys()))
        n_levels = int((float(max_ask) - float(min_bid))/self.tick_size) + 1
        rows = [float(min_bid) + self.tick_size * i for i in range(n_levels)]
        rows = [f"{r:.2f}" for r in rows]
        grid = {}
        
        # populate grid based on bid/ask levels
        for price in rows:
            if price in self.book['bids']:
                player_qty = sum([x[1] for x in self.book['bids'][price] if x[0] == player])
                market_qty = sum([x[1] for x in self.book['bids'][price]])
                grid[price] = {
                    'your bid qty': player_qty,
                    'market bid qty': market_qty,
                    'price': price,
                    'market ask qty': 0,
                    'your ask qty': 0
                }
            elif price in self.book['asks']:
                player_qty = sum([x[1] for x in self.book['asks'][price] if x[0] == player])
                market_qty = sum(x[1] for x in self.book['asks'][price])
                grid[price] = {
                    'your bid qty': 0,
                    'market bid qty': 0,
                    'price': price,
                    'market ask qty': market_qty,
                    'your ask qty': player_qty
                }
            else:
                grid[price] = {
                    'your bid qty': 0,
                    'market bid qty': 0,
                    'price': price,
                    'market ask qty': 0,
                    'your ask qty': 0
                }

        return rows, self.columns, grid

    def get_top_data(self):
        bid_grid = {}
        ask_grid = {}
        bid_prices = sorted(self.book['bids'].keys(), key=lambda p: float(p)) [::-1]
        bid_rows = []
        local_oid = 0
        for bid in bid_prices:
            orders = self.book['bids'][bid]
            for player, qty in orders:
                bid_grid[local_oid] = {
                    'player': player,
                    'qty': qty,
                    'px': bid
                }
                bid_rows.append(local_oid)
                local_oid += 1
                
        ask_prices = sorted(self.book['asks'].keys(), key = lambda p: float(p))
        ask_rows = []
        for ask in ask_prices:
            orders = self.book['asks'][ask]
            for player, qty in orders:
                ask_grid[local_oid] = {
                    'player': player,
                    'qty': qty,
                    'px': ask
                }
                ask_rows.append(local_oid)
                local_oid += 1
        
        bid_columns = ['player', 'qty', 'px']
        ask_columns = ['px', 'qty', 'player']
        return bid_rows, bid_columns, bid_grid, ask_rows, ask_columns, ask_grid

    def add_order(self, player, price, qty, side):
        print(f"adding order: {player=}, {price=}, {qty=}, {side=}", flush=True)
        px_str = f'{(price/(10**self.precision)):.2f}'
        if side == 0:
            if qty == 0:
                # cancel all of the orders for this guy
                indices_to_remove = []
                for i, x in enumerate(self.book['bids'][px_str]):
                    if x[0] == player:
                        indices_to_remove.append(i)

                self.book['bids'][px_str] = [
                    self.book.bids[px_str][j]
                    for j in range(len(self.book['bids'][px_str]))
                    if j not in indices_to_remove
                ]
            
            else:
                if px_str not in self.book['bids']:
                    self.book['bids'][px_str] = []
                self.book['bids'][px_str].append([player, qty])
        else:
            if qty == 0:
                indices_to_remove = []
                for i, x in enumerate(self.book['asks'][px_str]):
                    if x[0] == player:
                        indices_to_remove.append(i)
                
                self.book['asks'][px_str] = [
                    self.book['asks'][px_str][j]
                    for j in range(len(self.book['asks'][px_str]))
                    if j not in indices_to_remove
                ]

            else:
                if px_str not in self.book['asks']:
                    self.book['asks'][px_str] = []
                self.book['asks'][px_str].append([player, qty])

        print(f"after adding order, {self.book=}", flush=True)

    def process_trade(self, passive_player, price, qty, passive_side):
        px_str = f'{(price/(10**self.precision)):.2f}'
        if passive_side == 0:
            qty_left = qty
            indices_to_remove = []
            for i, x in enumerate(self.book['bids'][px_str]):
                if qty_left > 0:
                    u, q = x[0], x[1]
                    if u == passive_player:
                        if q > 0:
                            if qty_left < q:
                                x[1] -= qty_left
                            else:
                                indices_to_remove.append(i)
                            qty_left -= q
            self.book['bids'][px_str] = [
                self.book['bids'][px_str][j] 
                for j in range(len(self.book['bids'][px_str])) 
                if j not in indices_to_remove
            ]

            self.trades.append({
                "passive_player": passive_player,
                "passive_side": "BUY",
                "qty": qty,
                "price": price/(10**self.precision),
                "id": self.trade_id
            })

        else:
            qty_left = qty
            indices_to_remove = []
            for i, x in enumerate(self.book['asks'][px_str]):
                if qty_left > 0:
                    u, q = x[0], x[1]
                    if u == passive_player:
                        if q > 0:
                            if qty_left < q:
                                x[1] -= qty_left
                            else:
                                indices_to_remove.append(i)
                            qty_left -= q
            self.book['asks'][px_str] = [
                self.book['asks'][px_str][j] 
                for j in range(len(self.book['asks'][px_str])) 
                if j not in indices_to_remove
            ]

            self.trades.append({
                "passive_player": passive_player,
                "passive_side": "SELL",
                "qty": qty,
                "price": price/(10**self.precision),
                "id": self.trade_id
            })

        self.trade_id += 1
        self.time_left = max(self.time_left, self.seconds_per_update)
