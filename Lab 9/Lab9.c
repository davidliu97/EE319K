// UARTTestMain.c
// Runs on LM3S1968
// Used to test the UART.c driver
// Daniel Valvano
// July 26, 2011

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to the Arm Cortex M3",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2011

 Copyright 2011 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

//#include <stdio.h>
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "UART1.h"
#include "queue.h"
#include "lm3s1968.h"
#include "Output.h"
#include "SysTick.h"
#include "adc.h"
#include "LCD.h"

#define OLED_Print
#define SELF_TEST

unsigned long Counter;


//---------------------OutCRLF---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void OutCRLF(void){
  UART1_OutChar(CR);
  UART1_OutChar(LF);
}

unsigned long Convert(unsigned long pre)
{ return 7*(pre)/4; }

void toFixed(unsigned char *str, unsigned long num)
{
	str[0] = '*';
	str[1] = 0x2E;
	str[2] = '*';
	str[3] = '*';
	str[4] = '*';
	
	if (num > 9999) return;
	str[0] = num /1000 + '0';
	num = num % 1000;
	str[2] = num /100 + '0';
	num = num % 100;
	str[3] = num /10 + '0';
	num = num %10;
	str[4] = num + '0';
}

void transmit(void)
{
	ADC_Init(2);
	SysTick_Init(2000000);
	while(1);
}

void recieve(void)
{
	#ifdef SELF_TEST
	ADC_Init(2);
	SysTick_Init(10000000);
	#endif
	
	LCD_Open();
	LCD_Clear();
	while(1)
	{
		unsigned char a = 0x00, data[6] = {0}, i;
		while (a != 0x02)
			Fifo_Get(&a);
		for (i = 0; i < 5; i++)
		{
			while (Fifo_Get(&a) == 0);
			data[i] = a;
		}
		data[5] = 0;
		LCD_OutString(&data[0]);
		LCD_OutString("cm");
		LCD_GoTo(0x00);
		#ifdef OLED_Print
		printf("\n%s\n",&data[0]);
		#endif
	}
}

int main(void){
	unsigned char i;
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                 SYSCTL_XTAL_8MHZ);
	errorCount = 0;
  UART1_Init();
	Output_Init();
	Output_Clear();
	printf("Lab 9\rOmar Medjaouri &\rKevin George\r\r");
	#ifdef OLED_Print
	printf("Printing read/send:\r");
	printf("Read:     Send:");
	#endif
  //OutCRLF();
	printf("\r\n");
	//transmit();
	recieve();
}

void SysTick_Handler(void)
{
	unsigned char message[9];
	int i;
	unsigned long ADC_Data;
	
	GPIO_PORTG_DATA_R ^= 0x04;
	
	ADC_Data = ADC_In();
	ADC_Data = Convert(ADC_Data);
	GPIO_PORTG_DATA_R ^= 0x04;
	
	message[0] = 0x02;
	message[6] = 0x0D;
	message[7] = 0x03;
	message[8] = 0x00;
	toFixed(&message[1], ADC_Data);
	
	for (i = 0; i < 8; i++)
		UART1_OutChar(message[i]);
	#ifdef OLED_Print
	printf("\n          %c.%c%c%c\n",message[1],message[3],message[4],message[5]);
	#endif
	Counter++;
	GPIO_PORTG_DATA_R ^= 0x04;
}
