/*
 * Transmit_to_display.c
 *
 * Created: 2/20/2023 8:21:54 AM
 * Author : Uma Sivadasan//
 */ 

#define F_CPU 4000000
#define USART2_BAUD_RATE(BAUD_RATE) ((float)(4000000 * 64 / (16 * (float) BAUD_RATE)) + 0.5) //defining what the baud rate value should be

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#define period 16
uint16_t count_millis = 0;

#include "controller.h"

//********Headers to implement the circular buffer*****************
#define BUFFER_SIZE 16 //size of the buffer
#define USART_RX_BUFFER_MASK (BUFFER_SIZE -1)
#if (BUFFER_SIZE & USART_RX_BUFFER_MASK)
#error RX buffer size is not a power of 2
#endif

static uint8_t RX_tail; //tail for the buffer
static uint8_t RX_head; //head for the buffer
static unsigned char RX_buffer[BUFFER_SIZE]; //defining the buffer array
volatile uint8_t count = 0; //count for the buffer

//1. set the baud rate to 9600                                      ---->>> done using #define
//2.  set the frame format and mode of operation by using (USARTn.CTRLC) and use 8N1 as the single stop bit
//3. configure the TXD pin as the output                            --> done in the main loop
//4. enable the transmitter by using (USARTn.CTRLB). 
//PF4 is the TX for USART2

void (*writeFnc)(uint8_t); //function pointer to transmit characters
uint8_t(*readFnc)(void);//function pointer to receive transmitted characters
uint32_t (* millisFnc)(void); //function to setup the millisecond function
uint8_t (* rxCountFnc)(void); //function to setup wait_for_acknowledge function

void tca_initialize(void)  //function to initialize timer counter
{
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	TCA0.SINGLE.CTRLB  = TCA_SINGLE_WGMODE_NORMAL_gc;
	TCA0.SINGLE.PER = period;
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV256_gc |TCA_SINGLE_ENABLE_bm;
}

void writeFnc_x(uint8_t c)
{
	while(!(USART2.STATUS & USART_DREIF_bm)) //bit mask for interrupts
{
	//waiting for the DATA register is empty
}	
	USART2.TXDATAL = c; //if the register is empty, then we can send the character
	//return 0; //this statement had to be added for the error "warning:control reaches end of non-void function" to be rid of
}
uint8_t readFnc_x()
{
	while(!(USART2.STATUS & USART_RXCIF_bm)) //bit mask for interrupts to receive
	{
		//waiting to receive data
	}
	return USART2.RXDATAL; //if the register is empty, then we can read in the character
}
	uint32_t millisFnc_x()
{	
	 while(count_millis<500) //waiting for the timer to reach 500 ms
	 {
		 //do nothing??
	 }
	// if(count==500)
	// {
		//return count;
	// }
	return count_millis; //returns the count in milliseconds 
}

void RX_initial() //initializing the receiver
{
	PORTF.DIR &= ~PIN5_bm; //setting PORTF5 as an input (RX) for USART2
	//PORTMUX.USARTROUTEA |= PORTMUX_USART20_bm;
	USART2.CTRLB |= (USART_RXEN_bm); //enabling the receiver (RX) as the output
	USART2.CTRLA |= (USART_RXCIE_bm); //enable interrupt
}

//********ISR for Circular Buffer**************//

ISR(USART2_RXC_vect) //ISR for the circular buffer
{
	uint8_t data;
	uint8_t temphead;
	
	cli();//clear the interrupt
	
	data = USART2.RXDATAL; //read the data
	
	temphead = (RX_head+1) & USART_RX_BUFFER_MASK; //incrementing the head after receiving data
	if(temphead == RX_tail)
	{
		//if head and tail are equal, we have reached the end of the array
	}
	RX_buffer[temphead] = data; //if the temp head doesn't equal the tail (array is not empty) store data at head
	RX_head = temphead;
	count++; //increment the count in terms of head.
	sei();//enable global interrupts
} 

//*************Circular Buffer Implementation************************//
uint8_t rxCountFnc_x(void)
	{	
		uint8_t data = 0;	
		while(0==count)
		{
			//if the count is 0, we have reached the end of the array
		}
		data = RX_buffer[RX_tail++]; //increment the location of the tail 
		if(sizeof(RX_buffer)<=RX_tail)
		{
			RX_tail = 0; //if the tail is greater than the array size, move the tail pointer to the start of the array
		}
		count--; //decrement the count 
		return data; //returning the data located at the tail (implementing FIFO)
	}

int main(void)
{
	PORTF.DIR |= PIN4_bm; //setting PORTF4 as an output (TX) for USART2
	PORTMUX.USARTROUTEA |= PORTMUX_USART20_bm;
	RX_initial(); //setting up receiver 
	
	USART2.BAUD = (uint16_t)USART2_BAUD_RATE(9600); //initializing the baud rate. 
	
	USART2.CTRLB |= USART_TXEN_bm; //enabling the transmitter (TX) as the output
	
	USART2_CTRLC = USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc | USART_SBMODE_1BIT_gc; //8 bits, no parity, 1 stop bit
	
	 tca_initialize(); //initialize timer counter
	 sei(); //enable global interrupt
	
	writeFnc = writeFnc_x;
	readFnc = readFnc_x;
	millisFnc = millisFnc_x;
	rxCountFnc = rxCountFnc_x;
	
	mates_attachWriteFnc(writeFnc);
	mates_attachReadFnc(readFnc);
	mates_attachMillisFnc(millisFnc); 
	mates_attachRxCountFnc(rxCountFnc);
	
 // uint8_t c = 'S'; //sending out letter S
   uint16_t index = 0;
   char str[] = "ABC";
   const char *str_display = str;

//	mates_setPage(0); //choose page 0 -->this function works
    while (1) 
    {
		mates_updateDotMatrix(index, str_display);
		 _delay_ms(500);//delay 500 ms before sending the following data bit
	}
return 0;
}


//mates_setPage(1)

/*
		Notes:
	1. if I am calling the delay interrupt, how and where do I put sei() and cli() to enable and clear the interrupt. 
	2. Naming the ISR and having the pre-defined function call the interrupt.
*/