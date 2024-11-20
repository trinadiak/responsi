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
int failedAttempts = 0; // Counter for failed attempts

void setup() {
    Serial.begin(9600); // Start serial communication
    WiFi.begin(ssid, password);

    // Wait for Wi-Fi connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

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

void updateFlaskStatus(int newVoucherChange, int newVoucherEligible) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(updateServerURL);
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<200> doc;
        doc["user_voucherChange"] = newVoucherChange;
        doc["user_voucherEligible"] = newVoucherEligible;
        doc["modbus_value"] = receivedModbusCode;
        doc["server_value"] = voucher_code;

        String payload;
        serializeJson(doc, payload);

        int httpResponseCode = http.POST(payload);
        if (httpResponseCode == 200) {
//            Serial.println("HTTP POST successful.");
        } else {
//            Serial.println("HTTP POST failed. Code: " + String(httpResponseCode));
        }
        http.end();
    } else {
//        Serial.println("Wi-Fi not connected. Cannot send data.");
    }
}

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

void compareCodes() {
    // Check if the voucher code is 0, if so, ignore it
    Serial.println(receivedModbusCode);
    if (voucher_code == 0) {
        //Serial.println("Voucher code is 0, ignoring...");
        return; // Exit the function if voucher code is 0
    }
    
//    Serial.print("Comparing Modbus code: ");
//    Serial.print(receivedModbusCode);
    Serial.print(" with Voucher code: ");
    Serial.println(voucher_code);

    if (receivedModbusCode == voucher_code) {
        if (status != 11) {
            status = 11;
            Serial.println("Status updated to 11");
            updateFlaskStatus(2, 1);  // Send the update to Flask server
        }
        failedAttempts = 0;  // Reset failed attempts if codes match
//        Serial.println("Failed attempts reset to 0.");
    } else {
        failedAttempts++;
        Serial.print("Failed attempts: ");
//        Serial.println(failedAttempts);

        if (failedAttempts >= 3) {
            if (status != 10) {
                status = 10;
//                Serial.println("Status updated to 10 after 3 failed attempts.");
                updateFlaskStatus(1, 0);  // Send the update to Flask server
            }
            failedAttempts = 0;  // Reset the failed attempts after sending update
//            Serial.println("Failed attempts reset to 0 after update.");

            // Restart the ESP32
//            Serial.println("Restarting ESP32...");
            ESP.restart();  // Reset the ESP32 after 3 failed attempts
        }
    }
}


void loop() {
    static unsigned long lastUpdateTime = 0;
    receiveVoucherCodeFromFlask();
    checkForModbusRequest();
    delay(1000);
}
