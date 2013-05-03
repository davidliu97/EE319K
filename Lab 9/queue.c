#include "queue.h"

unsigned char *Putpt;
unsigned char *Getpt;
unsigned char Fifo[Fifo_Size];

void Fifo_Init(void){
	long sr;
	sr = StartCritical();
	Putpt = Getpt = &Fifo[0];
	EndCritical(sr);
}

int Fifo_Put(unsigned char data){
	unsigned char *TempPt;
	//Checks if there is space.
	TempPt = Putpt + 1;
	//If the Put pointer reaches the bottom of the array, it wraps around to the beginning of the array.
	if(TempPt==&Fifo[Fifo_Size]){
		TempPt = &Fifo[0];
	}
	if(TempPt == Getpt){
		return 0; //The queue is full.
	}
	else{
		*(Putpt) = data;
		Putpt = TempPt;
		return 1;
	}
}

int Fifo_Get(unsigned char *datapt){
	if(Putpt == Getpt){
		return 0; //The queue is empty
	}
	else{
		*datapt = *(Getpt++);
		//Wrap Around
		if(Getpt == &Fifo[Fifo_Size]){
			Getpt = &Fifo[0];
		}
		return 1;
	}
}

unsigned short Fifo_Length(void){
	signed long size = Putpt - Getpt;
	if (Putpt < Getpt)
		return  size + Fifo_Size;
	return size;
}
 
int Status[20]; // entries 0,7,12,19 should be false, others true 
unsigned char GetData[10]; // entries 1 2 3 4 5 6 7 8 should be 1 2 3 4 5 6 7 8 
int testqueue(void){ 
 Fifo_Init(); 
 for(;;){ 
 Status[0] = Fifo_Get(&GetData[0]); // should fail, empty 
 Status[1] = Fifo_Put(1); // should succeed, 1 
 Status[2] = Fifo_Put(2); // should succeed, 1 2 
 Status[3] = Fifo_Put(3); // should succeed, 1 2 3 
 Status[4] = Fifo_Put(4); // should succeed, 1 2 3 4 
 Status[5] = Fifo_Put(5); // should succeed, 1 2 3 4 5 
 Status[6] = Fifo_Put(6); // should succeed, 1 2 3 4 5 6 
 Status[7] = Fifo_Put(7); // should fail, 1 2 3 4 5 6 
 Status[8] = Fifo_Get(&GetData[1]); // should succeed, 2 3 4 5 6 
 Status[9] = Fifo_Get(&GetData[2]); // should succeed, 3 4 5 6 
 Status[10] = Fifo_Put(7); // should succeed, 3 4 5 6 7 
 Status[11] = Fifo_Put(8); // should succeed, 3 4 5 6 7 8 
 Status[12] = Fifo_Put(9); // should fail, 3 4 5 6 7 8 
 Status[13] = Fifo_Get(&GetData[3]); // should succeed, 4 5 6 7 8 
 Status[14] = Fifo_Get(&GetData[4]); // should succeed, 5 6 7 8 
 Status[15] = Fifo_Get(&GetData[5]); // should succeed, 6 7 8 
 Status[16] = Fifo_Get(&GetData[6]); // should succeed, 7 8 
 Status[17] = Fifo_Get(&GetData[7]); // should succeed, 8 
 Status[18] = Fifo_Get(&GetData[8]); // should succeed, empty 
 Status[19] = Fifo_Get(&GetData[9]); // should fail, empty 
 } 
}
