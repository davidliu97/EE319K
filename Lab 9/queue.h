#ifndef __queue_h__
#define __queue_h__

#define Fifo_Size 32

long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value

//Initializes the Fifo Queue.
void Fifo_Init(void);
//Puts values into the queue.
//Returns 1 if successful.
int Fifo_Put(unsigned char data);
//Removes entries from the queue in the order that they were put in.
//Returns 1 if successful.
int Fifo_Get(unsigned char *datapt);
//Obtains the size of the fifo queue.
unsigned short Fifo_Length(void);

#endif
