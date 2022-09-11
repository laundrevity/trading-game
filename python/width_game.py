from typing import List
from enum import Enum
from socketio import AsyncServer
import json


class WidthGameState(Enum):
    INITIAL = 1
    IN_PROGRESS = 2
    OVER = 3


class WidthGame:
    def __init__(self, sio: AsyncServer, players: List[str]):
        self.sio = sio
        self.players = players
        self.state = WidthGameState.INITIAL
        self.best_width = None
        self.best_player = None
        self.time_left = 120
        self.seconds_per_update = 30
    
    async def update(self, width: float, player: str):
        print(f"WidthGame.update, {width=}, {player=}")
        if player not in self.players:
            raise Exception("player not in WidthGame.players")
        if self.best_width is None or width < self.best_width:
            print(f"updating best_width={width} from {player}", flush=True)
            self.best_width = width
            self.best_player = player
            payload = {
                "width": float(width),
                "player": player
            }
            await self.sio.emit("best_width_update", json.dumps(payload), broadcast=True)
            self.time_left = max(self.time_left, self.seconds_per_update)

    async def handle_tick(self):
        if self.time_left == 1:
            self.state = WidthGame.OVER
        else:
            self.time_left -= 1
        