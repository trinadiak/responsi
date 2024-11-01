#include "project.h"

#define RX_BUFFER_SIZE 8       // Size for the expected Modbus response frame
#define SLAVE_ADDRESS 1        // Address of the Modbus slave

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

// Serial monitor purpose
void byteToHex(uint8_t byte, char* hexString) {
    sprintf(hexString, "%02X", byte);
}
void sendString(const char* str) {
    while (*str) {
        UART_PutChar(*str++);
    }
}

int main(void) {
    CyGlobalIntEnable;
    UART_Start();  // Serial monitor
    UART2_Start(); 
    
    char hexString[3];
    uint8_t receivedFrame[RX_BUFFER_SIZE];
    uint8_t requestFrame[8] = {SLAVE_ADDRESS, 0x03, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00};

    // Calculate CRC for the request frame
    uint16_t crc = calculateCRC(requestFrame, 6);
    requestFrame[6] = (uint8_t)(crc & 0xFF);
    requestFrame[7] = (uint8_t)((crc >> 8) & 0xFF);

    for (;;) {
        UART_ClearRxBuffer();

        // ---- SEND REQUEST ----
        for (int i = 0; i < 8; i++) {
            UART2_PutChar(requestFrame[i]);
        }

        CyDelay(100);

        // ---- RECEIVE RESPONSE FROM SLAVE ----
        if (UART2_GetRxBufferSize() >= RX_BUFFER_SIZE) {
            for (int i = 0; i < RX_BUFFER_SIZE; i++) {
                receivedFrame[i] = UART2_GetByte();
            }
            
            if (receivedFrame[0] == SLAVE_ADDRESS && receivedFrame[1] == 0x03) {
                uint16_t calculatedCRC = calculateCRC(receivedFrame, 6);
                uint16_t receivedCRC = (receivedFrame[7] << 8) | receivedFrame[6];

                if (calculatedCRC == receivedCRC) {
                    uint16_t registerValue = (receivedFrame[3] << 8) | receivedFrame[4];
                    
                    char decimalString[12];
                    
                    
                    // PRINT THE SUCCESSFULLY RECEIVED DATA
                    //sendString("Received Register Value: ");
                    //byteToHex(registerValue >> 8, hexString);  // High byte
                    //sendString(hexString);
                    //sendString(" ");
                    //byteToHex(registerValue & 0xFF, hexString);  // Low byte
                    //sendString(hexString);
                    //sendString("\r\n");
                    sendString("Decimal: ");
                    sprintf(decimalString, "%u", registerValue);
                    UART_PutString(decimalString);
                    sendString("\r\n");
                } else {
                    sendString("CRC Error\r\n");
                    //CySoftwareReset(); // Reset w/o clicking the physical button
                    // Without reset => wrong dataframe order
                }
            } else {
                sendString("Address or Function Code Mismatch\r\n");
                //CySoftwareReset();
            }
            
            // Display received frame for debugging
            sendString("Received Frame: ");
            for (int i = 0; i < RX_BUFFER_SIZE; i++) {
                byteToHex(receivedFrame[i], hexString);
                sendString(hexString);
                sendString(" ");
            }
            sendString("\r\n");
        } else {
            sendString("No response received\r\n");
        }
        CySoftwareReset();

        CyDelay(1000);
    }
}
