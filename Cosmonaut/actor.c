
#include "actor.h"
#include "sound.h"
#include "SysTick.h"
#include "utils.h"
#include "graphics.h"
#include "draw.h"
#include "rit128x96x4.h"
#include <stdio.h>

extern const unsigned char shoot[];
extern void (*fire_func)(unsigned short, unsigned short, unsigned short);

const unsigned char enemy_1[], enemy_2[], 
	enemy_3[], enemy_4[], player_ship[], boss[], life[],
	gamefail[], gamewin[], pause[];
	
const enemy enemy_list[] = {
	{ enemy_1, 0x47, 0x02, 0xC2, 0x03, -1, 4, 1, 100 },
	{ enemy_1, 0xAF, 0xC2, 0x02, 0x03, -1, 4, 1, 100 },
	{ enemy_2, 0x8F, 0x0B, 0x38, 0x03, 4, -1, 2, 1000 },
	{ enemy_2, 0x8F, 0x07, 0x34, 0x03, 123, -1, 2, 1000 },
	{ enemy_3, 0x8F, 0x12, 0xC2, 0x03, -1, 4, 2, 10000 },
	{ enemy_4, 0x8F, 0x25, 0xC2, 0x03, -1, 4, 2, 20000 },
  { boss,    0x0F, 0xFF, 0xF0, 0x06, 64, 7, 10, 100000 }
};

player_ player;
enemy_ref enemies[MAX_ENEMIES];
bullet bullets[MAX_BULLETS];
unsigned long tick, score;
short queue_bullet;
char score_string[14];

void addbullet(short info, short x, short y, short aux) {
	int i;
	for (i = 0; i < MAX_BULLETS; i++)
	{
		if (bullets[i].info > 0) continue;
		bullets[i].info = info;
		bullets[i].x = x;
		bullets[i].y = y;
		break;
	}
}

void addenemy(unsigned short type) {
	int i; enemy *typ;
	if (type >= sizeof(enemy_list)) type = sizeof(enemy_list)-1;
	typ = (enemy*)&enemy_list[type];
	for (i = 0; i < MAX_ENEMIES; i++) {
		enemy_ref *curr = &enemies[i];
		if (curr->health >= 0) continue;
		curr->type = typ;
		curr->health = typ->start_health;
		curr->counter = 0;
		if (typ->start_x < 0) 
			curr->x = (random_int() % (127-(typ->size<<1))) +
				typ->size + 1;
		else
			curr->x = typ->start_x;
		
		if (typ->start_y < 0)
			curr->y = (random_int() % (95-(typ->size<<1))) + 
				typ->size + 1;
		else
			curr->y = typ->start_y;
		break;
	}
}

