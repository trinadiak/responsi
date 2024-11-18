#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Constants
#define SLAVE_ADDRESS 1
#define FUNCTION_CODE_REQ 3
#define FUNCTION_CODE_RECEIVE 6

// Wi-Fi Credentials
const char* ssid = "Pihzo Slate";
const char* password = "dQw4w9WgXcQ";
const char* getServerURL = "http://192.168.253.56:5000/get_data";
const char* updateServerURL = "http://192.168.253.56:5000/update_data";

// Variables
uint16_t voucher_code = 0; // Voucher code from the server
int receivedModbusCode = 0; // Code received via Modbus
int REGISTER_VALUE = 0; // Modbus register value
int status = 0; // Current status

void setup() {
    Serial.begin(9600); // Start serial communication
//    Serial.println("Starting ESP32...");
    WiFi.begin(ssid, password);

    // Wait for Wi-Fi connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
//        Serial.print(".");
    }
//    Serial.println("\nConnected to Wi-Fi!");
}

// Calculate CRC for Modbus communication
uint16_t calculateCRC(uint8_t* buffer, int length) {
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

// Fetch voucher code from the Flask server
void receiveVoucherCodeFromFlask() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(getServerURL);

        int httpResponseCode = http.GET();
        if (httpResponseCode == 200) {
            String payload = http.getString();
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (!error) {
                voucher_code = doc["user_voucher"].as<uint16_t>();
//                Serial.println("Voucher code received: " + String(voucher_code));
            } else {
//                Serial.println("Failed to parse JSON.");
            }
        } else {
//            Serial.println("GET request failed. HTTP code: " + String(httpResponseCode));
        }
        http.end();
    } else {
//        Serial.println("Wi-Fi not connected. Cannot fetch voucher code.");
    }
}

// Update status on the Flask server
void updateFlaskStatus(int newVoucherChange, int newVoucherEligible) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(updateServerURL);
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<200> doc;
        doc["user_voucherChange"] = newVoucherChange;
        doc["user_voucherEligible"] = newVoucherEligible;

        String payload;
        serializeJson(doc, payload);

        int httpResponseCode = http.POST(payload);
        if (httpResponseCode == 200) {
//            Serial.println("Successfully updated Flask server.");
        } else {
//            Serial.println("POST request failed. HTTP code: " + String(httpResponseCode));
        }
        http.end();
    } else {
//        Serial.println("Wi-Fi not connected. Cannot update Flask server.");
    }
}

// Check for Modbus requests
void checkForModbusRequest() {
    while (Serial.available() >= 8) {
        uint8_t frame[8];
        for (int i = 0; i < 8; i++) {
            frame[i] = Serial.read();
        }

        uint16_t receivedCRC = (frame[7] << 8) | frame[6];
        uint16_t calculatedCRC = calculateCRC(frame, 6);

        if (calculatedCRC == receivedCRC) {
            if (frame[0] == SLAVE_ADDRESS && frame[1] == FUNCTION_CODE_REQ) {
                sendResponse(status, frame);
            } else if (frame[0] == SLAVE_ADDRESS && frame[1] == FUNCTION_CODE_RECEIVE) {
                receivedModbusCode = (frame[4] << 8) | frame[5];
                compareCodes();
            }
        } 
    }
}

// Send Modbus response
void sendResponse(int registerValue, uint8_t* frame) {
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
}

// Compare received Modbus code with voucher code
void compareCodes() {
    if (receivedModbusCode == voucher_code) {
        if (status != 1) {
            status = 1;
            updateFlaskStatus(2, 1);
        }
    } else {
        if (status != 0) {
            status = 0;
            updateFlaskStatus(1, 0);
        }
    }
}

void loop() {
    receiveVoucherCodeFromFlask();
    checkForModbusRequest();
    delay(1000);
}
