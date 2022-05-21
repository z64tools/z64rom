#ifndef __Z64_DMA_H__
#define __Z64_DMA_H__

#include <ExtLib.h>

typedef struct {
	u32 loadedRamAddr;
	u32 vromStart;
	u32 vromEnd;
	u32 vramStart;
	u32 vramEnd;
	u32 unk_14;
	u32 init;
	u32 destroy;
	u32 unk_20;
	u32 unk_24;
	s32 unk_28;
	u32 instanceSize;
} GameStateEntry;

typedef struct {
	u32 loadedRamAddr;
	u32 vromStart;
	u32 vromEnd;
	u32 vramStart;
	u32 vramEnd;
	u32 offset; // loadedRamAddr - vramStart
	u32 name;
} KaleidoEntry; // size = 0x1C

typedef struct ActorEntry {
	u32 vromStart;
	u32 vromEnd;
	u32 vramStart;
	u32 vramEnd;
	u32 loadedRamAddr;
	u32 initInfo;
	u32 name;
	u16 allocType;
	s8  numLoaded;
} ActorEntry;

typedef struct EffectEntry {
	u32 vromStart;
	u32 vromEnd;
	u32 vramStart;
	u32 vramEnd;
	u32 loadedRamAddr;
	u32 initInfo;
	u8  unk_18;
} EffectEntry;

typedef struct DmaEntry {
	u32 vromStart;
	u32 vromEnd;
	u32 romStart;
	u32 romEnd;
} DmaEntry;

typedef struct ObjectEntry {
	u32 vromStart;
	u32 vromEnd;
} ObjectEntry;

typedef struct SceneEntry {
	u32 vromStart;
	u32 vromEnd;
	u32 titleVromStart;
	u32 titleVromEnd;
	u8  unk_10;
	u8  config;
	u8  unk_12;
	u8  unk_13;
} SceneEntry;

typedef struct RomFile {
	union {
		void* data;
		PointerCast cast;
	};
	u32 romStart;
	u32 romEnd;
	u32 size;
} RomFile;

typedef struct DmaPrefix {
	union {
		struct {
			u32 titleVStart;
			u32 titleVEnd;
			u8  paramA;
			u8  sceneFuncId;
			u8  paramB;
		} scene;
		struct {
			u32 vramStart;
			u32 vramEnd;
			u16 allocType;
			u16 _pad;
			u32 vinitVar;
		} actor;
	};
} DmaPrefix;

typedef struct Slot {
	struct Slot* prev;
	struct Slot* next;
	u32 romStart;
	u32 romEnd;
} Slot;

typedef enum {
	DMA_ACTOR,
	DMA_STATE,
	DMA_KALEIDO,
	DMA_EFFECT,
	DMA_OBJECT,
	DMA_PLACE_NAME,
	DMA_SKYBOX_TEXEL,
	DMA_SCENES,
	DMA_UNUSED
} DmaBank;

typedef enum {
	DMA_ID_MAKEROM,
	DMA_ID_UNUSED_1, // boot
	DMA_ID_UNUSED_2, // dmadata
	DMA_ID_UNUSED_3, // SoundFont
	DMA_ID_UNUSED_4, // Sequence
	DMA_ID_UNUSED_5, // SampleTable
	DMA_ID_LINK_ANIMATION,
	DMA_ID_ICON_ITEM_STATIC,
	DMA_ID_ICON_ITEM_24_STATIC,
	DMA_ID_ICON_ITEM_FIELD_STATIC,
	DMA_ID_ICON_ITEM_DUNGEON_STATIC,
	DMA_ID_ICON_ITEM_GAMEOVER_STATIC,
	DMA_ID_ICON_ITEM_NES_STATIC,
	DMA_ID_ICON_ITEM_GER_STATIC,
	DMA_ID_ICON_ITEM_FRA_STATIC,
	DMA_ID_ITEM_NAME_STATIC,
	DMA_ID_MAP_NAME_STATIC,
	DMA_ID_DO_ACTION_STATIC,
	DMA_ID_MESSAGE_STATIC,
	DMA_ID_MESSAGE_TEXTURE_STATIC,
	DMA_ID_NES_FONT_STATIC,
	DMA_ID_MESSAGE_DATA_STATIC_NES,
	DMA_ID_MESSAGE_DATA_STATIC_GER,
	DMA_ID_MESSAGE_DATA_STATIC_FRA,
	DMA_ID_MESSAGE_DATA_STATIC_STAFF,
	DMA_ID_MAP_GRAND_STATIC,
	DMA_ID_MAP_I_STATIC,
	DMA_ID_MAP_48X85_STATIC,
	DMA_ID_CODE,
	DMA_ID_OVL_MAP_MARK_DATA = 35,
	DMA_ID_Z_SELECT_STATIC   = 937,
	DMA_ID_NINTENDO_ROGO_STATIC,
	DMA_ID_TITLE_STATIC,
	DMA_ID_PARAMETER_STATIC,
} DmaIndex;

struct Rom;

#define Dma_RomFile_Proto(name) \
	Dma_RomFile_ ## name(struct Rom* rom, s32 id)

RomFile Rom_GetRomFile(struct Rom* rom, u32 vromA, u32 vromB);
RomFile Dma_RomFile_Proto(Object);
RomFile Dma_RomFile_Proto(Actor);
RomFile Dma_RomFile_Proto(Effect);
RomFile Dma_RomFile_Proto(DmaEntry);
RomFile Dma_RomFile_Proto(GameState);
RomFile Dma_RomFile_Proto(Scene);

typedef enum {
	DMA_FIND_FREE = -1,
	DMA_NO_ENTRY  = -2,
} DmaParam;

u32 Dma_WriteEntry(struct Rom* rom, s32 id, MemFile* memFile, s32 compress);
u32 Dma_AllocEntry(struct Rom* rom, s32 id, Size size);
u32 Dma_GetRomSize(void);
u32 Dma_GetVRomEnd(void);

void Dma_FreeEntry(struct Rom* rom, u32 id, u32 dmaAlign);
void Dma_FreeSegment(struct Rom* rom, u32 romStart, u32 romEnd);
void Dma_FreeGroup(struct Rom* rom, DmaBank type);

void Dma_CompressRom(struct Rom* rom);
void Dma_CombineSlots(void);
void Dma_PrintfSlots(struct Rom* rom, const char* message);
void Dma_WriteFlag(u32 id, bool value);

extern Slot* gSlotHead;

#endif