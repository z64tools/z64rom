#ifndef __Z64_DMA_H__
#define __Z64_DMA_H__

#include <ext_type.h>

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

typedef struct {
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

typedef struct {
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
		void*       data;
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
	DMA_ID_BOOT, // boot
	DMA_ID_DMADATA, // dmadata
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
	DMA_ID_BUILD_INFO,
	DMA_ID_Z_SELECT_STATIC   = 937,
	DMA_ID_NINTENDO_ROGO_STATIC,
	DMA_ID_TITLE_STATIC,
	DMA_ID_PARAMETER_STATIC,
	DMA_ID_ELF_MESSAGE_FIELD = 1005,
	DMA_ID_ELF_MESSAGE_YDAN,
	
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

#define DMA_FIND_FREE    -1
#define COMPRESS         1
#define NOCACHE_COMPRESS 2

u32 Dma_Write(struct Rom* rom, s32 id, void* data, u32 __size, const char* name, s32 compress);
u32 Dma_WriteMemfile(struct Rom* rom, s32 id, Memfile* memFile, s32 compress);
u32 Dma_AllocEntry(struct Rom* rom, s32 id, size_t size);
void Dma_UpdateRomSize(struct Rom* rom);
u32 Dma_GetVRomEnd(void);

void Dma_RegEntry(struct Rom* rom, u32 id, u32 dmaAlign);
void Dma_RegSegment(struct Rom* rom, u32 romStart, u32 romEnd);
void Dma_RegGroup(struct Rom* rom, DmaBank type);
void Dma_Free(void);

void Dma_CompressRom(struct Rom* rom);
void Dma_CombineSlots(void);
void Dma_SlotInfo(struct Rom* rom, const char* message, Slot* head);
void Dma_WriteFlag(u32 id, bool value);

typedef struct {
	struct {
		bool writable : 1;
		bool compress : 1;
	}* entry;
	u32 written;
	u32 num;
	u32 highest;
} DmaContext;

extern DmaContext gDma;
extern Slot* gSlotHead;
extern Slot* gSlotYazHead;

#endif