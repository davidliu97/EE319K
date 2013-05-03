#define MAX_ENEMIES 10
#define MAX_BULLETS 1000

#define PLAYER_SPEED 2
#define ENEMY_SPEED 1
#define BULLET_SPEED 3

#define abs(x) ( (x<0) ? -(x) : (x) )

struct enemy { // ALIGN 2
	const unsigned char * bitmap;
	unsigned short freq;				// counter mod (freq&0xF0)>>4+3) fire rate.
	unsigned short weapon;
	unsigned short movement;	// top 4 bits is alternating; bottom 4 bits signifying up, down, left, right.
	unsigned short size;
	short start_x, start_y, start_health;
	unsigned int score;
};

struct enemy_ref { // ALIGN 4
	const struct enemy * type;
	short x, y, health;
	unsigned short counter;
};

struct player_{
	short x, y, health, aux;
	unsigned long score;
};

struct bullet { short info, x, y, aux; }; // ALIGN 4

typedef struct enemy enemy;
typedef struct enemy_ref enemy_ref;
typedef struct player_ player_;
typedef struct bullet bullet;

extern const enemy enemy1, enemy2;
extern player_ player;

void update_actors(unsigned long);
void init_actors(void);
void addbullet(short, short, short, short);
void addenemy(unsigned short type);
