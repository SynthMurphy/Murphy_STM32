#include <math.h>
#include <stdbool.h>

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

#include "llist.h"
#include "pitch.h"
#include "synth.h"
#include "wavetable_24.h"


extern uint16_t			audiobuff[BUFF_LEN];			// Circular audio buffer
extern synth_params		params;							// Synthesizer parameters
extern bool				trig;
extern bool				new_note_event;
extern llist 		note_list ;
extern float_t	global_fc, global_pitch;


float fastsin(uint32_t);


void Make_Sound(uint16_t start_index)
{
	// Local variables
		uint8_t   temp =0;
		uint16_t 			i;															// Audio buffer index
		float_t				pitch;
		float_t 			vel;
		float_t		osc_1_wtb_pointer ;										// Pointer in OSC1 wavetable
		float_t		osc_2_wtb_pointer;
		float_t		osc_1_wtb_incr;												// OSC1 sample increment
		float_t 	osc_2_wtb_incr;												// OSC2 sample increment
		uint16_t 			a,b;														// Index of bounding samples
		float_t				da, db;														// Distances to bounding samples
		float_t		osc_1;												// Individual oscillator audio output
		float_t		osc_2;												// Individual oscillator audio output
		float_t		signal=0;
		note* 		play_note;
		static float_t		lfo1_wtb_pointer=0;
		float_t				lfo1_wtb_incr=0;
		float_t				lfo1_output=0;

		static float_t		lfo2_wtb_pointer=0;
		float_t				lfo2_wtb_incr=0;
		float_t				lfo2_output=0;

		// Step 1 : Audio pre-processing (compute every half buffer)
		// ---------------------------------------------------------

		GPIO_SetBits(GPIOD, GPIO_Pin_13);					// Set LED_13 (for debug)
		// Find if there is a note to play

		play_note = (note_list);

		if (play_note == NULL)
		{
			trig = 0;
		}
		else
		{
			trig = 1;
		}

		// Step 2 : Fills the buffer with individual samples
		// -------------------------------------------------


		i = 0;

		while(i<BUFF_LEN_DIV2 )
		{
			// Compute LFO1 parameters

			lfo1_wtb_incr = WTB_LEN * params.lfo1_frequency / SAMPLERATE;		// Increment value of the LFO wavetable pointer

			lfo1_wtb_pointer = lfo1_wtb_pointer + lfo1_wtb_incr;

			if (lfo1_wtb_pointer > WTB_LEN)
			{
				lfo1_wtb_pointer = lfo1_wtb_pointer - WTB_LEN;
			}

			lfo1_output = params.lfo1_depth * square[(uint16_t)lfo1_wtb_pointer];

			if (params.lfo1_depth==0) lfo1_output = 0;


			// Compute LFO2 parameters

			lfo2_wtb_incr = WTB_LEN * params.lfo2_frequency / SAMPLERATE;		// Increment value of the LFO wavetable pointer

			lfo2_wtb_pointer = lfo2_wtb_pointer + lfo2_wtb_incr;

			if (lfo2_wtb_pointer > WTB_LEN)
			{
				lfo2_wtb_pointer = lfo2_wtb_pointer - WTB_LEN;
			}

			lfo2_output = params.lfo2_depth * sinewave[(uint16_t)lfo2_wtb_pointer];

			if (params.lfo2_depth==0) lfo2_output = 0;


			play_note = (note_list);
			temp=0;
			signal=0;
			while(play_note != NULL  && temp <=12)
			{
				// Compute pitch & oscillators increments
				pitch =  pitch_table[(play_note->midi_note)-24] * (1 + lfo2_output);// + params.bend;


				// OSC1 Perform Linear Interpolation between 'a' and 'b' points

				a  = (int)  (play_note->osc_1_wtb_pointer);									// Compute 'a' and 'b' indexes
				da =  (play_note->osc_1_wtb_pointer) - a;										// and both 'da' and 'db' distances
				b  = a + 1;
				db = b -  (play_note->osc_1_wtb_pointer);

				if (b==WTB_LEN) b=0;											// if 'b' passes the end of the table, use 0
				switch (params.osc1_waveform)
				{
					case 0 :
					{
						osc_1 = db*square[a] + da*square[b];					// Linear interpolation (same as weighted average)
						break;
					}

					case 1 :
					{
						osc_1 = db*triangle[a] + da*triangle[b];
						break;
					}

					case 2 :
					{
						osc_1 = db*sawtooth[a] + da*sawtooth[b];
						break;
					}

					case 3 :
					{
						osc_1 = db*distosine[a] + da*distosine[b];
						break;
					}

					default :
					{
						osc_1 = db*sinewave[a] + da*sinewave[b];
						break;
					}
				}
				// OSC2 Perform Linear Interpolation between 'a' and 'b' points

				a  = (int) (play_note->osc_2_wtb_pointer);									// Compute 'a' and 'b' indexes
				da = (play_note->osc_2_wtb_pointer) - a;										// and both 'da' and 'db' distances
				b  = a + 1;
				db = b - (play_note->osc_2_wtb_pointer);

				if (b==WTB_LEN) b=0;											// if 'b' passes the end of the table, use 0

				switch (params.osc2_waveform)
				{
					case 0 :
					{
						osc_2 = db*square[a] + da*square[b];					// Linear interpolation (same as weighted average)
						break;
					}

					case 1 :
					{
						osc_2 = db*triangle[a] + da*triangle[b];
						break;
					}

					case 2 :
					{
						osc_2 = db*sawtooth[a] + da*sawtooth[b];
						break;
					}

					case 3 :
					{
						osc_2 = db*distosine[a] + da*distosine[b];
						break;
					}

					default :
					{
						osc_2 = db*sinewave[a] + da*sinewave[b];
						break;
					}
				}

				vel =  (float) (play_note->velocity)/127;//(float) (  (  (float)   ( (play_note->velocity)*(play_note->velocity) )*1.5    )   /16129.0f);

				// Oscillator Mixer section

				// VCA section
				// Oscillator Mixer section

				signal =  signal+((params.osc1_mix * osc_1) + (params.osc2_mix * osc_2))*vel;

				osc_1_wtb_incr = WTB_LEN * (pitch * params.osc1_octave) / SAMPLERATE;						// Increment value of the OSC1 wavetable pointer
				osc_2_wtb_incr = WTB_LEN * (pitch * params.osc2_octave * params.detune) / SAMPLERATE;		// Increment value of the OSC2 wavetable pointer


				osc_1_wtb_pointer = (play_note->osc_1_wtb_pointer) + osc_1_wtb_incr;				// WTB buffer
				if (osc_1_wtb_pointer>WTB_LEN)										// return to zero if end is reached
				{
					osc_1_wtb_pointer = osc_1_wtb_pointer - WTB_LEN;
				}
				(play_note->osc_1_wtb_pointer) = osc_1_wtb_pointer;

				osc_2_wtb_pointer = (play_note->osc_2_wtb_pointer) + osc_2_wtb_incr;				// WTB buffer
				if (osc_2_wtb_pointer>WTB_LEN)										// return to zero if end is reached
				{
					osc_2_wtb_pointer = osc_2_wtb_pointer - WTB_LEN;
				}
				(play_note->osc_2_wtb_pointer)= osc_2_wtb_pointer;
		temp++;
				play_note = play_note->nxt;
			}
			signal = (signal) *7000  ;

			if (signal >  32767.0f) signal =  32767.0f;
			if (signal < -32767.0f) signal = -32767.0f;

			// Fill the buffer

			audiobuff[start_index+i] =   (uint16_t)((int16_t) signal);		// Left Channel value
			audiobuff[start_index+i+1] = (uint16_t)((int16_t) signal);		// Right Channel Value


			// Update pointers for next loop

			i = i+2;															// Audio buffer (L+R)



		}

		//USART_SendData(USART2, temp);
		GPIO_ResetBits(GPIOD, GPIO_Pin_13);


}







