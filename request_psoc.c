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
            
            CyDelay(100);
            // Validate CRC
            uint16_t responseCRC = response[5] | (response[6] << 8); //CRC from Slave
            if (calculateCRC(response, 6) == responseCRC) { // CRC from calculator vs CRC from Slave
                uint16_t receivedValue = (response[3] << 8) | response[4];
                UART_PutString("Received value: ");
                //uint8_t apapun = 'y';
                //UART_PutChar(apapun);
                UART_PutChar(receivedValue);  // Display received value
            } else {
                UART_PutString("CRC Error\r\n");
                UART_PutString("Calculated: ");
                UART_PutChar(calculateCRC(response, 6));
                UART_PutString("Respons CRC: ");
                UART_PutChar(responseCRC);
            }
        }

        CyDelay(1000);  // Request data every second
    }
}
