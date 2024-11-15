#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Pihzo Slate";           // Replace with your WiFi network name
const char* password = "dQw4w9WgXcQ";   // Replace with your WiFi network password

String serverName = "http://192.168.1.56:5000";  // Replace with your Flask server IP address

int user_voucherChange = 1;  // Start at 1
int user_voucherEligible = 0;  // Start at 0

void setup() {
  Serial.begin(9600); // FOR SERIAL MONITOR (DEBUGGING)
  Serial2.begin(9600, SERIAL_8N1, 17, 16); // Serial2 for COMMUNICATION --- UNCOMMENT FOR DEBUGGING USING ESP32-S3 ONLY!!!!

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Cycle through the values on each reset
  cycleVariables();

  // Print the current values to the Serial Monitor
  Serial.println("Current user_voucherChange: " + String(user_voucherChange));
  Serial.println("Current user_voucherEligible: " + String(user_voucherEligible));

  // Send the updated values to the Flask server
  sendUpdatedDataToServer();
}

void loop() {
  // The loop() can remain empty since the cycling happens on reset
}

void cycleVariables() {
  // Cycle through the values for user_voucherChange
  if (user_voucherChange == 1) {
    user_voucherChange = 2;
  } else if (user_voucherChange == 2) {
    user_voucherChange = 3;
  } else if (user_voucherChange == 3) {
    user_voucherChange = 1;  // Reset to 1 after reaching 3
  }

  // Toggle the value for user_voucherEligible between 0 and 1
  user_voucherEligible = (user_voucherEligible == 0) ? 1 : 0;
}

void sendUpdatedDataToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName + "/update_data");
    http.addHeader("Content-Type", "application/json");

    // Create the JSON payload with updated values
    String jsonPayload = "{\"user_voucherChange\": " + String(user_voucherChange) + 
                         ", \"user_voucherEligible\": " + String(user_voucherEligible) + "}";

    // Send HTTP POST request
    int httpResponseCode = http.POST(jsonPayload);

    // Check response
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server response: " + response);
    } else {
      Serial.println("Error on sending POST: " + String(httpResponseCode));
    }

    http.end();  // Free resources
  } else {
    Serial.println("Error in WiFi connection");
  }
}
