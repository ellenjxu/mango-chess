"""
This file is the interface between the Stockfish engine and the serial port,
which is connected to the Mango Pi.

The serial communication works as follows (all messages are newline terminated):
 - The Pi sends either GAME_WHITE or GAME_BLACK to the engine, depending on
    which side the cheater is playing.
 - The host responds with READY.
 - If playing WHITE, the host sends the first move, which we hardcode as e2e4
    (statistically the best move).
 - The host then waits for the opponent's move, which it receives from the
    serial port, preceded by MOVE_BEGIN. For example "MOVE_BEGIN\ne3e5\n". The
    move is fed to Stockfish, which determines whether it is valid.
 - If the move is valid, the host responds with Stockfish's choice of best move.
    Otherwise, the host sends NOPE.
 - The two previous steps are repeated until the game ends.
"""

from stockfish import Stockfish
import serial
from sys import platform

# Ellen's config (default)
STOCKFISH_PATH = "/usr/bin/stockfish"
SERIAL_PORT = "/dev/ttyUSB0"

if platform.startswith("darwin"):
    # If we detect the Darwin platform, assume Javier is running the script and
    # use his config.
    print("Hi Javier!")

    # Javier's config
    STOCKFISH_PATH = "/opt/homebrew/bin/stockfish"
    SERIAL_PORT = "/dev/tty.usbserial-0001"
else:
    # Not Darwin, assume it's Ellen (who runs Linux)
    print("Hi Ellen!")

#----------------------------------------------------------------

stockfish = Stockfish(path=STOCKFISH_PATH, depth=20, parameters={
    "Threads": 4,       # Faster computations
    "Skill Level": 20,  # Max Skill
    "Hash": 2048,       # 2GB of memory
    "MultiPV": 1,       # Only compute the best move for efficiency
    })

def get_move():
    while True:
        # Read next line
        start = ser.readline().decode("ascii").strip()

        if start == "MOVE_BEGIN": # Found start of a move
            move = ser.readline().decode("ascii").strip()
            break
        elif start: # Not a move, print for debugging purposes
            print("Pi >", start)
    print("Opp move: ", move)
    return move

def send_command(cmd):
    if (len(cmd) > 128):
        raise Exception("Command too long")
    # Commands are preceded by / and end with \n.
    cmd = "/" + cmd + "\n"
    ser.write(cmd.encode())

def stats():
    # Evaluate Win, Draw, Lose stats
    print("Getting stats")
    WDL = stockfish.get_wdl_stats()
    if WDL:
        percentage = lambda x: "{: >2}".format(str(int(x * 100)))
        W, D, L = WDL
        pW = percentage(W / (W + D + L))
        send_command("SW" + pW)

        pD = percentage(D / (W + D + L))
        send_command("SD" + pD)

        pL = percentage(L / (W + D + L))
        send_command("SL" + pL)
    else:
        print("No WDL stats")

def send_move(move):
    if len(move) > 5: # brain.c reads max 5 chars
        raise Exception("Move too long")
    move += "\n"
    ser.write(move.encode())

# Open serial connection
with serial.Serial(SERIAL_PORT, 115200, timeout=1) as ser:
    # Wait for game start message
    start = ser.readline().decode("ascii").strip()
    while start != "GAME_BLACK" and start != "GAME_WHITE":
        start = ser.readline().decode("ascii").strip()
    print(start)

    player = start.split("_")[1] # WHITE or BLACK

    # Send game start message
    ser.write("READY\n".encode())

    # When playing white, default to King's pawn.
    if player == "WHITE":
        stockfish.make_moves_from_current_position(["e2e4"]) # default starting move
        send_move("e2e4")

    while True:
        # Get move
        opp_move = get_move().strip()

        try:
            # Stockfish will throw an exception if a move is invalid
            stockfish.make_moves_from_current_position([opp_move])
        except:
            print("Invalid move")
            send_move("NOPE")
            continue
        
        # Compute our best move. Stockfish will return None if checkmate.
        best_move = stockfish.get_best_move()

        if best_move is None:
            best_move = "/MATE"
        else:
            stockfish.make_moves_from_current_position([best_move])

        print("Stockfish move: ", best_move)

        # stats()
        send_move(best_move)

