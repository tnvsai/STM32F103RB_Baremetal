import serial
import time
import argparse
import sys
import struct
import threading
from contextlib import contextmanager

# Check for Windows to import msvcrt for keyboard input
try:
    import msvcrt
except ImportError:
    msvcrt = None

# Commands
CMD_GO         = 0x55
CMD_ERASE_APP  = 0x56
CMD_WRITE_MEM  = 0x57
CMD_READ_MEM   = 0x59

ACK  = 0x06
NACK = 0x15

# Configuration Constants
CHUNK_SIZE = 64  # Maximum safe size (matches bootloader buffer)
BYTE_TX_DELAY = 0.003
CHUNK_DELAY = 0.01
RETRY_COUNT = 3
ERASE_TIMEOUT = 10
WRITE_TIMEOUT = 2

# Flash Memory Map
FLASH_START = 0x08000000
FLASH_END = 0x08020000  # 128KB total
BOOTLOADER_SIZE = 0x8000  # 32KB (matches STM32F103RBTX_BOOT.ld)
APP_START = FLASH_START + BOOTLOADER_SIZE
APP_END = FLASH_END

def get_crc(data):
    return 0

@contextmanager
def temp_timeout(ser, timeout_sec):
    """Context manager for temporary serial timeout"""
    old_timeout = ser.timeout
    ser.timeout = timeout_sec
    try:
        yield
    finally:
        ser.timeout = old_timeout

def validate_address(addr, length, operation="access"):
    """Validate flash address range"""
    if addr < FLASH_START or addr >= FLASH_END:
        raise ValueError(f"Address 0x{addr:08X} out of flash range (0x{FLASH_START:08X}-0x{FLASH_END:08X})")
    if addr + length > FLASH_END:
        raise ValueError(f"{operation.capitalize()} of {length} bytes at 0x{addr:08X} exceeds flash end")
    return True

def send_cmd(ser, cmd):
    ser.write(bytes([cmd]))

rx_buffer = bytearray()
quiet_mode = False

def read_one_byte(ser):
    global rx_buffer
    while True:
        if len(rx_buffer) > 0:
            return bytes([rx_buffer.pop(0)])
            
        b = ser.read(1)
        if not b: return b # Timeout
        
        if b == b'[':
            potential_suffix = ser.read(5)
            
            if potential_suffix == b'LOG] ':
                line = ser.readline()
                if not quiet_mode:
                    try:
                        msg = line.decode('utf-8', errors='ignore').strip()
                        print(f"[LOG] {msg}") 
                    except:
                        print(f"[Raw Log] {line}")
                continue
            else:
                rx_buffer.extend(b)
                rx_buffer.extend(potential_suffix)
                return bytes([rx_buffer.pop(0)])
        else:
            return b

def read_bytes(ser, n):
    data = bytearray()
    while len(data) < n:
        b = read_one_byte(ser)
        if not b: break
        data.extend(b)
    return bytes(data)

# --- Commands ---

def cmd_erase(ser):
    print("Erasing Application Region...")
    send_cmd(ser, CMD_ERASE_APP)
    
    start_time = time.time()
    with temp_timeout(ser, ERASE_TIMEOUT):
        resp = read_bytes(ser, 1)
    
    elapsed_time = time.time() - start_time
    
    if resp == bytes([ACK]):
        print(f"Erase Success! ({elapsed_time:.2f}s)")
        return True
    elif resp == bytes([NACK]):
        print("Erase Failed!")
        return False
    else:
        print(f"Erase Timeout or Error. Resp: {resp}")
        return False




def cmd_read(ser, addr, length):
    global rx_buffer
    
    try:
        validate_address(addr, length, "read")
    except ValueError as e:
        print(f"Error: {e}")
        return
    
    print(f"Reading {length} bytes from 0x{addr:08X}...")
    ser.reset_input_buffer()
    rx_buffer = bytearray()
    
    send_cmd(ser, CMD_READ_MEM)
    if read_bytes(ser, 1) != bytes([ACK]):
        print("Cmd NACK")
        return
        
    addr_bytes = struct.pack('<I', addr)
    for b in addr_bytes:
        ser.write(bytes([b]))
        time.sleep(BYTE_TX_DELAY)
        
    if read_bytes(ser, 1) != bytes([ACK]):
        print("Addr NACK")
        return
        
    ser.write(bytes([length]))
    if read_bytes(ser, 1) != bytes([ACK]):
        print("Len NACK")
        return
        
    data = read_bytes(ser, length)
    print("Data: " + " ".join([f"{b:02X}" for b in reversed(data)]))

