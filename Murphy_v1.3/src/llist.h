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
//	Data type declaration

	typedef struct note note;
	struct note
	{
	    uint8_t midi_note;
	    uint8_t velocity;

	    struct note *nxt;
	};

	typedef note* llist;

// Functions prototypes

	llist add_note_first(llist, uint8_t, uint8_t);
	llist add_note_last(llist, uint8_t, uint8_t);
	llist delete_note(llist, uint8_t);
	void  print_all_note(llist);
	note* get_last_note(llist);

