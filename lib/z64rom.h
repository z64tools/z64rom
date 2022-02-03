#ifndef __Z64ROM_H__
#define __Z64ROM_H__

#include "ExtLib.h"
#include "Rom.h"
#include "Dma.h"
#include "Audio.h"
#include "Mips64.h"

typedef void (* PatchFunc)(struct Rom*, MemFile*, MemFile*, char*);

typedef struct {
	u32 hi;
	u32 lo;
} HiLo;

typedef struct {
	u8 sceneIndex;
	struct {
		u8 bottles : 2;
		u8 aButton : 2;
		u8 bButton : 2;
		u8 unused  : 2;
	};
	struct {
		u8 warpSong  : 2;
		u8 ocarina   : 2;
		u8 hookshot  : 2;
		u8 tradeItem : 2;
	};
	struct {
		u8 all     : 2;
		u8 din     : 1;
		u8 nayry   : 1;
		u8 farore  : 2;
		u8 sunSong : 2;
	};
} RestrictionFlag;

typedef enum {
	NoRom = 0,
	Zelda_OoT_Debug,
	Zelda_OoT_1_0,
	Zelda_MM_U,
} AttPacked RomType;

typedef struct {
	struct {
		u32 dmaTable;
		u32 objTable;
		u32 actorTable;
		u32 effectTable;
		u32 stateTable;
		u32 sceneTable;
		u32 kaleidoTable;
		
		u32 seqFontTbl;
		u32 seqTable;
		u32 fontTable;
		u32 sampleTable;
		
		u32 restrictionFlags;
		struct {
			HiLo init;
			HiLo dest;
			HiLo updt;
			HiLo draw;
		} player;
		struct {
			HiLo init;
			HiLo updt;
		} pauseMenu;
	} table;
	struct {
		u32 seqRom;
		u32 fontRom;
		u32 smplRom;
	} segment;
} RomOffset;

typedef struct {
	u32 start : 28;
	// 4
	u32 size  : 24;
	u32 free  : 1;
	// 7
} Dma;

typedef struct Rom {
	RomType   type;
	MemFile   file;
	MemFile   config;
	RomOffset offset;
	struct {
		DmaEntry*        dma;
		ActorEntry*      actor;
		SceneEntry*      scene;
		EffectEntry*     effect;
		ObjectEntry*     object;
		KaleidoEntry*    kaleido;
		GameStateEntry*  state;
		
		RestrictionFlag* restrictionFlags;
		struct {
			u16 dma;
			u16 obj;
			u16 actor;
			u16 effect;
			u16 state;
			u16 scene;
			u16 kaleido;
		} num;
	} table;
	struct {
		MemFile sampleTbl;
		MemFile fontTbl;
		MemFile seqTbl;
		MemFile seqFontTbl;
	} mem;
	Dma dma[2000];
} Rom;

typedef struct N64AudioInfo {
	u32   sampleRate;
	u8    halfPrec;
	const struct N64AudioInfo* dublicate;
	char* name;
} N64AudioInfo;

typedef struct  {
	union {
		struct {
			u32 codec     : 4;
			u32 medium    : 2;
			u32 unk_bit26 : 1;
			u32 unk_bit25 : 1;
			u32 size      : 24;
		} infoBE;
		struct {
			u32 size      : 24;
			u32 unk_bit25 : 1;
			u32 unk_bit26 : 1;
			u32 medium    : 2;
			u32 codec     : 4;
		} infoLE;
		u32 data;
	};
	void32 sampleAddr; // u8*
	void32 loop; // AdpcmLoop*
	void32 book; // AdpcmBook*
	f32    tuning;
	u8 isPrim;
	u8 splitLo;
	u8 splitHi;
} SampleInfo;

extern const char* gObjectName[];
extern const char* gActorName[];
extern const char* gEffectName[];
extern const char* gStateName[6];
extern const char* gKaleidoName[2];
extern const char* gSceneName[];
extern const char* gBankName[];
extern const char* gSequenceName[];
extern const N64AudioInfo gSampleInfo[];

extern s32 gExtractAudio;
extern s32 gLog;
extern s32 gGenericNames;

void fix_crc(unsigned char* rom);

#endif