void update_actors(unsigned long playerdat) {
	int i, j, player_x = player.x, player_y = player.y;
	
	buffer_clear();
	// Handle player.
	if (playerdat & 0x01) player.y -= PLAYER_SPEED;
	if (playerdat & 0x02) player.y += PLAYER_SPEED;
	if (playerdat & 0x04) player.x -= PLAYER_SPEED;
	if (playerdat & 0x08) player.x += PLAYER_SPEED;
	if (player.x > 124) player.x = 124;
	if (player.x < 4) player.x = 4;
	if (player.y > 92) player.y = 92;
	if (player.y < 4) player.y = 4;
	if (player.health > 0)
		buffer_draw(player_x-4, player_y-4, player_ship);
	
	for (i = 0; i < MAX_BULLETS; i++) { // Handle bullets.
		short bullet_x, bullet_y, dir;
		bullet *curr = &bullets[i];
		if (curr->info <= 0) continue;
		bullet_x = curr->x;
		bullet_y = curr->y;
		dir = curr->info & 0x0F;
		// Collision detection.
		if (curr->info & 0x10) {	// Enemy bullet.
			if (abs(bullet_x - player.x) < 3
				&& abs(bullet_y - player.y) < 3)
			{
				player.health -= 1; curr->info = -1; 
				playWAV((unsigned char *) &shoot[0], 4080, 0);
				continue;
			}
		}
			
		else {
			for (j = 0; j < MAX_ENEMIES; j++) {// Player bullet.
				short size;
				enemy_ref * curr_enemy = &enemies[j];
				if (!curr_enemy->health) continue;
				size = curr_enemy->type->size;
				if (abs(bullet_x - curr_enemy->x) < size && 
					abs(bullet_y - curr_enemy->y) < size)
				{
					curr_enemy->health -= 1; 
					curr->info = -1;
					break;
				}
			}
			if (j < MAX_ENEMIES) continue;
		}
		
		
		if (bullet_x > 128 - BULLET_SPEED || bullet_x < BULLET_SPEED
			|| bullet_y > 96 - BULLET_SPEED || bullet_y < BULLET_SPEED)
			{ curr->info = -1; continue; }
		// If it survived all of that, update position and redraw.
		if (dir & 0x01) bullet_y -= BULLET_SPEED;
		if (dir & 0x02) bullet_y += BULLET_SPEED;
		if (dir & 0x04) bullet_x -= BULLET_SPEED;
		if (dir & 0x08) bullet_x += BULLET_SPEED;
		curr->x = bullet_x;
		curr->y = bullet_y;
		buffer_drawbullet(bullet_x, bullet_y, dir);
	}

	if (playerdat & 0x20) queue_bullet = 1;
	if(queue_bullet) { // Fix this
		queue_bullet = 0;
		fire_func(player.x, player.y, tick%4);
	}
	for (i = 0; i < MAX_ENEMIES; i++) { // Handle enemies.
		enemy *info;
		unsigned short movement, freq, 
			size, weapon, px, py, counter;
		enemy_ref *curr = &enemies[i];
		
		if (curr->health < 0) continue;
		if (curr->health == 0)
		{
			score += curr->type->score;
			curr->health = -1;
			playWAV((unsigned char *)&shoot[0], 4080, 0);
			if (curr->type == &enemy_list[6])
			{
				SysTick_Init(0);
				buffer_draw(0, 40, (unsigned char *) &gamewin[0]);
				sprintf(&score_string[0], "%d", (score>>6));
				RIT128x96x4_Buffer();
				RIT128x96x4StringDraw(&score_string[0], 0, 0, 0x0F);
				while(1);
			}
		}
		info = (enemy *)curr->type;
		counter = curr->counter;
		movement = info->movement;
		freq = (info->freq & 0x0F) + 1;
		size = info->size;
		weapon = info->weapon;
		// Move enemy.
		if (movement & 0x01) curr->y -= ENEMY_SPEED;
		if (movement & 0x02) curr->y += ENEMY_SPEED;
		if (movement & 0x04) curr->x -= ENEMY_SPEED;
		if (movement & 0x08) curr->x += ENEMY_SPEED;
		if ((counter % (freq<<2)) <= freq<<1) { // Conditional movement
			if (movement & 0x20) curr->y += ENEMY_SPEED;
			if (movement & 0x80) curr->x += ENEMY_SPEED;
		}
		else { // Allows for alternation.
			if (movement & 0x10) curr->y -= ENEMY_SPEED;
			if (movement & 0x40) curr->x -= ENEMY_SPEED;
		}
		
		if ( curr->x < size || curr->x > 128 - size ||
			curr->y < size || curr->y > 96 - size )
		{ curr->health = -1; continue; }
		
		// Fire weapon
		freq = ((info->freq & 0xF0) >> 4) + 4;
		if (!(counter % freq)) {
			px = curr->x+size;
			py = curr->y+size;
			if(weapon & 0x01) addbullet(0x11, px, py, 0);
			if(weapon & 0x02) addbullet(0x12, px, py, 0);
			if(weapon & 0x04) addbullet(0x14, px, py, 0);
			if(weapon & 0x08) addbullet(0x18, px, py, 0);
			if(weapon & 0x10) addbullet(0x15, px, py, 0);
			if(weapon & 0x20) addbullet(0x19, px, py, 0);
			if(weapon & 0x40) addbullet(0x16, px, py, 0);
			if(weapon & 0x80) addbullet(0x1A, px, py, 0);
		}
		curr->counter++;
		buffer_draw(curr->x-size, curr->y-size, info->bitmap);
	}
	for (i = 0; i < player.health; i++)
		buffer_draw(122-8*i, 0, (unsigned char *)&life[0]);
	
	if (player.health <= 0)
	{
		SysTick_Init(0);
		buffer_draw(18, 40, (unsigned char *) &gamefail[0]);
		sprintf(&score_string[0], "%d", (score>>6));
		RIT128x96x4_Buffer();
		RIT128x96x4StringDraw(&score_string[0], 0, 0, 0x0F);
		while(1);
	}
	
	tick++;
	if (tick == 50) addenemy(2);
	
	RIT128x96x4_Buffer();
}

