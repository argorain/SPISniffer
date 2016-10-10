/*
MIT License

Copyright (c) 2016 Vojtech Vladyka

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stddef.h>
#include <stdio.h>

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART */
	while(!USART_GetFlagStatus(USART1, USART_FLAG_TXE));
	USART_SendData(USART1, ch);

	return ch;
}
/* USER CODE END PFP */


USART_InitTypeDef USART_InitStructure;

#define SPIBUFFER 32 // How many bytes is going to be captured
uint8_t SPIReceivedValue[SPIBUFFER]; // Capture array


void DMA1_Channel4_5_IRQHandler() { // When data are complete, transmit
	if(DMA_GetITStatus(DMA1_IT_TC4) == SET) {
		DMA_ClearITPendingBit(DMA1_IT_TC4);

		DMA_Cmd(DMA1_Channel4, DISABLE);

		uint16_t datacount = DMA_GetCurrDataCounter(DMA1_Channel4);
		for(int i=0; i<datacount; i++) {
			printf("%x ", SPIReceivedValue[i]);
		}
		printf("\r\n");

		DMA_Cmd(DMA1_Channel4, ENABLE);
		DMA_SetCurrDataCounter(DMA1_Channel4, 0);

	}
}

/**
 **===========================================================================
 **
 **  Abstract: main program
 **
 **===========================================================================
 */


int main(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_1); //UART Tx

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_0); //SPI SCK
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_0); //SPI MISO
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_0); //SPI NSS

	GPIO_InitTypeDef gpiocfg;
	gpiocfg.GPIO_Speed=GPIO_Speed_50MHz;

	gpiocfg.GPIO_Mode = GPIO_Mode_AF; //UART Debug TX
	gpiocfg.GPIO_Pin = GPIO_Pin_9;
	gpiocfg.GPIO_PuPd = GPIO_PuPd_NOPULL;
	gpiocfg.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOA, &gpiocfg);

	gpiocfg.GPIO_Mode = GPIO_Mode_AF; //MISO
	gpiocfg.GPIO_Pin = GPIO_Pin_14;
	gpiocfg.GPIO_PuPd = GPIO_PuPd_DOWN;
	gpiocfg.GPIO_OType = GPIO_OType_OD;
	GPIO_Init(GPIOB, &gpiocfg);

	gpiocfg.GPIO_Mode = GPIO_Mode_AF; //SCK
	gpiocfg.GPIO_Pin = GPIO_Pin_13;
	gpiocfg.GPIO_PuPd = GPIO_PuPd_UP;
	gpiocfg.GPIO_OType = GPIO_OType_OD;
	GPIO_Init(GPIOB, &gpiocfg);

	gpiocfg.GPIO_Mode = GPIO_Mode_AF; //NSS
	gpiocfg.GPIO_Pin = GPIO_Pin_12;
	gpiocfg.GPIO_PuPd = GPIO_PuPd_UP;
	gpiocfg.GPIO_OType = GPIO_OType_OD;
	GPIO_Init(GPIOB, &gpiocfg);



	// Debug
	USART_InitTypeDef uartcfg;
	uartcfg.USART_BaudRate = 115200;
	uartcfg.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uartcfg.USART_Mode = USART_Mode_Tx;
	uartcfg.USART_Parity = USART_Parity_No;
	uartcfg.USART_StopBits = USART_StopBits_1;
	uartcfg.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &uartcfg);
	USART_Cmd(USART1, ENABLE);

	// Sniffing SPI interface
	SPI_InitTypeDef spicfg;
	spicfg.SPI_Direction = SPI_Direction_1Line_Rx;
	spicfg.SPI_Mode = SPI_Mode_Slave;
	spicfg.SPI_DataSize = SPI_DataSize_8b;
	spicfg.SPI_CPOL = SPI_CPOL_Low; // Change this according your application
	spicfg.SPI_CPHA = SPI_CPHA_1Edge; // Change this according your application
	spicfg.SPI_NSS = SPI_NSS_Hard;
	spicfg.SPI_FirstBit = SPI_FirstBit_MSB; // Change this according your application
	spicfg.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_Init(SPI2, &spicfg);

	//Enable DMA1 clock--
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_InitTypeDef DMA_InitStructure;
	//==Configure DMA1 - Channel4== (SPI -> memory)
	DMA_DeInit(DMA1_Channel4); //Set DMA registers to default values
	DMA_StructInit(&DMA_InitStructure);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR; //Address of peripheral the DMA must map to
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&SPIReceivedValue[0]; //Variable to which received data will be stored
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = SPIBUFFER; //Buffer size
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel4, &DMA_InitStructure); //Initialise the DMA
	DMA_Cmd(DMA1_Channel4, ENABLE); //Enable the DMA1 - Channel4

	NVIC_InitTypeDef nvicStructure;
	//Enable DMA1 channel IRQ Channel
	nvicStructure.NVIC_IRQChannel = DMA1_Channel4_5_IRQn;
	nvicStructure.NVIC_IRQChannelPriority = 0;
	nvicStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvicStructure);

	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx, ENABLE); // We want IRQ when Rx is done
	SPI_Cmd(SPI2, ENABLE);


	printf("SPI Sniffer\r\n");
	/* Infinite loop */
	while (1)
	{
		__asm("WFI");
	}
	return 0;
}

/*
 * Minimal __assert_func used by the assert() macro
 * */
void __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
	while(1)
	{}
}

/*
 * Minimal __assert() uses __assert__func()
 * */
void __assert(const char *file, int line, const char *failedexpr)
{
	__assert_func (file, line, NULL, failedexpr);
}
