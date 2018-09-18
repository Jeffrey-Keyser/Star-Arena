//*****************************************************************************
// main.c
// Author: jkrachey@wisc.edu
//*****************************************************************************
#include "lab_buttons.h"
#include "sprite.h"
#include "stdlib.h"
#include <math.h>

/*
#define DEVICE_IS_HOST

#ifdef DEVICE_IS_HOST
#define LOCAL_ID     1
#define REMOTE_ID    0
#endif

#ifdef DEVICE_IS_CLIENT
#define LOCAL_ID     0
#define REMOTE_ID    1
#endif
*/

#define SCREEN_WIDTH 240
#define SCREEN_HIEGHT 320

#define AIMER_RADIUS_PXL 15

#define JOYSTICK_ANGULAR_TOLERANCE 200 

#define MAX_PROJECTILES 32
#define PROJECTILE_SPEED 5

#define NUMBER_OF_POWERUPS 2

#define POWERUP_DAMAGE_EXTRA 1
#define POWERUP_DEFENSE_EXTRA 0

#define SPRITE_MAX_HEALTH 100

//#define POWERUP_DAMAGE_TIMEOUT 500//in game cycles
//#define POWERUP_DEFENSE_TIMEOUT 750//in game cycles

//#define POWERUP_DAMAGE_ACTIVE_TIMEOUT 250//in game cycles
//#define POWERUP_DEFENSE_ACTIVE_TIMEOUT 775//in game cycles

#define CYCLE_TIMEOUT 20//in milliseconds, how long each game cycle is

#define DIR_TO_RAD(t) ((double)t*(2*PI/255.f) + PI)

#define PI 3.14159265
#define JOY_MAX 4095

//this is for assigning owners to projectiles
#define NO_ONE 0
#define HOST 1
#define CLIENT 2

struct board_state{
	uint32_t up: 1;
	uint32_t down: 1;
	uint32_t left: 1;
	uint32_t right: 1;
	uint32_t joy_x: 13;
	uint32_t joy_y: 13;
	uint32_t extra: 2;
};

//all sprites are to be the same size
struct sprite{
	uint8_t image[SPRITE_IMAGE_SIZE];
	int8_t width;
	int8_t height;
	int16_t pos_x;
	int16_t pos_y;
	uint8_t dir_t;//this is an angular measure where 0 -> 0 radians, and 255 -> 2PI radians
	float speed;
	int16_t health;
	uint16_t color;
};

struct aimer{
	uint8_t image[AIMER_IMAGE_SIZE];
	uint8_t width;
	uint8_t height;
	uint16_t pos_x;
	uint16_t pos_y;
};

struct proj{
	uint8_t belongs_to;
	int32_t pos_x;
	int32_t pos_y;
	uint8_t dir_t;//direction in which the projectile is travelling
	//this is an angular measure where 0 -> 0 radians, and 255 -> 2PI radians
	uint8_t damage;
};
/*
struct power{
	uint8_t active: 1;
	uint8_t belongs_to: 3;//NO_ONE, HOST, CLIENT
	uint8_t type: 1;
	uint8_t extra: 3;
	uint16_t color_f;//foreground
	uint16_t color_b;//foreground
	uint16_t count_down;//in game cycles
	uint16_t pos_x;
	uint16_t pos_y;
};
*/

void init_sprite(struct sprite *sp, const uint8_t *image);
void init_aimer(struct aimer *aim, const uint8_t *image);
void init_projectiles(struct proj *pj);
//void init_powerups(struct power *pwr, uint16_t b_color, uint16_t f_color);

void draw_sprite(struct sprite *sp, uint8_t clear);
void draw_aimer(struct aimer *aim, uint16_t color);
void draw_projectiles(struct proj *pj, uint16_t color);
//void draw_powerups(struct power *pwr, struct sprite *host, struct sprite *client);

void update_sprite(struct sprite *sp, struct board_state *bs, struct proj *enemy_pj);
void update_aimer(struct aimer *aim, struct sprite *sp);
void update_projectiles(struct proj *pj, struct sprite *sp, struct board_state *bs, struct aimer *aim, uint8_t owner);
//void update_powerups(struct power *pwr);