void init_actors() {
	int i;
	player.x = 64;
	player.y = 84;
	player.health = 3;
	tick = score = 0;
	queue_bullet = 1;
	for (i = 0; i < MAX_ENEMIES; i++)
		enemies[i].health = -1;
	buffer_clear();
}

const unsigned char player_ship[] ={
	0x42, 0x4D, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xF0, 0x0F, 0xF0, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0F,
	0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0xFF,
};

const unsigned char enemy_1[] ={
 0x42, 0x4D, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0xFF,
 0xFF, 0x00, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xFF, 0xFF, 0xF0, 0x0F, 0x0F, 0xF0, 0xF0, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
};

const unsigned char enemy_2[] ={
 0x42, 0x4D, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0xF0, 0x0F, 0xFF,
 0xFF, 0xF0, 0x0F, 0x0F, 0xF0, 0xF0, 0x0F, 0xFF, 0xFF, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0xFF,
};

const unsigned char enemy_3[] ={
 0x42, 0x4D, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x0F, 0xF0, 0x00, 0xFF,
 0xFF, 0x00, 0x0F, 0x0F, 0xF0, 0xF0, 0x0F, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
};

const unsigned char enemy_4[] ={
 0x42, 0x4D, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0xFF,
 0xFF, 0x00, 0x00, 0xF0, 0x0F, 0x00, 0x0F, 0xFF, 0xFF, 0xF0, 0x0F, 0x0F, 0xF0, 0xF0, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
};

const unsigned char boss[] ={
 0x42, 0x4D, 0x76, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
};

