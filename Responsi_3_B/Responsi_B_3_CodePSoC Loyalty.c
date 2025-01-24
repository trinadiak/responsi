#include "project.h"
#include <stdio.h>
#include "Enter.h"
#include "OutputBiner1.h"
#include "UARTT.h"
#include "UARTR.h"
#include "Tombol_3.h"
#include "Reset.h"
#include "cytypes.h"

// INTEGRASII
#define RX_BUFFER_SIZE 8       // Size for the expected Modbus response frame
#define SLAVE_ADDRESS 1        // Address of the Modbus slave
#define REGISTER_ADDRESS 0x0001

//---------------- INPUT VOUCHER ------------------
// Fungsi konversi biner ke desimal
uint8_t binaryToDecimal(uint8_t binaryData)
{
    uint8_t decimalValue = 0;
    int base = 1;
    
    while (binaryData > 0)
    {
        uint8_t last_digit = binaryData % 10;
        binaryData = binaryData / 10;
        decimalValue += last_digit * base;
        base = base * 2;
    }
    return decimalValue;
}

void reset_program()
{
    CySoftwareReset(); // Melakukan reset software pada PSoC
}

// Fungsi untuk mengirim data melalui UART untuk debugging
void sendDebugData(const char *str)
{
    UART_PutString(str);
}

