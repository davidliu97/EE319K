// Logo.c
// Runs on LM3S1968 or LM3S8962
// Jonathan Valvano
// November 12, 2012

/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2012
   
   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2012
 Copyright 2012 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#include "rit128x96x4.h"

// *****************************************************************************
//
// The size of the header on the bitmap image. For a typical 4bpp Windows
// bitmap this is 0x76 bytes comprising the total number of bytes in the
// bitmap file header, the bitmap info header and the palette.
//
// *****************************************************************************
#define BITMAP_HEADER_SIZE   0x76

// *****************************************************************************
//
// The byte offsets into the image at which we can find the height and width.
// Each of these values are encoded in 4 bytes.
//
// *****************************************************************************
#define BITMAP_WIDTH_OFFSET  0x12
#define BITMAP_HEIGHT_OFFSET 0x16
// *****************************************************************************
//

//*************RIT128x96x4_BMP********************************************
//  Displays a 16 color BMP image 
//  (xpos,ypos) is the screen location of the lower left corner of BMP image
//  Inputs: xpos horizontal position to display this image, columns from the left edge 
//             must be less than 128 and even
//             0 is on the left 126 on the right
//          ypos vertical position to display this image, rows from the top edge
//             2 is on the top, 95 is bottom, 80 is near the bottom
//          pointer to a 16 color BMP image
//  Outputs: none
//  Must be less than or equal to 128 pixels wide by 80 rows high
//  The BMP image width must be an even number
void RIT128x96x4_BMP(unsigned long xpos, unsigned long ypos, const unsigned char *Buffer){
    unsigned long ulRow, ulWidth, ulHeight;
    unsigned char *pucRow;

    // Extract the width and height from the bitmap data.  These are encoded
    // in 4 byte fields but this application can't support images wider than
    // 128 pixels or taller than 80 rows so we merely read the least
    // significant byte.
    //
    ulHeight = (unsigned long)Buffer[BITMAP_HEIGHT_OFFSET];
    ulWidth = (unsigned long)Buffer[BITMAP_WIDTH_OFFSET];

    // Display the BMP Image.
    // The image is an ulWidth by ulHeight 4-bit gray scale image in BMP
    // format.  The divides by two are to account for the fact that there
    // are 2 pixels in each byte of image data.

    // Get a pointer to the first row of pixels in the image (which maps to the
    // bottom row on the display since bitmaps are encoded upside down).
    //************ ypos is 94 (bottom) 80 (starter file) ****************
    pucRow = (unsigned char *)&Buffer[BITMAP_HEADER_SIZE];
    for(ulRow = 0; ulRow < ulHeight; ulRow++){
        // Display in reverse row order.  
        RIT128x96x4ImageDraw(pucRow, xpos, (ypos - ulRow), ulWidth, 1);
        
//        pucRow += (ulWidth / 2);   // Move to the next row in the source image.
        pucRow += (((ulWidth + 7) / 8) * 4);   // Move to the next row in the source image.
    }  
}

extern unsigned char buffer[96][64];

void RIT128x96x4_Buffer(){
	unsigned long i;
	for (i = 0; i < 96; i++)
		RIT128x96x4ImageDraw(buffer[i], 0, i, 128, 1);
}
