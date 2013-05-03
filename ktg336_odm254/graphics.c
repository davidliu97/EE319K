
// A rather interesting file. The functions which take the longest
// to execute are the shortest; conversely, the fastest function
// claims more than half the lines of code.

#include "graphics.h"
#include "rit128x96x4.h"

unsigned char buffer[96][64];

void buffer_clear(void) { int i, j;
	//unsigned long ** buff = (unsigned long **) buffer;
	for (i = 0; i < 96; i++)
		for (j = 0; j < 64; j++)
			buffer[i][j] = 0;
}

void buffer_draw(short xpos, short ypos, 
	const unsigned char *Buffer) {
	short i, j, height, width;
	unsigned char * pt;
	height = Buffer[BITMAP_HEIGHT_OFFSET];
	width = Buffer[BITMAP_WIDTH_OFFSET] >> 1;
	xpos>>=1;
	if (ypos + height > 96) return;
	pt = (unsigned char *) &Buffer[BITMAP_HEADER_SIZE] + width * height;
	for (i = 0; i < height; i++)
	{
		pt -= width;
		for(j = 0; j < width; j++)
			buffer[ypos+i][xpos+j] = pt[j];
	}
}

void buffer_drawbullet(short xpos, short ypos, short dir) {
	if (dir & 0x01) {
		buffer[ypos][xpos>>1] = 0xF0;
		if (!ypos) return;
		if (dir & 0x04) {
			if (xpos) buffer[ypos - 1][(xpos>>1) - 1] = 0x0F;
			return;
		}
		if (dir & 0x08) {
			if (xpos < 128) buffer[ypos - 1][xpos>>1] = 0x0F;
			return;
		}
		buffer[ypos - 1][xpos>>1] = 0xF0;
		return;
	}
	else if (dir & 0x02) {
		buffer[ypos][xpos>>1] = 0xF0;
		if (! ypos < 96) return;
		if (dir & 0x04) {
			if (xpos) buffer[ypos - 1][xpos>>1 - 1] = 0x0F;
			return;
		}
		if (dir & 0x08) {
			if (xpos < 127) buffer[ypos - 1][xpos>>1] = 0x0F;
			return;
		}
		buffer[ypos + 1][xpos>>1] = 0xF0;
		return;
	}
	else if (dir & 0x04) { // Horizontal
		buffer[ypos][xpos>>1] = 0xF0;
		if (xpos) buffer[ypos][(xpos>>1) - 1] = 0x0F;
		return;
	}
	buffer[ypos][xpos>>1] = 0xFF;
}


