// UART2 pin RX/TX : 2.0, 2.1
// UART2 RX 2 bytes

#include "project.h"

#define RX_BUFFER_SIZE 7 // Define buffer size for receiving response

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

// Function to convert a byte to a hexadecimal string
void byteToHex(uint8_t byte, char* hexString) {
    sprintf(hexString, "%02X", byte);  // Format the byte as a two-digit hex
}

// Function to send a string via UART
void sendString(const char* str) {
    while (*str) {
        UART_PutChar(*str++);
    }
}

int main(void) {
    CyGlobalIntEnable;  // Enable global interrupts
    UART_Start();       // Start UART component in full duplex mode
    UART2_Start();

    uint8_t request[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00}; // Modbus request frame

    for (;;) {
        uint16_t crc = calculateCRC(request, 6);
        request[6] = (uint8_t)(crc & 0xFF);       
        request[7] = (uint8_t)((crc >> 8) & 0xFF); 

        // Send request frame in full-duplex
        for (int i = 0; i < 8; i++) {
            UART2_PutChar(request[i]);
        }

        CyDelay(100);  // Wait for the response

        // Check if response is ready to be read
        if (UART2_GetRxBufferSize() >= RX_BUFFER_SIZE) {
            uint8_t response[RX_BUFFER_SIZE];
            for (int i = 0; i < RX_BUFFER_SIZE; i++) {
                response[i] = UART2_GetChar();
            }

            // Validate CRC
            uint16_t responseCRC = response[5] | (response[6] << 8); // CRC from Slave
            if (calculateCRC(response, 6) == responseCRC) { // CRC from calculator vs CRC from Slave
                uint16_t receivedValue = (response[3] << 8) | response[4];
                
                // Prepare and send messages
                char hexString[3];  // Buffer for hexadecimal string
                sendString("Received value: ");
                byteToHex(receivedValue & 0xFF, hexString); // Send lower byte
                sendString(hexString);
                sendString(" ");
                byteToHex((receivedValue >> 8) & 0xFF, hexString); // Send upper byte
                sendString(hexString);
                sendString("\r\n");
            } else {
                sendString("CRC Error\r\n");
                char hexString[3];

                sendString("Calculated CRC: ");
                byteToHex(calculateCRC(response, 6), hexString);
                sendString(hexString);
                sendString(" Response CRC: ");
                byteToHex(responseCRC, hexString);
                sendString(hexString);
                sendString("\r\n");
            }
        }

        CyDelay(1000);  // Request data every second
    }
}
