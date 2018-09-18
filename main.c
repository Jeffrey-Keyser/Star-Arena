//*****************************************************************************
//STAR ARENA
//*****************************************************************************

#include "lab_buttons.h"
#include "ece210_api.h"
#include "sprite.h"
#include "stdlib.h"
#include <math.h>

#define SCREEN_WIDTH 240
#define SCREEN_HIEGHT 320

#define AIMER_RADIUS_PXL 15

#define JOYSTICK_ANGULAR_TOLERANCE 200 

#define HEALTH_LED_TIMEOUT 20

#define MAX_PROJECTILES 32
#define PROJECTILE_SPEED 3

#define SPRITE_MAX_HEALTH 100

#define CYCLE_TIMEOUT 20//in milliseconds, how long each game cycle is

#define DIR_TO_RAD(t) ((double)t*(2*PI/255.f) + PI)

#define PI 3.14159265
#define JOY_MAX 4095

//this is for assigning owners to projectiles
#define NO_ONE 0
#define HOST 1
#define CLIENT 2

#define LOCAL_ID     0x01
#define REMOTE_ID    0x00

//game states
#define GAME_IN_SESSION 0
#define GAME_LOST 1
#define GAME_WON 2

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

void init_sprite(struct sprite *sp, const uint8_t *image);
void init_aimer(struct aimer *aim, const uint8_t *image);
void init_projectiles(struct proj *pj);

void draw_sprite(struct sprite *sp, uint8_t clear);
void draw_aimer(struct aimer *aim, uint16_t color);
void draw_projectiles(struct proj *pj, uint16_t color);

void update_sprite(struct sprite *sp, struct board_state *bs, struct proj *enemy_pj, uint16_t normal_color, struct sprite *enemy, bool bot);
void update_aimer(struct aimer *aim, struct sprite *sp);
void update_projectiles(struct proj *pj, struct sprite *sp, struct board_state *bs, struct aimer *aim, uint8_t owner, bool auto_shoot);

void set_board_state(struct board_state *bs);

//static variables
static int32_t health_led_counter;
static uint8_t game_state = true;

