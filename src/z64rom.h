#ifndef __Z64ROM_H__
#define __Z64ROM_H__

#include <ext_lib.h>
#include "rom.h"
#include "dma.h"
#include "audio.h"
#include "mips64.h"
#include "yaz.h"
#include "texture.h"
#include "lang.h"

/* available space:
 *
 * gActorOverlayTable
 * ─ ram:   801162A0
 * ─ rom:     B8D440
 * ─ size:      3AE0
 *
 * gDmaDataTable
 * ─ ram:   80016DA0
 * ─ rom:      12F70
 * ─ size:      60C0
 * 40 entries reserved
 * ─ size:      280
 *
 * gGameStateOverlayTable
 * ─ ram:   8011F830
 * ─ rom:     B969D0
 * ─ size:       120
 *
 * gObjectTable
 * ─ ram:   80127528
 * ─ rom:     B9E6C8
 * ─ size:      2010
 *
 * gSceneTable
 * ─ ram:   80129A10
 * ─ rom:     BA0BB0
 * ─ size:       898
 *
 * gEffectSsOverlayTable
 * ─ ram:   801159B0
 * ─ rom:     B8CB50
 * ─ size:       40C
 *
 */

#define ROM_RELEASE 0
#define ROM_DEV     1

#define SEG_CODE 0xDE
#define SEG_BOOT 0xDF

#define RELOC_CODE   0x00A94000
#define RELOC_BOOT   0x00001060
#define RELOC_PLAYER 0x00C010B0

typedef void (* PatchFunc)(struct Rom*, Memfile*, Memfile*, char*);

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
		u8 all       : 2;
		u8 din_naury : 2;
		u8 farore    : 2;
		u8 sunSong   : 2;
	};
} RestrictEntry;

typedef struct StructBE {
	u16 frameCount;
	u16 __pad;
	u32 segment;
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
	u16 textId;
	u8  typePos;
	u32 segment;
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
	Memfile   file;
	Toml      toml;
	RomOffset offset;
	struct {
		DmaEntry*       dma;
		ActorEntry*     actor;
		SceneEntry*     scene;
		EffectEntry*    effect;
		ObjectEntry*    object;
		KaleidoEntry*   kaleido;
		GameStateEntry* state;
		
		MessageTableEntry* nesMsg;
		MessageTableEntry* staffMsg;
		
		EntranceInfo*  entrance;
		RestrictEntry* restrictionFlags;
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
		Memfile sampleTbl;
		Memfile fontTbl;
		Memfile seqTbl;
		Memfile seqFontTbl;
	} mem;
	struct {
		u32 dmaNum, actorNum, objectNum, sceneNum, effectNum;
	} ext;
	Memfile code;
	Memfile boot;
	Memfile playerAnim;
	
	bool ootDebug;
} Rom;

typedef struct N64AudioInfo {
	u32 sampleRate;
	u8  halfPrec;
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
	u32 sampleAddr; // u8*
	u32 loop; // AdpcmLoop*
	u32 book; // AdpcmBook*
	f32 tuning;
	u8  isPrim;
	u8  splitLo;
	u8  splitHi;
} SampleInfo;

typedef struct {
	struct {
		Memfile* file;
		s32      num;
	} config;
	struct {
		Memfile* file;
		u32*     offset;
		s32      num;
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

void fix_crc(unsigned char* rom);
const char* Transition_GetName(s32 type);
s32 Transition_GetType(const char* str);
void Migrate(const char* type, const char* path);
void GetSymInfo(const char* symbol);

void MajorasMaskImport(bool);
void Chill(void);

typedef struct {
	const char* suffix[2];
	const char* build[2];
	const char* baseRom;
	const char* buildName;
	const char* baseWad;
	const char* vanilla;
	const char* input;
	
	struct {
		const char* z64hdr;
		const char* gcc64;
	} file;
	
	struct {
		const char* gcc;
		const char* main;
		const char* actor;
		const char* code;
		const char* kaleido;
		const char* state;
	} gccFlags;
	
	struct {
		const char* base;
		const char* code;
		const char* scene;
		const char* ulib;
	} linkerFlags;
	
	struct {
		bool use;
		u8   scene;
		u8   spawn;
		u8   header;
		u8   age;
	} instant;
	
	const char* workDir;
	const char* dolphinDir;
	const char* ccdefine;
	
	u8 threadNum;
	u8 buildID;
	
	s8 dump;
	s8 autoInstall;
	s8 info;
	s8 threading;
	s8 buildVC;
	
	const char* makeTarget;
	s8 makeForce;
	s8 makeOnly;
	s8 noMake;
	
	s8 noWait;
	s8 noPlay;
	
	s8 chill;
	s8 checkUpdates;
	
	s8 compress;
	s8 noCache;
	
	s8 audioOnly;
	s8 audioUnk;
	
	s8 cleanDump;
	s8 reconfig;
	
	s8 yazHeader;
	
	Toml app_data;
} StateZ;

typedef enum {
	EXIT_CONTINUE,
	EXIT_INSTANT,
	EXIT_PROMPT,
} Exit;

extern StateZ g64;

extern bool gForceCodeMake;
extern Patch gPatch;
extern char* gWaveSample[8];
extern const N64AudioInfo gOoT_Sample[];
extern const char* gOoT_Object[402];
extern const char* gOoT_Actor[471];
extern const char* gOoT_Effect[37];
extern const char* gOoT_State[6];
extern const char* gOoT_Kaleido[2];
extern const char* gOoT_Scene[110];
extern const char* gOoT_Soundfont[41];
extern const char* gOoT_Sequence[114];
extern const char* gOoT_Skybox[32];
extern const SystemInfo gOoT_Static[26];
extern const char* gOoT_PlayerAnim[573];
extern s32 gOoT_BetaActor[7];
extern s32 gOoT_BetaObject[47];
extern s32 gOoT_BetaScene[9];

extern const char* gToolName;
extern char gProjectConfig[32];

#endif
