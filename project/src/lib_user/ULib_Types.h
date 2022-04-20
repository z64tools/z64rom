#ifndef __ULIB_TYPES_H__
#define __ULIB_TYPES_H__

#include <oot_mq_debug/z64hdr.h>

typedef struct {
	u32 myMagicValue;
} LibContext;

typedef struct {
	// HIGH
	struct {
		u32 blockEpona   : 1;
		u32 lowerSurface : 1;
		u32 floorParams  : 4;
		u32 wallParams   : 5;
		u32 unk_001C0000 : 3;
		u32 behaviour    : 5;
		u32 exit         : 5;
		u32 camDataIndex : 8;
	};
	// LOW
	struct {
		u32 pad           : 4;
		u32 wallDamage    : 1;
		u32 conveyorDir   : 6;
		u32 conveyorSpeed : 3;
		u32 hookshot      : 1;
		u32 echo          : 6;
		u32 lightParams   : 5;
		u32 slope         : 2;
		u32 sfx           : 4;
	};
} PolygonTypes;

typedef enum {
	SURFACE_FLOOR_VOID_SMALL        = 0x5,
	SURFACE_FLOOR_HANG_LEDGE        = 0x6,
	SURFACE_FLOOR_STOP_AIR_MOMENTUM = 0x8,
	SURFACE_FLOOR_NO_LEDGE_JUMP     = 0x9,
	SURFACE_FLOOR_DIVE              = 0xB,
	SURFACE_FLOOR_VOID              = 0xC,
} SurfaceFloorParams;

typedef enum {
	SURFACE_WALL_NO_LEDGE_GRAB = 0x1,
	SURFACE_WALL_LADDER        = 0x2,
	SURFACE_WALL_LADDER_TOP    = 0x3,
	SURFACE_WALL_VINE          = 0x4,
	SURFACE_WALL_CRAWL_A       = 0x5,
	SURFACE_WALL_CRAWL_B       = 0x6,
	SURFACE_WALL_PUSH          = 0x7,
} SurfaceWallParams;

typedef enum {
	SURFACE_BEHAVIOUR_SPECIAL_UNK_1 = 0x1,
	SURFACE_BEHAVIOUR_HURT_SPIKES,
	SURFACE_BEHAVIOUR_HURT_LAVA,
	SURFACE_BEHAVIOUR_SAND,
	SURFACE_BEHAVIOUR_SLIPPERY,
	SURFACE_BEHAVIOUR_NO_FALL_DAMAGE,
	SURFACE_BEHAVIOUR_QUICKSAND,
	SURFACE_BEHAVIOUR_JABU_WALL,
	SURFACE_BEHAVIOUR_VOID_ON_CONTACT,
	SURFACE_BEHAVIOUR_UNK_A,
	SURFACE_BEHAVIOUR_LINK_LOOK_UP,
	SURFACE_BEHAVIOUR_QUICKSAND_EPONA
} SurfaceSpecialParams;

#endif