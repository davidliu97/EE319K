// UART.c
// Runs on LM3S811, LM3S1968, LM3S8962
// Simple device driver for the UART.
// Daniel Valvano
// July 26, 2011
// Modified by EE345L students Charlie Gough && Matt Hawk
// Modified by EE345M students Agustinus Darmawan && Mingjie Qiu

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to the Arm Cortex M3",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2011
   Program 4.12, Section 4.9.4, Figures 4.26 and 4.40

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

#include "UART1.h"
#include "queue.h"
#include "lm3s1968.h"

unsigned long errorCount;

//------------UART_Init------------
// Wait for new serial port input
// Initialize the UART for 115,200 baud rate (assuming 50 MHz UART clock),
// 8 bit word length, no parity bits, one stop bit, FIFOs enabled
// Input: none
// Output: none
void UART1_Init(void){
	Fifo_Init();
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_UART1; // activate UART0
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOG; // activate port G
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD; // activate port D
	
  UART1_CTL_R &= ~UART_CTL_UARTEN;      // disable UART
  UART1_IBRD_R = 31;                    // IBRD = int(50,000,000 / (16 * 100,000)) = int(31.25)
  UART1_FBRD_R = 16;                     // FBRD = int(0.25 * 64 + 0.5) = 16
                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART1_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);
	
	UART1_IFLS_R = UART_IFLS_RX4_8;
	UART1_IM_R |= UART_IM_RXIM;
	NVIC_PRI1_R = (NVIC_PRI1_R & 0xFF1FFFFF) | 0x00200000;
	NVIC_EN0_R |= NVIC_EN0_INT6;
	
  UART1_CTL_R |= UART_CTL_UARTEN;       // enable UART
  GPIO_PORTD_AFSEL_R |= 0x0C;           // enable alt funct on PD3-2
  GPIO_PORTD_DEN_R |= 0x0C;             // enable digital I/O on PD3-2
	
  GPIO_PORTG_DIR_R |= 0x04;   // make PG2 out
	GPIO_PORTG_AFSEL_R &= ~0x04;
	GPIO_PORTG_PUR_R |= 0x04;
  GPIO_PORTG_DEN_R |= 0x04;   // enable digital I/O on PG2
	GPIO_PORTG_DATA_R |= 0x04;
	
	EnableInterrupts();
}

void UART1_Handler(void)
{
	char dat;
	//GPIO_PORTG_DATA_R ^= 0x04;
	while ((UART1_FR_R&UART_FR_RXFE) == 0)
	{
		dat = UART1_DR_R;
		if(!Fifo_Put(dat))
		{
			errorCount++;
			break;
		}
	}
	UART1_ICR_R = UART_ICR_RXIC;
	GPIO_PORTG_DATA_R ^= 0x04;
}

//------------UART_InChar------------
// Wait for new serial port input
// Input: none
// Output: ASCII code for key typed
unsigned char UART1_InChar(void){
	unsigned char dat;
	while(Fifo_Get(&dat) == 0);
	return dat;
}
//------------UART_OutChar------------
// Output 8-bit to serial port
// Input: letter is an 8-bit ASCII character to be transferred
// Output: none
void UART1_OutChar(unsigned char data){
  while((UART1_FR_R&UART_FR_TXFF) != 0);
  UART1_DR_R = data;
}


//------------UART_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART1_OutString(char *pt){
  while(*pt){
    UART1_OutChar(*pt);
    pt++;
  }
}

//------------UART_InUDec------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 32-bit unsigned number
//     valid range is 0 to 4294967295 (2^32-1)
// Input: none
// Output: 32-bit unsigned number
// If you enter a number above 4294967295, it will return an incorrect value
// Backspace will remove last digit typed
unsigned long UART1_InUDec(void){
unsigned long number=0, length=0;
char character;
  character = UART1_InChar();
  while(character != CR){ // accepts until <enter> is typed
// The next line checks that the input is a digit, 0-9.
// If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9')) {
      number = 10*number+(character-'0');   // this line overflows if above 4294967295
      length++;
      UART1_OutChar(character);
    }
// If the input is a backspace, then the return number is
// changed and a backspace is outputted to the screen
    else if((character==BS) && length){
      number /= 10;
      length--;
      UART1_OutChar(character);
    }
    character = UART1_InChar();
  }
  return number;
}

//-----------------------UART_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART1_OutUDec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  if(n >= 10){
    UART1_OutUDec(n/10);
    n = n%10;
  }
  UART1_OutChar(n+'0'); /* n is between 0 and 9 */
}

//---------------------UART_InUHex----------------------------------------
// Accepts ASCII input in unsigned hexadecimal (base 16) format
// Input: none
// Output: 32-bit unsigned number
// No '$' or '0x' need be entered, just the 1 to 8 hex digits
// It will convert lower case a-f to uppercase A-F
//     and converts to a 16 bit unsigned number
//     value range is 0 to FFFFFFFF
// If you enter a number above FFFFFFFF, it will return an incorrect value
// Backspace will remove last digit typed
unsigned long UART1_InUHex(void){
unsigned long number=0, digit, length=0;
char character;
  character = UART1_InChar();
  while(character != CR){
    digit = 0x10; // assume bad
    if((character>='0') && (character<='9')){
      digit = character-'0';
    }
    else if((character>='A') && (character<='F')){
      digit = (character-'A')+0xA;
    }
    else if((character>='a') && (character<='f')){
      digit = (character-'a')+0xA;
    }
// If the character is not 0-9 or A-F, it is ignored and not echoed
    if(digit <= 0xF){
      number = number*0x10+digit;
      length++;
      UART1_OutChar(character);
    }
// Backspace outputted and return value changed if a backspace is inputted
    else if((character==BS) && length){
      number /= 0x10;
      length--;
      UART1_OutChar(character);
    }
    character = UART1_InChar();
  }
  return number;
}

//--------------------------UART_OutUHex----------------------------
// Output a 32-bit number in unsigned hexadecimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1 to 8 digits with no space before or after
void UART1_OutUHex(unsigned long number){
// This function uses recursion to convert the number of
//   unspecified length as an ASCII string
  if(number >= 0x10){
    UART1_OutUHex(number/0x10);
    UART1_OutUHex(number%0x10);
  }
  else{
    if(number < 0xA){
      UART1_OutChar(number+'0');
     }
    else{
      UART1_OutChar((number-0x0A)+'A');
    }
  }
}

//------------UART_InString------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed
//    or until max length of the string is reached.
// It echoes each character as it is inputted.
// If a backspace is inputted, the string is modified
//    and the backspace is echoed
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer, size of buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void UART1_InString(char *bufPt, unsigned short max) {
int length=0;
char character;
  character = UART1_InChar();
  while(character != CR){
    if(character == BS){
      if(length){
        bufPt--;
        length--;
        UART1_OutChar(BS);
      }
    }
    else if(length < max){
      *bufPt = character;
      bufPt++;
      length++;
      UART1_OutChar(character);
    }
    character = UART1_InChar();
  }
  *bufPt = 0;
}
