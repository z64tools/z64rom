#ifndef __Z64_DMA_H__
#define __Z64_DMA_H__

#include "ExtLib.h"

typedef struct {
	void32 loadedRamAddr;
	u32    vromStart;
	u32    vromEnd;
	void32 vramStart;
	void32 vramEnd;
	void32 unk_14;
	void32 init;
	void32 destroy;
	void32 unk_20;
	void32 unk_24;
	s32    unk_28;
	u32    instanceSize;
} GameStateEntry;

typedef struct {
	void32 loadedRamAddr;
	u32    vromStart;
	u32    vromEnd;
	void32 vramStart;
	void32 vramEnd;
	u32    offset; // loadedRamAddr - vramStart
	void32 name;
} KaleidoEntry; // size = 0x1C

typedef struct ActorEntry {
	u32    vromStart;
	u32    vromEnd;
	void32 vramStart;
	void32 vramEnd;
	void32 loadedRamAddr;
	void32 initInfo;
	void32 name;
	u16    allocType;
	s8 numLoaded;
} ActorEntry;

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

#define EXT_COMPRESSED(x)     (x << 15)
#define EXT_PHYSICAL_START(x) (x & 0x7FFFFFFC)
#define EXT_DMA_PREFIX(x)     (x << 1)
#define EXT_OVERLAP(x)        (x << 0)

typedef struct ExtEntry {
	u16 dmaIndex;
} ExtEntry;

typedef struct ExtActor {
	u16    dmaIndex;
	u16    allocType;
	void32 ramAddr;
} ExtActor;

typedef struct ExtDma {
	void32 virtualStart;
	u32    bitfield;
	u32    virtualEnd;
} ExtDma;

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

struct Rom;

#define Dma_RomFile_Proto(name) \
	Dma_RomFile_ ## name(struct Rom* rom, s32 id)

RomFile Rom_GetRomFile(struct Rom* rom, u32 vromA, u32 vromB);
RomFile Dma_RomFile_Proto(Object);
RomFile Dma_RomFile_Proto(Actor);
RomFile Dma_RomFile_Proto(DmaEntry);
RomFile Dma_RomFile_Proto(GameState);
RomFile Dma_RomFile_Proto(Scene);

#endif