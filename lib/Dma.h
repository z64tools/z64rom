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

typedef struct {
	s16   id;
	u8    category;
	u32   flags;
	s16   objectId;
	u32   instanceSize;
	void* init;
	void* destroy;
	void* update;
	void* draw;
} ActorInit;

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

typedef enum SampleMedium {
	/* 0 */ MEDIUM_RAM,
	/* 1 */ MEDIUM_UNK,
	/* 2 */ MEDIUM_CART,
	/* 3 */ MEDIUM_DISK_DRIVE
} AttPacked SampleMedium;

typedef enum SeqPlayer {
	/* 0 */ SEQPLAY_SFX,
	/* 1 */ SEQPLAY_FANFARE,
	/* 2 */ SEQPLAY_BGM,
	/* 3 */ SEQPLAY_DEMO_SFX
} AttPacked SeqPlayer;

typedef struct AudioEntry {
	void32       romAddr;
	u32          size;
	SampleMedium medium;
	SeqPlayer    seqPlayer;
	s8  audioTable1;
	s8  audioTable2;
	u8  numInst;
	u8  numDrum;
	u16 numSfx;
} AudioEntry;

typedef struct AudioEntryHead {
	s16  numEntries;
	s16  unkMediumParam;
	u32  romAddr;
	char pad[0x8];
	AudioEntry entries[];
} AudioEntryHead;

typedef struct RomFile {
	union {
		void* data;
		PointerCast cast;
	};
	u32 romStart;
	u32 romEnd;
	u32 size;
} RomFile;

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