#include "project.h"

#define RX_BUFFER_SIZE 8       // Size for the expected Modbus response frame
#define SLAVE_ADDRESS 1        // Address of the Modbus slave
#define REGISTER_ADDRESS 0x0001

// --- INPUT VOUCHER CODE ---
/*
inputByte = ... // INSERT CODE TO READ THE VOUCHER CODE FROM INPUT
char inputHex[3];
sprintf(inputHex, "%02X", inputByte);

*/

// --- MODBUS ---
void sendModbusFrame(uint8_t slaveAddress, uint8_t functionCode, uint16_t registerAddress, uint16_t data) {
    uint8_t frame[8];  // Modbus frame

    if (functionCode == 0x06){
        frame[0] = slaveAddress;
        frame[1] = functionCode;
        frame[2] = (uint8_t)(registerAddress >> 8); // High byte of register address
        frame[3] = (uint8_t)(registerAddress & 0xFF); // Low byte of register address
        frame[4] = (uint8_t)(data >> 8);  // High byte of data
        frame[5] = (uint8_t)(data & 0xFF);  // Low byte of data

        // Calculate CRC
        uint16_t crc = calculateCRC(frame, 6);
        frame[6] = (uint8_t)(crc & 0xFF);       // Low byte of CRC
        frame[7] = (uint8_t)((crc >> 8) & 0xFF); // High byte of CRC
    }
    else if (functionCode == 3){
        frame[0] = slaveAddress;
        frame[1] = functionCode;
        frame[2] = (uint8_t)(registerAddress >> 8); // High byte of register address
        frame[3] = (uint8_t)(registerAddress & 0xFF); // Low byte of register address
        frame[4] = 0x00;
        frame[5] = 0x01;

        uint16_t crc = calculateCRC(frame, 6);
        frame[6] = (uint8_t)(crc & 0xFF);
        frame[7] = (uint8_t)((crc >> 8) & 0xFF);
    }
    
    // Send frame over UART
    for (int i = 0; i < 8; i++) {
        UART2_PutChar(frame[i]);
    }
    CyDelay(100);

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
    

    for (;;) {
        sendModbusFrame(1, 6, REGISTER_ADDRESS, 45); // 1-way data transmission
        CyDelay(1000);
        sendModbusFrame(1, 3, REGISTER_ADDRESS, 1); // request sending

        CyDelay(500);

        // ---- RECEIVE RESPONSE FROM SLAVE ----
        if (UART2_GetRxBufferSize() >= RX_BUFFER_SIZE) {
            for (int i = 0; i < RX_BUFFER_SIZE; i++) {
                receivedFrame[i] = UART2_GetByte();
            }

            // FUNCTION CODE: 3 (SENDS REQUEST AND RECEIVES THE REGISTER)
            if (receivedFrame[0] == SLAVE_ADDRESS && receivedFrame[1] == 0x03) {
                uint16_t calculatedCRC = calculateCRC(receivedFrame, 6);
                //uint16_t receivedCRC = (receivedFrame[7] << 8) | receivedFrame[6];
                uint16_t receivedCRC = receivedFrame[6];

                if (calculatedCRC == receivedCRC) {
                  // READ DATA
                    uint16_t registerValue = (receivedFrame[3] << 8) | receivedFrame[4];
                    
                    char decimalString[12];
                    
                    // PRINT THE SUCCESSFULLY RECEIVED DATA
                    sendString("Decimal: ");
                    sprintf(decimalString, "%u", registerValue);
                    UART_PutString(decimalString);
                    sendString("\r\n");
                } else {
                    sendString("CRC Error\r\n");
                }
            } else {
                sendString("Address or Function Code Mismatch\r\n");
                CySoftwareReset(); // Reset w/o clicking the physical button
            }
            
            // Display received frame for debugging
            sendString("Received Frame: ");
            for (int i = 0; i < RX_BUFFER_SIZE; i++) {
                byteToHex(receivedFrame[i], hexString);
                sendString(hexString);
                sendString(" ");
            }
            sendString("\r\n");

            UART2_ClearRxBuffer();
        } else {
            sendString("No response received\r\n");
            CySoftwareReset();
        }

        CyDelay(1000);
    }
}
