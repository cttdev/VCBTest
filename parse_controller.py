import serial
import time
import re
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque
import threading
from queue import Queue

# Configuration
COM_PORT = 'COM3'
BAUD_RATE = 115200
NUM_CHANNELS = 16
CENTER_VALUE = 992
MAX_VALUE = 1811
MIN_VALUE = 172
MAX_HISTORY = 100  # Number of data points to display on plot

class SerialDataReader:
    def __init__(self, port, baudrate):
        self.port = port
        self.baudrate = baudrate
        self.serial_conn = None
        self.data_queue = Queue()
        self.running = False
        
    def connect(self):
        try:
            self.serial_conn = serial.Serial(self.port, self.baudrate, timeout=1)
            self.running = True
            print(f"Connected to {self.port} at {self.baudrate} baud")
            return True
        except serial.SerialException as e:
            print(f"Failed to connect: {e}")
            return False
    
    def read_data(self):
        """Read and parse serial data in a separate thread"""
        buffer = ""
        while self.running:
            try:
                if self.serial_conn.in_waiting:
                    data = self.serial_conn.read(self.serial_conn.in_waiting).decode('utf-8', errors='ignore')
                    buffer += data
                    
                    # Look for complete channel data blocks
                    if "Parsed RC Channels:" in buffer:
                        # Check if we have a complete block (look for the next "Parsed RC Channels:" or end of buffer)
                        next_header_pos = buffer.find("Parsed RC Channels:", buffer.find("Parsed RC Channels:") + 1)
                        if next_header_pos != -1:
                            # We have a complete block
                            block = buffer[:next_header_pos]
                            channels = self.parse_channels(block)
                            if channels:
                                self.data_queue.put(channels)
                            # Remove the processed block
                            buffer = buffer[next_header_pos:]
                        elif len(buffer) > 1000:  # Buffer getting too large, process what we have
                            channels = self.parse_channels(buffer)
                            if channels:
                                self.data_queue.put(channels)
                            buffer = ""
                else:
                    time.sleep(0.01)
            except Exception as e:
                print(f"Error reading serial data: {e}")
                time.sleep(0.1)
    
    def parse_channels(self, data):
        """Parse channel values from the data string"""
        channels = {}
        # More flexible pattern to match "Channel X: value" with possible extra whitespace
        pattern = r'Channel\s+(\d+)\s*:\s*(\d+)'
        matches = re.findall(pattern, data)
        
        # Debug: print what we found
        if len(matches) != NUM_CHANNELS:
            print(f"Debug: Found {len(matches)} channel matches, expected {NUM_CHANNELS}")
            return None
        
        for channel_num, value in matches:
            try:
                ch_num = int(channel_num)
                ch_val = int(value)
                # Validate channel number only
                if 1 <= ch_num <= NUM_CHANNELS:
                    channels[ch_num] = ch_val
                else:
                    print(f"Debug: Invalid channel {ch_num}")
                    return None  # Invalid channel number
            except ValueError:
                print(f"Debug: ValueError parsing channel {channel_num}: {value}")
                return None
        
        # Only return channels if we have exactly all 16 channels
        if len(channels) == NUM_CHANNELS and all(i in channels for i in range(1, NUM_CHANNELS + 1)):
            return [channels[i] for i in range(1, NUM_CHANNELS + 1)]
        print(f"Debug: Missing channels. Found: {sorted(channels.keys())}")
        return None
    
    def start(self):
        """Start the serial reading thread"""
        thread = threading.Thread(target=self.read_data, daemon=True)
        thread.start()
    
    def close(self):
        self.running = False
        if self.serial_conn:
            self.serial_conn.close()

class ChannelVisualizer:
    def __init__(self):
        self.reader = SerialDataReader(COM_PORT, BAUD_RATE)
        self.history = {i: deque(maxlen=MAX_HISTORY) for i in range(NUM_CHANNELS)}
        self.timestamps = deque(maxlen=MAX_HISTORY)
        self.data_point_count = 0
        
        # Create figure with subplots for all 16 channels
        self.fig, self.axes = plt.subplots(4, 4, figsize=(16, 12))
        self.fig.suptitle('RC Channel Real-Time Monitor', fontsize=16)
        self.axes = self.axes.flatten()
        
        # Initialize plot axes
        for i in range(NUM_CHANNELS):
            ax = self.axes[i]
            ax.set_ylim(MIN_VALUE - 50, MAX_VALUE + 50)
            ax.axhline(y=CENTER_VALUE, color='g', linestyle='--', alpha=0.5, label='Center')
            ax.axhline(y=MAX_VALUE, color='r', linestyle=':', alpha=0.3, label='Max')
            ax.axhline(y=MIN_VALUE, color='b', linestyle=':', alpha=0.3, label='Min')
            ax.set_title(f'Channel {i+1}', fontsize=10)
            ax.set_ylabel('Value')
            ax.grid(True, alpha=0.3)
            if i >= 12:
                ax.set_xlabel('Time (samples)')
        
        self.lines = [ax.plot([], [], 'b-', lw=2)[0] for ax in self.axes]
        plt.tight_layout()
    
    def update_plot(self, frame):
        """Update plot with new data"""
        # Get latest data from queue
        while not self.reader.data_queue.empty():
            try:
                channels = self.reader.data_queue.get_nowait()
                self.data_point_count += 1
                self.timestamps.append(self.data_point_count)
                
                for i, value in enumerate(channels):
                    self.history[i].append(value)
                
            except:
                break
        
        # Update lines
        for i in range(NUM_CHANNELS):
            if len(self.history[i]) > 0:
                self.lines[i].set_data(range(len(self.history[i])), list(self.history[i]))
                self.axes[i].set_xlim(0, max(10, len(self.history[i])))
        
        return self.lines
    
    def run(self):
        """Start visualization"""
        if not self.reader.connect():
            print("Failed to establish serial connection")
            return
        
        self.reader.start()
        print("Starting real-time visualization... Press Ctrl+C to stop")
        
        try:
            ani = FuncAnimation(self.fig, self.update_plot, interval=100, blit=True)
            plt.show()
        except KeyboardInterrupt:
            print("\nStopping visualization...")
        finally:
            self.reader.close()

def main():
    visualizer = ChannelVisualizer()
    visualizer.run()

if __name__ == "__main__":
    main()