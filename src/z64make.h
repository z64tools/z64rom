#include "tools.h"

struct Rom;

typedef enum {
	/* 0x00 */ ACTORCAT_SWITCH,
	/* 0x01 */ ACTORCAT_BG,
	/* 0x02 */ ACTORCAT_PLAYER,
	/* 0x03 */ ACTORCAT_EXPLOSIVE,
	/* 0x04 */ ACTORCAT_NPC,
	/* 0x05 */ ACTORCAT_ENEMY,
	/* 0x06 */ ACTORCAT_PROP,
	/* 0x07 */ ACTORCAT_ITEMACTION,
	/* 0x08 */ ACTORCAT_MISC,
	/* 0x09 */ ACTORCAT_BOSS,
	/* 0x0A */ ACTORCAT_DOOR,
	/* 0x0B */ ACTORCAT_CHEST
} ActorCategory;

#define OBJECT_ID_MAX 0x0400
#define ACTOR_ID_MAX  0x0400

typedef struct {
	/* 0x00 */ u16 id;
	/* 0x02 */ u8  category;
	/* 0x04 */ u32 flags;
	/* 0x08 */ u16 objectId;
	/* 0x0C */ u32 instanceSize;
	/* 0x10 */ u32 init;
	/* 0x14 */ u32 destroy;
	/* 0x18 */ u32 update;
	/* 0x1C */ u32 draw;
} ActorInit; // size = 0x20

typedef enum {
	PRE_GCC,
	POST_GCC,
	PRE_LD,
	POST_LD,
	MID_GCC,
} MakeCallType;

typedef enum {
	CB_BREAK = -1,
	CB_MAKE  = 1
} MakeCallbackReturn;

typedef s32 (* BinutilCallback)(const char*, MakeCallType, const char*, void*);

typedef struct MakeArg {
	const char* path;
	const char* flag;
	List*       itemList;
	void (* func)(struct MakeArg*);
	BinutilCallback callback;
	u32 i;
} MakeArg;

typedef void ThreadFunc;

void Make_Sequence(void);
void Make_Sound(void);
void Make_Code(void);
void Make(struct Rom* rom, s32 message);