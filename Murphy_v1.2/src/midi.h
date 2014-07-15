#define MIDI_BUFFER_LENGTH		64


#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_dac.h"

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"

#include "stm32f4xx_rcc.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_dac.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_spi.h"
// Start MIDI interface and return a pointer to the MIDI buffer
uint8_t* MIDI_Start (void);

// Returns the number of new bytes in the MIDI buffer

uint8_t	MIDI_GetNbNewBytes (uint8_t*);

// Parse MIDI bytes

void MIDI_parser (uint8_t*, uint8_t);

void ChangeParam(uint8_t, uint8_t);
