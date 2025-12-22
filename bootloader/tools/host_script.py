import serial
import time
import argparse
import sys
import struct

# Commands
CMD_GET_HELP   = 0x50
CMD_GET_VER    = 0x51
CMD_GET_CID    = 0x52
CMD_GO         = 0x55
CMD_ERASE_APP  = 0x56
CMD_WRITE_MEM  = 0x57

ACK  = 0x06
NACK = 0x15

def get_crc(data):
    # If using CRC protocol later. Not used in simple implementation.
    return 0

def send_cmd(ser, cmd):
    ser.write(bytes([cmd]))

def cmd_get_ver(ser):
    send_cmd(ser, CMD_GET_VER)
    ver = ser.read(1)
    if len(ver) == 1:
        print(f"Bootloader Version: 0x{ver[0]:02X}")
    else:
        print("Error: No response")

def cmd_get_cid(ser):
    send_cmd(ser, CMD_GET_CID)
    cid = ser.read(2)
    if len(cid) == 2:
        print(f"Chip ID bytes: 0x{cid[0]:02X} 0x{cid[1]:02X}")
    else:
        print("Error: No response")

def cmd_erase(ser):
    print("Erasing Application Region...")
    send_cmd(ser, CMD_ERASE_APP)
    # Erase takes time, wait
    ser.timeout = 10 # Increase timeout
    resp = ser.read(1)
    ser.timeout = 1  # Restore timeout
    
    if len(resp) == 1 and resp[0] == ACK:
        print("Erase Success!")
    else:
        print(f"Erase Failed! Resp: {resp}")

def cmd_write(ser, filepath, start_address):
    print(f"Writing {filepath} to 0x{start_address:08X}...")
    try:
        with open(filepath, 'rb') as f:
            data = f.read()
    except FileNotFoundError:
        print("File not found.")
        return

    # Pad data to 2 bytes multiple if needed
    if len(data) % 2 != 0:
        data += b'\xFF'

    # Write in chunks (max 255 bytes per packet per protocol, but let used smaller)
    # Our protocol: [ADDR 4B] [LEN 1B] [DATA...]
    # Max LEN is 255.
    
    chunk_size = 16 # Reduce to 16 bytes (safe for small stacks/slow serial)
    total_written = 0
    
    for i in range(0, len(data), chunk_size):
        chunk = data[i:i+chunk_size]
        addr = start_address + i
        length = len(chunk)
        
        # Send Command
        send_cmd(ser, CMD_WRITE_MEM)
        
        # PROTCOL V2: Wait for ACK after CMD
        resp = ser.read(1)
        if len(resp) != 1 or resp[0] != ACK:
             print(f"No ACK after CMD at 0x{addr:08X}. Resp: {resp}")
             return

        # Address (Little Endian)
        # Send bytes manually with delay to prevent Overrun on Target
        addr_bytes = struct.pack('<I', addr)
        for b in addr_bytes:
            ser.write(bytes([b]))
            time.sleep(0.005) # 5ms delay per byte
        
        # PROTCOL V2: Wait for ACK after ADDR
        resp = ser.read(1)
        if len(resp) != 1 or resp[0] != ACK:
             print(f"No ACK after ADDR at 0x{addr:08X}. Resp: {resp}")
             return

        # Length
        ser.write(bytes([length]))

        # PROTCOL V2: Wait for ACK after LEN
        resp = ser.read(1)
        if len(resp) != 1 or resp[0] != ACK:
             print(f"No ACK after LEN at 0x{addr:08X}. Resp: {resp}")
             return
        
        # Data - send byte-by-byte with delays to prevent overrun
        for b in chunk:
            ser.write(bytes([b]))
            time.sleep(0.005)
        
        # Wait for Final ACK (Flash Write)
        ser.timeout = 2 # Increase timeout for flash write
        resp = ser.read(1)
        ser.timeout = 1 # Restore
        
        if len(resp) != 1 or resp[0] != ACK:
            print(f"Write error at 0x{addr:08X}. Resp: {resp}")
            return
            
        total_written += length
        print(f"\rWritten {total_written}/{len(data)} bytes", end='')
        
        # Small delay between chunks to let Flash controller settle
        time.sleep(0.05) # 50ms
        
    print("\nWrite Complete.")

