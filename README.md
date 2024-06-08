# Mango Chess 🥭♟️

Stockfish and Bluetooth protocol on RISC-V [MangoPi](https://mangopi.org/mqpro) bare-metal (a "tiny, tiny computer").

> **Stockfish** → **Servo** → **Next Move**
> 
> **Stockfish** ← **Rotary Encoder** ← **Opponent Move**

\+ live streams moves and chess board state to gui

## Usage

- `make run`: send raw AT commands to the Bluetooth HC-05 module (this is mostly for testing).
- `make brain`: executes code on "Brain" Mango Pi, whichs talks to host running Stockfish. You must separately run `python engine.py`, (having previously installed all requirements in `requirements.txt`).
- `make hand`: executes program on "Hand" Mango Pi, which the player would secretly have in their pocket.

**Please read our code because we spent a lot of time making it well documented, specially `jnxu.c`, `jnxu.h`, `bt_ext.c`, and `bt_ext.h`!**

## Hardware
 - 2 Mango Pi
 - 2 HC-05 Bluetooth modules
 - 1 rotary encoder
 - 1 servo
 - Laptop and HDMI monitor

<img src="https://github.com/ellenjxu/mango-chess/blob/master/photos/hardware.jpg?raw=true" width="500" />
Two Mango Pis connected via Bluetooth. "Hand" Pi interfaces with rotary encoder + servo

---

Created for Stanford [CS 107E](http://cs107e.github.io/)