const unsigned char gamefail[] ={
 0x42, 0x4D, 0x76, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x08, 0x88, 0x80, 0x08, 0x88, 0x80, 0x08, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x08, 0x08, 0x88, 0x88, 0x00, 0x08, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x8F, 0xFF, 0xF0, 0x8F, 0xFF, 0xF8, 0x08, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x08, 0x08, 0xFF, 0xFF, 0x80, 0x8F, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08, 0x88, 0x88, 0x00, 0x88, 0x88, 0x88, 0x08, 0x00, 0x00, 0x80, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x00, 0x80, 0x08, 0x08, 0xFF, 0xFF, 0x80, 0x8F, 0xFF, 0xF8, 0x08, 0x00, 0x00, 0x80, 0x8F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x08, 0xF8, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0xF8, 0x88, 0x80, 0xF8, 0x88, 0x8F, 0x0F, 0x8F, 0x0F, 0x8F, 0x0F, 0x88, 0x88, 0xF0, 0xF8, 0x88, 0x8F, 0x08, 0x88, 0x88, 0xF0, 0xF8, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x0F, 0xFF, 0xF0, 0x0F, 0xFF, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xFF, 0xFF, 0x00, 0x0F, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0x00, 0x0F, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x08, 0x88, 0x88, 0x08, 0x88, 0x80,
 0x08, 0x88, 0x80, 0x08, 0x88, 0x88, 0x00, 0x88, 0x88, 0x00, 0x80, 0x00, 0x00, 0x80, 0x08, 0x00, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x88, 0x88, 0x00, 0x88, 0x88, 0x80, 0x08, 0x88, 0x88, 0x08,
 0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x0F, 0xF8, 0xFF, 0x0F, 0xFF, 0xF8,
 0x0F, 0xFF, 0xF8, 0x0F, 0xF8, 0xFF, 0x08, 0xFF, 0xFF, 0x80, 0x80, 0x00, 0x08, 0x80, 0x08, 0x00, 0x00, 0x00, 0x80, 0x00, 0x08, 0x0F, 0xF8, 0xFF, 0x08, 0xFF, 0xFF, 0xF0, 0x8F, 0xFF, 0xFF, 0x08,
 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08,
 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x8F, 0x80, 0x08, 0x00, 0x00, 0x00, 0x80, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x08,
 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x88, 0x8F,
 0x00, 0x88, 0x8F, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x80, 0x80, 0x08, 0xF0, 0x80, 0x08, 0x88, 0x80, 0x00, 0x88, 0x88, 0x88, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00, 0x88, 0x88, 0x00, 0x08,
 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x80, 0x08, 0x00, 0x08, 0x00, 0x08, 0xFF, 0xF0,
 0x08, 0xFF, 0xF0, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x80, 0x80, 0x8F, 0x00, 0x80, 0x08, 0xFF, 0xF0, 0x00, 0x8F, 0xFF, 0xF8, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00, 0x8F, 0xFF, 0x00, 0x08,
 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0xF8, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00,
 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x80, 0x88, 0xF0, 0x00, 0x80, 0x08, 0x00, 0x00, 0x00, 0x80, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x08,
 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x8F, 0x0F, 0x8F, 0x08, 0x88, 0x88, 0x0F, 0x88, 0x88,
 0x0F, 0x88, 0x88, 0x08, 0x88, 0x88, 0x0F, 0x88, 0x88, 0xF0, 0x8F, 0x00, 0x00, 0x80, 0x0F, 0x88, 0x88, 0x80, 0xF8, 0x88, 0x8F, 0x08, 0x88, 0x88, 0x08, 0x00, 0x00, 0x00, 0xF8, 0x88, 0x88, 0x08,
 0x88, 0x88, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0xF0, 0x0F, 0xFF, 0xFF, 0x00, 0xFF, 0xFF,
 0x00, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xF0, 0x00, 0x00, 0xF0, 0x00, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0x0F,
 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
};

