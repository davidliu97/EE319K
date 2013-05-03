
unsigned long RandomSeed;

void random_init(unsigned long Seed)
{
	RandomSeed = Seed;
}

__asm unsigned long random_int(void)
{
	IMPORT RandomSeed
	LDR R2,=RandomSeed    //; R2 = &M, R2 points to M
	LDR R0,[R2]  //; R0=M
	LDR R1,=1664525
	MUL R0,R0,R1 //; R0 = 1664525*M
	LDR R1,=1013904223
	ADD R0,R1    //; 1664525*M+1013904223 
	STR R0,[R2]  //; store M
	BX  LR
	ALIGN
}

unsigned long log2(unsigned long in)
{
	unsigned long out = 0x20;
	while ( !(in & (1<<--out)) && out);
	return ++out;
}
