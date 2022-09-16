from market_game import MarketGame
from width_game import WidthGame
from enum import Enum
import json

class GameManagerState(Enum):
    INITIAL = 1
    WIDTH = 2
    TRADING_OPEN = 3
    TRADING_ACTIVE = 4
    TRADING_DONE = 5

class GameManager:
    def __init__(self, sio):
        self.sio = sio
        self.players = []
        self.current_width_game = None
        self.current_market_game = None
        self.browser_ids = {}
        self.credentials = []
    
    def register_player(self, player):
        if player not in self.players:
            self.players.append(player)
            print(f"registered {player=}", flush=True)
    
    def initialize_width_game(self):
        self.current_width_game = WidthGame(self.sio, self.players)
    
    def initialize_market_game(self, open_player, open_bid, open_ask):
        self.current_width_game = None
        initial_qty = len(self.players) - 1
        open_book = {
            'bids': {f'{open_bid:.2f}': [(open_player, 0)]}, 
            'asks': {f'{open_ask:.2f}': [(open_player, 0)]}
        }
        print(f"initializing market game with book={open_book}", flush=True)
        self.current_market_game = MarketGame(
            self.sio, 
            self.players, 
            open_book, 
            open_player,
            open_bid,
            open_ask)

    def get_book_json(self, player="conor"):
        if self.current_market_game is not None:
            rows, columns, grid = self.current_market_game.get_levels_grid(player)
            rows = sorted(rows)[::-1]
            payload = {
                'rows': rows,
                'columns': columns,
                'grid': grid,
                'player': player
            }
            return json.dumps(payload)
