#include "stm32f4xx.h"
#include "llist.h"

#include <malloc.h>
#include <stdio.h>


// Add a note at the beginning of the list
// -------------------------------------------

llist add_note_first(llist list, uint8_t midi_note, uint8_t velocity)
{
    // Add a new element in memory

    note* new_note = malloc(sizeof(note));

    // Set the new element values

    new_note->midi_note = midi_note;
      new_note->velocity = velocity;
      new_note->osc_1_wtb_pointer = 0;
      new_note->osc_2_wtb_pointer = 0;
      new_note->adsr1_output=0;												// ADSR output
      new_note->adsr1_state=1;												// ADSR state (0=Off, 1=Attack, 2=Decay, 3=Sustain, 4=Release)
      new_note->adsr2_output=0;												// ADSR output
           new_note->adsr2_state=1;												// ADSR state (0=Off, 1=Attack, 2=Decay, 3=Sustain, 4=Release)
      new_note->attack1_incr=(1 - 0)/(params.adsr1_attack * SAMPLERATE);											// ADSR increment
      new_note->attack2_incr=(1 - 0)/(params.adsr2_attack * SAMPLERATE);											// ADSR increment
   //   new_note->decay1_incr=0;
      new_note->release1_incr=0;
      new_note->release2_incr=0;
      new_note->in1 =0;
         new_note->in2 =0;
         new_note->in3 =0;
         new_note->in4 =0;
         new_note->out1 =0;
            new_note->out2 =0;
            new_note->out3 =0;
            new_note->out4 =0;
    // Set pointer to the previous first element

    new_note->nxt = list;

    // return the pointer to the new first element

    return new_note;
}


// Add a note at the end of the list
// ---------------------------------

llist add_note_last(llist list, uint8_t midi_note, uint8_t velocity)
{
    // Add a new element in memory

    note* new_note = malloc(sizeof(note));

    // Set the new element values

    new_note->midi_note = midi_note;
    new_note->velocity = velocity;
    new_note->osc_1_wtb_pointer = 0;
    new_note->osc_2_wtb_pointer = 0;
    new_note->adsr1_output=0;												// ADSR output
          new_note->adsr1_state=1;												// ADSR state (0=Off, 1=Attack, 2=Decay, 3=Sustain, 4=Release)
          new_note->adsr2_output=0;												// ADSR output
               new_note->adsr2_state=1;												// ADSR state (0=Off, 1=Attack, 2=Decay, 3=Sustain, 4=Release)
          new_note->attack1_incr=(1 - 0)/(params.adsr1_attack * SAMPLERATE);											// ADSR increment
          new_note->attack2_incr=(1 - 0)/(params.adsr2_attack * SAMPLERATE);											// ADSR increment
       //   new_note->decay1_incr=0;
          new_note->release1_incr=0;
          new_note->release2_incr=0;
          new_note->in1 =0;
                   new_note->in2 =0;
                   new_note->in3 =0;
                   new_note->in4 =0;
                   new_note->out1 =0;
                      new_note->out2 =0;
                      new_note->out3 =0;
                      new_note->out4 =0;
    // There is no next element

    new_note->nxt = NULL;

    // If list is empty, then simply return the newly created element

    if(list == NULL)
    {
        return new_note;
    }

    // Else, walk through the list to find the actual last element

    else
    {
    	note* temp=list;
        while(temp->nxt != NULL)
        {
            temp = temp->nxt;
        }
        temp->nxt = new_note;
        return list;
    }
}

// Delete a note based on the midi_note
// ------------------------------------

llist delete_note(llist list, uint8_t midi_note)
{
    // If list is empty, then just returns

    if(list == NULL)
        return NULL;

    // If the current element is the one to delete

    if(list->midi_note == midi_note)
    {
        note* tmp = list->nxt;
        free(list);
        tmp = delete_note(tmp, midi_note);
        return tmp;
    }

    // Else, the current element is not the one to delete

    else
    {
        list->nxt = delete_note(list->nxt, midi_note);
        return list;
    }
}


// Delete a note based on the midi_note
// ------------------------------------

note* delete_note_ret(llist list, uint8_t midi_note)
{
    // If list is empty, then just returns
	note *tmp = list;

	note *tmp2 = NULL;
    if(list == NULL)
        return NULL;


    while(tmp != NULL)
    {
        if(tmp2->midi_note == midi_note)
		{
			note* tmp3 = list->nxt;
			free(tmp2);
			tmp->nxt = tmp3;
			return tmp3;
		}
        tmp2 = tmp;
        tmp = tmp->nxt;
    }

}


void change_note_state(llist list,uint8_t midi_note, uint8_t state)
{
    note *tmp = list;

    while(tmp != NULL)
    {
    	 if(tmp->midi_note == midi_note)
		{
    		 tmp->attack1_incr=(1 - (tmp->adsr1_output))/(params.adsr1_attack * SAMPLERATE);											// ADSR increment
    		 tmp->release1_incr = 	((tmp->adsr1_output) /(params.adsr1_release * SAMPLERATE));		// Compute increment
    		 tmp->adsr1_state=state;
    		 tmp->attack2_incr=(1 - (tmp->adsr2_output))/(params.adsr2_attack * SAMPLERATE);											// ADSR increment
			 tmp->release2_incr = 	((tmp->adsr2_output) /(params.adsr2_release * SAMPLERATE));		// Compute increment
			 tmp->adsr2_state=state;
			return ;
		}
        tmp = tmp->nxt;
    }
}


void note_exists(llist list,uint8_t midi_note)
{
    note *tmp = list;

    while(tmp != NULL)
    {
    	 if(tmp->midi_note == midi_note)
		{
			return 1 ;
		}
        tmp = tmp->nxt;
    }
    return 0;
}

// Get pointer to last note in the list
// ------------------------------------

note* get_last_note(llist list)
{

    if(list == NULL)
    {
        return NULL;
    }

    else
        {
        	note* temp=list;
            while(temp->nxt != NULL)
            {
                temp = temp->nxt;
            }
            return temp;
        }
}


// Print the whole list (for debug only)
// -------------------------------------

void print_all_note(llist list)
{
    note *tmp = list;

    printf("Notes in the list : ");

    while(tmp != NULL)
    {
        printf("%d ", tmp->midi_note);
        tmp = tmp->nxt;
    }
}
