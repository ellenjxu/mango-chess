## Project title

Mango Chess

## Team members

Javier Garcia Nieto and Ellen Xu

## Project description

Using a Mango Pi and Bluetooth communication to enable Stockfish chess cheating!

One player has a "Hand" Mango Pi in their pocket, which is talking to the "Brain" Mango Pi running Stockfish (a chess engine). The player encodes the game state using a rotary encoder, and receives buzzes encoding the next move from Stockfish. The Brain Pi also runs a live chess GUI which streams the current game. Since Stockfish benefits from running on a hosted system with gigabytes of RAM and several cores, the engine itself runs on a laptop and communicates its results to the "Brain" Pi over serial.

We took this project as an opportunity to explore different ways for different computers to communicate. To that end, we established a Bluetooth connection between two Pi's, developed a protocol to speak reliably over that connection, communicated with Bluetooth modules over serial, and came up with a protocol to send information between the "Brain" Pi and the laptop running Stockfish over serial. Additionally, we worked on our graphical abilities by implementing a GUI, and our HCI abilities by deciding how the user (that is, the cheater) would interact with the device.

How to use:
- `make run` runs a program to send raw AT commands to the Bluetooth HC-05 module (this is mostly for testing).
- `make brain` runs the code that should run on the "Brain" Mango Pi, connected to the host laptop which itself runs Stockfishs.
- `make hand` runs the program that should run on the "Hand" Mango Pi, which the player would secretly have in their pocket.

## Member contribution

Javier:

- Bluetooth communication driver for HC-05 module (`bt_ext.c`)
- Protocol to reliably send "events" over Bluetooth between two Pi's (`jnxu.c`).
- "Hand" Pi (`hand.c`)
- Part of Chess GUI (`chess_gui.c`)
- Peripherals for Servo, which we used for buzzing (see `hand.c`), and rotary encoder (`re.c`).

Ellen:

- Stockfish engine interface (`engine.py`)
- Receive commands from Stockfish on the Pi side (`chess.c`)
- "Brain" Pi (`brain.c`)
- Chess GUI (`chess_gui.c`)

## References

We were inspired to do something cool with Bluetooth and the fact that the Pi is a "tiny, tiny computer" to do some magic. Eventually we came up with the idea to allow Pi's to talk to one another and act as an accomplice for chess.

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

## Self-evaluation

The Pi's are able to talk to one another! We were able to get Bluetooth communication with two Pi's, the engine running on the computer, monitor displaying the GUI, and two peripheries all working live for the chess game. We didn't end up pursuing other tricks for Mango magic, but went deeper on the chess idea and are happy with how it turned out.

Moments we're proud of:
 - Bluetooth connecting the two Pi's!
 - Getting stockfish working! and seeing the moves appear live (e2e4 d7d5 e4xe5 moment... :D).
 - Rotary encoder + cursor logic for highlighting which piece is being moved.

Moments of pain:
 - Chess has sooo many weird rules we had to take into account, such as taking 'en passant,' castling and, worst of all, pawn promotion and underpromotion. We could probably write a dissertation on the problems of handling paw promotion but, to make a very long story short, this one edge case that happens in ~4.5% of games added around 500 lines of code in total, including a specific GUI and overhauling our communication protocol. Suffice to say, we take chess rules perfection seriously!
 - Bluetooth can sometimes become unreliable, so we had to include many checks to ensure the connection was still alive at any given time, including "ping" and "echo" packets we created.

## Photos
