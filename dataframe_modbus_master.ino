// Master Code - Arduino

const byte SLAVE_ADDRESS = 1;        // Slave address
const byte FUNCTION_CODE = 6;        // Function code to write a register
const int REGISTER_ADDRESS = 0x0001; // Example register address
const int VALUE = 45;                // Example value to send

void setup() {
  Serial.begin(9600); // UART setup
  delay(1000);        // Allow time for the serial connection to establish
}

void loop() {
  sendModbusFrame(SLAVE_ADDRESS, FUNCTION_CODE, REGISTER_ADDRESS, VALUE);
  delay(1000); // Send every second
}

void sendModbusFrame(byte slaveAddress, byte functionCode, int registerAddress, int data) {
  byte frame[8];  // Modbus frame
  frame[0] = slaveAddress;
  frame[1] = functionCode;
  frame[2] = highByte(registerAddress);
  frame[3] = lowByte(registerAddress);
  frame[4] = highByte(data);
  frame[5] = lowByte(data);

  // Calculate CRC
  unsigned int crc = calculateCRC(frame, 6);
  frame[6] = lowByte(crc);
  frame[7] = highByte(crc);

  // Send the frame
  for (int i = 0; i < 8; i++) {
    Serial.write(frame[i]);
  }
  Serial.flush();
}

unsigned int calculateCRC(byte *buffer, int length) {
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < length; pos++) {
    crc ^= buffer[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}
