#include <ext_type.h>

struct Rom;

typedef struct AdpcmLoop {
	u32 start;
	u32 end;
	u32 count;
	u32 origSpls;
	s16 state[]; // elements: count != 0 ? 16 : 0.
} StructAligned(16) AdpcmLoop; // size = 0x10 or 0x30

typedef struct AdpcmBook {
	s32 order;
	s32 npredictors;
	s16 book[]; // elements: 8 * order * npredictors
} StructAligned(16) AdpcmBook;

typedef struct Sample {
	union {
		struct {
			u32 codec     : 4;
			u32 medium    : 2;
			u32 unk_bit26 : 1;
			u32 unk_bit25 : 1;
			u32 size      : 24;
		} info;
		u32 data;
	};
	u32 sampleAddr; // u8*
	u32 loop; // AdpcmLoop*
	u32 book; // AdpcmBook*
} StructAligned(16) Sample; // size = 0x10

typedef struct Sound {
	u32 sample; // Sample*
	union {
		f32 tuning;
		u32 swap32;
	};
} Sound; // size = 0x8

typedef struct Adsr {
	s16 rate;
	s16 level;
} Adsr; // size = 0x4

typedef struct Instrument {
	u8  loaded;
	u8  splitLo;
	u8  splitHi;
	u8  release;
	u32 envelope; // AdsrEnvelopePoint*
	union {
		struct {
			Sound lo;
			Sound prim;
			Sound hi;
		};
		Sound sound[3];
	};
}  Instrument; // size >= 0x20

typedef struct Drum {
	u8    release;
	u8    pan;
	u8    loaded;
	Sound sound;
	u32   envelope;
} Drum; // size >= 0x14

typedef struct SoundFont {
	u32 drums;
	u32 sfx;
	u32 instruments[];
} SoundFont;

typedef enum SampleMedium {
	/* 0 */ MEDIUM_RAM,
	/* 1 */ MEDIUM_UNK,
	/* 2 */ MEDIUM_CART,
	/* 3 */ MEDIUM_DISK_DRIVE
} StructPacked SampleMedium;

typedef enum SeqPlayer {
	/* 0 */ SEQPLAY_SFX,
	/* 1 */ SEQPLAY_FANFARE,
	/* 2 */ SEQPLAY_BGM,
	/* 3 */ SEQPLAY_DEMO_SFX
} StructPacked SeqPlayer;

typedef struct AudioEntry {
	u32 romAddr;
	u32 size;
	u8  medium;
	u8  seqPlayer;
	s8  audioTable1;
	s8  audioTable2;
	u8  numInst;
	u8  numDrum;
	u16 numSfx;
} AudioEntry;

typedef struct AudioEntryHead {
	s16        numEntries;
	s16        unkMediumParam;
	u32        romAddr;
	char       pad[0x8];
	AudioEntry entries[];
} AudioEntryHead;

struct N64AudioInfo;
struct SampleInfo;

typedef struct {
	const struct N64AudioInfo* sample;
	const struct Rom* rom;
	const struct SampleInfo* tbl;
	const char* path;
	u32 i;
} SampleDumpArg;

void Audio_InitDump();
void Audio_InitBuild();
void Audio_Free();

void Audio_DumpSoundFont(struct Rom* rom, Memfile* dataFile, Memfile* config);
void Audio_DumpSequence(struct Rom* rom, Memfile* dataFile, Memfile* config);
void Audio_DumpSampleTable(struct Rom* rom, Memfile* dataFile, Memfile* config);

void Audio_UpdateSegments(struct Rom* rom);
void Audio_BuildSampleTable(struct Rom* rom, Memfile* dataFile, Memfile* config);
void Audio_BuildSoundFont(struct Rom* rom, Memfile* dataFile, Memfile* config);
void Audio_BuildSequence(struct Rom* rom, Memfile* dataFile, Memfile* config);
void Audio_BuildSfxTable(struct Rom* rom, Memfile* dataFile, Memfile* config);

void Audio_DeleteUnreferencedSamples(void);