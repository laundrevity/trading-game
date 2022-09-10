from width_game import WidthGame
from enum import Enum

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
    
    def register_player(self, player):
        if player not in self.players:
            self.players.append(player)
            print(f"registered {player=}", flush=True)
    
    def initialize_width_game(self):
        self.current_width_game = WidthGame(self.sio, self.players)
    