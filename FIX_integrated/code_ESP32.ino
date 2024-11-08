#define SLAVE_ADDRESS 1
#define FUNCTION_CODE 3
#define REGISTER_ADDRESS 0x0001
#define REGISTER_VALUE 0x002E  // Example register value to send back
#define PINR 18

void setup() {
  Serial.flush();
  Serial.begin(9600);  // Initialize Serial communication for Modbus at 9600 baud rate
  delay(1000);  // Allow some time for setup
}

uint16_t calculateCRC(uint8_t *buffer, int length) {
  uint16_t crc = 0xFFFF;
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

void respondToMaster() {
  if (Serial.available() >= 8) {  // Check for a full request frame
    uint8_t request[8];
    for (int i = 0; i < 8; i++) {
      request[i] = Serial.read();
    }

    // Verify address and function code in the received request
    if (request[0] == SLAVE_ADDRESS && request[1] == FUNCTION_CODE) {
      // Calculate CRC for received request
      uint16_t receivedCRC = (request[7] << 8) | request[6];
      uint16_t calculatedCRC = calculateCRC(request, 6);

      if (calculatedCRC == receivedCRC) {
        // Construct response frame with function code 3
        uint8_t response[8];
        response[0] = SLAVE_ADDRESS;                // Slave address
        response[1] = FUNCTION_CODE;                // Function code 3 (read holding registers)
        response[2] = 2;                            // Byte count (1 register = 2 bytes)
        response[3] = (REGISTER_VALUE >> 8) & 0xFF; // High byte of register value
        response[4] = REGISTER_VALUE & 0xFF;        // Low byte of register value

        // Calculate CRC for the response
        uint16_t responseCRC = calculateCRC(response, 5);
        response[5] = responseCRC & 0xFF;           // Low byte of CRC
        response[6] = (responseCRC >> 8) & 0xFF;    // High byte of CRC

        // Send response frame back to the master
        Serial.write(response, sizeof(response));

      } else {
        // CRC error in the received request (could handle error here if needed)
        // Serial.println("CRC Error in Request");
      }
    } else {
      // Address or function code mismatch
      // Serial.println("Address or Function Code Mismatch in Request");
    }
  }
}

void loop() {
  respondToMaster();
  delay(100);  // Small delay to avoid rapid polling
}
