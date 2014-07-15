#include "main.h"
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

