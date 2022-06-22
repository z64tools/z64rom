#ifndef __Z64ROM_H__
#define __Z64ROM_H__

#include <ExtLib.h>
#include "Rom.h"
#include "Dma.h"
#include "Audio.h"
#include "Mips64.h"
#include "yaz.h"
#include "Texture.h"
#include "Text.h"

#define ROM_RELEASE 0
#define ROM_DEV     1

#define SEG_CODE 0xDE
#define SEG_BOOT 0xDF

#define RELOC_CODE   0x00A94000
#define RELOC_BOOT   0x00001060
#define RELOC_PLAYER 0x00C010B0

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

typedef struct StructBE {
	u16    frameCount;
	u16    __pad;
	void32 segment;
} PlayerAnimEntry;

typedef struct StructBE {
	s16 pos[3];
	s16 rot[21][3];
	struct {
		u16 _null : 8;
		u16 mouth : 4;
		u16 eye   : 4;
	} face;
} PlayerAnimFrame;

typedef struct StructBE {
	u16    textId;
	u8     typePos;
	void32 segment;
} MessageTableEntry;

typedef struct StructBE {
	s8 scene;
	s8 spawn;
	union {
		struct StructBE {
			u16 continueBgm : 1;
			u16 titleCard   : 1;
			u16 fadeIn      : 7;
			u16 fadeOut     : 7;
		};
		u16 field;
	};
} EntranceInfo;

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
		
		u32 nesEntryTable;
		u32 staffEntryTable;
		
		u32 entranceTable;
		u32 restrictionFlags;
		struct {
			HiLo init;
			HiLo dest;
			HiLo updt;
			HiLo draw;
		} player;
		struct {
			HiLo updt;
			HiLo draw;
		} pauseMenu;
	} table;
	struct {
		u32 seqRom;
		u32 fontRom;
		u32 smplRom;
	} segment;
} RomOffset;

typedef struct Rom {
	MemFile   file;
	MemFile   config;
	RomOffset offset;
	struct {
		DmaEntry*          dma;
		ActorEntry*        actor;
		SceneEntry*        scene;
		EffectEntry*       effect;
		ObjectEntry*       object;
		KaleidoEntry*      kaleido;
		GameStateEntry*    state;
		
		MessageTableEntry* nesMsg;
		MessageTableEntry* staffMsg;
		
		EntranceInfo*      entrance;
		RestrictionFlag*   restrictionFlags;
		struct {
			u16 dma;
			u16 obj;
			u16 actor;
			u16 effect;
			u16 state;
			u16 scene;
			u16 kaleido;
			u16 skybox;
			u16 entrance;
		} num;
	} table;
	struct {
		MemFile sampleTbl;
		MemFile fontTbl;
		MemFile seqTbl;
		MemFile seqFontTbl;
	} mem;
	struct {
		u32 dmaNum, actorNum, objectNum, sceneNum, effectNum;
	} ext;
	MemFile code;
	MemFile boot;
	MemFile playerAnim;
} Rom;

typedef struct N64AudioInfo {
	u32   sampleRate;
	u8    halfPrec;
	const struct N64AudioInfo* dublicate;
	char* name;
} N64AudioInfo;

typedef struct SampleInfo {
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

typedef struct {
	struct {
		MemFile* file;
		s32 num;
	} cfg;
	struct {
		MemFile* file;
		u32* offset;
		s32  num;
	} bin;
} Patch;

typedef struct {
	N64AudioInfo* sample;
	const char*   object;
	const char*   actor;
	const char*   effect;
	const char*   state;
	const char*   kaleido;
	const char*   scene;
	const char*   bank;
	const char*   sequence;
	const char*   system;
	const char*   skybox;
} RomNameTable;

typedef struct {
	u32 id;
	const char* name;
} SystemInfo;

extern Patch gPatch;

extern char* gWaveSample[8];
extern const N64AudioInfo gSampleInfo[];
extern const char* gObjectName_OoT[402];
extern const char* gActorName_OoT[471];
extern const char* gEffectName_OoT[37];
extern const char* gStateName_OoT[6];
extern const char* gKaleidoName_OoT[2];
extern const char* gSceneName_OoT[110];
extern const char* gBankName_OoT[41];
extern const char* gSequenceName_OoT[114];
extern const char* gSkyboxName_OoT[32];
extern const SystemInfo gSystem_OoT[28];
extern const char* gPlayerAnimName[573];

extern s32 gBetaFlag_Actor_OoT[7];
extern s32 gBetaFlag_Object_OoT[47];
extern s32 gBetaFlag_Scene_OoT[9];

extern const char* gToolName;

extern s32 gDumpRom;
extern s32 gDumpAudio;
extern s32 gAutoInstall;
extern const char* gFile_z64hdr;
extern const char* gFile_mips64;

extern u32 gCompressFlag;
extern s32 gPrintInfo;
extern s32 gMakeForce;
extern const char* gMakeTarget;
extern u32 gThreading;
extern s32 gDumpFlag;

extern s32 gAudioOnly;
extern s32 gBuildTarget;
extern char gBuildrom[2][128];

extern char* gVanilla;
extern const char* gProjectConfig;
extern const char* gBaserom;

void fix_crc(unsigned char* rom);

#endif