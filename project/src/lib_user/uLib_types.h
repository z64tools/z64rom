#ifndef __ULIB_TYPES_H__
#define __ULIB_TYPES_H__

#include <z64hdr.h>

typedef struct {
    struct {
        u32 vanillaOsPrintf : 1;
        u32 dmaLog          : 1;
        u32 playerPrint     : 1;
        u32 isPlayGameMode  : 1;
    };
} LibState;

typedef struct {
    OSTime start;
    OSTime buffer[20];
    u8     ringId;
} DebugProfiler;

typedef struct {
    LibState state;
    
#ifdef DEV_BUILD
    
    struct {
        DebugProfiler fps;
        
        DebugProfiler actorUpdate;
        DebugProfiler ovlDraw;
        
        DebugProfiler cameraUpdate;
        
        DebugProfiler sceneDraw;
        
        DebugProfiler commonA;
        DebugProfiler commonB;
        DebugProfiler commonC;
        struct {
            u32 enabled : 1;
        };
    } profiler;
    struct {
        u32 cinematic : 1;
    };
    
#endif
    
    u32 __ctxInitValue;
} LibContext;

typedef struct {
    union {
        struct {
            u32 __isExit    : 1; // O*** **** ─ **** **** ─ **** **** ─ **** ****
            u32 musicOn     : 1; // *O** **** ─ **** **** ─ **** **** ─ **** ****
            u32 titleCard   : 1; // **O* **** ─ **** **** ─ **** **** ─ **** ****
            u32 fadeIn      : 6; // ***O OOOO ─ O*** **** ─ **** **** ─ **** ****
            u32 fadeOut     : 6; // **** **** ─ *OOO OOO* ─ **** **** ─ **** ****
            u32 headerIndex : 4; // **** **** ─ **** ***O ─ OOO* **** ─ **** ****
            u32 spawnIndex  : 5; // **** **** ─ **** **** ─ ***O OOOO ─ **** ****
            u32 sceneIndex  : 8; // **** **** ─ **** **** ─ **** **** ─ OOOO OOOO
        };
        struct {
            union {
                struct {
                    u16 upper;
                    u16 lower;
                };
                u32 value;
            };
        };
    };
} NewExit;

typedef struct  {
    u16 __pad1;
    union {
        struct {
            u16 isExit : 1;
            u16 __pad2 : 15;
        };
        u16 nextEntranceIndex;
    };
    NewExit exit;
    NewExit respawn[RESPAWN_MODE_MAX];
} ExitParam;

typedef struct Time {
    u8 hour;
    u8 minute;
} Time;

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

typedef struct {
    f32 h;
    f32 s;
    f32 l;
} Color_HSL;

typedef struct {
    f32 h;
    f32 s;
    f32 l;
    u8  alpha;
} Color_HSLA;

typedef struct {
    Vec3f pos, prevPos, vel;
    f32   mass;
} Particle;

typedef struct {
    Particle* p1;
    Particle* p2;
    f32       length;
} Chain;

#endif