int main(void)
{
	ece210_initialize_board();
	
	struct board_state host_bs;
	struct board_state client_bs;
	
	//set and initialize host and client sprites, aimers, and projectiles
	struct sprite host_player;
	struct sprite client_player;
	struct aimer host_aimer;
	struct aimer client_aimer;
	
	struct proj *host_proj = (struct proj *)malloc(sizeof(struct proj) * MAX_PROJECTILES);
	struct proj *client_proj = (struct proj *)malloc(sizeof(struct proj) * MAX_PROJECTILES);
	
	init_sprite(&host_player, sprite_image);
	init_sprite(&client_player, sprite_image);
	
	init_aimer(&host_aimer, aimer_image);
	init_aimer(&client_aimer, aimer_image);
	
	init_projectiles(host_proj);
	init_projectiles(client_proj);
	
	uint8_t led_status = 0;
	game_state = GAME_IN_SESSION;
	//======================================
		
	ece210_lcd_add_msg("Hold the board with joystick on the top. Click down to be the host, up to be the client.", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
	ece210_lcd_add_msg("", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
	ece210_lcd_add_msg("You will need a host and client board for the game to start.", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
	ece210_lcd_add_msg("", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
	ece210_lcd_add_msg("Choose wisely.", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);


	static bool started = false;
	static bool initialized = false; 
	static bool host = false;
	static bool client = false;
	
	while(!started){
		if(AlertButtons)
		{
				AlertButtons = false;
				if(btn_up_pressed())
				{
					host = true;
					ece210_wireless_init(LOCAL_ID,REMOTE_ID);
					ece210_lcd_add_msg("", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
					ece210_lcd_add_msg("You are the host. Ensure the other board has the client initiated. Then, click left and right to start.", TERMINAL_ALIGN_CENTER,LCD_COLOR_YELLOW);
					started = true;
				}

				if(btn_down_pressed())
				{
					client = true;
					ece210_wireless_init(REMOTE_ID,LOCAL_ID);
					ece210_lcd_add_msg("", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
					ece210_lcd_add_msg("You are the client. Wait for host to start.", TERMINAL_ALIGN_CENTER,LCD_COLOR_YELLOW);
					started = true;
				}
		}
	}
	
	while (!initialized){
		if (client) {
			uint32_t init_data;
			if (ece210_wireless_data_avaiable()) 
					init_data = ece210_wireless_get();
					if (init_data == 0xFFFFFFFF){ //make sure not getting data from other board	
						initialized = true;
				}
		}
		
		if (host){
			if(AlertButtons)
			{
				AlertButtons = false;
				if(btn_left_pressed())
				{
					ece210_lcd_add_msg("Game will commence soon. Get Hyped.", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
					ece210_lcd_add_msg("", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);

					ece210_lcd_add_msg("Use push buttons to move. Make sure to move at start.", TERMINAL_ALIGN_CENTER, LCD_COLOR_GREEN);
					ece210_lcd_add_msg("", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
					ece210_lcd_add_msg("Shoot by moving the joystick.", TERMINAL_ALIGN_CENTER,LCD_COLOR_GREEN);
					ece210_lcd_add_msg("", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
					ece210_lcd_add_msg("Avoid white projectiles from the boss.", TERMINAL_ALIGN_CENTER,LCD_COLOR_GREEN);
					ece210_wait_mSec(4000);
					ece210_wireless_send(0xFFFFFFFF);
					initialized = true;
				}
			}		
			}
	}

	if (client){
		ece210_lcd_add_msg("I am the client and will enjoy this game.", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
		ece210_wait_mSec(1000);
		ece210_lcd_draw_rectangle(0,240,0,320, LCD_COLOR_BLACK);
		while(1){
			ece210_lcd_draw_circle(0x78, 0xA0, 0x30, 0x12);
			ece210_lcd_print_string("Star Arena", 0x78, 0xA0, LCD_COLOR_GREEN, LCD_COLOR_BLUE2);
			
			while(1){   //240x320 
				for (int i =0; i<8;i++){
				ece210_ws2812b_write(i, i+0x10, i+0x10, i+0x10);
				ece210_wait_mSec(50);
				}
				for (int i = 7; i>=0;i--){
				ece210_ws2812b_write(i, 0, 0x50, 0);
				ece210_wait_mSec(50);
				}
				ece210_wait_mSec(50);
			}
			
		}
	}

	if(host) {
		ece210_lcd_add_msg("I am the host and will enjoy this game.", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
		ece210_wait_mSec(1000);
		for(int i = 0; i < 30; i++){
			ece210_lcd_add_msg(" ", TERMINAL_ALIGN_CENTER,LCD_COLOR_BLACK);
		}
		
		ece210_lcd_draw_rectangle(0,240,0,320, LCD_COLOR_BLACK);
		while(game_state == GAME_IN_SESSION){
			
			ece210_wait_mSec(CYCLE_TIMEOUT);
			
			if(health_led_counter > 0){
				health_led_counter -= CYCLE_TIMEOUT;
				led_status = 0xFF;
			}else{
				led_status = 0x00;
			}
			ece210_red_leds_write(led_status);
			
			//update host player button state
			set_board_state(&host_bs);
			
			//update host/client sprite position + orientation
			
			//draw black over previous images
			
			draw_sprite(&host_player, 1);
			draw_aimer(&host_aimer, 0x0000);
			draw_projectiles(host_proj, 0x0000);
			
			draw_sprite(&client_player, 1);
			draw_aimer(&client_aimer, 0x0000);
			draw_projectiles(client_proj, 0x0000);
			
			//update the host sprite, aimer, and projectiles
			update_sprite(&host_player, &host_bs, client_proj, LCD_COLOR_MAGENTA, &client_player, false);
			update_aimer(&host_aimer, &host_player);
			update_projectiles(host_proj, &host_player, &host_bs, &host_aimer, HOST, false);
			
			update_sprite(&client_player, &client_bs, host_proj, LCD_COLOR_MAGENTA, &host_player, true);
			update_aimer(&client_aimer, &client_player);
			update_projectiles(client_proj, &client_player, &client_bs, &client_aimer, CLIENT, true);
			
			
			draw_sprite(&host_player, 0);
			draw_aimer(&host_aimer, LCD_COLOR_BLUE);
			draw_projectiles(host_proj, LCD_COLOR_ORANGE);
			
			draw_sprite(&client_player, 0);
			draw_aimer(&client_aimer, LCD_COLOR_GREEN);
			draw_projectiles(client_proj, LCD_COLOR_WHITE);
			
		}
		
		if(game_state == GAME_LOST){
			ece210_lcd_add_msg("You Lost!", TERMINAL_ALIGN_CENTER, 0xE304);
		}else{
			ece210_lcd_add_msg("You Won!", TERMINAL_ALIGN_CENTER, 0xE746);
		}
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
		pj[i].damage = 2;
	}
	
}

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

void update_sprite(struct sprite *sp, struct board_state *bs, struct proj *enemy_pj, uint16_t normal_color, struct sprite *enemy, bool bot){
	
	if(!bot){
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
	}else{//if bot
		
		float joy_y = (float)(enemy->pos_y + enemy->height)/2.f - (float)(sp->pos_y + sp->height)/2.f;
		float joy_x = (float)(enemy->pos_x + enemy->width)/2.f - (float)(sp->pos_x + sp->width)/2.f;
		
		float dir = atan2f(joy_y, joy_x);
		
		dir += PI;
		
		/*
		if(dir < 0){
			dir += 2*PI;
		}
		*/
		
		sp->dir_t = (uint8_t)((dir)* (((float)255)/(2*PI)));
	}
	
	//taking damage
	sp->color = normal_color;
	if(sp->health > 0){
		for(int i = 0; i < MAX_PROJECTILES; i ++){//if hit by a projectile, remove projectile and lower health
			if(enemy_pj[i].belongs_to != NO_ONE){
				if(enemy_pj[i].pos_x >= sp->pos_x && enemy_pj[i].pos_x <= sp->pos_x + sp->width){
					if(enemy_pj[i].pos_y >= sp->pos_y && enemy_pj[i].pos_y <= sp->pos_y + sp->height){
						enemy_pj[i].belongs_to = NO_ONE;
						sp->health -= enemy_pj[i].damage;
						sp->color = LCD_COLOR_RED;
						if(!bot){
							health_led_counter = HEALTH_LED_TIMEOUT;
						}
						
					}
				}
			}
		}
	}else{
		sp->color = LCD_COLOR_GRAY;
		if(!bot){
			game_state = GAME_LOST;
		}else{
			game_state = GAME_WON;
		}
		
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

void update_projectiles(struct proj *pj, struct sprite *sp, struct board_state *bs, struct aimer *aim, uint8_t owner, bool auto_shoot){
	
	float joy_y = (float)bs->joy_y - ((float)(JOY_MAX>>3))/2;
	float joy_x = (float)bs->joy_x - ((float)(JOY_MAX>>3))/2;
	
	//if projectiles to be added
	if(!auto_shoot){
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
	}else{
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