# --- UI Helpers ---
def print_progress_bar(iteration, total, prefix='', suffix='', length=30, fill='#'):
    percent = ("{0:.1f}").format(100 * (iteration / float(total)))
    filled_length = int(length * iteration // total)
    bar = fill * filled_length + '-' * (length - filled_length)
    sys.stdout.write(f'\r{prefix} |{bar}| {percent}% {suffix}')
    sys.stdout.flush()
    if iteration == total: 
        print()

def verify_flash_content(ser, addr, data_chunk):
    """
    Reads back memory from addr and compares with data_chunk.
    Returns True if match, False otherwise.
    """
    global rx_buffer
    try:
        ser.reset_input_buffer()
        rx_buffer = bytearray()
        send_cmd(ser, CMD_READ_MEM)
        if read_bytes(ser, 1) != bytes([ACK]): return False
        
        addr_bytes = struct.pack('<I', addr)
        for b in addr_bytes:
            ser.write(bytes([b]))
            time.sleep(BYTE_TX_DELAY)
        if read_bytes(ser, 1) != bytes([ACK]): return False
        
        ser.write(bytes([len(data_chunk)]))
        if read_bytes(ser, 1) != bytes([ACK]): return False
        
        read_data = read_bytes(ser, len(data_chunk))
        return read_data == data_chunk
        
    except Exception as e:
        print(f"Verify Ex: {e}")
        return False

def cmd_write(ser, filepath, start_address):
    print(f"Reading {filepath}...")
    try:
        with open(filepath, 'rb') as f:
            data = f.read()
    except FileNotFoundError:
        print("File not found.")
        return False

    if len(data) % 2 != 0:
        data += b'\xFF'

    chunk_size = CHUNK_SIZE 
    total_len = len(data)
    total_written = 0
    start_time = time.time()
    
    print(f"Writing {total_len} bytes to 0x{start_address:08X}...")
    print_progress_bar(0, total_len, prefix='Writing:', suffix='Complete', length=40)

    for i in range(0, total_len, chunk_size):
        chunk = data[i:i+chunk_size]
        addr = start_address + i
        
        success = False
        for attempt in range(RETRY_COUNT):
            try:
                ser.reset_input_buffer()
                send_cmd(ser, CMD_WRITE_MEM)
                if read_bytes(ser, 1) != bytes([ACK]):
                    print(f"\n[Retry {attempt+1}] No ACK for CMD at 0x{addr:08X}"); continue

                addr_bytes = struct.pack('<I', addr)
                for b in addr_bytes:
                    ser.write(bytes([b]))
                    time.sleep(BYTE_TX_DELAY)
                
                if read_bytes(ser, 1) != bytes([ACK]):
                    print(f"\n[Retry {attempt+1}] No ACK for ADDR at 0x{addr:08X}"); continue

                ser.write(bytes([len(chunk)]))
                if read_bytes(ser, 1) != bytes([ACK]):
                     print(f"\n[Retry {attempt+1}] No ACK for LEN at 0x{addr:08X}"); continue
                
                for b in chunk:
                    ser.write(bytes([b]))
                    time.sleep(BYTE_TX_DELAY)
                
                with temp_timeout(ser, WRITE_TIMEOUT):
                    resp = read_bytes(ser, 1)
                
                if resp != bytes([ACK]):
                     # Check if it was a Lost ACK for a successful write?
                     if verify_flash_content(ser, addr, chunk):
                         print(f"\n[Retry {attempt+1}] Verify OK! Lost ACK recovered.")
                         success = True
                         break
                     else:
                         print(f"\n[Retry {attempt+1}] Write err at 0x{addr:08X}. Resp: {resp}"); continue
                
                success = True
                break
            except Exception as e:
                print(f"\n[Retry {attempt+1}] Exception: {e}")
                if verify_flash_content(ser, addr, chunk):
                     print(f"[Retry {attempt+1}] Verify OK! Exception recovered.")
                     success = True
                     break
                     
                time.sleep(0.5)

        if not success:
            print(f"\nFailed to write chunk at 0x{addr:08X} after 3 attempts.")
            return False

        total_written += len(chunk)
        print_progress_bar(total_written, total_len, prefix='Writing:', suffix='Complete', length=40)
        time.sleep(CHUNK_DELAY)
        
    elapsed_time = time.time() - start_time
    print(f"\nWrite Complete in {elapsed_time:.2f} seconds ({total_len/elapsed_time:.0f} bytes/sec)")
    return True

def cmd_jump(ser):
    print("Sending Jump Command...")
    send_cmd(ser, CMD_GO)
    print("Jump command sent.")

def monitor_rx_thread(ser, stop_event):
    """Background thread to read from Serial and print to screen."""
    while not stop_event.is_set():
        try:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                try:
                    print(data.decode('utf-8'), end='', flush=True)
                except:
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()
            else:
                time.sleep(0.01)
        except Exception:
            break

def cmd_monitor(ser):
    print("--- Serial Monitor (Ctrl+C to exit) ---")
    print("--- Tx enabled (Keyboard -> Serial) ---")
    
    stop_event = threading.Event()
    rx_thread = threading.Thread(target=monitor_rx_thread, args=(ser, stop_event))
    rx_thread.daemon = True
    rx_thread.start()
    
    try:
        while True:
            if msvcrt:
                if msvcrt.kbhit():
                    ch = msvcrt.getch()
                    if ch == b'\x03': 
                        raise KeyboardInterrupt
                    ser.write(ch)
            else:
                i = input()
                ser.write(i.encode() + b'\n')
            
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        print("\n--- Returning to Shell ---")
        stop_event.set()
        rx_thread.join(timeout=1.0)

def run_shell(ser):
    print("------------------------------------------------")
    print(" STM32 Bootloader Shell (Type 'help' for list)")
    print("------------------------------------------------")
    
    while True:
        try:
            cmd_line = input("(STM32-BL) > ").strip()
        except KeyboardInterrupt:
            print("\nExiting Shell.")
            break
            
        if not cmd_line: continue
        
        parts = cmd_line.split()
        cmd = parts[0].lower()
        args = parts[1:]
        
        global rx_buffer
        ser.reset_input_buffer()
        rx_buffer = bytearray()
        
        if cmd in ["exit", "quit"]:
            break
            
        elif cmd == "help":
            print("Commands:")
            print("  erase        - Erase App Region")
            print("  flash <file> - Write Binary File")
            print("  read <addr> <len> - Read Memory")
            print("  jump         - Jump to App")
            print("  monitor      - Serial Monitor (Ctrl+C to quit)")
            print("  help         - Show this list")
            print("  exit         - Quit Shell")
        
        elif cmd == "monitor":
            cmd_monitor(ser)
            
        elif cmd == "erase":
            cmd_erase(ser)
            time.sleep(0.1)
            
        elif cmd == "flash":
            if not args:
                print("Usage: flash <filename>")
                continue
            
            flash_start = time.time()
            if not cmd_erase(ser):
                print("Aborting Flash: Erase Failed.")
                continue
                
            time.sleep(0.1) 
            
            if cmd_write(ser, args[0], APP_START):
                total_elapsed = time.time() - flash_start
                print(f"\n{'='*50}")
                print(f"Flashing Complete! Total time: {total_elapsed:.2f}s")
                print(f"{'='*50}")
                print("Jumping to Application...")
                time.sleep(0.5)
                cmd_jump(ser)
                
                print("Switching to Monitor Mode...")
                time.sleep(0.5)
                cmd_monitor(ser)
            
        elif cmd == "jump":
            cmd_jump(ser)
            

        elif cmd == "read":
            if len(args) < 2:
                print("Usage: read <addr> <len>")
                continue
            addr = int(args[0], 0)
            length = int(args[1], 0)
            cmd_read(ser, addr, length)
            
        else:
            print("Unknown command. Type 'help'.")


def main():
    parser = argparse.ArgumentParser(description="STM32 Bootloader Host")
    parser.add_argument("port", help="Serial Port (e.g. COM3 or /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    
    parser.add_argument("--erase", action="store_true", help="Erase Application")
    parser.add_argument("--write", type=str, help="Binary file to write")
    parser.add_argument("--addr", type=lambda x: int(x,0), default=APP_START, help=f"Start Address (default 0x{APP_START:08X})")
    parser.add_argument("--jump", action="store_true", help="Jump to Application")
    parser.add_argument("-q", "--quiet", action="store_true", help="Suppress bootloader logs")

    args = parser.parse_args()
    
    global quiet_mode
    quiet_mode = args.quiet

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except Exception as e:
        print(f"Error opening port: {e}")
        return
    
    ser.read_all()
    
    actions = [args.erase, args.write, args.jump]
    
    if any(actions):
        if args.erase: 
            cmd_erase(ser)
            time.sleep(0.1)
        if args.write: cmd_write(ser, args.write, args.addr)
        if args.jump: cmd_jump(ser)
        
    else:
        run_shell(ser)

    ser.close()

if __name__ == "__main__":
    main()
