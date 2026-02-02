# pip install pyserial
# https://www.pyserial.com/docs
import sys
import os
from datetime import datetime
import serial
import serial.tools.list_ports
import traceback

def gen_filename() -> str:
    dirname = os.path.join(os.path.dirname(__file__), "out/")
    os.makedirs(dirname, exist_ok=True)
    full_path = dirname + datetime.now().strftime("%Y%m%d-%H%M%S") + ".csv"
    assert not os.path.exists(full_path)
    return full_path

def list_devices():
    print("Usage: python main.py {PORT}")
    print("No port specified. Available ports:")
    ports = serial.tools.list_ports.comports()
    for port in ports:
        print(f"{port.device}: {port.description}")

def main(port: str):
    ser = serial.Serial(port, 9600, timeout=1)
    
    while not ser.closed:
        try:
            command = input("Command ('25' = lighter active for 25ms, '10 200' = lighter activated for 10ms twice with a 200ms gap): ").strip()
            if ' ' in command:
                t,g = map(int, command.split())
            else:
                t = int(command)
        except:
            traceback.print_exc()
            continue
        
        

        response = ser.readline()
        print(response.decode('utf-8').strip())

    ser.close()


if __name__ == "__main__":
    if len(sys.argv) == 1:
        list_devices()
    else:
        main(sys.argv[1])