// #include <SoftwareSerial.h>

// #define MYPORT_RX 12 
// #define MYPORT_TX 13

// EspSoftwareSerial::UART modbusPort;  // RX, TX pins for Modbus communication
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
  Serial.println("Arduino Ready");
  // modbusPort.begin(9600);  // Set baud rate for modbus
  // modbusPort.begin(9600,SWSERIAL_8N1, MYPORT_RX, MYPORT_TX, false);
}

void loop() {
  // if (modbusPort.available()) {
  if (Serial.available()){
    uint8_t request[8];
    // modbusPort.readBytes(request, 8);
    Serial.readBytes(request,8);

    // Validate CRC
    uint16_t requestCRC = request[6] | (request[7] << 8);
    if (calculateCRC(request, 6) == requestCRC && request[1] == 0x03) {
      uint8_t response[7] = {request[0], request[1], 0x02, highByte(registerValue), lowByte(registerValue)};

      // Calculate CRC for response
      uint16_t crc = calculateCRC(response, 5);
      response[5] = crc & 0xFF;
      response[6] = (crc >> 8) & 0xFF;

      // modbusPort.write(response, 7);  // Send response back to master
      Serial.write(response,7);
    }
  }
}