const unsigned char gamewin[] ={
 0x42, 0x4D, 0x76, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x88, 0x80, 0x08, 0x88, 0x80, 0x08, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x08, 0x08, 0x88, 0x88, 0x00, 0x08,
 0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8F, 0xFF, 0xF0, 0x8F, 0xFF, 0xF8, 0x08, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x08, 0x08, 0xFF, 0xFF, 0x80, 0x8F,
 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08, 0x88, 0x88, 0x00, 0x88, 0x88, 0x88, 0x08, 0x00, 0x00, 0x80, 0x88,
 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x00, 0x80, 0x08, 0x08, 0xFF, 0xFF, 0x80, 0x8F, 0xFF, 0xF8, 0x08, 0x00, 0x00, 0x80, 0x8F,
 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x08, 0xF8, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x88, 0x80, 0xF8, 0x88, 0x8F, 0x0F, 0x8F, 0x0F, 0x8F, 0x0F, 0x88, 0x88, 0xF0, 0xF8, 0x88, 0x8F, 0x08, 0x88, 0x88, 0xF0, 0xF8,
 0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xF0, 0x0F, 0xFF, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xFF, 0xFF, 0x00, 0x0F, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0x00, 0x0F,
 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x08, 0x88, 0x88, 0x08, 0x88, 0x80,
 0x08, 0x88, 0x80, 0x08, 0x88, 0x88, 0x00, 0x88, 0x88, 0x00, 0x80, 0x00, 0x00, 0x80, 0x08, 0x00, 0x00, 0x80, 0x08, 0x88, 0x80, 0x08, 0x88, 0x80, 0x08, 0x88, 0x80, 0x08, 0x00, 0x00, 0x08, 0x08,
 0x00, 0x00, 0x00, 0x08, 0x88, 0x80, 0x88, 0x88, 0x80, 0x88, 0x88, 0x80, 0x08, 0x00, 0x00, 0x80, 0x08, 0x88, 0x80, 0x88, 0x88, 0x00, 0x08, 0x00, 0x00, 0x08, 0x0F, 0xF8, 0xFF, 0x0F, 0xFF, 0xF8,
 0x0F, 0xFF, 0xF8, 0x0F, 0xF8, 0xFF, 0x08, 0xFF, 0xFF, 0x80, 0x80, 0x00, 0x08, 0x80, 0x08, 0x00, 0x00, 0x80, 0x8F, 0xFF, 0xF0, 0x8F, 0xFF, 0xF0, 0x8F, 0xFF, 0xF8, 0x08, 0x00, 0x00, 0x08, 0x08,
 0x00, 0x00, 0x00, 0x8F, 0xFF, 0xF0, 0xFF, 0x8F, 0xF0, 0xFF, 0xFF, 0xF8, 0x08, 0x00, 0x00, 0x80, 0x8F, 0xFF, 0xF0, 0x8F, 0xFF, 0x80, 0x08, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08,
 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x8F, 0x80, 0x08, 0x88, 0x88, 0x80, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08,
 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x00, 0x80, 0x08, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x88, 0x8F,
 0x00, 0x88, 0x8F, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x80, 0x80, 0x08, 0xF0, 0x80, 0x08, 0xFF, 0xFF, 0x80, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08,
 0x88, 0x88, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x08, 0x88, 0x8F, 0x08, 0x88, 0x88, 0x80, 0x88, 0x88, 0x00, 0x80, 0x00, 0x80, 0x08, 0x00, 0x80, 0x08, 0x00, 0x08, 0x00, 0x08, 0xFF, 0xF0,
 0x08, 0xFF, 0xF0, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x80, 0x80, 0x8F, 0x00, 0x80, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x00, 0x80, 0x08, 0x08,
 0xFF, 0xFF, 0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x8F, 0xFF, 0xF0, 0x08, 0xFF, 0xFF, 0x80, 0x8F, 0xFF, 0x00, 0x80, 0x00, 0x80, 0x08, 0x08, 0xF8, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00,
 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x80, 0x88, 0xF0, 0x00, 0x80, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x08, 0x08, 0x08, 0xF8, 0x08, 0x08,
 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x08, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x00, 0x80, 0x0F, 0x8F, 0x0F, 0x8F, 0x08, 0x88, 0x88, 0x0F, 0x88, 0x88,
 0x0F, 0x88, 0x88, 0x08, 0x88, 0x88, 0x0F, 0x88, 0x88, 0xF0, 0x8F, 0x00, 0x00, 0x80, 0x0F, 0x88, 0x88, 0xF0, 0xF8, 0x88, 0x80, 0xF8, 0x88, 0x80, 0xF8, 0x88, 0x8F, 0x0F, 0x8F, 0x0F, 0x8F, 0x0F,
 0x88, 0x88, 0xF0, 0x80, 0x00, 0x00, 0x88, 0x88, 0x80, 0xF8, 0x88, 0x88, 0x08, 0x00, 0x00, 0x80, 0xF8, 0x88, 0x80, 0x88, 0x88, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x0F, 0xFF, 0xFF, 0x00, 0xFF, 0xFF,
 0x00, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xF0, 0x00, 0x00, 0xF0, 0x00, 0xFF, 0xFF, 0x00, 0x0F, 0xFF, 0xF0, 0x0F, 0xFF, 0xF0, 0x0F, 0xFF, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00,
 0xFF, 0xFF, 0x00, 0xF0, 0x00, 0x00, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0xF0, 0x0F, 0xFF, 0xF0, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

const unsigned char pause[] ={
 0x42, 0x4D, 0x36, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
 0x0F, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0,
 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0xF0,
 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0xFF,
 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0xFF, 0xFF, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF,
 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xF0, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x0F, 0x0F,
 0x00, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xF0, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0xF0,
 0xF0, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00,
 0xF0, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0,
 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x0F, 0xFF, 0xFF, 0xF0, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0xFF, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF,
};

const unsigned char life[] ={
 0x42, 0x4D, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 

	0x00, 0xFF, 0x00, 
	0x0F, 0xFF, 0xF0, 
	0xFF, 0xFF, 0xFF, 
	0x0F, 0x00, 0xF0,
	
	0x00, 0x00, 0x00,
	0x00, 0x00,

};