void set_board_state(struct board_state *bs);

int main(void) {
  
  ece210_initialize_board();
	
  //ece210_wireless_init(LOCAL_ID, REMOTE_ID);
  
	struct board_state bs;
	
	struct sprite host_player;
	//struct sprite client_player;
	struct aimer host_aimer;
	
	//allocates 8 projectiles
	struct proj *host_proj = (struct proj *)malloc(sizeof(struct proj) * MAX_PROJECTILES);
	
	init_sprite(&host_player, sprite_image);
	init_aimer(&host_aimer, aimer_image);
	init_projectiles(host_proj);
	
  while(1)
  {
    ece210_wait_mSec(CYCLE_TIMEOUT);
		
		//draw black over previous images
		draw_sprite(&host_player, 1);
		draw_aimer(&host_aimer, 0x0000);
		draw_projectiles(host_proj, 0x0000);
		
		//set the button state of this board
		set_board_state(&bs);
		
		//update the host sprite, aimer, and projectiles
		update_sprite(&host_player, &bs, host_proj);
		update_aimer(&host_aimer, &host_player);
		update_projectiles(host_proj, &host_player, &bs, &host_aimer, HOST);
		
		//draw the host sprite, aimer, and projectiles
		draw_sprite(&host_player, 0);
		draw_aimer(&host_aimer, LCD_COLOR_BLUE);
		draw_projectiles(host_proj, LCD_COLOR_ORANGE);
		
		//char msg[80];
		//sprintf(msg, "u:%d,d:%d,l:%d,r:%d,jx:%x,jy:%x", bs.up, bs.down, bs.left, bs.right, bs.joy_x, bs.joy_y);
		//ece210_lcd_add_msg(msg, TERMINAL_ALIGN_CENTER, LCD_COLOR_BLUE);
		
		/*
		if(ece210_wireless_data_avaiable())
    {
      rx_data = ece210_wireless_get();
      if( rx_data == UP_BUTTON)
      {
        ece210_lcd_add_msg("Remote UP BUTTON Pressed", TERMINAL_ALIGN_CENTER, LCD_COLOR_RED);
      }
    }
    */
		
  }
	
	return 0;
}

void init_sprite(struct sprite *sp, const uint8_t *image){
	
	for(int i = 0, n = SPRITE_IMAGE_SIZE; i < n; i++){
	
		sp->image[i] = image[i];
	
	}
	
	sp->width = SPRITE_WIDTH_PXL;
	sp->height = SPRITE_HEIGHT_PXL;
	sp->pos_x = SCREEN_WIDTH/2;
	sp->pos_y = SCREEN_HIEGHT/2;
	sp->dir_t = PI/2;
	sp->speed = 4.f;
	sp->health = SPRITE_MAX_HEALTH;
}

void init_aimer(struct aimer *aim, const uint8_t *image){
	for(int i = 0, n = AIMER_IMAGE_SIZE; i < n; i++){
	
		aim->image[i] = image[i];
	
	}
	
	aim->width = AIMER_WIDTH_PXL;
	aim->height = AIMER_HEIGHT_PXL;
	aim->pos_x = SCREEN_WIDTH/2;
	aim->pos_y = SCREEN_HIEGHT/2;
	
}

void init_projectiles(struct proj *pj){
	
	for(int i = 0; i < MAX_PROJECTILES; i++){
		pj[i].belongs_to = NO_ONE;
		pj[i].pos_x = 0;
		pj[i].pos_y = 0;
		pj[i].damage = 5;
	}
	
}

/*
void init_powerups(struct power *pwr, uint16_t b_color, uint16_t f_color){
	//pwr is an array of struct power
	for(int i = 0; i < NUMBER_OF_POWERUPS; i++){
		pwr[i].active = 0;
		pwr[i].belongs_to = NO_ONE;
		pwr[i].color_b = b_color;
		pwr[i].color_f = f_color;
		pwr[i].extra = 0;
		pwr[i].type = POWERUP_DAMAGE_EXTRA;
		pwr[i].count_down = 0;
	}
}
*/

