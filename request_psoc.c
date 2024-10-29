// UART FULL DUPLEX

#include "project.h"

#define RX_BUFFER_SIZE 7  // Define buffer size for receiving response

// Function to calculate CRC
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
    UART_Start();       // Start the UART component
    
    uint8_t request[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00};  // Modbus RTU request frame

    for (;;) {
        // Calculate and add CRC to the request
        uint16_t crc = calculateCRC(request, 6);
        request[6] = (uint8_t)(crc & 0xFF);       // Low byte of CRC
        request[7] = (uint8_t)((crc >> 8) & 0xFF); // High byte of CRC

        // Switch to transmit mode
        //UART_LoadTxConfig();

        // Send request frame
        for (int i = 0; i < 8; i++) {
            UART_PutChar(request[i]);
        }

        // Ensure transmission completes before switching modes
        while (!(UART_ReadTxStatus() & UART_TX_STS_COMPLETE));

        CyDelay(10);  // Brief delay before switching to RX mode

        // Switch to receive mode
        //UART_LoadRxConfig();

        // Wait to allow time for the response
        CyDelay(100);

        // Check if response received
        if (UART_GetRxBufferSize() >= RX_BUFFER_SIZE) {
            uint8_t response[RX_BUFFER_SIZE];
            for (int i = 0; i < RX_BUFFER_SIZE; i++) {
                response[i] = UART_GetChar();
            }

            // Switch back to transmit mode
          //  UART_LoadTxConfig();

            // Validate response CRC
            uint16_t responseCRC = response[5] | (response[6] << 8);
            if (calculateCRC(response, 6) == responseCRC) {
                // If CRC is correct, extract data value
                uint16_t receivedValue = (response[3] << 8) | response[4];
                UART_PutString("Received value: ");
                UART_PutChar(receivedValue); // Display received value (as a character, can be modified)
            } else {
                UART_PutString("CRC Error\r\n");
            }
        }

        // Delay between requests
        CyDelay(1000);
    }
}
