import serial

# Configure the serial port for the slave (change to your COM port)
slave_serial = serial.Serial(port='COM4', baudrate=9600, bytesize=8, parity='N', stopbits=1, timeout=1)

def listen_for_data():
    while True:
        if slave_serial.in_waiting >= 5:  # Wait for at least 5 bytes in the buffer
            frame = slave_serial.read(5)  # Read the entire frame (1 byte addr, 1 byte func, 2 bytes addr, 1 byte data)
            
            # Unpack the frame to get data
            slave_address = frame[0]
            function_code = frame[1]
            register_address = (frame[2] << 8) | frame[3]
            data = frame[4]
            
            print("Received data from master:")
            print(f"Slave Address: {slave_address}")
            print(f"Function Code: {function_code}")
            print(f"Register Address: {register_address}")
            print(f"Data (2-digit number): {data}")

# Start listening for data
listen_for_data()

# Close the serial port after communication
slave_serial.close()
