#include <math.h>
#include <stdbool.h>
#include <stdio.h>
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
 float_t		f, fb,  out1=0, out2=0, out3=0, out4=0;
 float_t		f_square;

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
		float_t		signal_pf=0;
		note* 		play_note;

		note* 		delete_note_p;
		static float_t		lfo1_wtb_pointer=0;
		float_t				lfo1_wtb_incr=0;
		float_t				lfo1_output=0;

		static float_t		lfo2_wtb_pointer=0;
		float_t				lfo2_wtb_incr=0;
		float_t				lfo2_output=0;
		static float_t		lfo3_wtb_pointer=0;
		float_t				lfo3_wtb_incr=0;
		float_t				lfo3_output=0;
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

			lfo1_output = 1 + (params.lfo1_depth * sinewave[(uint16_t)lfo1_wtb_pointer]);

			if (params.lfo1_depth==0) lfo1_output = 1;


			// Compute LFO2 parameters

			lfo2_wtb_incr = WTB_LEN * params.lfo2_frequency / SAMPLERATE;		// Increment value of the LFO wavetable pointer

			lfo2_wtb_pointer = lfo2_wtb_pointer + lfo2_wtb_incr;

			if (lfo2_wtb_pointer > WTB_LEN)
			{
				lfo2_wtb_pointer = lfo2_wtb_pointer - WTB_LEN;
			}

			lfo2_output = params.lfo2_depth * sinewave[(uint16_t)lfo2_wtb_pointer];

			if (params.lfo2_depth==0) lfo2_output = 0;

			// Compute LFO3 parameters

			/*lfo3_wtb_incr = WTB_LEN * params.lfo2_frequency / SAMPLERATE;		// Increment value of the LFO wavetable pointer

			lfo3_wtb_pointer = lfo3_wtb_pointer + lfo3_wtb_incr;

			if (lfo3_wtb_pointer > WTB_LEN)
			{
				lfo3_wtb_pointer = lfo3_wtb_pointer - WTB_LEN;
			}

			lfo3_output = params.lfo2_depth *		sinewave[(uint16_t)lfo3_wtb_pointer];

			if (params.lfo2_depth==0) lfo3_output = 0;*/

			play_note = (note_list);
			temp=0;
			signal=0;
			while(play_note != NULL  && temp <=12)
			{
				// Compute pitch & oscillators increments
				pitch =  pitch_table[(play_note->midi_note)-24]* (1 + lfo2_output);// params.bend + pitch_table[(int) (params.bend2*((play_note->midi_note)-24))/100];

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




			// ADSR1 section

			switch ((play_note->adsr1_state))
			{
				case 0 :											// Off
				{
					(play_note->adsr1_output) = 0;

				//	printf("buh");
				}

				case 1 :											// Attack
				{
					(play_note->adsr1_output) = (play_note->adsr1_output) + (play_note->attack1_incr);

					if ((play_note->adsr1_output) > 1)
					{
						(play_note->adsr1_output) = 1;
						(play_note->adsr1_state) = 2;
						GPIO_SetBits(GPIOD, GPIO_Pin_15);
					}

					break;
				}

				case 2 :											// Decay
				{
					(play_note->adsr1_output) -= ((1-params.adsr1_sustain) / (params.adsr1_decay * SAMPLERATE));

					if ((play_note->adsr1_output) < params.adsr1_sustain)
					{
						(play_note->adsr1_output) = params.adsr1_sustain;
						(play_note->adsr1_state) = 3;
					}
					break;
				}

				case 3 :											// Sustain
				{
					(play_note->adsr1_output) = params.adsr1_sustain;
					//printf("buh");
					break;
				}

				case 4 :											// Release
				{

					if(params.pedal==1)
					(play_note->adsr1_output) = (play_note->adsr1_output) - ((play_note->adsr1_output) /(36000));	 	// Compute increment params * 48000
					else
						(play_note->adsr1_output) = (play_note->adsr1_output) - (play_note->release1_incr);

					if ((play_note->adsr1_output) < 0.01f)
					{
						(play_note->adsr1_output) = 0;
						(play_note->adsr1_state) = 0;
						GPIO_ResetBits(GPIOD, GPIO_Pin_15);
					}
					break;
				}
			}
			vel =  (float) (play_note->velocity)/127;//(float) (  (  (float)   ( (play_note->velocity)*(play_note->velocity) )*1.5    )   /16129.0f);
			signal_pf = ((params.osc1_mix * osc_1) + (params.osc2_mix * osc_2));
			// ADSR2 section
			if(params.pedal==5)
			{
				switch ((play_note->adsr2_state))

				{
					case 0 :											// Off
					{
						(play_note->adsr2_output) = 0;

					//	printf("buh");
					}

					case 1 :											// Attack
					{
						(play_note->adsr2_output) = (play_note->adsr2_output) + (play_note->attack2_incr);

						if ((play_note->adsr2_output) > 1)
						{
							(play_note->adsr2_output) = 1;
							(play_note->adsr2_state) = 2;
						}

						break;
					}

					case 2 :											// Decay
					{
						(play_note->adsr2_output) -= ((1-params.adsr2_sustain) / (params.adsr2_decay * SAMPLERATE));

						if ((play_note->adsr2_output) < params.adsr2_sustain)
						{
							(play_note->adsr2_output) = params.adsr2_sustain;
							(play_note->adsr2_state) = 3;
						}
						break;
					}

					case 3 :											// Sustain
					{
						(play_note->adsr2_output) = params.adsr2_sustain;
						//printf("buh");
						break;
					}

					case 4 :											// Release
					{

						(play_note->adsr2_output) = (play_note->adsr2_output) - (play_note->release2_incr);

						if ((play_note->adsr2_output) < 0.01f)
						{
							(play_note->adsr2_output) = 0;
							(play_note->adsr2_state) = 0;

						}
						break;
					}
					}

					// Pseudo MOOG Filter Section

						f =  (float_t) params.cutoff * (1.0f+ lfo1_output) * (1.0f + (play_note->adsr2_output)) * pitch * 1.16f / (SAMPLERATE/2) ;

						if (f>1.0f) f=1.0f;
						f_square = f*f;

						fb = (float_t) params.reso * (1.0f - 0.15f * f_square);


						signal_pf -= ((play_note->out4) * fb);
						signal_pf *= 0.35013f * f_square * f_square;
						(play_note->out1) = signal_pf + 0.3f * (play_note->in1) + (1-f) * (play_note->out1); 				// Pole 1
						(play_note->in1) = signal_pf;
						(play_note->out2) = (play_note->out1) + 0.3f * (play_note->in2) + (1-f) * (play_note->out2); 				// Pole 2
						(play_note->in2) = (play_note->out1);
						(play_note->out3) = (play_note->out2) + 0.3f * (play_note->in3) + (1-f) * (play_note->out3); 				// Pole 3
						(play_note->in3) = (play_note->out2);
						(play_note->out4) = (play_note->out3) + 0.3f * (play_note->in4) + (1-f) * (play_note->out4); 				// Pole 4
						(play_note->in4) = (play_note->out3);
						signal_pf = (play_note->out4);



					//	f=  (float_t) params.cutoff * pitch * (1.0f + (play_note->adsr2_output) )* ( lfo1_output) * 0.0000483333f ;//adsr2_output* (1.0f+ lfo1_output) * (1.0f + adsr2_output )

					//if (f>1.0f) f=1.0f;
				//	f_square = f*f;
				//	fb = (float_t) params.reso * (1.0f - 0.15f * f_square );

				//	signal_pf -= (play_note->out4) * fb;
				//	signal_pf *= 0.35013f * f_square * f_square;
				/*	(play_note->out1) = signal_pf + 0.3f * (play_note->in1) + (1-f) * (play_note->out1); 				// Pole 1
					(play_note->in1) = signal_pf;
					(play_note->out2) = (play_note->out1) + 0.3f * (play_note->in2) + (1-f) * (play_note->out2); 				// Pole 2
					(play_note->in2) = (play_note->out1);
					(play_note->out3) = (play_note->out2) + 0.3f * (play_note->in3) + (1-f) * (play_note->out3); 				// Pole 3
					(play_note->in3) = (play_note->out2);
					(play_note->out4) = (play_note->out3) + 0.3f * (play_note->in4) + (1-f) * (play_note->out4); 				// Pole 4
					(play_note->in4) = (play_note->out3);
					signal_pf = (play_note->out4);*/
					// Oscillator Mixer section


			}
			// VCA section
					// Oscillator Mixer section
				signal =  signal+signal_pf*vel * (play_note->adsr1_output);

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
				if(play_note->adsr1_state == 0 )
				{
					delete_note_p = play_note;
				//	printf("buh");
					play_note = play_note->nxt;
					note_list = delete_note(note_list, delete_note_p->midi_note);
				}else{

					play_note = play_note->nxt;
				}
			}

			signal = (signal) *22767 ;
			//signal = (signal) *22767  ;
			//signal = (signal) *7000  ;
			//signal = 8*atan(signal/8)  ;
			//sinal *= 32767;
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


//efects




