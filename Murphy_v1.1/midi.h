#include "main.h"
#define MIDI_BUFFER_LENGTH		64


// Start MIDI interface and return a pointer to the MIDI buffer
uint8_t* MIDI_Start (void);

// Returns the number of new bytes in the MIDI buffer

uint8_t	MIDI_GetNbNewBytes (uint8_t*);

// Parse MIDI bytes

void MIDI_parser (uint8_t*, uint8_t);

void ChangeParam(uint8_t, uint8_t);
