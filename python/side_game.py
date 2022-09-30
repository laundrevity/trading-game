from typing import List
from socketio import AsyncServer
from enum import Enum
import random
import json

class SideGameState(Enum):
    IN_PROGRESS = 1
    OVER = 2

class SideGame:
    def __init__(
        self, 
        sio: AsyncServer, 
        players: List[str],
        mm: str,                # initial market maker, an element of players
        bid: float,        # initial mm bid
        ask: float         # initial mm ask
    ):
        self.sio = sio
        self.players = players
        self.state = SideGameState.IN_PROGRESS
        self.time_left = 60
        self.mm = mm
        self.bid = bid
        self.ask = ask
        self.buyers = []
        self.sellers = []

    
    async def handle_tick(self):
        if self.state != SideGameState.OVER:
            if self.time_left == 1:
                await self.resolve()
            else:
                self.time_left -= 1
    
    async def resolve(self):
        self.state = SideGameState.OVER
        undecideds = [
            p for p in self.players 
            if p not in self.buyers 
            and p not in self.sellers
            and p != self.mm  
        ]
        for undecided in undecideds:
            x = random.random()
            if x < 0.5:
                self.buyers.append(undecided)
            else:
                self.sellers.append(undecided)
        
        # now send the list of buyers/sells to the GameManager via sio
        payload = {
            'buyers': self.buyers,
            'sellers': self.sellers
        }
        print(f"emitting side results: {payload}", flush=True)
        await self.sio.emit('side_results', json.dumps(payload), broadcast=True)
    
    async def handle_update(self, data):
        if data['user'] not in self.players:
            print(f"got side from unknown user {data['user']}", flush=True)
            return
        if data['side'] == "BUY":
            self.buyers.append(data['user'])
        else:
            self.sellers.append(data['user'])
        
        # end the game if everyone has decided
        if len(self.buyers) + len(self.sellers) == (len(self.players) - 1):
            await self.resolve()
