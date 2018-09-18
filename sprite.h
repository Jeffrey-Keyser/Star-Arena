#ifndef __SPRITE_H__
#define __SPRITE_H__

#include "ece210_api.h"

extern const uint8_t sprite_image[];
#define SPRITE_WIDTH_PXL 16
#define SPRITE_HEIGHT_PXL 16
#define SPRITE_IMAGE_SIZE 32

extern const uint8_t aimer_image[];

#define AIMER_WIDTH_PXL 2
#define AIMER_HEIGHT_PXL 2
#define AIMER_IMAGE_SIZE 2

extern const uint8_t projectile_image[];

#define PROJECTILE_WIDTH_PXL 2
#define PROJECTILE_HEIGHT_PXL 2
#define PROJECTILE_IMAGE_SIZE 2

extern const uint8_t powerup_image[];

#define POWERUP_WIDTH_PXL 8
#define POWERUP_HEIGHT_PXL 8
#define POWERUP_IMAGE_SIZE 8

#endif
