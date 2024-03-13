from stockfish import Stockfish
import serial
import time

stockfish = Stockfish(path="/usr/bin/stockfish")

def get_move():
    while True:
        move = ser.readline().decode('utf-8').strip()
        if move:
            break
    print(move)
    return move

with serial.Serial('/dev/ttyUSB0', 115200, timeout=1) as ser:
    try:
        start = ser.readline().decode('utf-8').strip()
        while (start != "GAME_BEGIN"):
            start = ser.readline().decode('utf-8').strip()
        print(start)
        ser.write("READY\n".encode())

        while True:
            opp_move = get_move().strip()
            stockfish.make_moves_from_current_position([opp_move])
            best_move = stockfish.get_best_move()
            stockfish.make_moves_from_current_position([best_move])
            print(best_move)
            best_move += "\n"
            ser.write(best_move.encode())
    except serial.serialutil.SerialException:
        print("Error")
        time.sleep(0.1)