def cmd_jump(ser, address):
    print(f"Jumping to 0x{address:08X}...")
    send_cmd(ser, CMD_GO)
    ser.write(struct.pack('<I', address))
    # No response expected usually, as MCU jumps
    print("Jump command sent.")

def main():
    parser = argparse.ArgumentParser(description="STM32 Bootloader Host")
    parser.add_argument("port", help="Serial Port (e.g. COM3 or /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    parser.add_argument("--ver", action="store_true", help="Get Version")
    parser.add_argument("--cid", action="store_true", help="Get Chip ID")
    parser.add_argument("--erase", action="store_true", help="Erase Application")
    parser.add_argument("--test-sig", action="store_true", help="Test critical write (2 bytes)")
    parser.add_argument("--echo-test", action="store_true", help="Debug Echo (4 bytes)")
    parser.add_argument("--write", type=str, help="Binary file to write")
    parser.add_argument("--addr", type=lambda x: int(x,0), default=0x08004000, help="Start Address (default 0x08004000)")
    parser.add_argument("--jump", action="store_true", help="Jump to Application")

    args = parser.parse_args()

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except Exception as e:
        print(f"Error opening port: {e}")
        return
    
    # Flush junk
    ser.read_all()
    
    if args.ver:
        cmd_get_ver(ser)
    
    if args.cid:
        cmd_get_cid(ser)
        
    if args.erase:
        cmd_erase(ser)
        # Give Flash controller time to settle after mass erase
        time.sleep(0.1)
        
    if args.test_sig:
        # Write 2 bytes (0xDEAD) to start address
        data = b'\xAD\xDE' # Little endian 0xDEAD
        print(f"Testing Write of 2 bytes {data.hex()} to 0x{args.addr:08X}...")
        
        # Send Command
        send_cmd(ser, CMD_WRITE_MEM)
        
        # CMD ACK
        if ser.read(1) != bytes([ACK]): 
            print("No ACK after CMD"); return

        # Address (send byte-by-byte with delays)
        addr_bytes = struct.pack('<I', args.addr)
        for b in addr_bytes:
            ser.write(bytes([b]))
            time.sleep(0.005)
        
        # ADDR ACK
        if ser.read(1) != bytes([ACK]): 
            print("No ACK after ADDR"); return

        # Length (2 bytes)
        ser.write(bytes([2]))
        # LEN ACK
        if ser.read(1) != bytes([ACK]): 
            print("No ACK after LEN"); return
            
        # Data (send byte-by-byte with delays)
        for b in data:
            ser.write(bytes([b]))
            time.sleep(0.005)
        
        # Final ACK
        resp = ser.read(1)
        if resp == bytes([ACK]):
            print("Test Write SUCCESS!")
        else:
            print(f"Test Write FAILED! Resp: {resp}")
        return

    if args.write:
        cmd_write(ser, args.write, args.addr)
        
    if args.echo_test:
        print("Testing 4-byte Echo...")
        send_cmd(ser, 0x58)
        
        if ser.read(1) != bytes([ACK]):
             print("No ACK for Echo CMD"); return
             
        # Send 4 bytes carefully
        payload = b'\x11\x22\x33\x44'
        for b in payload:
            ser.write(bytes([b]))
            time.sleep(0.005)
            
        # Read back
        echoed = ser.read(4)
        if echoed == payload:
            print(f"Echo SUCCESS! Received {echoed.hex()}")
        else:
            print(f"Echo FAIL! Sent {payload.hex()}, Recv {echoed.hex()}")
        return

    if args.jump:
        cmd_jump(ser, args.addr)

    ser.close()

if __name__ == "__main__":
    main()
