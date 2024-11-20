#include "project.h"
#include <stdio.h>
#include "Enter.h"
#include "OutputBiner1.h"
#include "UARTT.h"
#include "UARTR.h"
#include "Tombol_3.h"
#include "Reset.h"
#include "cytypes.h"


void reset_program()
{
    CySoftwareReset(); // Melakukan reset software pada PSoC
}

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

// Fungsi untuk mengirim data melalui UART untuk debugging
void sendDebugData(const char *str)
{
    UART_PutString(str);
}

// Fungsi untuk mengirim data melalui Modbus (contoh sederhana)
void sendModbusData(uint8_t *data, uint8_t length)
{
    char buffer[20];
    for(uint8_t i = 0; i < length; i++)
    {
        UART_PutChar(data[i]);   // Mengirim data byte per byte melalui UART
        snprintf(buffer, sizeof(buffer), "Data %d: %d\n", i+1, data[i]);
        sendDebugData(buffer);     // Menampilkan data untuk debugging
    }
}

int main(void)
{
    CyGlobalIntEnable;      // Mengaktifkan global interrupt
    UART_Start();          // Memulai UART untuk komunikasi Modbus
       
    uint8_t combinedData1 = 0; // Variabel untuk menyimpan data 4 bit yang digabungkan
    uint8_t combinedData2 = 0;
    uint8_t combinedData3 = 0;

    for(;;)
    {
        // Membaca setiap pin output dan menggeser bit untuk membuat data 4 bit utuh
        combinedData1 = (OutputBiner1_3_Read() << 3) |   
                       (OutputBiner1_2_Read() << 2) |   
                       (OutputBiner1_1_Read() << 1) |   
                       (OutputBiner1_0_Read());
                    
        combinedData2 = (OutputBiner2_3_Read() << 3) |   
                       (OutputBiner2_2_Read() << 2) |   
                       (OutputBiner2_1_Read() << 1) |   
                       (OutputBiner2_0_Read());
        
        CyDelay(0); // Delay 1 detik sebelum mengulangi pembacaan

        // Memeriksa apakah tombol Enter ditekan
        if (Tombol_3_Read() == 0) // Anggap tombol enter aktif low (0 = ditekan)
        {
            // Membaca data biner dari pin output
            uint8_t binaryData1 = combinedData1;
            uint8_t binaryData2 = combinedData2;
            
            // Konversi biner ke desimal
            uint8_t decimalData1 = binaryToDecimal(binaryData1);
            uint8_t decimalData2 = binaryToDecimal(binaryData2);

            combinedData3 = (decimalData2 << 1) |
                           (decimalData1);
                           
            // Membuat list untuk menyimpan data yang telah dikonversi
            uint8_t dataList[2];
            dataList[0] = decimalData1;
            dataList[1] = decimalData2;
            
            // Mengirim data list melalui protokol Modbus
            //sendModbusData(dataList, 2);
            
            // Debugging: Menampilkan data yang dikirim
            char debugBuffer[50];
            snprintf(debugBuffer, sizeof(debugBuffer), "Data1:%d, Data2: %d\n", decimalData1, decimalData2);
            sendDebugData(debugBuffer);
            

            //CyDelay(1000); // Delay 1 detik sebelum mengulangi pembacaan
                    
            // Tunggu sampai tombol dilepaskan untuk menghindari pengiriman berulang
            while (Tombol_3_Read() == 0);
        }
        
        if (Reset_Read() == 1)
        {
            CyDelay(3000);
            reset_program();
            UART_PutString("reset");
            
            while (Enter_Read() == 1);
        }
    }
}