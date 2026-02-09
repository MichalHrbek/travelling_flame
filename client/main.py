# Je potreba nainstalovat knihovnu pyserial: 'pip install pyserial'
import sys
import os
from datetime import datetime
import serial
import serial.tools.list_ports
import traceback
import time
from dataclasses import dataclass, field

@dataclass
class FlameRun:
    lighter_start_time: int = field(default=0)
    lighter_duration: int = field(default=0)
    flame_arrive_time: int = field(default=0)
    start_temps: list[int] = field(default_factory=list)

    def to_csv(self) -> str:
        return f"{self.lighter_start_time}, {self.lighter_duration}, {self.flame_arrive_time}, {', '.join([str(i) for i in self.start_temps])}"
    
    def generate_csv_header(n_temps: int) -> str:
        return f"lighter_start_time, lighter_duration, flame_arrive_time, {', '.join([f'start_temp_{i}' for i in range(n_temps)])}"


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
            command = input("Command ('25' = lighter active for 25ms, '10 200' = lighter activated for 10ms twice with a 200ms gap): \n").strip()
            if (command == 'f'):
                ser.write('f')
                continue

            ser.write(("d " + command + '\n'))
            
            command_args = map(int, command.split())
            duration = command_args[0]
            delays = command_args[1:]

            n_runs = command.count(' ') + 1
            run_index = 0
            flame_index = 0
            last_flame_time = time.time()

            runs: list[FlameRun] = []

            while True:
                if ser.in_waiting == 0:
                    if time.time()-last_flame_time > 0:
                        print("Waited too long for a flame. Ending")
                        break
                    continue
                line = ser.readline().strip().decode()

                if not line: continue
                args = line.split()
                match args[0]:
                    case 'S':
                        runs.append(FlameRun(int(args[1]), duration))
                    case 'T':
                        runs[run_index].start_temps = [int(i) for i in args]
                    case 'E':
                        # TODO: record the real flame on time in micros
                        # runs[run_index].duration
                        run_index += 1
                    case 'F':
                        if int(args[1]) == 0: # TODO: idk
                            runs[flame_index].flame_arrive_time = int(args[2])
                            flame_index += 1
                            last_flame_time = time.time()
                if flame_index == n_runs and run_index == n_runs:
                    break

            with open(gen_filename(), 'w') as f:
                f.write(FlameRun.generate_csv_header(len(runs[0].start_temps)))
                f.write('\n')
                for i in runs:
                    f.write(i.to_csv())
                    f.write('\n')

            
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