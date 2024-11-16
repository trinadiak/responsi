#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>  // Add this library to parse JSON

// Constants
#define SLAVE_ADDRESS 1
#define FUNCTION_CODE_REQ 3
#define FUNCTION_CODE_RECEIVE 6

// Wi-Fi Credentials
const char* ssid = "Hotsauce";
const char* password = "akunssomu1";
const char* serverURL = "http://192.168.235.47:5000/get_data";

// Variables
uint16_t voucher_code = 0;           // Code from Flask server
int receivedModbusCode = 0;          // Code from PSoC via Modbus
int REGISTER_VALUE = 0;              // Store the data value from ESP32
int status = 0;

void setup() {
    // Serial Monitor
    Serial.begin(9600);

    // Wi-Fi Setup
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        // Serial.println("Connecting to WiFi...");
    }
    // Serial.println("Connected to WiFi");
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
    uint16_t receivedCRC = (frame[7] << 8) | frame[6];
    uint16_t calculatedCRC = calculateCRC(frame, 6);

    if (calculatedCRC == receivedCRC) {
        uint8_t response[8];
        response[0] = SLAVE_ADDRESS;
        response[1] = FUNCTION_CODE_REQ;
        response[2] = 2;
        response[3] = (registerValue >> 8) & 0xFF;
        response[4] = registerValue & 0xFF;

        uint16_t responseCRC = calculateCRC(response, 5);
        response[5] = responseCRC & 0xFF;
        response[6] = (responseCRC >> 8) & 0xFF;

        Serial.write(response, sizeof(response));
        // Serial.println("Response sent to Modbus master.");
    } else {
        // Serial.println("CRC check failed.");
    }
}

void receiveVoucherCodeFromFlask() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverURL);
        int httpResponseCode = http.GET();

        if (httpResponseCode == 200) {
            String payload = http.getString();
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
                // voucher_code = doc["voucher_code"].as<int>();
                voucher_code = doc["user_voucher"].as<int>();
                // Serial.print("Received voucher_code from Flask: ");
                // Serial.println(voucher_code);
            } else {
                // Serial.print("JSON parsing error: ");
                // Serial.println(error.c_str());
            }
        } else {
            // Serial.print("HTTP request failed, code: ");
            // Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        // Serial.println("WiFi disconnected.");
    }
}

void changeStatusFlask(int newVoucherChange, int newVoucherEligible) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverURL + "/update_data");  // Ensure this is the correct endpoint

        http.addHeader("Content-Type", "application/json");  // Set the content type
        StaticJsonDocument<200> doc;

        // Populate JSON object
        doc["user_voucherChange"] = newVoucherChange;
        doc["user_voucherEligible"] = newVoucherEligible;

        String payload;
        serializeJson(doc, payload);

        int httpResponseCode = http.POST(payload);  // Send the POST request

        if (httpResponseCode == 200) {
            String response = http.getString();
            // Optional: Parse and handle the server response if needed
        } else {
            // Handle HTTP error
        }
        http.end();
    } else {
        // Handle WiFi not connected case
    }
}




int receiveModbusValue(uint8_t *frame) {
    return (frame[4] << 8) | frame[5];
}

void checkForModbusRequest() {
    while (Serial.available() >= 8) { // Check for at least 8 bytes (minimum Modbus frame length)
        uint8_t frame[8];
        for (int i = 0; i < 8; i++) {
            frame[i] = Serial.read();
        }

        // Debug: Print the received frame
        // Serial.print("Received Modbus frame: ");
        // for (int i = 0; i < 8; i++) {
        //     Serial.print(frame[i], HEX);
        //     Serial.print(" ");
        // }
        // Serial.println();

        // CRC Validation
        uint16_t receivedCRC = (frame[7] << 8) | frame[6];
        uint16_t calculatedCRC = calculateCRC(frame, 6);
        if (calculatedCRC != receivedCRC) {
            // Serial.println("CRC mismatch! Ignoring frame.");
            return;
        }

        // Handle Function Codes
        if (frame[0] == SLAVE_ADDRESS && frame[1] == FUNCTION_CODE_REQ) {
            // Serial.println("Function Code 03: Read Holding Registers");
            if (status == 1){
              sendResponse(1, frame);
            }
            else {
              sendResponse(0, frame);
            }
        } else if (frame[0] == SLAVE_ADDRESS && frame[1] == FUNCTION_CODE_RECEIVE) {
            // Serial.println("Function Code 06: Receive Data Value");
            receivedModbusCode = receiveModbusValue(frame);
            compareCodes(frame);

        } else {
            // Serial.println("Unknown function code or address mismatch.");
        }
    }
}

void compareCodes(uint8_t *frame){
  if (receivedModbusCode == voucher_code){
    status = 1;
    // 
  }
  else {
    status = 0;
  }
}

void loop() {
    receiveVoucherCodeFromFlask();
    checkForModbusRequest();        // Handle Modbus requests
    delay(1000);                     // Small delay to reduce loop rate
    checkForModbusRequest();        // Handle Modbus requests
    delay(1000);
    // Serial.flush();
}