void draw_sprite(struct sprite *sp, uint8_t clear){
	if(clear){
		ece210_lcd_draw_image(sp->pos_x, sp->width, sp->pos_y, sp->height, sp->image, 0x0000, LCD_COLOR_BLACK); 
	}else{
		ece210_lcd_draw_image(sp->pos_x, sp->width, sp->pos_y, sp->height, sp->image, sp->color, LCD_COLOR_BLACK); 
	}
}

void draw_aimer(struct aimer *aim, uint16_t color){
	ece210_lcd_draw_image(aim->pos_x, aim->width, aim->pos_y, aim->height, aim->image, color, LCD_COLOR_BLACK); 
}

void draw_projectiles(struct proj *pj, uint16_t color){
	for(int i = 0; i < MAX_PROJECTILES; i++){
		
		if(pj[i].belongs_to != NO_ONE){
			ece210_lcd_draw_image(pj[i].pos_x, PROJECTILE_WIDTH_PXL, pj[i].pos_y, PROJECTILE_HEIGHT_PXL, projectile_image, color, LCD_COLOR_BLACK); 
		}
		
	}
}

/*
void draw_powerups(struct power *pwr, struct sprite *host, struct sprite *client){
	for(int i = 0; i < NUMBER_OF_POWERUPS; i++){
		if(pwr[i].active){
			
			if(pwr[i].belongs_to == HOST){//belongs to host
				ece210_lcd_draw_image(host->pos_x, POWERUP_WIDTH_PXL, host->pos_y, POWERUP_HEIGHT_PXL, powerup_image, pwr[i].color_f, pwr[i].color_b);
			}else{//belongs to client
				ece210_lcd_draw_image(client->pos_x, POWERUP_WIDTH_PXL, client->pos_y, POWERUP_HEIGHT_PXL, powerup_image, pwr[i].color_f, pwr[i].color_b);
			}
			
			
			//else is waiting to be taken
			
		}//else do not draw
	}
}

void update_powerups(struct power *pwr){
	for(int i = 0; i < NUMBER_OF_POWERUPS; i++){
		if(!pwr[i].active){//if is not active
			
		}
	}
}
*/

void update_sprite(struct sprite *sp, struct board_state *bs, struct proj *enemy_pj){
	
	float y_net = 0.f;
	if(bs->up > bs->down){
		y_net = -1.f;
	}else if(bs->up < bs->down){
		y_net = 1.f;
	}else{
		y_net = 0.f;
	}
	
	float x_net = 0.f;
	if(bs->right > bs->left){
		x_net = 1.f;
	}else if(bs->right < bs->left){
		x_net = -1.f;
	}else{
		x_net = 0.f;
	}
	
	float hypot = sqrt(x_net*x_net + y_net*y_net);
	
	sp->pos_x += (int8_t)(sp->speed*x_net/hypot);
	sp->pos_y += (int8_t)(sp->speed*y_net/hypot);
	
	if(sp->pos_x + sp->width > SCREEN_WIDTH){
		sp->pos_x = SCREEN_WIDTH - sp->width;
	}else if(sp->pos_x < 0){
		sp->pos_x = 0;
	}
	
	if(sp->pos_y + sp->height > SCREEN_HIEGHT){
		sp->pos_y = SCREEN_HIEGHT - sp->width;
	}else if(sp->pos_y < 0){
		sp->pos_y = 0;
	}
	
	float joy_y = (float)bs->joy_y - ((float)(JOY_MAX>>3))/2;
	float joy_x = (float)bs->joy_x - ((float)(JOY_MAX>>3))/2;
	
	float dir = atan2f(joy_y, joy_x);
	//this is a crude mapping between angular potentiometer values and direction of the projection on the board
	
	if(dir < 0){
		dir += 2*PI;
	}
	
	//now dir is between 0 and 2PI
	
	sp->dir_t = (uint8_t)(dir * (((float)255)/(2*PI)));
	//now sp->dir_t is angular measure where 0 -> 0 radians, and 255 -> 2PI radians
	
	//taking damage
	sp->color = 0xE0DD;
	if(sp->health > 0){
		for(int i = 0; i < MAX_PROJECTILES; i ++){//if hit by a projectile, remove projectile and lower health
			if(enemy_pj[i].belongs_to != NO_ONE){
				if(enemy_pj[i].pos_x >= sp->pos_x && enemy_pj[i].pos_x <= sp->pos_x + sp->width){
					if(enemy_pj[i].pos_y >= sp->pos_y && enemy_pj[i].pos_y <= sp->pos_y + sp->height){
						enemy_pj[i].belongs_to = NO_ONE;
						sp->health -= enemy_pj[i].damage;
						sp->color = 0xE8A2;
					}
				}
			}
		}
	}else{
		sp->color = 0xA4D3;
	}
	
}

