/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.7
        Device            :  PIC16F1454
        Driver Version    :  2.00
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "mcc_generated_files/mcc.h"


uint8_t CDCRxBuffer[64];
uint8_t CDCRxLength = 0;

void EUSART_clear_rx(void){
    
}

void Reset(void){
    IO_RC3_SetDigitalOutput();
    IO_RC3_SetLow();
    __delay_ms(20);
    IO_RC3_SetDigitalInput();     //go high impedance to allow other methods of reset
}

void CDCRxService(){
    uint8_t GPBuffer[64];
   
    uint8_t numBytesIn = getsUSBUSART(GPBuffer, 64);
    
    for(uint8_t i = 0; i<numBytesIn; i++){
        CDCRxBuffer[i+CDCRxLength] = GPBuffer[i];
    }
    
    CDCRxLength += numBytesIn;

}

uint8_t readUSBUART(){
    uint8_t byteOut = CDCRxBuffer[0];         //store first bit
    
    for(uint8_t i = 0; i<CDCRxLength; i++){   //shifts up all other significant bits
        CDCRxBuffer[i] = CDCRxBuffer[i+1];
    }
    
    if(CDCRxLength){
        CDCRxLength--;
    }
    
    return byteOut;
}


/*
                         Main application
 */
void main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    CDCInitEP();
    
    uint8_t byte;
    static uint8_t writeBuffer[2];
    bool write = false;
    bool escape = false;
    bool enable_uart_to_usb = true; 
    
    while (1)
    {
       USBDeviceTasks();                                               //need to run every cycle for proper usb operation
       
       if( USBUSARTIsTxTrfReady()){                                    //check that USB is ready
           if(!write & (CDCRxLength > 0)) {                            //check if previous write is complete and data available
               byte = readUSBUART();
               
               if(escape){                 //if last bit was escape bit
                   escape = false;         //reset escape flag                           
                   switch(byte){
                       case 0x05:          //send escape bit
                           write = true;
                           break;
                       case 0xCC:          //disable 
                           enable_uart_to_usb = false;
                           break;
                       case 0xCD: 
                           enable_uart_to_usb = true;
                           break;
                       case 0xBB:         //reset PIC18                           
                           Reset();   //reset
                           
                           writeBuffer[0] = 0x01;
                           putUSBUSART(writeBuffer,1);   //acknowledge 
                           
                           while(EUSART_is_rx_ready()){  //clear messages received before reset
                               EUSART_Read(); 
                           }
                           
                           break;
                       case 0xAA:         //handshake, return 0x99
                           writeBuffer[0] = 0x99;
                           putUSBUSART(writeBuffer,1);
                           break;
                       default:           //unexpected, return 0xff 
                           writeBuffer[0] = 0xee;
                           putUSBUSART(writeBuffer,1);
                   }
                   
               } else if(byte == 0x05){
                   escape = true;
               } else {
                   write = true;
               }
           }
           
            if(EUSART_is_tx_ready() && write){             
                EUSART_Write(byte);
                write = false;
            }
            
            if(EUSART_is_rx_ready() && !escape && enable_uart_to_usb){  //if escaped, should not relay UART messages this cycle
                writeBuffer[0] = EUSART_Read();   
                putUSBUSART(writeBuffer,1);
            }
           
        }
       
       CDCRxService();
       CDCTxService();
    }  
}







