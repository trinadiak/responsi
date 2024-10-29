#include <project.h>
#define TXEN_ON 1u
#define TXEN_OFF 0u

void main()
{
    uint8 recByte;
    uint8 tmpStat;
    CyGlobalIntEnable; /* Enable interrupts */
    UART_1_Start(); /* Start UART */
    //UART_1_LoadTxConfig(); /* Configure UART for transmitting */
    TXEN_Write(TXEN_ON);
    
    UART_1_PutString("Half Duplex Test"); /* Send message */
    /* make sure that data has been transmitted */
    CyDelay(30); /* Appropriate delay could be used */
                 /* Alternatively, check TX_STS_COMPLETE status bit */
    
    //UART_1_LoadRxConfig(); /* Configure UART for receiving */
    TXEN_Write(TXEN_OFF);
    while(1)
    {
        recByte = UART_1_GetChar(); /* Check for receive byte */
        if(recByte > 0) /* If byte received */
        {
            //UART_1_LoadTxConfig(); /* Configure UART for transmitting */
            TXEN_Write(TXEN_ON);
            
            UART_1_PutChar(recByte); /* Send received byte back */
            do /* wait until transmission complete */
            { /* Read Status register */
                tmpStat = UART_1_ReadTxStatus();
                /* Check the TX_STS_COMPLETE status bit */
            }
            while(~tmpStat & UART_1_TX_STS_COMPLETE);
            //UART_1_LoadRxConfig(); /* Configure UART for receiving */
            TXEN_Write(TXEN_OFF);
            
        }
    }
}
