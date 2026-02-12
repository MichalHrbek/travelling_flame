# Je potreba nainstalovat knihovnu pyserial: 'pip install pyserial'
import sys
import os
from datetime import datetime
import serial
import serial.tools.list_ports
import traceback
import select
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

def stdin_available():
    return select.select([sys.stdin], [], [], 0.0)[0]

def main(port: str):
    ser = serial.Serial(port, 9600, timeout=1)
    print(ser.read())
    while not ser.closed:
        try:
            # "Command ('25' = lighter active for 25ms, '10 200' = lighter activated for 10ms twice with a 200ms gap): \n"
            command = input("Command: ").strip()
            if ser.in_waiting:
                print(ser.read().strip().decode())

            if (not command.startswith('d')):
                ser.write(command.encode())
                while True:
                    if ser.in_waiting == 0:
                        if stdin_available():
                            print("Cancelled")
                            sys.stdin.read(1)
                            break
                    else:
                        print(ser.readline().strip().decode())

                continue
            

            ser.write(command.encode() + b'\n')
            
            command_args = list(map(int, command.split()[1:]))
            delays = command_args[:]

            n_runs = command.count(' ') + 1
            run_index = 0
            flame_index = 0

            runs: list[FlameRun] = []

            while True:
                if ser.in_waiting == 0:
                    if stdin_available():
                        print("Cancelled")
                        sys.stdin.read(1)
                        break
                    continue
                line = ser.readline().strip().decode()
                print(line)

                if not line: continue
                args = line.split()
                match args[0]:
                    case 'S':
                        runs.append(FlameRun(int(args[1])))
                    case 'T':
                        runs[run_index].start_temps = [float(i) for i in args[1:]]
                    case 'E':
                        runs[run_index].lighter_duration = int(args[1])
                        run_index += 1
                    case 'F':
                        if int(args[1]) == 0: # TODO: idk
                            runs[flame_index].flame_arrive_time = int(args[2])
                            flame_index += 1
                if flame_index == n_runs and run_index == n_runs:
                    print("All data recorded")
                    break

            print("Exporting")
            if len(runs):
                with open(gen_filename(), 'w') as f:
                    f.write(FlameRun.generate_csv_header(len(runs[0].start_temps)))
                    f.write('\n')
                    for i in runs:
                        f.write(i.to_csv())
                        f.write('\n')
            else:
                print("Error: recieved no data")

            
        except Exception:
            traceback.print_exc()
            continue

        except KeyboardInterrupt:
            break
        
        

        response = ser.readline()
        print(response.decode('utf-8').strip())

    ser.close()


if __name__ == "__main__":
    if len(sys.argv) == 1:
        list_devices()
    else:
        main(sys.argv[1])