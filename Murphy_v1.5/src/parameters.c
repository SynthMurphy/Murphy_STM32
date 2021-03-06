#include <stdio.h>
#include <stdbool.h>

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

#include "llist.h"

#include "parameters.h"
#include "synth.h"
#include "cc_lut.h"

extern llist 			note_list;						// Note list
extern synth_params		params;							// Synthesizer parameters

extern bool				new_note_event;



// Start PARAMETERS Input with USART1 and DMA2
// -------------------------------------

uint8_t* PARAMETERS_Start()
{
	static uint8_t PARAMETERS_buffer[PARAMETERS_BUFFER_LENGTH];

	GPIO_InitTypeDef  	GPIO_InitStructure;
	USART_InitTypeDef 	USART_InitStructure;
	NVIC_InitTypeDef 	NVIC_InitStructure;
	DMA_InitTypeDef  	DMA_InitStructure;

	// Start USART1 clock

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	// Start DMA1 Clock

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

	// Initialize TX & RX Pins (USART1 TX & RX on PA9 & PA10)

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// Connect the RX and TX pins to USART1 alternative function

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

	// Setup the properties of USART1

	USART_InitStructure.USART_BaudRate = 9600;												// MIDI baudrate (31250)
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;								// 8 bits data
	USART_InitStructure.USART_StopBits = USART_StopBits_1;									// 1 stop bit
	USART_InitStructure.USART_Parity = USART_Parity_No;										// No parity bit
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; 		// No flow control
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; 										// RX only is enabled
	USART_Init(USART1, &USART_InitStructure);

	// Enable the USART1 RX DMA Interrupt

	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// Configure the Priority Group to 2 bits

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	// finally this enables the complete USART1 peripheral

	USART_Cmd(USART1, ENABLE);

	// Configure DMA to store MIDI bytes directly into PARAMETERS_buffer

	DMA_DeInit(DMA2_Stream2);

	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)PARAMETERS_buffer;
	DMA_InitStructure.DMA_BufferSize = (uint16_t)sizeof(PARAMETERS_buffer);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

	DMA_Init(DMA2_Stream2, &DMA_InitStructure);

	// Enable the USART Rx DMA request

	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

	// Enable the DMA RX Stream

	DMA_Cmd(DMA2_Stream2, ENABLE);

	return (PARAMETERS_buffer);
}


uint8_t PARAMETERS_GetNbNewBytes(uint8_t* PARAMETERS_Buffer)
{
	static uint16_t 	dma_cpt_prev=PARAMETERS_BUFFER_LENGTH;
		uint16_t			dma_cpt, n=0;

		// Get current DMA counter

		dma_cpt = DMA_GetCurrDataCounter(DMA2_Stream2);

		// If DMA counter has changed, compute the number of received MIDI bytes

		if (dma_cpt != dma_cpt_prev)
		{
			if (dma_cpt < dma_cpt_prev)
			{
				n = dma_cpt_prev - dma_cpt;
			}
			else
			{
				n = dma_cpt_prev - (dma_cpt - PARAMETERS_BUFFER_LENGTH);
			}

			// Store the new DMA counter

			dma_cpt_prev = dma_cpt;
		}
		return(n);
}


volatile uint8_t TempStr[30];
static const long dectable[] = {
	[0 ... 255] = -1,
	['0'] = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
};

//Retorna o numero em decimal convertido pela string
//Retorna -1 em caso de erro
long decdec(unsigned const char *dec) {
	long ret = 0;
	while (*dec && ret >= 0) {
		ret *= 10;
		ret += dectable[*dec++];
	}
	return ret;
}

