#include <math.h>
#include <stdbool.h>

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

#include "synth.h"
//	Data type declaration
extern synth_params	params;
	typedef struct note note;
	struct note
	{
	    uint8_t midi_note;
	    uint8_t velocity;
	    float_t		osc_1_wtb_pointer;
	    float_t		osc_2_wtb_pointer;
	    float_t		adsr1_output;												// ADSR output
		uint8_t		adsr1_state;												// ADSR state (0=Off, 1=Attack, 2=Decay, 3=Sustain, 4=Release)
		float_t		attack1_incr;												// ADSR increment
		float_t		release1_incr;
		float_t 	in1, in2, in3, in4;
		float_t 	out1, out2, out3, out4;
		float_t		adsr2_output;												// ADSR output
		uint8_t		adsr2_state;												// ADSR state (0=Off, 1=Attack, 2=Decay, 3=Sustain, 4=Release)
		float_t		attack2_incr;												// ADSR increment
		float_t		release2_incr;
	    struct note *nxt;
	};

	typedef note* llist;

// Functions prototypes

	llist add_note_first(llist, uint8_t, uint8_t);
	llist add_note_last(llist, uint8_t, uint8_t);
	llist delete_note(llist, uint8_t);
	void  print_all_note(llist);
	note* get_last_note(llist);

