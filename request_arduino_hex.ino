int registerValue = 45;  // Example register value to return

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

void setup() {
  Serial.begin(9600);
  Serial.println("Arduino Ready");  // Initial feedback to check connection
}

void loop() {
  if (Serial.available() >= 8) {  // Wait until enough bytes for a full request
    uint8_t request[8];
    Serial.readBytes(reinterpret_cast<char*>(request), 8);  // Read the Modbus request

    // Debug: print received bytes in hexadecimal format
    Serial.print("Received request: ");
    for (int i = 0; i < 8; i++) {
      Serial.print(request[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // Extract and print received CRC
    uint16_t requestCRC = request[6] | (request[7] << 8);
    Serial.print("Received CRC: ");
    Serial.println(requestCRC, HEX);

    // Calculate CRC for comparison
    uint16_t calculatedCRC = calculateCRC(request, 6);
    Serial.print("Calculated CRC: ");
    Serial.println(calculatedCRC, HEX);

    // Validate CRC and function code
    if (calculatedCRC == requestCRC && request[1] == 0x03) {
      // Create response frame
      uint8_t response[7] = {request[0], request[1], 0x02, highByte(registerValue), lowByte(registerValue)};

      // Calculate CRC for the response
      uint16_t responseCRC = calculateCRC(response, 5);
      response[5] = responseCRC & 0xFF;
      response[6] = (responseCRC >> 8) & 0xFF;

      // Send the response
      
      Serial.println("Sent: ");
      // Serial.println(response);
      Serial.write(response, 7);
      Serial.println("Response sent.");
    } else {
      Serial.println("Invalid CRC or unsupported function code.");
    }
  }
}
