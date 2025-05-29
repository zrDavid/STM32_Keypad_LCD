#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//#if !defined(__SOFT_FP__) && defined(__ARM_FP)
//  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
//#endif

//General configuration
uint32_t volatile *pRCC_AHB1ENR	= (uint32_t*)0x40023830;	// Peripheral clock enable register
uint32_t volatile *pGPIOA_MODER	= (uint32_t*)0x40020000;
uint32_t volatile *pGPIOB_MODER	= (uint32_t*)0x40020400;
uint32_t volatile *pGPIOC_MODER	= (uint32_t*)0x40020800;	// Mode register
uint32_t volatile *pGPIOC_ODR	= (uint32_t*)0x40020814;	// Output data register
uint32_t volatile *pGPIOC_BSRR	= (uint32_t*)0x40020818;
uint32_t volatile *pGPIOA_IDR 	= (uint32_t*)0x40020010;
uint32_t volatile *pGPIOA_PUPDR = (uint32_t*)0x4002000C;
uint32_t volatile *pGPIOB_BSRR	= (uint32_t*)0x40020418;

void delayDebounce(){
	for(uint32_t i=0; i<100000; i++);
}

//Later use timers to get delays.
void delay(uint32_t ms)
{
	uint32_t i;
	for (i = 0; i < ms; i++)
	{
		volatile uint32_t j;
		for (j = 0; j < 8000; j++); // Assuming 8MHz clock, 1ms = 8000 cycles
	}
}

void initializeGPIO(void)
{
	*pRCC_AHB1ENR |= 0x07;	// Enable GPIOA, GPIOB, GPIOC
	// PC0-PC7 as data output.
	// PC8: RS
	// PC9: RW
	// PC10: EN
	*pGPIOC_MODER &= 0x00000000;	// Clear Port C
	*pGPIOC_MODER |= 0x155555;		// Set PC0-PC10 as output
	*pGPIOC_BSRR = 0x04000000;		// Clear EN
}

void sendCommand(int command)
{
	*pGPIOC_BSRR = 0x01000000;	// Clear RS. This is a command
	*pGPIOC_ODR = (*pGPIOC_ODR & 0xFFFFFF00) | (command & 0xFF);	// Send command
	*pGPIOC_BSRR = 0x00000400;	// Set EN
	delay(1);					// changed from strict 1ms to 2ms to include all other commands that take longer
	*pGPIOC_BSRR = 0x04000000;	// Clear EN
	delay(1);
}

void initializeLCD()
{
	delay(50);	// Wait for 20ms
	sendCommand(0x38);
	delay(5);	// Wait for 5ms
	sendCommand(0x38);
	delay(1);	// Wait for 1ms
	sendCommand(0x38);
	delay(1);	// Wait for 1ms

	sendCommand(0x38);
	sendCommand(0x08);	// Display on, cursor off
	//sendCommand(0x0F);	// Display on, cursor blinking. Initially it worked here. I grayed it out to test the line at the end of the sequence.
	sendCommand(0x01);	// Clear display
	sendCommand(0x06);	// Shift cursor right
	sendCommand(0x0F);	// Display on, cursor blinking. Testing with the line here to see if now it works.
}

void writeCharacterInLCD(char data)
{
	*pGPIOC_BSRR = 0x00000100;	// Set RS. This is data
	*pGPIOC_BSRR = 0x02000000;	// Clear RW. Write data into LCD
	*pGPIOC_ODR = (*pGPIOC_ODR & 0xFFFFFF00) | (data & 0xFF);			// Send data
	*pGPIOC_BSRR = 0x00000400;	// Set EN
	delay(2);					// changed from strict 1ms to 2ms to include all other commands that take longer
	*pGPIOC_BSRR = 0x04000000;	// Clear EN
	delay(2);
}


int main(void)
{
	// This comes from Keypad main code. This is keypad initialization
	//1. Enable the peripheral clock for PA and PB and PC
		*pRCC_AHB1ENR |= 0x07;

		//2. PA4, PA5, PA6, PA7: INPUTS (COLUMNS)
		*pGPIOA_MODER &= ~(0xFF << 8);

		//3. Internal Pull-Up resistors for COLUMNS PA4, PA5, PA6, PA7
		*pGPIOA_PUPDR |= (0x55 << 8);

		//4. Configure external LED to blink
		*pGPIOC_MODER |= (0x01 << 14);	// PC7 as output
	// Finished keypad initialization


	//	*pRCC_AHB1ENR |= 0x07;
	//	*pGPIOC_MODER |= 0x5555;	// PC as output
	initializeGPIO();
	initializeLCD();

	while(1){

		sendCommand(0x80);	// Clear display
		delay(1);	// Wait for 1ms
		writeCharacterInLCD('R');
		writeCharacterInLCD('o');
		writeCharacterInLCD('w');
		writeCharacterInLCD(' ');
		writeCharacterInLCD('0');
		writeCharacterInLCD('1');
		delay(500);

		sendCommand(0xC0);	// Clear display
		delay(1);	// Wait for 1ms
		writeCharacterInLCD('R');
		writeCharacterInLCD('o');
		writeCharacterInLCD('w');
		writeCharacterInLCD(' ');
		writeCharacterInLCD('0');
		writeCharacterInLCD('2');
		delay(500);

		sendCommand(0x94);	// Clear display
		delay(1);	// Wait for 1ms
		writeCharacterInLCD('R');
		writeCharacterInLCD('o');
		writeCharacterInLCD('w');
		writeCharacterInLCD(' ');
		writeCharacterInLCD('0');
		writeCharacterInLCD('3');
		delay(500);

		sendCommand(0xD4);	// Clear display
		delay(1);	// Wait for 1ms
		writeCharacterInLCD('R');
		writeCharacterInLCD('o');
		writeCharacterInLCD('w');
		writeCharacterInLCD(' ');
		writeCharacterInLCD('0');
		writeCharacterInLCD('4');
		delay(500);

		sendCommand(0x01);	// Clear display
		delay(2);	// Wait for 2ms
		sendCommand(0x02);
		delay(2);	// Wait for 2ms
	}

    return 0;
}
