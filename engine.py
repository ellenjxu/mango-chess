from stockfish import Stockfish
import serial
import time
from sys import platform

player = "white" # TODO: send this from mangopi at init

# Ellen's config
STOCKFISH_PATH = "/usr/bin/stockfish"
SERIAL_PORT = "/dev/ttyUSB0"

if platform.startswith("darwin"):
    print("Hi Javier!")

    # Javier's config
    STOCKFISH_PATH = "/opt/homebrew/bin/stockfish"
    SERIAL_PORT = "/dev/tty.usbserial-0001"
else:
    print("Hi Ellen!")

#----------------------------------------------------------------

stockfish = Stockfish(path=STOCKFISH_PATH)

def get_move():
    while True:
        start = ser.readline().decode("utf-8").strip()
        if start == "MOVE_BEGIN":
            move = ser.readline().decode("utf-8").strip()
            break
    print(move)
    return move

def send_move(move):
    best_move += "\n"
    ser.write(best_move.encode())

with serial.Serial(SERIAL_PORT, 115200, timeout=1) as ser:
    # try:
    start = ser.readline().decode("utf-8").strip()
    while (start != "GAME_BEGIN"):
        start = ser.readline().decode("utf-8").strip()
    print(start)
    ser.write("READY\n".encode())

    if player == "white":
        stockfish.make_moves_from_current_position(["e2e4"]) # defualt starting move
        send_move("e2e4")

    while True:
        opp_move = get_move().strip()
        stockfish.make_moves_from_current_position([opp_move])
        best_move = stockfish.get_best_move()

        if best_move is None:
            best_move = "MATE"
        else:
            stockfish.make_moves_from_current_position([best_move])

        print(best_move)
        send_move(best_move)
    # except serial.serialutil.SerialException:
    #     print("Error")
    #     time.sleep(0.1)
