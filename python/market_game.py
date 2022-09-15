from socketio import AsyncServer
from enum import Enum
from typing import List, Dict

class MarketGameState(Enum):
    OPEN = 1
    ACTIVE = 2
    OVER = 3

class MarketGame:
    def __init__(self, sio: AsyncServer, players: List[str], book: Dict):
        self.sio = sio
        self.players = players
        self.state = MarketGameState.OPEN
        self.book = book
        self.tick_size = 0.1
        self.columns = ['your bid qty', 'market bid qty', 'price', 'market ask qty', 'your ask qty']
        self.unsent = True

    def get_levels_grid(self):
        # first populate rows based on min/max price in book
        min_bid = min(list(self.book['bids'].keys()))
        max_ask = max(list(self.book['asks'].keys()))
        n_levels = int((float(max_ask) - float(min_bid))/self.tick_size) + 1
        rows = [float(min_bid) + self.tick_size * i for i in range(n_levels)]
        rows = [f"{r:.1f}" for r in rows]
        grid = {}
        
        # populate grid based on bid/ask levels
        for price in rows:
            if price in self.book['bids']:
                grid[price] = {
                    'your bid qty': 0,
                    'market bid qty': self.book['bids'][price],
                    'price': price,
                    'market ask qty': 0,
                    'your ask qty': 0
                }
            elif price in self.book['asks']:
                grid[price] = {
                    'your bid qty': 0,
                    'market bid qty': 0,
                    'price': price,
                    'market ask qty': self.book['asks'][price],
                    'your ask qty': 0
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