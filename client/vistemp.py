import sys, os
import pandas as pd
import matplotlib.pyplot as plt
from glob import glob
from iterfzf import iterfzf

def plot_csv_data(file_path: str):
    try:
        df = pd.read_csv(file_path)

        df['time_sec'] = (df['timestamp_ms'] - df['timestamp_ms'].iloc[0]) / 1000.0

        # plt.style.use('ggplot')
        plt.figure(figsize=(10, 6))

        temp_columns = [col for col in df.columns if 'temp' in col.lower()]
        
        for col in temp_columns:
            plt.plot(df['time_sec'], df[col], label=col, linewidth=2, marker='.', markersize=4)

        plt.title('Temp x Time', fontsize=14, fontweight='bold')
        plt.xlabel('Time (seconds)', fontsize=12)
        plt.ylabel('Temperature (°C)', fontsize=12)
        plt.legend(title="Thermistors", loc='best')
        plt.grid(True, linestyle='--', alpha=0.7)

        plt.tight_layout()
        # plt.savefig(file_path.removesuffix("csv")+"png")
        plt.show()

    except FileNotFoundError:
        print(f"Error: The file '{file_path}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        plot_csv_data(sys.argv[1])
    else:
        plot_csv_data(iterfzf(glob(os.path.join(os.path.dirname(__file__), "out/temps*"))))