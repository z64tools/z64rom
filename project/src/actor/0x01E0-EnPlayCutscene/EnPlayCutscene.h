#ifndef __EN_PLAY_CUTSCENE_ACTOR_H__
#define __EN_PLAY_CUTSCENE_ACTOR_H__

/**
 * Param (bit): XXYY IIZZ ZZZZ ZZZZZ
 *
 * X = Set Flag Type
 * Y = Dep Flag Type
 * I = Behaviour
 * Z = Distance (units) * 10.0f
 *
 * RotX (hex): XXYY
 *
 * X = Set Flag
 * Y = Dep Flag
 *
 * RotY (hex): ZZXY
 *
 * Z = ParamFlags (bit)
 *     0000 000X
 *     X = Dep Flag Flip
 *
 * X = Distance Check Type
 * Y = Scene Cutscene Header Index
 *
 */

#include <uLib.h>

struct EnPlayCutscene;

typedef void (*EnPlayCutsceneFunc)(struct EnPlayCutscene*, PlayState*);

typedef enum {
    FLAG_TYPE_SWITCH = 0,
    FLAG_TYPE_CHEST,
    FLAG_TYPE_COLLECTIBLE,
    FLAG_TYPE_GLOBAL,
} FlagType;

typedef enum {
    BEHAVIOUR_QUEUE = 0,
    BEHAVIOUR_FORCE,
} Behaviour;

typedef enum {
    DIST_XZ = 0,
    DIST_XYZ,
} DistType;

typedef struct {
    u8 type : 2;
    u8 val;
} Flag;

typedef enum {
    PARAM_FLIP_DEP_FLAG = 1 << 0,
} ParamSettings;

typedef struct EnPlayCutscene {
    Actor actor;
    Flag  depFlag;
    Flag  setFlag;
    f32   distance;
    struct {
        u16 queued      : 1;
        u16 headerIndex : 4;
        u16 behaviour   : 2;
        u16 distType    : 2;
    };
    ParamSettings settings;
} EnPlayCutscene;

#endif // __EN_PLAY_CUTSCENE_ACTOR_H__