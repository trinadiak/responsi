// Slave Code - Arduino
#define TXEN 3

const byte SLAVE_ADDRESS = 1;
const byte FUNCTION_CODE = 6;
const int REGISTER_ADDRESS = 0x0001;

void setup() {
  Serial.begin(9600); // UART setup
  
}

void loop() {
  if (Serial.available() >= 8) {
    byte frame[8];
    for (int i = 0; i < 8; i++) {
      frame[i] = Serial.read();
    }

    // Validate CRC
    unsigned int receivedCRC = (frame[7] << 8) | frame[6];
    if (calculateCRC(frame, 6) == receivedCRC) {
      processModbusFrame(frame);
    }
  }
}

void processModbusFrame(byte *frame) {
  byte slaveAddress = frame[0];
  byte functionCode = frame[1];
  int registerAddress = (frame[2] << 8) | frame[3];
  int data = (frame[4] << 8) | frame[5];

  if (slaveAddress == SLAVE_ADDRESS && functionCode == FUNCTION_CODE && registerAddress == REGISTER_ADDRESS) {
    Serial.print("Received value: ");
    Serial.println(data);

    // Send acknowledgment back to the master
    // sendAcknowledge(slaveAddress, functionCode);
  }
}

void sendAcknowledge(byte slaveAddress, byte functionCode) {
  byte response[8];
  response[0] = slaveAddress;
  response[1] = functionCode;
  response[2] = highByte(REGISTER_ADDRESS);
  response[3] = lowByte(REGISTER_ADDRESS);
  response[4] = 0; // Data (for acknowledgment, no data sent back)
  response[5] = 0;

  unsigned int crc = calculateCRC(response, 6);
  response[6] = lowByte(crc);
  response[7] = highByte(crc);

  for (int i = 0; i < 8; i++) {
    Serial.write(response[i]);
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
