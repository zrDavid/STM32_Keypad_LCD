#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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

uint8_t numberColumns = 0;
uint8_t numberRows = 0;
bool lineOverflow = false; // Flag to indicate if 20th character written on 4th line.

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
		for (j = 0; j < 4000; j++); // Assuming 8MHz clock, 1ms = 8000 cycles
	}
}

void initializeGPIO(void)
{
	//1. Enable GPIOA, GPIOB, GPIOC
	*pRCC_AHB1ENR |= 0x07;

	//2. KEYPAD Section. PA4, PA5, PA6, PA7: INPUTS (COLUMNS)
	*pGPIOA_MODER &= ~(0xFF << 8);

	//3. KEYPAD Section. Internal Pull-Up resistors for COLUMNS PA4, PA5, PA6, PA7
	*pGPIOA_PUPDR |= (0x55 << 8);

	//4. LCD Section.
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

	sendCommand(0x38);
	sendCommand(0x08);		// Display on, cursor off
	//sendCommand(0x0F);	// Display on, cursor blinking. Initially it worked here. I grayed it out to test the line at the end of the sequence.
	sendCommand(0x01);		// Clear display
	sendCommand(0x06);		// Shift cursor right
	sendCommand(0x0F);		// Display on, cursor blinking. Testing with the line here to see if now it works.
}

void writeCharacterInLCD(char data)
{
	*pGPIOC_BSRR = 0x00000100;	// Set RS (PC8). This is data
	*pGPIOC_BSRR = 0x02000000;	// Clear RW (PC9). Write data into LCD
	*pGPIOC_ODR = (*pGPIOC_ODR & 0xFFFFFF00) | (data & 0xFF);			// Send data
	*pGPIOC_BSRR = 0x00000400;	// Set EN
	delay(1);					// changed from strict 1ms to 2ms to include all other commands that take longer
	*pGPIOC_BSRR = 0x04000000;	// Clear EN
	delay(1);
}

void setCursor(uint8_t col, uint8_t row)
{
	uint8_t address = 0x00;	// Default address

	// Set the cursor position based on the current row and column
	switch (row) {
		case 0:
			address = (0x00 + col);	// Set cursor position to the first line
			break;
		case 1:
			address = (0x40 + col);	// Set cursor position to the second line
			break;
		case 2:
			address = (0x14 + col);	// Set cursor position to the third line
			break;
		case 3:
			address = (0x54 + col);	// Set cursor position to the fourth line
			break;
		default:
			address = (0x00);	// Reset to first line if out of bounds
			break;
	}

	sendCommand(0x80 | address);	// Set cursor position command
}

void nextCursorPosition(void)
{
	numberColumns++;
	if (numberColumns >= 20) {	// Reset column count if it exceeds 15
		numberColumns = 0;
		numberRows++;
		if (numberRows >= 4) {	// Reset row count if it exceeds 3
			lineOverflow = true;	// Set overflow flag
		}
	}
	setCursor(numberColumns, numberRows);
}

void lineOverflowCheck(void)
{
	if (lineOverflow) {
		// Clear the display and reset the cursor position
		sendCommand(0x01);	// Clear display
		delay(1);
		numberColumns = 0;	// Reset column count
		numberRows = 0;		// Reset row count
		lineOverflow = false; // Reset overflow flag
		setCursor(numberColumns, numberRows); // Set cursor to 0,0 position
	}
}

