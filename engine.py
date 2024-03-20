from stockfish import Stockfish
import serial
from sys import platform

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

stockfish = Stockfish(path=STOCKFISH_PATH, depth=20, parameters={
    "Threads": 4,
    "Skill Level": 20,
    "Hash": 2048, # 2GB
    "MultiPV": 1,
    })

def get_move():
    while True:
        start = ser.readline().decode("ascii").strip()
        if start == "MOVE_BEGIN":
            move = ser.readline().decode("ascii").strip()
            break
        elif start:
            print("Pi >", start)
    print("Opp move: ", move)
    return move

def send_command(cmd):
    if (len(cmd) > 128):
        raise Exception("Command too long")
    cmd = "/" + cmd + "\n"
    get_move()
    print(cmd)
    ser.write(cmd.encode())

def stats():
    # evaluate
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

with serial.Serial(SERIAL_PORT, 115200, timeout=1) as ser:
    # try:
    start = ser.readline().decode("ascii").strip()
    while (start != "GAME_BLACK" and start != "GAME_WHITE"):
        start = ser.readline().decode("ascii").strip()
    print(start)
    player = start.split("_")[1]
    ser.write("READY\n".encode())

    if player == "WHITE":
        stockfish.make_moves_from_current_position(["e2e4"]) # default starting move
        send_move("e2e4")

    while True:
        opp_move = get_move().strip()
        try:
            stockfish.make_moves_from_current_position([opp_move])
        except:
            print("Invalid move")
            send_move("NOPE")
            continue

        best_move = stockfish.get_best_move()
        if best_move is None:
            best_move = "/MATE"
        else:
            stockfish.make_moves_from_current_position([best_move])

        print("Stockfish move: ", best_move)
        # stats()
        send_move(best_move)

    # except serial.serialutil.SerialException:
    #     print("Error")
    #     time.sleep(0.1)
