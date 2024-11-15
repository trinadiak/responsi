/* ESP32 S3 (for debugging) ONLY, DONT USE THIS CODE IF YOU'RE USING REGULAR ESP32 BOARD*/

#include <WiFi.h>
#include <HTTPClient.h>

#define SLAVE_ADDRESS 1
#define FUNCTION_CODE_REQ 3
#define FUNCTION_CODE_RECEIVE 6
#define REGISTER_ADDRESS 0x0001

const char* ssid = "Pihzo Slate";      // Replace with your Wi-Fi network
const char* password = "dQw4w9WgXcQ";
const char* serverURL = "http://192.168.169.56:5000/get_data";  // Replace with your Flask server address

uint16_t voucher_code = 0;           // Code from Flask server
int receivedModbusCode = 0;          // Code from PSoC via Modbus
int REGISTER_VALUE = 0;         // store the data value from ESP32

void setup() {
    Serial.begin(9600);  // SERIAL MONITOR
    Serial2.begin(9600, SERIAL_8N1, 17, 16); // COMMUNICATION
  
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
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

void sendResponse(int registerValue, uint8_t *frame) {
    // Calculate CRC for received request
    uint16_t receivedCRC = (frame[7] << 8) | frame[6];
    uint16_t calculatedCRC = calculateCRC(frame, 6);

    if (calculatedCRC == receivedCRC) {
        // Construct response frame with function code 3
        uint8_t response[8];
        response[0] = SLAVE_ADDRESS;                // Slave address
        response[1] = FUNCTION_CODE_REQ;            // Function code 3 (read holding registers)
        response[2] = 2;                            // Byte count (1 register = 2 bytes)
        response[3] = (registerValue >> 8) & 0xFF;  // High byte of register value
        response[4] = registerValue & 0xFF;         // Low byte of register value

        // Calculate CRC for the response
        uint16_t responseCRC = calculateCRC(response, 5);
        response[5] = responseCRC & 0xFF;           // Low byte of CRC
        response[6] = (responseCRC >> 8) & 0xFF;    // High byte of CRC

        // Send response to master
        Serial2.write(response, sizeof(response));
    } else {
        // Optional: Handle CRC error (if needed)
        Serial.println("CRC unmatched");
        Serial.print("Calculated CRC:");
        Serial.println(calculatedCRC);
    }
}

#include <ArduinoJson.h>  // Add this library to parse JSON

void receiveVoucherCodeFromFlask() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverURL);
        int httpResponseCode = http.GET();

        if (httpResponseCode == 200) {
            String payload = http.getString();

            // Parse JSON payload
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
                voucher_code = doc["voucher_code"].as<int>();
                Serial.print("Received voucher_code from Flask: ");
                Serial.println(voucher_code);
            } else {
                Serial.print("JSON parsing error: ");
                Serial.println(error.c_str());
            }
        } else {
            Serial.print("Error in HTTP request, code: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    }
}


int receiveModbusValue(uint8_t *frame) {
    return (frame[4] << 8) | frame[5];
}

void checkForModbusRequest() {
    if (Serial2.available() >= 8) {
        uint8_t frame[8];
        for (int i = 0; i < 8; i++) {
            frame[i] = Serial2.read();
        }
        // FUNCTION CODE 6: RECEIVE DATA VALUE FROM PSOC
        if (frame[0] == SLAVE_ADDRESS && frame[1] == FUNCTION_CODE_RECEIVE) {
            receiveVoucherCodeFromFlask();
            receivedModbusCode = receiveModbusValue(frame);
            Serial.print("Received Modbus code from PSoC: ");
            Serial.println(receivedModbusCode);
        }
        
        // FUNCTION CODE 3: RECEIVE EMPTY DATAFRAME AND SEND BACK THE EXPECTED VALUE
        else if (frame[0] == SLAVE_ADDRESS && frame[1] == FUNCTION_CODE_REQ) {
            // sendResponse(1, frame);
            compareCodes();
        }
    }
}

void compareCodes() {
    if (receivedModbusCode == voucher_code) {
        sendResponse(1, frame);
        Serial.println("Codes Match!");
    } else {
        sendResponse(0, frame);
        Serial.println("Codes Do Not Match.");
        
    }
}

void loop() {
    // receiveVoucherCodeFromFlask();   // Fetch the voucher code via HTTP from Flask server
    // delay(2000);                     // Wait before checking Modbus communication
    checkForModbusRequest();         // Check for Modbus communication from PSoC
    // compareCodes();
    // if (receivedModbusCode != 0 && voucher_code != 0) {
    //     compareCodes();              // Compare codes after both are received
    // }
    
    delay(5000);                     // Small delay between loops
}
