import serial
import time

# Configure the serial port for the master (change to your COM port)
master_serial = serial.Serial(port='COM3', baudrate=9600, bytesize=8, parity='N', stopbits=1, timeout=1)

def send_data_to_slave(slave_address, function_code, register_address, data):
    # Frame structure for Modbus RTU
    frame = bytearray()
    frame.append(slave_address)           # Slave Address
    frame.append(function_code)           # Function Code
    frame.append(register_address >> 8)   # High byte of Register Address
    frame.append(register_address & 0xFF) # Low byte of Register Address
    frame.append(data)                    # Data (the 2-digit number to send)
    
    # Send the frame over UART
    master_serial.write(frame)
    print("Data sent to slave:", frame)

# Send a 2-digit number (for example, 25) to slave at address 1, using function code 6 (write single register)
send_data_to_slave(slave_address=0x01, function_code=0x06, register_address=0x0001, data=25)

# Close the serial port after sending
time.sleep(1)  # Optional: wait for a response if needed
master_serial.close()