int convertArrayToInt(uint8_t *array, int length) {
    int result = 0;
    for (int i = 0; i < length; i++) {
        result = result * 10 + array[i];  // Multiply result by 10 and add the next digit
    }
    return result;
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

// Fungsi untuk mengirim data melalui Modbus (contoh sederhana)
//void sendModbusData(uint8_t *data, uint8_t length)
//{
//    char buffer[20];
//    for(uint8_t i = 0; i < length; i++)
//    {
//        UARTT_PutChar(data[i]);   // Mengirim data byte per byte melalui UART
//        snprintf(buffer, sizeof(buffer), "Data %d: %d\n", i+1, data[i]);
//        sendDebugData(buffer);     // Menampilkan data untuk debugging
//    }
//}

void sendModbusFrame(uint8_t slaveAddress, uint8_t functionCode, uint16_t registerAddress, uint16_t data) {
    uint8_t frame[8];  // Modbus frame

    if (functionCode == 0x06){ // function code 6: only send register (data)
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
    else if (functionCode == 3){ // function code 3: request data
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
    CyDelay(1000);

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


int main(void)
{
    CyGlobalIntEnable;      // Mengaktifkan global interrupt
    UART_Start();          // Serial monitor
    UART2_Start();

    // Variabel untuk menyimpan data 4 bit yang digabungkan
    // ?? WHY NEGATIVE?? ada kemungkinan kode vouchernya 00
    uint8_t combinedData1 = 0; 
    uint8_t combinedData2 = 0;
    
    uint8_t countSalah = 0;
    uint8_t countBenar = 0;

    char hexString[3];
    uint8_t receivedFrame[RX_BUFFER_SIZE];
    
    //uint8_t dataList[2];
    //int data = 0;
    
    for(;;)
    {
        //uint8_t dataList[2];
        //int data = 0;
        // Membaca setiap pin output dan menggeser bit untuk membuat data 4 bit utuh
        combinedData1 = (OutputBiner1_3_Read() << 3) |   
                       (OutputBiner1_2_Read() << 2) |   
                       (OutputBiner1_1_Read() << 1) |   
                       (OutputBiner1_0_Read());
                    
        combinedData2 = (OutputBiner2_3_Read() << 3) |   
                       (OutputBiner2_2_Read() << 2) |   
                       (OutputBiner2_1_Read() << 1) |   
                       (OutputBiner2_0_Read());
        
        //CyDelay(1000); // Delay 1 detik sebelum mengulangi pembacaan

        // Memeriksa apakah tombol Enter ditekan
        // Anggap tombol enter aktif low (0 = ditekan)
        // Set state ke -1 jika tidak ada data yang di-enter
        // Read button state
        static uint8_t buttonState = 1;
        int stateSend = 0;
        //static uint32_t buttonPressTime = 0;
        if (Tombol_3_Read() == 0 && buttonState == 1) {
            //buttonPressTime = currentTime;
            buttonState = 0;  // Button pressed
        } else if (Tombol_3_Read() == 1 && buttonState == 0) 
        {
             buttonState = 1;
            // Membaca data biner dari pin output
            uint8_t binaryData1 = combinedData1;
            uint8_t binaryData2 = combinedData2;
            
            // Konversi biner ke desimal
            uint8_t decimalData1 = binaryToDecimal(binaryData1);
            uint8_t decimalData2 = binaryToDecimal(binaryData2);
            
            // Membuat list untuk menyimpan data yang telah dikonversi
            uint8_t dataList[2];
            int data = 0;
            dataList[1] = decimalData1;
            dataList[0] = decimalData2;

            data = convertArrayToInt(dataList, 2);
            
            // Debugging: Menampilkan data yang dikirim
            char debugBuffer[50];
            snprintf(debugBuffer, sizeof(debugBuffer), "Data1:%d, Data2: %d\n", binaryData2, binaryData1);
            sendDebugData(debugBuffer);
            
            
            // Mengirim data list melalui protokol Modbus
            sendModbusFrame(SLAVE_ADDRESS, 6, REGISTER_ADDRESS, data);
            CyDelay(1000);

            // Request data dari ESP32
            sendModbusFrame(SLAVE_ADDRESS, 3, REGISTER_ADDRESS, 0);
            
            //CyDelay(500);
            
            stateSend = 1;

        }

        CyDelay(1000);
        
        // ---------- RECEIVE DATA ----------------
        if (UART2_GetRxBufferSize() >= RX_BUFFER_SIZE && stateSend == 1) {
            for (int i = 0; i < RX_BUFFER_SIZE; i++) {
                receivedFrame[i] = UART2_GetByte();
            }

            // FUNCTION CODE: 3 (SENDS REQUEST AND RECEIVES THE REGISTER)
            if (receivedFrame[0] == SLAVE_ADDRESS && receivedFrame[1] == 0x03) {
                uint16_t calculatedCRC = calculateCRC(receivedFrame, 6);
                //uint16_t receivedCRC = receivedFrame[6] | (receivedFrame[7] << 8);
                uint16_t receivedCRC = receivedFrame[6];

                if (calculatedCRC == receivedCRC) {
                  // --- READ THE RECEIVED DATA ---
                    uint16_t registerValue = (receivedFrame[3] << 8) | receivedFrame[4];
                    
                    char decimalString[12];

                    // ISI LIST (ubah hex dari registerValue jadi 2 digit integer terpisah, LALU
                    // masukin ke list)
                    uint8_t controlDigits[2]; // List to store the two decimal digits
                    controlDigits[0] = registerValue / 10; // Tens place
                    controlDigits[1] = registerValue % 10; // Units place
                    
                    Control_Reg_1_Write((controlDigits[1] << 0) | (controlDigits[0] << 1));
                    CyDelay(500);
                    Control_Reg_1_Write(0 << 0 | 0 << 1);
                    
                    CyDelay(3000);
                    if (controlDigits[1] == 0){
                        countSalah++;
                    }
                    else if (controlDigits[1] == 1){
                        countBenar = 1;
                    }
                    
                    if (countSalah == 3 || countBenar == 1){
                        CySoftwareReset();
                    }
                    
                    
                    // PRINT THE SUCCESSFULLY RECEIVED DATA
                    sendString("Decimal: ");
                    sprintf(decimalString, "%u", registerValue);
                    UART_PutString(decimalString);
                    sendString("\r\n");
                } else {
                    sendString("CRC Error\r\n");
                    
                    // DEBUGGING CRC ERROR
                    char front[10];
                    sendString("Received CRC[6]: ");
                    sprintf(front, "%04X", receivedFrame[6]);  // Format as hexadecimal, 4 digits
                    sendString(front);
                    sendString("\r\n");
                    //debugCalculateCRC(receivedFrame, 6);
                    
                    char crcBuffer[10];  // Buffer to hold the formatted CRC values

                    sendString("Calculated CRC: ");
                    sprintf(crcBuffer, "%04X", calculatedCRC);  // Format as hexadecimal, 4 digits
                    sendString(crcBuffer);
                    sendString("\r\n");

                    sendString("Received CRC: ");
                    sprintf(crcBuffer, "%04X", receivedCRC);  // Format as hexadecimal, 4 digits
                    sendString(crcBuffer);
                    sendString("\r\n");
                    
                    uint16_t registerValue = (receivedFrame[3] << 8) | receivedFrame[4];
                    
                    char decimalString[12];
                    
                    
                    // PRINT THE SUCCESSFULLY RECEIVED DATA
                    sendString("Decimal error: ");
                    sprintf(decimalString, "%u", registerValue);
                    UART_PutString(decimalString);
                    sendString("\r\n");

                    //CySoftwareReset(); // Reset w/o clicking the physical button
                }
            } else {
                sendString("Address or Function Code Mismatch\r\n");
                //CySoftwareReset(); // Reset w/o clicking the physical button
            }
            
            // Display received frame for debugging
            sendString("Received Frame: ");
            for (int i = 0; i < RX_BUFFER_SIZE; i++) {
                byteToHex(receivedFrame[i], hexString);
                sendString(hexString);
                sendString(" ");
            }
            sendString("\r\n");
            char dataStr[10];
            sprintf(dataStr, "%d", UART2_GetRxBufferSize());
            sendString(dataStr);
            sendString("\r\n");

            UART2_ClearRxBuffer();
            stateSend = 0;
        } else if (UART2_GetRxBufferSize() < RX_BUFFER_SIZE && stateSend == 1) {
            sendString("No response received\r\n");
            char dataStr[10];
            sprintf(dataStr, "%d", UART2_GetRxBufferSize());
            sendString(dataStr);
            //printf("RX_BUFFER_SIZE: %d\n", RX_BUFFER_SIZE);
            sendString("\r\n");
            stateSend = 0;
            //CySoftwareReset();
        }
        UART2_ClearRxBuffer();
        CyDelay(1000);
        
        if (Reset_Read() == 1)
        {
          CyDelay(1000);
          reset_program();
          //UART_PutString("reset");
                        
          //while (Reset_Read() == 1);
        }
    }
}
