
#define BITMAP_HEADER_SIZE   0x76
#define BITMAP_WIDTH_OFFSET  0x12
#define BITMAP_HEIGHT_OFFSET 0x16

void buffer_clear(void);
void buffer_draw(short, short, const unsigned char *);
void buffer_drawbullet(short, short, short);
