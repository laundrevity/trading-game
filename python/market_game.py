from socketio import AsyncServer
from enum import Enum
from typing import List, Dict

class MarketGameState(Enum):
    OPEN = 1
    ACTIVE = 2
    OVER = 3

class MarketGame:
    def __init__(self, sio: AsyncServer, players: List[str], book: Dict, precision=2):
        self.sio = sio
        self.players = players
        self.state = MarketGameState.OPEN
        self.book = book
        self.tick_size = 1 / (10**precision)
        self.columns = ['your bid qty', 'market bid qty', 'price', 'market ask qty', 'your ask qty']
        self.unsent = True
        self.precision = precision

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

    def add_order(self, player, price, qty, side):
        if side == 0:
            px_str = f'{(price/(10**self.precision)):.2f}'
            if px_str not in self.book['bids']:
                self.book['bids'][px_str] = []
            self.book['bids'][px_str].append([player, qty])
        else:
            px_str = f'{(price/(10**self.precision)):.2f}'
            if px_str not in self.book['asks']:
                self.book['asks'][px_str] = []
            self.book['asks'][px_str].append([player, qty])

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
