# Mango Chess 🥭♟️

Stockfish and Bluetooth communications over a RISC-V [MangoPi](https://mangopi.org/mqpro) (a "tiny, tiny computer").

## Usage

Buzzes the next move to you from Stockfish with servo + gets opponent moves from rotary encoder. Also live streams moves and chess board state to a GUI.

How to use:
- `make run` runs a program to send raw AT commands to the Bluetooth HC-05 module (this is mostly for testing).
- `make brain` runs the code that should run on the "Brain" Mango Pi, connected to the host laptop which itself runs Stockfishs. You must separately run `python engine.py`, (having previously installed all requirements in `requirements.txt`).
- `make hand` runs the program that should run on the "Hand" Mango Pi, which the player would secretly have in their pocket.

**Please read our code because we spent a lot of time making it well documented, specially `jnxu.c`, `jnxu.h`, `bt_ext.c`, and `bt_ext.h`!**

## References

We used the following hardware:
 - 2 Mango Pi's
 - 2 HC-05 Bluetooth modules
 - 1 laptop (for Stockfish)
 - 1 screen to visualize the chessboard
 - 1 servo (for buzzing the instructions to the cheater)
 - 1 rotary encoder (for the cheater to encode the state of the game)

Code sources:
 - Our Python code `engine.py` interfaces with the Stockfish engine, which is open source
 - We adapted Julie's ringbuffer to be able to store pointers (see `ringbuffer_ptr.c`)
 - When writing the `bt_ext.c` module, we copied some code from Julie's `uart.c` module.

## Demo

Two Mango Pis connected via Bluetooth. "Hand" Pi interfaces with rotary encoder + servo
<img src="https://github.com/ellenjxu/mango-chess/blob/master/photos/hardware.jpg?raw=true" width="500" />

https://github.com/ellenjxu/mango-chess/assets/56745453/db248e11-a9dd-4169-abea-85a70d04ebb2


---

Created for Stanford [CS 107E](http://cs107e.github.io/) (bare metal programming) final project!