void scanButtons(void)
{
	delayDebounce();	// Delay needed to avoid the red buttons from detecting multiple strokes
			// Scan columns when ROW 1 is LOW
			*pGPIOB_MODER &= ~(0xF0 << 8);	// Reset bits PB4-PB7
			*pGPIOB_MODER |= (0x55 << 8);	// PB4-PB7 as Outputs (ROWS).
			*pGPIOB_BSRR |= 0x000000F0;		// PB4-PB7 set to high
			*pGPIOB_MODER &= ~(0xFF << 8);	// Disable all outputs PB4-PB7
			*pGPIOB_MODER |= (1 << 8);		// Enable only PB4
			*pGPIOB_BSRR |= (0x0F << 4);	// First all outputs to 1
			*pGPIOB_BSRR = (0x01 << 20);	// then, only PB4 = 0

			// COLUMN 1
			if (!(*pGPIOA_IDR & (0x0001 << 4))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 4))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('1');
					nextCursorPosition();
				}
			}

			// COLUMN 2
			if (!(*pGPIOA_IDR & (0x0001 << 5))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 5))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('2');
					nextCursorPosition();
				}
			}

			// COLUMN 3
			if (!(*pGPIOA_IDR & (0x0001 << 6))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 6))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('3');
					nextCursorPosition();

				}
			}

			// COLUMN 4
			if (!(*pGPIOA_IDR & (0x0001 << 7))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 7))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('A');
					nextCursorPosition();
				}
			}

			// Scan columns when ROW 2 is LOW
			*pGPIOB_MODER |= (0x55 << 8);	// PB4-PB7 as Outputs (ROWS).
			*pGPIOB_BSRR |= 0x000000F0;		// PB4-PB7 set to high
			*pGPIOB_MODER &= ~(0xFF << 8);	// Disable all outputs PB4-PB7
			*pGPIOB_MODER |= (1 << 10);	// Enable only PB5
			*pGPIOB_BSRR |= (0xF << 4);		// First all outputs to 1
			*pGPIOB_BSRR = (0x01 << 21);	// then, only PB5 = 0

			// COLUMN 1
			if (!(*pGPIOA_IDR & (0x0001 << 4))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 4))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('4');
					nextCursorPosition();
				}
			}

			// COLUMN 2
			if (!(*pGPIOA_IDR & (0x0001 << 5))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 5))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('5');
					nextCursorPosition();
				}
			}

			// COLUMN 3
			if (!(*pGPIOA_IDR & (0x0001 << 6))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 6))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('6');
					nextCursorPosition();
				}
			}

			// COLUMN 4
			if (!(*pGPIOA_IDR & (0x0001 << 7))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 7))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('B');
					nextCursorPosition();
				}
			}

			// Scan columns when ROW 3 is LOW
			*pGPIOB_MODER |= (0x55 << 8);	// PB4-PB7 as Outputs (ROWS).
			*pGPIOB_BSRR |= 0x000000F0;		// PB4-PB7 set to high
			*pGPIOB_MODER &= ~(0xFF << 8);	// Disable all outputs PB4-PB7
			*pGPIOB_MODER |= (1 << 12);		// Enable only PB6
			*pGPIOB_BSRR |= (0xF << 4);		// First all outputs to 1
			*pGPIOB_BSRR = (0x01 << 22);	// then, only PB6 = 0

			// COLUMN 1
			if (!(*pGPIOA_IDR & (0x0001 << 4))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 4))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('7');
					nextCursorPosition();
				}
			}

			// COLUMN 2
			if (!(*pGPIOA_IDR & (0x0001 << 5))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 5))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('8');
					nextCursorPosition();
				}
			}

			// COLUMN 3
			if (!(*pGPIOA_IDR & (0x0001 << 6))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 6))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('9');
					nextCursorPosition();
				}
			}

			// COLUMN 4
			if (!(*pGPIOA_IDR & (0x0001 << 7))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 7))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('C');
					nextCursorPosition();
				}
			}

			// Scan columns when ROW 4 is LOW
			*pGPIOB_MODER |= (0x55 << 8);	// PB4-PB7 as Outputs (ROWS).
			*pGPIOB_BSRR |= 0x000000F0;		// PB4-PB7 set to high
			*pGPIOB_MODER &= ~(0xFF << 8);	// Disable all outputs PB4-PB7
			*pGPIOB_MODER |= (1 << 14);		// Enable only PB7
			*pGPIOB_BSRR |= (0xF << 4);		// First all outputs to 1
			*pGPIOB_BSRR = (0x01 << 23);	// then only PB7 = 0

			// COLUMN 1
			if (!(*pGPIOA_IDR & (0x0001 << 4))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 4))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('*');
					nextCursorPosition();
				}
			}

			// COLUMN 2
			if (!(*pGPIOA_IDR & (0x0001 << 5))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 5))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('0');
					nextCursorPosition();
				}
			}

			// COLUMN 3
			if (!(*pGPIOA_IDR & (0x0001 << 6))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 6))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('#');
					nextCursorPosition();
				}
			}

			// COLUMN 4
			if (!(*pGPIOA_IDR & (0x0001 << 7))){
				delayDebounce();
				if (!(*pGPIOA_IDR & (0x0001 << 7))){
					lineOverflowCheck(); // Check if line overflow occurred
					writeCharacterInLCD('D');
					nextCursorPosition();
				}
			}
}

int main(void)
{
	initializeGPIO();
	initializeLCD();
	sendCommand(0x80);	// Clear display once at the beginning
	delay(1);	// Wait for 1ms

	while(1){
		// Scan buttons and print the pressed key
		scanButtons();
	}

    return 0;
}