void update_aimer(struct aimer *aim, struct sprite *sp){
	int16_t center_x = sp->pos_x + sp->width/2;
	int16_t center_y = sp->pos_y + sp->height/2;
	
	int16_t offset_x = AIMER_RADIUS_PXL * cos(DIR_TO_RAD(sp->dir_t));
	int16_t offset_y = AIMER_RADIUS_PXL * sin(DIR_TO_RAD(sp->dir_t));
	
	aim->pos_x = (uint16_t)(center_x + offset_x);
	aim->pos_y = (uint16_t)(center_y + offset_y);
	
}

void update_projectiles(struct proj *pj, struct sprite *sp, struct board_state *bs, struct aimer *aim, uint8_t owner){
	
	float joy_y = (float)bs->joy_y - ((float)(JOY_MAX>>3))/2;
	float joy_x = (float)bs->joy_x - ((float)(JOY_MAX>>3))/2;
	
	//if projectiles to be added
	if(sqrt(joy_y*joy_y + joy_x*joy_x) > JOYSTICK_ANGULAR_TOLERANCE){
		
		//iterate until can set a new projectile
		for(int i = 0; i < MAX_PROJECTILES; i++){
			
			if(pj[i].belongs_to == NO_ONE){
				pj[i].belongs_to = owner;
				pj[i].dir_t = sp->dir_t;
				pj[i].pos_x = aim->pos_x;
				pj[i].pos_y = aim->pos_y;
				
				i = MAX_PROJECTILES;//exit out of the for loop
			}
			
		}
	}
	
	//update position of all projectiles along direction
	for(int i = 0; i < MAX_PROJECTILES; i++){
	
		if(pj[i].belongs_to == owner){
			pj[i].pos_x += cos(DIR_TO_RAD(pj[i].dir_t))*PROJECTILE_SPEED;
			pj[i].pos_y += sin(DIR_TO_RAD(pj[i].dir_t))*PROJECTILE_SPEED;
		}
		
		//if out of range of screen, remove
		if(pj[i].pos_x > 240 || pj[i].pos_y > 320 || pj[i].pos_x <= 0 || pj[i].pos_y <= 0){
			pj[i].pos_x = 0;
			pj[i].pos_y = 0;
			pj[i].belongs_to = NO_ONE;
		}
	}	
}

void set_board_state(struct board_state *bs){
	
	//read joystick values
		
	uint16_t joy_x = ece210_ps2_read_x();
	uint16_t joy_y = ece210_ps2_read_y();
	
	//downresolution by a factor of 8 (2^3)
	bs->joy_x = joy_x >> 3;
	bs->joy_y = joy_y >> 3;
	
	if(AlertButtons){
		AlertButtons = false;
		
		//read button values
		
		uint8_t buttons = ece210_buttons_read();
		
		if((buttons & 0x01) == 0x01){//if up pressed
			bs->up = 1;
    }else{
			bs->up = 0;
		}
		
		if((buttons & 0x02) == 0x02){//if down pressed
			bs->down = 1;
    }else{
			bs->down = 0;
		}
      
		if((buttons & 0x04) == 0x04){//if left pressed
			bs->left = 1;
    }else{
			bs->left = 0;
		}
		
		if((buttons & 0x08) == 0x08){//if right pressed
			bs->right = 1;
    }else{
			bs->right = 0;
		}
		
	}
	
	bs->extra = 0;
	
}