void PARAMETERS_parser(uint8_t* PARAMETERS_buffer, uint8_t nb_PARAMETERS_bytes)
{/*
	static uint8_t 	index = 0;
	static uint8_t	state = 0;

	uint8_t			PARAMETER_byte;

	// Process message
	uint8_t TempChar;
	uint8_t loop = 0;
	int16_t PARAM_TYPE = 0;
	int16_t PARAM_VALUE = 0;
	Parse_State =0;
	//printf ("parameters: 0x%x @index = %d    %d\n", nb_PARAMETERS_bytes, index, PARAMETER_byte);
	// Read a new byte from the MIDI buffer
	while((PARAMETER_byte = PARAMETERS_buffer[index])!= ';'   && (nb_PARAMETERS_bytes != 0 ))
	{

		//printf ("parameters: 0x%x @index = %d    %d  ->  %c\n", nb_PARAMETERS_bytes, index, PARAMETER_byte,PARAMETER_byte);
		Str_Received[loop] = PARAMETER_byte;
		loop ++;
		if (index == (PARAMETERS_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
		else index ++;
		nb_PARAMETERS_bytes--;
	}
	Str_Received[loop] = PARAMETER_byte;
	if (index == (PARAMETERS_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
			else index ++;
	Str_Received[loop+1] = '\0';
	//printf ("\n %s",Str_Received);
		//while(1)
	//	{
	if(Str_Read() == '*')
	{
		PARAM_TYPE = DecodeNumber(':');

		PARAM_VALUE = DecodeNumber(';');
		printf ("\n %d; %d",PARAM_TYPE,PARAM_VALUE);
		ChangeParam(PARAM_TYPE,PARAM_VALUE);
	}
		//}

*/
	static uint8_t 	index = 0;
	static uint8_t	state = 0;
	static uint8_t	state_number = 0;
	static uint8_t  PARAM_TYPE, PARAM_VALUE;

	uint8_t			PARAMETER_byte;

	uint16_t		PB_wheel;

	// Process message

	while (nb_PARAMETERS_bytes != 0)
	{
		// Read a new byte from the MIDI buffer

		PARAMETER_byte = PARAMETERS_buffer[index];
		// printf("\n\n %c --> %d ",PARAMETER_byte,PARAMETER_byte);
		switch (state)
		{

			// State 0 = Starting point for a new PARAMETER message

			case 0 :
			{
				switch (PARAMETER_byte )
				{

					case '#' :														// Note ON message
					{
						state = 1;													// Next state is 10
						state_number =0;
						// printf ("parameter # event\n");

						if (index == (PARAMETERS_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
						else index ++;
						nb_PARAMETERS_bytes--;

						break;
					}

					default :														// Other type of message, move to next byte but stays in state 0
					{
						if (index == (PARAMETERS_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
						else index ++;
						nb_PARAMETERS_bytes--;
						state = 0;
						break;
					}
				}


			}break;


			// State 1: ID_NUMBER

			case 1 :
			{
				if (PARAMETER_byte>= '0' && PARAMETER_byte<= '9')												// If the following byte is not a note number
				{
					state = 1;
					TempStr[state_number]= PARAMETER_byte; // Return to state 0 without moving to next byte
					state_number++;
				}else{
					state =0;
				}
				if (index == (PARAMETERS_BUFFER_LENGTH-1))
					index = 0;				// Move to next MIDI byte
				else
					index ++;
				nb_PARAMETERS_bytes--;
				if(state_number == 3)
				{
					TempStr[state_number]= '\0';
					PARAM_TYPE = decdec(TempStr);
					if(PARAM_TYPE == -1)
						state= 0;
					else
						state =2;
				}

			}break;

			case 2 :
			{
				switch (PARAMETER_byte )
				{
					case ':' :														// Note ON message
					{
						state = 3;													// Next state is 10
						state_number =0;
					//	 printf ("parameter : event\n");

						if (index == (PARAMETERS_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
						else index ++;
						nb_PARAMETERS_bytes--;

						break;
					}

					default :														// Other type of message, move to next byte but stays in state 0
					{
						if (index == (PARAMETERS_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
						else index ++;
						nb_PARAMETERS_bytes--;
						state = 0;
						break;
					}
				}


			}break;
			// State 3: VALUE_NUMBER

			case 3 :
			{
				if (PARAMETER_byte>= '0' && PARAMETER_byte<= '9')												// If the following byte is not a note number
				{
					state = 3;													// Return to state 0 without moving to next byte
					TempStr[state_number]= PARAMETER_byte;
					state_number++;
				}else{
					state =0;
				}
				if (index == (PARAMETERS_BUFFER_LENGTH-1))
					index = 0;				// Move to next MIDI byte
				else
					index ++;
				nb_PARAMETERS_bytes--;
				if(state_number == 3)
				{
					TempStr[state_number]= '\0';
					PARAM_VALUE = decdec(TempStr);
					if(PARAM_VALUE == -1)
						state= 0;
					else
						state =2;
					state =4;
				}

			}break;
			case 4 :
			{
				switch (PARAMETER_byte )
				{
					case ';' :														// Note ON message
					{
						state = 0;
					//	 printf ("parameter ; event\n");
						// printf ("\nCC %d; %d",PARAM_TYPE,PARAM_VALUE);
						 ChangeParam(PARAM_TYPE,PARAM_VALUE);
						if (index == (PARAMETERS_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
						else index ++;
						nb_PARAMETERS_bytes--;

						break;
					}

					default :														// Other type of message, move to next byte but stays in state 0
					{
						if (index == (PARAMETERS_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
						else index ++;
						nb_PARAMETERS_bytes--;
						state = 0;
						break;
					}
				}


			}break;


		}
	}

}

void ChangeParam(uint8_t param_number, uint8_t param_value)
{
	switch (param_number)
	{
		case 1 :								// Button A1 -> Browse OSC1 Waveforms
		{
			if(param_value>=0 && param_value<5)
				params.osc1_waveform = param_value;
		//	printf ("Osc1 waveform set to %d\n", params.osc1_waveform);
			break;
		}

		case 2 :								// Button A2 -> Browse OSC2 Waveforms
		{
			if(param_value>=0 && param_value<5)
				params.osc2_waveform = param_value;
		//	printf ("Osc2 waveform set to %d\n", params.osc2_waveform);
			break;
		}

		case 3 :								// Button A2 -> Browse OSC2 Waveforms
		{
			//0->1
		    // 1->2
		    // 2->4
		    // -1->0.5
				if (params.osc1_octave == 5)
					params.osc1_octave = 0.5f;
				else
					params.osc1_octave = (float_t) param_value;
				break;
		}

		case 4 :								// Button A2 -> Browse OSC2 Waveforms
		{
			if (params.osc2_octave == 5)
				params.osc2_octave = 0.5f;
			else
				params.osc2_octave = (float_t) param_value;
			break;
		}

		case 5 :								// OSC1 mix level
		{
			params.osc1_mix = (float_t) param_value / 100;
			break;
		}

		case 6 :								// OSC2 mix level
		{
			params.osc2_mix = (float_t) param_value / 100;
			break;
		}

		case 7 :								// Filter cutoff
		{
			params.cutoff = cutoff_CC[param_value];
			//params.cutoff = (float_t) 1.0f + (200.0f * param_value / 127);
			break;
		}

		case 8 :								// Filter resonance
		{
			params.reso = 4.0f * (float_t) param_value / 100;
			break;
		}

		case 9 :								// ADRS Attack Time
		{
			params.adsr1_attack = 0.001f + (float_t) param_value / 100;
			break;
		}

		case 10 :								// ADRS Decay Time
		{
			params.adsr1_decay = 0.001f + (float_t) param_value / 100;
			break;
		}

		case 11 :								// ADRS Sustain Level
		{
			params.adsr1_sustain = (float_t) param_value / 100;
			break;
		}

		case 12 :								// ADRS Release Time
		{
			params.adsr1_release = 0.001f + (float_t) param_value / 100;
			break;
		}


		case 13 :								// ADRS Attack Time
		{
			params.adsr2_attack = 0.001f + (float_t) param_value / 100;
			break;
		}

		case 14 :								// ADRS Decay Time
		{
			params.adsr2_decay = 0.001f + (float_t) param_value / 100;
			break;
		}

		case 15 :								// ADRS Sustain Level
		{
			params.adsr2_sustain = (float_t) param_value / 100;
			break;
		}

		case 16 :								// ADRS Release Time
		{
			params.adsr2_release = 0.001f + (float_t) param_value / 100;
			break;
		}

		case 17 :								// lfo1_frequency
		{
			params.lfo1_frequency = 10.0f * (float_t) param_value / 100;
			break;
		}

		case 18 :								// lfo1_depth
		{
			params.lfo1_depth = 1.0f * (float_t) param_value / 100;
			break;
		}

		case 19 :								// lfo2_frequency
		{
			params.lfo2_frequency = 10.0f * (float_t) param_value / 100;
			break;
		}

		case 20 :								// lfo2_depth
		{
			params.lfo2_depth = 0.1f * (float_t) param_value / 100;
			break;
		}

		case 21 :								// detune osc 2
				{
					params.detune = 1.0f + 0.0004f*(param_value-64);
					break;
				}


	}

}



