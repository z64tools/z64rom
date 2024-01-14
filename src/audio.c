#include "z64rom.h"
#include "tools.h"
#include <ext_proc.h>

char** sDumpBankFileTbl;
s32 sDumpBankFileNum;
SampleInfo* sDumpSampleTbl;
s32 sDumpSampleNum;

typedef struct {
	char name[128];
	u32  segment;
	u32  size;
	f32  tuninOverride;
	char dir[512];
	
	u32 data;
	struct {
		u32 loopStart;
		u32 loopEnd;
		u32 loopCount;
		u32 tailEnd;
	} config;
	
	Memfile book;
	Memfile loopBook;
	Memfile sample;
} BuildSample;

BuildSample* sBuildSampleTbl;
s32 sBuildSampleNum;

const char* sMediumType[] = {
	"ram",
	"unk",
	"cart",
	"ddrive"
};
const char* sSeqPlayerType[] = {
	"sfx",
	"fanfare",
	"bgm",
	"demo"
};
const char* sInstSectionNames[3] = {
	"low",
	"prim",
	"hi",
};

// # # # # # # # # # # # # # # # # # # # #
// # CONFIG                              #
// # # # # # # # # # # # # # # # # # # # #

#define __Config_Sample(wow) \
		Toml_SetVar(toml, x_fmt("%s.entry[%d]." wow "sample", variable, id), "0x%08X", ReadBE(sample->sampleAddr) + rom->offset.segment.smplRom + off); \
		Toml_SetVar(toml, x_fmt("%s.entry[%d]." wow "tuning", variable, id), "%f", *f); \
		if (sDumpBankFileNum < 0) { printf("\a\n"); exit(1); /* "go intentionally bonkers" */ } \
		sDumpSampleTbl[sDumpSampleNum].tuning = *f; \
		sDumpSampleTbl[sDumpSampleNum].data = sample->data; \
		sDumpSampleTbl[sDumpSampleNum].sampleAddr = ReadBE(sample->sampleAddr) + rom->offset.segment.smplRom + off; \
		sDumpSampleTbl[sDumpSampleNum].loop = VirtualToSegment(0x0, SegmentToVirtual(0x1, ReadBE(sample->loop))); \
		sDumpSampleTbl[sDumpSampleNum].book = VirtualToSegment(0x0, SegmentToVirtual(0x1, ReadBE(sample->book))); \
		sDumpSampleNum++; \
		osAssert(sDumpSampleNum < 8192);

#define __Config_Sample_NULL(wow) \
		Toml_SetTab(toml, "%s.entry[%d]." wow, variable, id);

static void Rom_Config_Envelope(Toml* toml, Adsr* env, const char* variable, u32 id) {
	if (!env) return;
	
	for (int i = 0; ; i++) {
		Toml_SetVar(toml, x_fmt("%s.entry[%d].env_rate[%d]", variable, id, i), "%d", ReadBE(env[i].rate));
		Toml_SetVar(toml, x_fmt("%s.entry[%d].env_level[%d]", variable, id, i), "%d", ReadBE(env[i].level));
		
		if (ReadBE(env[i].rate) < 0)
			break;
	}
}

static s32 Rom_Config_Instrument(Rom* rom, Toml* toml, u32 id, Instrument* instrument, const char* variable, u32 off) {
	Adsr* env = NULL;
	Sample* sample;
	u32 val;
	f32* f = (f32*)&val;
	Instrument tempI = { .splitHi = 127 };
	Adsr tempE[4] = { { -1, 0 }, { 0 }, { 0 }, { 0 } };
	
	if (instrument == NULL) {
		env = tempE;
		instrument = &tempI;
	} else {
		if (instrument->envelope)
			env = SegmentToVirtual(0x1, ReadBE(instrument->envelope));
	}
	
	if (instrument->lo.sample == 0 &&
		instrument->prim.sample == 0 &&
		instrument->hi.sample == 0) {
		return 0;
	}
	
	Toml_SetVar(toml, x_fmt("%s.entry[%d].split_hi", variable, id), "\"%s\"", Note_Name(instrument->splitHi + 21));
	Toml_SetVar(toml, x_fmt("%s.entry[%d].split_lo", variable, id), "\"%s\"", Note_Name(instrument->splitLo + 21));
	Toml_SetVar(toml, x_fmt("%s.entry[%d].release_rate", variable, id), "%d", instrument->release);
	Rom_Config_Envelope(toml, env, variable, id);
	
	if (instrument->lo.sample != 0) {
		osLog("lo   %08X", ReadBE(instrument->lo.sample));
		sample = SegmentToVirtual(0x1, ReadBE(instrument->lo.sample));
		val = ReadBE(instrument->lo.swap32);
		__Config_Sample("low.");
	} else {
		__Config_Sample_NULL("low");
	}
	
	if (instrument->prim.sample != 0) {
		osLog("prim %08X", ReadBE(instrument->prim.sample));
		
		sample = SegmentToVirtual(0x1, ReadBE(instrument->prim.sample));
		val = ReadBE(instrument->prim.swap32);
		
		sDumpSampleTbl[sDumpSampleNum].isPrim = true;
		sDumpSampleTbl[sDumpSampleNum].splitLo = instrument->splitLo;
		sDumpSampleTbl[sDumpSampleNum].splitHi = instrument->splitHi;
		
		__Config_Sample("prim.");
	} else {
		__Config_Sample_NULL("prim");
	}
	
	if (instrument->hi.sample != 0) {
		osLog("hi   %08X", ReadBE(instrument->hi.sample));
		sample = SegmentToVirtual(0x1, ReadBE(instrument->hi.sample));
		val = ReadBE(instrument->hi.swap32);
		__Config_Sample("hi.");
	} else {
		__Config_Sample_NULL("hi");
	}
	
	return 1;
}

static s32 Rom_Config_Sfx(Rom* rom, Toml* toml, u32 id, Sound* sfx, const char* variable, u32 off) {
	u32 val;
	f32* f = (f32*)&val;
	
	if (sfx->sample == 0) {
		return 0;
	}
	
	if (sfx->sample != 0) {
		Sample* sample = SegmentToVirtual(0x1, ReadBE(sfx->sample));
		val = ReadBE(sfx->swap32);
		__Config_Sample("");
	} else {
		__Config_Sample_NULL("");
	}
	
	return 1;
}

static s32 Rom_Config_Drum(Rom* rom, Toml* toml, u32 id, u32 drumSeg, const char* variable, u32 off) {
	Drum* drum;
	u32 val;
	Adsr* env;
	f32* f = (f32*)&val;
	Drum emDrum = { 0 };
	Adsr emEnv[4] = { { -1, 0 }, { 0 }, { 0 }, { 0 } };
	
	if (drumSeg == 0) {
		drum = &emDrum;
		env = emEnv;
	} else {
		drum = SegmentToVirtual(0x1, ReadBE(drumSeg));
		env = SegmentToVirtual(0x1, ReadBE(drum->envelope));
	}
	
	if (drum->sound.sample == 0) {
		return 0;
	}
	
	Toml_SetVar(toml, x_fmt("%s.entry[%d].pan", variable, id), "%d", drum->pan);
	Rom_Config_Envelope(toml, env, variable, id);
	Toml_SetVar(toml, x_fmt("%s.entry[%d].release_rate", variable, id), "%d", drum->release);
	
	if (drum->sound.sample != 0) {
		Sample* sample = SegmentToVirtual(0x1, ReadBE(drum->sound.sample));
		val = ReadBE(drum->sound.swap32);
		__Config_Sample("");
	} else {
		__Config_Sample_NULL("");
	}
	
	return 1;
}

static void Rom_Config_Sample(Toml* toml, const Sample* sample, const char* out) {
	AdpcmLoop* loop = SegmentToVirtual(0x0, sample->loop);
	u32 data = sample->data;
	
	Toml_SetVar(toml, "codec",      "%d", (u32)ReadBE(data) >> (32 - 4));
	Toml_SetVar(toml, "medium",     "%d", (u32)(ReadBE(data) >> (32 - 6)) & 2);
	Toml_SetVar(toml, "bitA",       "%d", (u32)(ReadBE(data) >> (32 - 7)) & 1);
	Toml_SetVar(toml, "bitB",       "%d", (u32)(ReadBE(data) >> (32 - 8)) & 1);
	Toml_SetVar(toml, "loop_start", "%d", (u32)ReadBE(loop->start));
	Toml_SetVar(toml, "loop_end",   "%d", (u32)ReadBE(loop->end));
	Toml_SetVar(toml, "loop_count", "%d", (u32)ReadBE(loop->count));
	Toml_SetVar(toml, "tail_end",   "%d", (u32)ReadBE(loop->origSpls));
	
	Toml_Save(toml, out);
}

// # # # # # # # # # # # # # # # # # # # #
// # DUMP                                #
// # # # # # # # # # # # # # # # # # # # #

typedef enum {
	SEQFLAG_ENEMY            = (1) << 0,
	SEQFLAG_FANFARE          = (1) << 1,
	SEQFLAG_FANFARE_GANON    = (1) << 2,
	SEQFLAG_RESTORE          = (1) << 3,
	SEQFLAG_SECTION_STORE    = (1) << 4,
	SEQFLAG_SECTION_PRESERVE = (1) << 5,
	SEQFLAG_IO7_1            = (1) << 6,
	SEQFLAG_NO_AMBIENCE      = (1) << 7,
	SEQFLAG_MAX              = (1) << 8,
} SeqFlag;

const char* sSeqFlagName[] = {
	"allow_enemy_bgm",
	"fanfare",
	"fanfare_ganon",
	"restore",
	"section_store",
	"section_preserve",
	"io7_1",
	"no_ambience",
};

void Audio_InitDump() {
	osAssert(sDumpBankFileTbl = new(char*[8192 * 2]));
	osAssert(sDumpSampleTbl = new(SampleInfo[8192 * 2]));
}

void Audio_InitBuild() {
	
}

void Audio_Free() {
	for (var_t i = 0; i < sDumpBankFileNum; i++)
		delete(sDumpBankFileTbl[i]);
	for (var_t i = 0; i < sBuildSampleNum; i++) {
		Memfile_Free(&sBuildSampleTbl[i].book);
		Memfile_Free(&sBuildSampleTbl[i].loopBook);
		Memfile_Free(&sBuildSampleTbl[i].sample);
	}
	
	delete(sDumpBankFileTbl, sDumpSampleTbl, sBuildSampleTbl);
}

static void Audio_PatchWavFiles(Memfile* dataFile, Memfile* config) {
	#define NOTE(note, octave) (note + (12 * (octave)))
	u8* instInfo;
	u32* smplInfo;
	const struct {
		u8 basenote;
		s8 finetune;
		const N64AudioInfo* info;
	} info[] = {
		{ NOTE(0,  3),  0,  &gOoT_Sample[367]  }, // LowPerc
		{ NOTE(1,  3),  0,  &gOoT_Sample[368]  }, // Snare
		{ NOTE(3,  3),  0,  &gOoT_Sample[369]  }, // SoftSnare
		{ NOTE(1,  4),  0,  &gOoT_Sample[370]  }, // Cymbal
		{ NOTE(0,  6),  0,  &gOoT_Sample[371]  }, // Timpani
		{ NOTE(5,  3),  0,  &gOoT_Sample[405]  }, // Gong
		{ NOTE(0,  6),  0,  &gOoT_Sample[406]  }, // WindChimes
		
		{ NOTE(0,  5),  0,  &gOoT_Sample[407]  }, // CongaOpen
		{ NOTE(0,  5),  0,  &gOoT_Sample[414]  }, // CongaSoft
		{ NOTE(8,  4),  0,  &gOoT_Sample[415]  }, // CongaMute
		
		{ NOTE(5,  3),  0,  &gOoT_Sample[393]  }, // LuteA
		{ NOTE(7,  3),  0,  &gOoT_Sample[394]  }, // LuteB
		
		{ NOTE(11, 3),  0,  &gOoT_Sample[378]  }, // Tambourine
		{ NOTE(0,  4),  0,  &gOoT_Sample[120]  }, // Tambourine
		{ NOTE(2,  4),  0,  &gOoT_Sample[121]  }, // Tambourine
		
		{ NOTE(0,  3),  0,  &gOoT_Sample[432]  }, // Cajon
		{ NOTE(2,  3),  0,  &gOoT_Sample[433]  }, // Cajon
	};
	
	foreach(i, info) {
		info_prog(gLang.rom.target[LANG_FIX_SMPL], i + 1, ArrCount(info));
		char* file = x_fmt("rom/sound/sample/%s/%s/Sample.wav", g64.vanilla, info[i].info->name);
		
		Memfile_Null(dataFile);
		Memfile_LoadBin(dataFile, file);
		instInfo = memmem(dataFile->data, dataFile->size, "inst", 4);
		smplInfo = memmem(dataFile->data, dataFile->size, "smpl", 4);
		
		/* basenote */ instInfo[8] = info[i].basenote;
		/* finetune */ instInfo[9] = info[i].finetune;
		/* basenote */ smplInfo[5] = info[i].basenote;
		/* finetune */ smplInfo[6] = info[i].finetune;
		Memfile_SaveBin(dataFile, file);
	}
#undef NOTE
}

void Audio_DumpSoundFont(Rom* rom, Memfile* dataFile, Memfile* config) {
	AudioEntryHead* head = SegmentToVirtual(0, rom->offset.table.fontTable);
	AudioEntryHead* sampHead = SegmentToVirtual(0, rom->offset.table.sampleTable);
	AudioEntry* entry;
	u32 num = ReadBE(head->numEntries);
	SoundFont* bank;
	Instrument* instrument;
	Sound* sfx;
	u32 off = 0;
	s32 i = 0;
	
	for (; i < num; i++) {
		const char* name = g64.audioUnk ? "Unk" : gOoT_Soundfont[i];
		char* path = fmt("rom/sound/soundfont/%s/", g64.vanilla);
		info_prog(gLang.rom.target[LANG_FONT], i + 1, num);
		
		entry = &head->entries[i];
		
		bank = SegmentToVirtual(0x0, ReadBE(entry->romAddr) + rom->offset.segment.fontRom);
		off = ReadBE(sampHead->entries[entry->audioTable1].romAddr);
		SegmentSet(0x1, bank);
		
		Toml toml = Toml_New();
		
		sys_mkdir(path);
		
		osLog("SoundFont: [%d / %d / %d]", entry->numInst, ReadBE(entry->numSfx), entry->numDrum);
		osLog("0x%08X", entry->romAddr);
		
		for (int j = 0; j < entry->numInst; j++) {
			osLog("Instrument [%08X] %d / %d", ReadBE(bank->instruments[j]), j + 1, entry->numInst);
			if (bank->instruments[j] == 0)
				instrument = NULL;
			else
				instrument = SegmentToVirtual(0x1, ReadBE(bank->instruments[j]));
			
			Rom_Config_Instrument(rom, &toml, j, instrument, "inst", off);
		}
		
		for (int j = 0; j < ReadBE(entry->numSfx); j++) {
			osLog("Instrument %d / %d", j + 1, ReadBE(entry->numSfx));
			sfx = SegmentToVirtual(0x1, ReadBE(bank->sfx));
			
			Rom_Config_Sfx(rom, &toml, j, &sfx[j], "sfx", off);
		}
		
		for (int j = 0; j < entry->numDrum; j++) {
			osLog("Instrument %d / %d", j + 1, entry->numDrum);
			u32* wow = SegmentToVirtual(0x1, ReadBE(bank->drums));
			
			Rom_Config_Drum(rom, &toml, j, wow[j], "drum", off);
		}
		
		Toml_SetVar(&toml, "#medium_type",     "\"ram\" / \"unk\"     / \"cart\" / \"ddrive\"");
		Toml_SetVar(&toml, "#sequence_player", "\"sfx\" / \"fanfare\" / \"bgm\"  / \"demo\"");
		Toml_SetVar(&toml, "medium_type", "\"%s\"", sMediumType[entry->medium]);
		Toml_SetVar(&toml, "sequence_player", "\"%s\"", sSeqPlayerType[entry->seqPlayer]);
		
		sDumpBankFileTbl[sDumpBankFileNum] = fmt("%s0x%02X-%s.toml", path, i, name);
		Toml_Save(&toml, sDumpBankFileTbl[sDumpBankFileNum]);
		Toml_Free(&toml);
		osAssert(++sDumpBankFileNum < 8192);
		
		delete(path);
	}
	
	SegmentSet(0x1, NULL);
}

void Audio_DumpSequence(Rom* rom, Memfile* dataFile, Memfile* config) {
	AudioEntryHead* head = SegmentToVirtual(0x0, rom->offset.table.seqTable);
	u8* seqFlag = SegmentToVirtual(0x0, 0xBA77F8);
	u8* seqFontTable;
	u16* segFontOffTable;
	AudioEntry* entry;
	RomFile romFile;
	u32 num = ReadBE(head->numEntries);
	
	SegmentSet(0x1, SegmentToVirtual(0x0, rom->offset.table.seqFontTbl));
	
	Memfile_Null(config);
	for (int i = 0; i < num; i++) {
		const char* name = g64.audioUnk ? "Unk" :  gOoT_Sequence[i];
		char* path = fmt("rom/sound/sequence/%s/0x%02X-%s/", g64.vanilla, i, name);
		List bankList = List_New();
		List flagList = List_New();
		u32 bankNum;
		u32 bankId;
		
		sys_mkdir(path);
		info_prog(gLang.rom.target[LANG_SEQ], i + 1, num);
		segFontOffTable = SegmentToVirtual(0x0, rom->offset.table.seqFontTbl);
		entry = &head->entries[i];
		romFile.data = SegmentToVirtual(0x0, ReadBE(entry->romAddr) + rom->offset.segment.seqRom);
		romFile.size = ReadBE(entry->size);
		
		Memfile_Null(config);
		seqFontTable = SegmentToVirtual(0x1, ReadBE(segFontOffTable[i]));
		
		List_Alloc(&bankList, 8);
		Ini_WriteComment(config, name);
		bankNum = ReadBE(seqFontTable[0]);
		
		for (int i = 0; i < bankNum; i++) {
			bankId = (ReadBE(seqFontTable[i + 1]) & 0xFF);
			List_Add(&bankList, x_fmt("0x%02X", bankId));
		}
		
		Ini_WriteArr(config, "bank_id", &bankList, false, 0);
		
		if (romFile.size != 0) {
			Rom_Extract(dataFile, romFile, x_fmt("%s%s.aseq", path, name));
		} else {
			Ini_WriteHex(config, "seq_pointer", ReadBE(entry->romAddr), "Sequence ID - Jumps into this sequence");
		}
		
		Memfile_Fmt(config, "# Sample Medium types [");
		for (s32 e = 0; e < ArrCount(sMediumType); e++) {
			if (e != 0)
				Memfile_Fmt(config, "/");
			Memfile_Fmt(config, "%s", sMediumType[e]);
		}
		Memfile_Fmt(config, "]\n");
		
		Ini_WriteStr(config, "medium_type", sMediumType[entry->medium], true, 0);
		
		Memfile_Fmt(config, "# Sequence Player types [");
		for (s32 e = 0; e < ArrCount(sSeqPlayerType); e++) {
			if (e != 0)
				Memfile_Fmt(config, "/");
			Memfile_Fmt(config, "%s", sSeqPlayerType[e]);
		}
		Memfile_Fmt(config, "]\n");
		Ini_WriteStr(config, "sequence_player", sSeqPlayerType[entry->seqPlayer], true, 0);
		
		List_Alloc(&flagList, 10);
		
		for (s32 b = (1) << 0, n = 0; b < SEQFLAG_MAX; b = (b) << 1, n++) {
			if (seqFlag[i] & b)
				List_Add(&flagList, sSeqFlagName[n]);
		}
		
		Ini_Fmt(config, "# [ ");
		for (s32 e = 0; e < ArrCount(sSeqFlagName); e++) {
			Ini_Fmt(config, "%s", sSeqFlagName[e]);
			
			if (e + 1 < ArrCount(sSeqFlagName))
				Ini_Fmt(config, "/", sSeqFlagName[e]);
		}
		Ini_Fmt(config, "]\n");
		
		Ini_WriteArr(config, "sequence_flags", &flagList, QUOTES, NO_COMMENT);
		
		Memfile_SaveStr(config, x_fmt("%sconfig.toml", path));
		
		List_Free(&bankList);
		List_Free(&flagList);
		
		delete(path);
	}
	
	SegmentSet(0x1, NULL);
}

bool sSlack;

static void Audio_DumpSampleTable_DumpEntry(SampleDumpArg* arg) {
	const N64AudioInfo* sample = arg->sample;
	const SampleInfo* tbl = arg->tbl;
	AdpcmLoop* loop = NULL;
	AdpcmBook* book = NULL;
	Memfile memF = Memfile_New();
	Toml toml = Toml_New();
	RomFile rf = {};
	char* name = sample->name;
	u32 sampRate = sample->sampleRate;
	u32 data = tbl->data;
	
	if (g64.audioUnk) {
		name = x_fmt("Unk_%03d", arg->i);
		sampRate = 16000;
	} else {
		if (sample->dublicate) return;
		if (name == NULL) errr(gLang.audio.err_missing_name, arg->i);
		if (sampRate == 0) errr(gLang.audio.err_missing_samprt, name, arg->i);
	}
	
	book = SegmentToVirtual(0x0, tbl->book);
	loop = SegmentToVirtual(0x0, tbl->loop);
	
	osLog("%s", name);
	sys_mkdir("%s%s/", arg->path, name);
	fs_set("%s%s/", arg->path, name);
	
	const char* fileWav = fs_item("Sample.wav");
	const char* fileVadpcm = fs_item("sample.vadpcm.bin");
	const char* fileCfg = fs_item("config.toml");
	const char* fileBook = fs_item("sample.book.bin");
	const char* fileLoopBook = fs_item("sample.loopbook.bin");
	
	rf.size = ReadBE(data) & 0x00FFFFFF;
	rf.data = SegmentToVirtual(0x0, tbl->sampleAddr);
	Rom_Extract(&memF, rf, fileVadpcm);
	
	rf.size = sizeof(s16) * 8 * ReadBE(book->order) * ReadBE(book->npredictors) + 8;
	rf.data = book;
	Rom_Extract(&memF, rf, fileBook);
	
	Rom_Config_Sample(&toml, (Sample*)tbl, fileCfg);
	
	if (loop->count) {
		rf.size = 0x20;
		rf.data = SegmentToVirtual(0x0, tbl->loop + 0x10);
		Rom_Extract(&memF, rf, fileLoopBook);
	}
	
	s8* instInfo;
	Proc* exe = Proc_New(
		"%s "
		
		"--i %s "
		"--o %s "
		
		"--S "
		
		"--srate %d "
		"--tuning %f "
		
		"--config-override",
		
		Tools_Get(z64audio),
		
		fileVadpcm,
		fileWav,
		
		sampRate,
		tbl->tuning
	);
	
	if (tbl->isPrim && (tbl->splitHi != 127 || tbl->splitLo != 0)) {
		Proc_AddArg(exe, "--split-hi");
		Proc_AddArg(exe, "%d", tbl->splitHi + 21);
		
		if (tbl->splitLo) {
			Proc_AddArg(exe, "--split-lo");
			Proc_AddArg(exe, "%d", tbl->splitLo + 21);
		}
	}
	
	if (sSlack)
		Proc_AddArg(exe, "--slack");
	
	Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE);
	Proc_Exec(exe);
	Proc_Join(exe);
	
	Memfile_LoadBin(&memF, fileWav);
	
	instInfo = memmem(memF.data, memF.size, "inst", 4);
	
	if (instInfo) {
		Toml_SetVar(&toml, "basenote", "%d", (u32)instInfo[8]);
		Toml_SetVar(&toml, "finetune", "%d", (u32)instInfo[9]);
		Toml_Save(&toml, fileCfg);
	} else {
		if (memF.size == 0)
			errr(gLang.audio.err_empty_sample, fileWav);
	}
	
	Memfile_Free(&memF);
	Toml_Free(&toml);
}

static int Audio_DumpSampleTable_SortSamples(const void* addr_a, const void* addr_b) {
	const SampleInfo* a = addr_a;
	const SampleInfo* b = addr_b;
	
	if (a->sampleAddr - b->sampleAddr)
		return a->sampleAddr - b->sampleAddr;
	// Skips bad books
	if (a->book - b->book)
		return a->book - b->book;
	return 0;
}

void Audio_DumpSampleTable(Rom* rom, Memfile* dataFile, Memfile* config) {
	SampleInfo* smplInfoTbl = new(SampleInfo[sDumpSampleNum]);
	u32 smplInfoNum = 0;
	u32 addr = 0xFEFACAF3;
	
	if (!rom->ootDebug)
		sSlack = true;
	
	qsort(sDumpSampleTbl, sDumpSampleNum, sizeof(SampleInfo), Audio_DumpSampleTable_SortSamples);
	
	// Remove Duplicates
	for (var_t i = 0; i < sDumpSampleNum; i++) {
		if (addr != 0xFEFACAF3 && sDumpSampleTbl[i].sampleAddr == addr)
			continue;
		
		smplInfoTbl[smplInfoNum++] = sDumpSampleTbl[i];
		addr = sDumpSampleTbl[i].sampleAddr;
	}
	
	SampleDumpArg* arg = new(SampleDumpArg[smplInfoNum]);
	
	for (int i = 0; i < smplInfoNum; i++) {
		arg[i].i = i;
		arg[i].rom = rom;
		arg[i].sample = &gOoT_Sample[i];
		arg[i].tbl = &smplInfoTbl[i];
		arg[i].path = FreeList_Que(fmt("rom/sound/sample/%s/", g64.vanilla));
		
		if (g64.threadNum != 1)
			Parallel_Add(Audio_DumpSampleTable_DumpEntry, &arg[i]);
		else {
			info_prog(gLang.rom.target[LANG_SMPL], i + 1, smplInfoNum);
			Audio_DumpSampleTable_DumpEntry(&arg[i]);
		}
	}
	
	if (g64.threadNum != 1) {
		gParallel_ProgMsg = gLang.rom.target[LANG_SMPL];
		Parallel_Exec(g64.threadNum);
	}
	
	osLog("Samples OK");
	FreeList_Free();
	
	for (int i = 0; i < sDumpBankFileNum; i++) {
		char* name;
		// char* replacedName = NULL;
		info_prog(gLang.rom.target[LANG_FIX_FONT], i + 1, sDumpBankFileNum);
		
		osLog("Load [%s]", sDumpBankFileTbl[i]);
		Memfile_LoadStr(config, sDumpBankFileTbl[i]);
		
		for (int i = 0; i < smplInfoNum; i++) {
			if (g64.audioUnk)
				name = x_fmt("Unk_%03d", i);
			else
				name = gOoT_Sample[i].dublicate == NULL ? gOoT_Sample[i].name : gOoT_Sample[i].dublicate->name;
			
			osLog("Rep [%s]", name);
			strrep(config->data, x_fmt("0x%08X", smplInfoTbl[i].sampleAddr), x_fmt("\"%s\"", name));
		}
		
		config->size = strlen(config->data);
		Memfile_SaveStr(config, sDumpBankFileTbl[i]);
	}
	
	if (!g64.audioUnk)
		Audio_PatchWavFiles(dataFile, config);
	
	delete(smplInfoTbl, arg);
}

// # # # # # # # # # # # # # # # # # # # #
// # BUILD                               #
// # # # # # # # # # # # # # # # # # # # #

static s32 Audio_LoadFile(Memfile* dataFile, char* file) {
	char* smpl;
	
	smpl = fs_item(file);
	
	if (smpl && sys_stat(smpl)) {
		if (Memfile_LoadBin(dataFile, smpl))
			return 1;
	} else {
		smpl = fs_find(x_rep(file, "sample", "*"));
		
		if (smpl) {
			if (Memfile_LoadBin(dataFile, smpl))
				return 1;
		} else
			return 1;
	}
	
	return 0;
}

void Audio_UpdateSegments(Rom* rom) {
	#define RAM_CODE ((u32)0x8001CE60)
	#define INST_ADDR(x, y) SegmentToVirtual(SEG_CODE, ((x) - RAM_CODE)), SegmentToVirtual(SEG_CODE, ((y) - RAM_CODE))
	#define RAM_ADDR rom->code.seekPoint + 0x8001CE60
	
	Memfile_Set(&rom->code, MEM_ALIGN, 16, MEM_END);
	Memfile_Seek(&rom->code, 0xD1C00 /* 0xB65C00 - RELOC_CODE */);
	
	Mips64_SplitLoad(INST_ADDR(0x800E330C, 0x800E3310), MIPS_REG_A1, rom->offset.segment.seqRom);
	Mips64_SplitLoad(INST_ADDR(0x800E3320, 0x800E3324), MIPS_REG_A1, rom->offset.segment.fontRom);
	Mips64_SplitLoad(INST_ADDR(0x800E3334, 0x800E3338), MIPS_REG_A1, rom->offset.segment.smplRom);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32C4, 0x800E32D4), MIPS_REG_T1, RAM_ADDR);
	Memfile_Append(&rom->code, &rom->mem.seqTbl);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32C8, 0x800E32D8), MIPS_REG_T2, RAM_ADDR);
	Memfile_Append(&rom->code, &rom->mem.fontTbl);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32CC, 0x800E32DC), MIPS_REG_T3, RAM_ADDR);
	Memfile_Append(&rom->code, &rom->mem.sampleTbl);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32D0, 0x800E32E0), MIPS_REG_T6, RAM_ADDR);
	Memfile_Append(&rom->code, &rom->mem.seqFontTbl);
	
	// Move Audio Heap
	Mips64_SplitLoad(INST_ADDR(0x800E3204, 0x800E3208), MIPS_REG_T9, 0x806C0000);
	
	Memfile_Set(&rom->code, MEM_ALIGN, 0, MEM_END);
	
	if (rom->code.seekPoint - 0xD1C00 > 0x318C - 0x10) {
		errr(gLang.audio.err_tbl_write_oos,
			BinToKb(rom->code.seekPoint - 0xD1C00), BinToKb(0x318C - 0x10));
	}
	
#undef INST_ADDR
#undef RAM_ADDR
#undef FUNC_RAM
}

typedef struct {
	const char*  path;
	BuildSample* entry;
} SampleThreadEntry;

void Audio_BuildSampleTable_WriteEntry(SampleThreadEntry* this) {
	BuildSample* entry = this->entry;
	Toml toml;
	char* file;
	char* cfg;
	
	fs_set(this->path);
	file = fs_item("sample.vadpcm.bin");
	cfg = fs_item("config.toml");
	
	Memfile_LoadBin(&entry->sample, file);
	Toml_Load(&toml, cfg);
	
	if (Toml_Var(&toml, "tuning"))
		entry->tuninOverride = Toml_GetFloat(&toml, "tuning");
	
	u32 codec = Toml_GetInt(&toml, "codec") << (32 - 4);
	u32 medium = Toml_GetInt(&toml, "medium") << (32 - 6);
	u32 bitA = Toml_GetInt(&toml, "bitA") << (32 - 7);
	u32 bitB = Toml_GetInt(&toml, "bitB") << (32 - 8);
	entry->data = codec | medium | bitA | bitB | entry->sample.size;
	SwapBE(entry->data);
	
	entry->config.loopStart = Toml_GetInt(&toml, "loop_start");
	entry->config.loopEnd = Toml_GetInt(&toml, "loop_end");
	entry->config.loopCount = Toml_GetInt(&toml, "loop_count");
	entry->config.tailEnd = Toml_GetInt(&toml, "tail_end");
	SwapBE(entry->config.loopStart);
	SwapBE(entry->config.loopEnd);
	SwapBE(entry->config.loopCount);
	SwapBE(entry->config.tailEnd);
	
	Audio_LoadFile(&entry->book, "sample.book.bin");
	if (entry->config.loopCount)
		Audio_LoadFile(&entry->loopBook, "sample.loopbook.bin");
	Toml_Free(&toml);
	
	entry->size = entry->sample.size;
	strcpy(entry->dir, fs_item(""));
	strcpy(entry->name, x_pathslot(this->path, -1));
	strrep(entry->name, "/", "");
}

void Audio_BuildSampleTable(Rom* rom, Memfile* dataFile, Memfile* config) {
	List list = List_New();
	AudioEntryHead head = { 0 };
	AudioEntry entry = { 0 };
	
	Memfile_Null(dataFile);
	Rom_ItemList(&list, "rom/sound/sample/", SORT_NO, LIST_FOLDERS);
	Memfile_Set(dataFile, MEM_ALIGN, 16, MEM_END);
	
	SampleThreadEntry* smplThdEntry = new(SampleThreadEntry[list.num]);
	sBuildSampleTbl = new(BuildSample[list.num]);
	
	for (int i = 0; i < list.num; i++) {
		smplThdEntry[i].path = list.item[i];
		smplThdEntry[i].entry = &sBuildSampleTbl[i];
		Parallel_Add(Audio_BuildSampleTable_WriteEntry, &smplThdEntry[i]);
	}
	
	gParallel_ProgMsg = gLang.rom.target[LANG_SMPL];
	Parallel_Exec(g64.threadNum);
	
	for (int i = 0; i < list.num; i++) {
		BuildSample* entry = &sBuildSampleTbl[i];
		
		if (entry->sample.data != NULL) {
			entry->segment = dataFile->seekPoint;
			SwapBE(entry->segment);
			Memfile_Append(dataFile, &entry->sample);
			sBuildSampleNum++;
		}
	}
	
	head.numEntries = 1;
	SwapBE(head.numEntries);
	entry.romAddr = 0;
	entry.size = ReadBE(dataFile->size);
	entry.medium = 2;
	entry.seqPlayer = 4;
	Memfile_Write(&rom->mem.sampleTbl, &head, 16);
	Memfile_Write(&rom->mem.sampleTbl, &entry, 16);
	
	if (g64.audioOnly)
		Memfile_SaveBin(dataFile, "samples.bin");
	
	else
		rom->offset.segment.smplRom = Dma_WriteMemfile(rom, DMA_FIND_FREE, dataFile, false);
	Memfile_Set(dataFile, MEM_ALIGN, 0, MEM_END);
	
	List_Free(&list);
	delete(smplThdEntry);
}

static void SoundFont_Error_NotFound(const char* sampleName) {
	Memfile mem = Memfile_New();
	
	osLog("Dumping [audio_log]");
	Memfile_Alloc(&mem, 0x90000);
	
	for (int i = 0; i < sBuildSampleNum; i++)
		Memfile_Fmt(&mem, "%-8s%s\n", x_fmt("%d:", i), sBuildSampleTbl[i].name);
	
	Memfile_SaveStr(&mem, "audio_log");
	Memfile_Free(&mem);
	
	errr(gLang.audio.err_bad_sample_reference, sampleName);
}

static s32 SoundFont_SmplID(const char* smpl) {
	s32 sampleID = 0;
	
	for (;; sampleID++) {
		if (sampleID == sBuildSampleNum)
			SoundFont_Error_NotFound(smpl);
		if (!strcmp(sBuildSampleTbl[sampleID].name, smpl))
			break;
	}
	
	return sampleID;
}

static void SoundFont_Instrument_Validate(Toml* toml, u32 id, const char* file, Instrument* inst, char** smpl) {
	if (inst->splitHi < inst->splitLo)
		errr(gLang.audio.warn_inst_bad_splits,
			id, file,
			Note_Name(inst->splitHi + 21), Note_Name(inst->splitLo + 21));
	
	if (smpl[1] && smpl[2] && smpl[0] && inst->splitHi == 127 && inst->splitLo == 0) {
		warn(gLang.audio.warn_inst_bad_splits,
			id, file,
			Note_Name(inst->splitHi + 21), Note_Name(inst->splitLo + 21));
	}
	
	if (smpl[1] && smpl[2] && inst->splitHi == 127) {
		warn(gLang.audio.warn_inst_bad_splits,
			id, file,
			Note_Name(inst->splitHi + 21), Note_Name(inst->splitLo + 21));
	}
	
	if ((!smpl[2] && inst->splitHi < 127) || inst->splitHi > 127) {
		warn(gLang.audio.warn_inst_fix_splits,
			id, file,
			Note_Name(127 + 21));
		
		Toml_SetVar(toml, x_fmt("inst.entry[%d].split_hi", id), "\"%s\"", Note_Name(127 + 21));
		Toml_Save(toml, file);
		inst->splitHi = 127;
	}
}

static s32 SoundFont_Instrument_AssignNames(Toml* toml, u32 id, char** smplNam, Memfile* memBank) {
	u32 smplNum = 0;
	
	for (s32 snd = 0; snd < 3; snd++) {
		if (!Toml_TabItemNum(toml, "inst.entry[%d].%s", id, sInstSectionNames[snd]))
			continue;
		
		smplNam[snd] = Toml_GetStr(toml, "inst.entry[%d].%s.sample", id, sInstSectionNames[snd]);
		if (smplNam[snd] == NULL) {
			continue;
		} else if (!strncmp(smplNam[snd], "NULL", 4)) {
			smplNam[snd] = NULL;
			continue;
		}
		smplNum++;
	}
	
	if (smplNum == 0) {
		u32 null = 0xFFFF; // Empty Instrument Entry
		Memfile_Write(memBank, &null, sizeof(u32));
		
		return 1;
	}
	
	return 0;
}

static void SoundFont_Instrument_AssignIndexes(Toml* toml, u32 id, char** smplNam, s32* smplID, Instrument* inst) {
	for (s32 snd = 0; snd < 3; snd++) {
		if (smplNam[snd] == NULL)
			continue;
		
		smplID[snd] = SoundFont_SmplID(smplNam[snd]);
		inst->sound[snd].tuning = sBuildSampleTbl[smplID[snd]].tuninOverride;
		
		if (inst->sound[snd].tuning == 0)
			inst->sound[snd].tuning =
				Toml_GetFloat(toml,
					"inst.entry[%d].%s.tuning", id, sInstSectionNames[snd]);
		SwapBE(inst->sound[snd].swap32);
	}
}

static void SoundFont_Read_Instrument(Toml* toml, u32 id, Instrument* inst) {
	const char* splitLo = Toml_GetStr(toml, "inst.entry[%d].split_lo", id);
	const char* splitHi = Toml_GetStr(toml, "inst.entry[%d].split_hi", id);
	
	inst->loaded = 0;
	inst->splitLo = Note_Index(splitLo) - 21;
	inst->splitHi = Note_Index(splitHi) - 21;
	inst->splitLo = clamp_min(inst->splitLo, 0);
	inst->splitHi = clamp_min(inst->splitHi, 0);
	inst->release = Toml_GetInt(toml, "inst.entry[%d].release_rate", id);
	
	delete(splitHi, splitLo);
}

static void SoundFont_Read_Adsr(Toml* toml, u32 id, const char* type, Adsr* adsr, s32* num, const char* filename) {
	s32 numRate = Toml_ArrCount(toml, "%s.entry[%d].env_rate", type, id);
	s32 numLevl = Toml_ArrCount(toml, "%s.entry[%d].env_level", type, id);
	
	if (numRate != numLevl)
		errr(gLang.audio.err_env_mismatch, filename);
	
	if (numRate >= 16)
		warn(gLang.audio.warn_max_16_env, filename);
	
	numRate = Min(numRate, 16);
	
	for (; *num < numRate; num[0]++) {
		adsr[*num].rate = Toml_GetInt(toml, "%s.entry[%d].env_rate[%d]", type, id, *num);
		adsr[*num].level = Toml_GetInt(toml, "%s.entry[%d].env_level[%d]", type, id, *num);
		SwapBE(adsr[*num].rate);
		SwapBE(adsr[*num].level);
	}
	
	adsr[*num].rate = -1;
	adsr[*num].level = 0;
	SwapBE(adsr[*num].rate);
	SwapBE(adsr[*num].level);
	
	num[0]++;
}

static void SoundFont_Write_Adsr(Memfile* mem, Adsr* adsr, s32 num, u32* setPtr) {
	const u32 size = sizeof(Adsr[num]);
	void* ptr = memmem_align(16, mem->data, mem->size, adsr, size);
	
	if (!ptr) {
		*setPtr = mem->seekPoint;
		Memfile_Write(mem, adsr, size);
		Memfile_Align(mem, 16);
	} else
		*setPtr = (uaddr_t)ptr - (uaddr_t)mem->data;
}

static void SoundFont_Write_Sample(s32 id, u32* setPtr, Memfile* memSample, Memfile* memBook, Memfile* memLoopBook, u32* sampleNum) {
	Sample smpl = { 0 };
	u32 loop[4 + 8] = {};
	u32 loopSize = sizeof(u32[4]);
	BuildSample* entry = &sBuildSampleTbl[id];
	
	if (id < 0) return;
	
	smpl.sampleAddr = entry->segment;
	smpl.data = entry->data;
	memcpy(loop, &entry->config.loopStart, sizeof(u32[4]));
	
	if (entry->config.loopCount) {
		if (!entry->loopBook.data) {
			warn(gLang.audio.warn_missing_loopbook, sBuildSampleTbl[id].name);
			loop[0] = 0;
			loop[2] = 0;
			loopSize = 0;
		} else {
			memcpy(&loop[4], entry->loopBook.data, sizeof(u32[8]));
			loopSize = sizeof(u32[4 + 8]);
		}
	}
	
	#define WRITE_DATA(TARGET, OPERATION, DST, SRC, SIZE) do { \
				void* ptr = memmem_align(16, DST->data, DST->size, SRC, SIZE); \
				if (!ptr) { \
					TARGET = DST->seekPoint; \
					Memfile_Write(DST, SRC, SIZE); \
					Memfile_Align(DST, 16); \
					OPERATION; \
				} else TARGET = (uaddr_t)ptr - (uaddr_t)DST->data; \
	} while (0)
	
	//crustify
    WRITE_DATA(smpl.loop, (void)0,        memLoopBook, loop,             loopSize);
    WRITE_DATA(smpl.book, (void)0,        memBook,     entry->book.data, entry->book.size);
    WRITE_DATA(setPtr[0], sampleNum[0]++, memSample,   &smpl,            16);
    #undef WRITE_DATA
	//uncrustify
}

typedef struct {
	u32 set;
	const char* fontFile;
	Memfile     memBank;
	u16 numSfx;
	u8  numInst;
	u8  numDrum;
	u8  medType;
	u8  seqPlyr;
} FontBuildInstance;

static void Font_BuildInstance(FontBuildInstance* this) {
	Memfile memBook = {}, memLoopBook = {}, memInst = {}, memEnv = {}, memSample = {}, memSfx = {}, memDrum = {};
	Memfile* memBank = &this->memBank;
	const char* fontFile = this->fontFile;
	Toml* toml = new(Toml);
	u32 bankSampleNum = 0;
	
	//crustify
    Memfile_Alloc(memBank,      KbToBin(400));
    Memfile_Alloc(&memBook,     KbToBin(400));
    Memfile_Alloc(&memLoopBook, KbToBin(400));
    Memfile_Alloc(&memInst,     KbToBin(400));
    Memfile_Alloc(&memEnv,      KbToBin(400));
    Memfile_Alloc(&memSample,   KbToBin(400));
    Memfile_Alloc(&memSfx,      KbToBin(400));
    Memfile_Alloc(&memDrum,     KbToBin(400));
	//uncrustify
	SegmentSet(0x1 + this->set, memBank->data);
	
	osAssert(fontFile != NULL && sys_stat(fontFile));
	fs_set("%s", fontFile);
	Toml_Load(toml, fontFile);
	
	const char* confMed = Toml_GetStr(toml, "medium_type");
	const char* confSeq = Toml_GetStr(toml, "sequence_player");
	
	for (this->medType = 0; ; this->medType++) {
		if (this->medType >= ArrCount(sMediumType) || !confMed)
			errr(gLang.audio.err_unk_medium, fontFile, confMed);
		if (!stricmp(sMediumType[this->medType], confMed))
			break;
	}
	
	for (this->seqPlyr = 0; ; this->seqPlyr++) {
		if (this->seqPlyr >= ArrCount(sSeqPlayerType) || !confSeq)
			errr(gLang.audio.err_unk_seqplr, fontFile, confSeq);
		if (!stricmp(sSeqPlayerType[this->seqPlyr], confSeq))
			break;
	}
	
	delete(confMed, confSeq);
	
	const s32 numInst = this->numInst = Toml_ArrCount(toml, "inst.entry");
	const s32 numSfx = this->numSfx = Toml_ArrCount(toml, "sfx.entry");
	const s32 numDrum = this->numDrum = Toml_ArrCount(toml, "drum.entry");
	
	// DDDDDDDD SSSSSSSS Drum Segment | Sfx Segment
	Memfile_Write(memBank, "\0\0\0\0\0\0\0\0", 8);
	
	for (int j = 0; j < numInst; j++) {
		Instrument instrument = { 0 };
		Adsr adsr[16] = { 0 };
		s32 adsrNum = 0;
		char* sampleName[3] = {};
		s32 sampleIndex[3] = { -1, -1, -1 };
		
		// List does not have entry for j index
		if (!Toml_TabItemNum(toml, "inst.entry[%d]", j)) {
			const u32 null = 0xFFFF; // Empty Instrument Entry
			Memfile_Write(memBank, &null, sizeof(u32));
			continue;
		}
		
		if (SoundFont_Instrument_AssignNames(toml, j, sampleName, memBank))
			continue;
		SoundFont_Instrument_AssignIndexes(toml, j, sampleName, sampleIndex, &instrument);
		SoundFont_Read_Instrument(toml, j, &instrument);
		SoundFont_Read_Adsr(toml, j, "inst", adsr, &adsrNum, fontFile);
		SoundFont_Instrument_Validate(toml, j, fontFile, &instrument, sampleName);
		SoundFont_Write_Adsr(&memEnv, adsr, adsrNum, &instrument.envelope);
		
		for (int k = 0; k < 3; k++)
			SoundFont_Write_Sample(sampleIndex[k], &instrument.sound[k].sample,
				&memSample, &memBook, &memLoopBook, &bankSampleNum);
		
		SwapBE(memInst.seekPoint);
		Memfile_Write(memBank, &memInst.seekPoint, sizeof(u32));
		SwapBE(memInst.seekPoint);
		
		Memfile_Write(&memInst, &instrument, sizeof(struct Instrument));
		
		delete(sampleName[0], sampleName[1], sampleName[2]);
	}
	
	for (int j = 0; j < numSfx; j++) {
		const char* prim;
		Sound sfx = { 0 };
		s32 idx;
		
		if (!Toml_TabItemNum(toml, "sfx.entry[%d]", j)) {
			Memfile_Write(&memSfx, &sfx, sizeof(struct Sound));
			continue;
		}
		
		prim = Toml_GetStr(toml, "sfx.entry[%d].sample", j);
		
		if (!prim || !strcmp(prim, "NULL")) {
			Memfile_Write(&memSfx, &sfx, sizeof(struct Sound));
			continue;
		}
		
		sfx.tuning = Toml_GetFloat(toml, "sfx.entry[%d].tuning", j);
		idx = SoundFont_SmplID(prim);
		
		if (sBuildSampleTbl[idx].tuninOverride > 0)
			sfx.tuning = sBuildSampleTbl[idx].tuninOverride;
		SwapBE(sfx.swap32);
		
		SoundFont_Write_Sample(idx, &sfx.sample, &memSample, &memBook, &memLoopBook, &bankSampleNum);
		Memfile_Write(&memSfx, &sfx, sizeof(struct Sound));
		
		delete(prim);
	}
	
	for (int j = 0; j < numDrum; j++) {
		const char* prim;
		Drum drum = { 0 };
		Adsr adsr[16] = { 0 };
		s32 adsrNum = 0;
		
		if (j == 0) {
			memDrum.seekPoint += 4 * numDrum;
			memset(memDrum.data, 0, memDrum.seekPoint);
			Memfile_Align(&memDrum, 16);
		}
		
		if (!Toml_TabItemNum(toml, "drum.entry[%d]", j)) {
			memDrum.cast.u32[j] = 0;
			
			continue;
		}
		
		prim = Toml_GetStr(toml, "drum.entry[%d].sample", j);
		
		if (!prim || !memcmp(prim, "NULL", 4)) {
			memDrum.cast.u32[j] = 0;
			
			continue;
		} else {
			memDrum.cast.u32[j] = memDrum.seekPoint;
		}
		
		drum.sound.tuning = Toml_GetFloat(toml, "drum.entry[%d].tuning", j);
		drum.loaded = 0;
		drum.pan = Toml_GetInt(toml, "drum.entry[%d].pan", j);
		drum.release = Toml_GetInt(toml, "drum.entry[%d].release_rate", j);
		SwapBE(drum.sound.swap32);
		
		SoundFont_Read_Adsr(toml, j, "drum", adsr, &adsrNum, fontFile);
		SoundFont_Write_Adsr(&memEnv, adsr, adsrNum, &drum.envelope);
		SoundFont_Write_Sample(SoundFont_SmplID(prim), &drum.sound.sample, &memSample, &memBook, &memLoopBook, &bankSampleNum);
		
		Memfile_Write(&memDrum, &drum, sizeof(struct Drum));
		Memfile_Align(&memDrum, 16);
		delete(prim);
	}
	
	Memfile_Align(memBank, 16);
	
	for (int j = 0; j < numInst; j++) {
		if (memBank->cast.u32[2 + j] == 0xFFFF) {
			continue;
		}
		SwapBE(memBank->cast.u32[2 + j]);
		memBank->cast.u32[2 + j] += memBank->seekPoint;
		SwapBE(memBank->cast.u32[2 + j]);
	}
	
	Memfile_Append(memBank, &memInst); Memfile_Align(memBank, 16);
	
	for (int j = 0; j < numInst; j++) {
		if (memBank->cast.u32[2 + j] == 0xFFFF) {
			continue;
		}
		Instrument* inst = SegmentToVirtual(0x1 + this->set, ReadBE(memBank->cast.u32[2 + j]));
		
		inst->envelope += memBank->seekPoint;
		SwapBE(inst->envelope);
	}
	
	for (int j = 0; j < numDrum; j++) {
		if (memDrum.cast.u32[j] == 0)
			continue;
		SegmentSet(0x80 + this->set, memDrum.data);
		Drum* drum = SegmentToVirtual(0x80 + this->set, memDrum.cast.u32[j]);
		
		drum->envelope += memBank->seekPoint;
		SwapBE(drum->envelope);
	}
	
	Memfile_Append(memBank, &memEnv); Memfile_Align(memBank, 16);
	
	for (s32 l = 0; l < bankSampleNum; l++) {
		Sample* smpl = memSample.data;
		
		smpl[l].book += memBank->seekPoint;
		SwapBE(smpl[l].book);
	}
	
	Memfile_Append(memBank, &memBook); Memfile_Align(memBank, 16);
	
	for (s32 l = 0; l < bankSampleNum; l++) {
		Sample* smpl = memSample.data;
		
		smpl[l].loop += memBank->seekPoint;
		SwapBE(smpl[l].loop);
	}
	
	Memfile_Append(memBank, &memLoopBook); Memfile_Align(memBank, 16);
	
	for (int j = 0; j < numInst; j++) {
		if (memBank->cast.u32[2 + j] == 0xFFFF) {
			memBank->cast.u32[2 + j] = 0;
			continue;
		}
		Instrument* inst = SegmentToVirtual(0x1 + this->set, ReadBE(memBank->cast.u32[2 + j]));
		
		if (inst->lo.swap32) {
			inst->lo.sample += memBank->seekPoint;
		}
		if (inst->prim.swap32) {
			inst->prim.sample += memBank->seekPoint;
		}
		if (inst->hi.swap32) {
			inst->hi.sample += memBank->seekPoint;
		}
		
		SwapBE(inst->lo.sample);
		SwapBE(inst->prim.sample);
		SwapBE(inst->hi.sample);
	}
	
	for (int j = 0; j < numSfx; j++) {
		Sound* sound = memSfx.data;
		
		sound[j].sample += memBank->seekPoint;
		SwapBE(sound[j].sample);
	}
	
	for (int j = 0; j < numDrum; j++) {
		if (memDrum.cast.u32[j] == 0)
			continue;
		SegmentSet(0x80 + this->set, memDrum.data);
		Drum* drum = SegmentToVirtual(0x80 + this->set, (memDrum.cast.u32[j]));
		
		drum->sound.sample += memBank->seekPoint;
		SwapBE(drum->sound.sample);
		SegmentSet(0x80 + this->set, NULL);
	}
	
	Memfile_Append(memBank, &memSample); Memfile_Align(memBank, 16);
	
	if (numSfx) {
		memBank->cast.u32[1] = ReadBE(memBank->seekPoint);
		Memfile_Append(memBank, &memSfx); Memfile_Align(memBank, 16);
	}
	
	if (numDrum) {
		for (int j = 0; j < numDrum; j++) {
			if (memDrum.cast.u32[j] == 0)
				continue;
			memDrum.cast.u32[j] += memBank->seekPoint;
			SwapBE(memDrum.cast.u32[j]);
		}
		memBank->cast.u32[0] = ReadBE(memBank->seekPoint);
		Memfile_Append(memBank, &memDrum); Memfile_Align(memBank, 16);
	}
	
	Memfile_Free(&memBook);
	Memfile_Free(&memLoopBook);
	Memfile_Free(&memInst);
	Memfile_Free(&memEnv);
	Memfile_Free(&memSample);
	Memfile_Free(&memSfx);
	Memfile_Free(&memDrum);
	
	Toml_Free(toml);
	delete(toml);
}

void Audio_BuildSoundFont(Rom* rom, Memfile* dataFile, Memfile* config) {
	List list = List_New();
	Memfile soundFontMem = Memfile_New();
	
	Memfile_Alloc(&soundFontMem, MbToBin(2.00));
	Rom_ItemList(&list, "rom/sound/soundfont/", SORT_NUMERICAL, LIST_FILES);
	
	if (shex(list.item[list.num - 1]) > 0x30)
		errr(gLang.rom.err_target_full,
			gLang.rom.target[LANG_FONT], shex(list.item[list.num - 1]), 0x30);
	
	AudioEntryHead sfHead = { .numEntries = list.num };
	SwapBE(sfHead.numEntries);
	Memfile_Write(&rom->mem.fontTbl, &sfHead, 16);
	
	const u32 tnum = g64.threading ? clamp_min(g64.threadNum / 2, 1) : 1;
	FontBuildInstance* fontPass = new(FontBuildInstance[tnum]);
	thread_t* thread = new(thread_t[tnum]);
	
	for (int i = 0; i < list.num; i += tnum) {
		const u32 target = clamp(list.num - i, 0, tnum);
		#define ID (i + j)
		
		for (int j = 0; j < target; j++) {
			FontBuildInstance* this = &fontPass[j];
			
			if (!(this->fontFile = list.item[ID]))
				continue;
			this->set = j;
			
			thd_create(&thread[j], Font_BuildInstance, this);
		}
		
		for (int j = 0; j < target; j++) {
			AudioEntry fontEntry = { .audioTable2 = -1 };
			FontBuildInstance* this = &fontPass[j];
			
			if (this->fontFile) {
				thd_join(&thread[j]);
				
				fontEntry.romAddr = soundFontMem.seekPoint;
				fontEntry.size = this->memBank.size;
				fontEntry.medium = this->medType;
				fontEntry.seqPlayer = this->seqPlyr;
				fontEntry.audioTable1 = 0;
				fontEntry.audioTable2 = -1;
				fontEntry.numInst = this->numInst;
				fontEntry.numDrum = this->numDrum;
				fontEntry.numSfx = this->numSfx;
				SwapBE(fontEntry.romAddr);
				SwapBE(fontEntry.size);
				SwapBE(fontEntry.numSfx);
				
				Memfile_Append(&soundFontMem, &this->memBank);
				Memfile_Align(&soundFontMem, 16);
				Memfile_Free(&this->memBank);
			}
			
			Memfile_Write(&rom->mem.fontTbl, &fontEntry, 16);
			
			info_prog(gLang.rom.target[LANG_FONT], ID + 1, list.num);
		}
#undef ID
	}
	
	if (g64.audioOnly)
		Memfile_SaveBin(&soundFontMem, "soundfonts.bin");
	
	else
		rom->offset.segment.fontRom = Dma_WriteMemfile(rom, DMA_FIND_FREE, &soundFontMem, false);
	
	List_Free(&list);
	Memfile_Free(&soundFontMem);
	delete(fontPass, thread);
}

void Audio_BuildSequence(Rom* rom, Memfile* dataFile, Memfile* config) {
	List itemList = List_New();
	Memfile memIndexTable = Memfile_New();
	Memfile memLookUpTable = Memfile_New();
	Memfile sequenceMem = Memfile_New();
	AudioEntryHead sqHead = { 0 };
	AudioEntry sqEntry = { 0 };
	u8* seqFlag = NULL;
	
	if (!g64.audioOnly)
		seqFlag = SegmentToVirtual(SEG_CODE, 0xBA77F8 - RELOC_CODE);
	
	Memfile_Alloc(&memIndexTable, 0x800);
	Memfile_Alloc(&memLookUpTable, 0x800);
	Memfile_Alloc(&sequenceMem, MbToBin(1.0));
	Rom_ItemList(&itemList, "rom/sound/sequence/", SORT_NUMERICAL, LIST_FOLDERS);
	
	if (itemList.num - 1 > 0x7F)
		errr(gLang.rom.err_target_full,
			gLang.rom.target[LANG_SEQ], shex(itemList.item[itemList.num - 1]), 0x7F);
	
	sqHead.numEntries = itemList.num;
	SwapBE(sqHead.numEntries);
	Memfile_Write(&rom->mem.seqTbl, &sqHead, 16);
	
	for (int i = 0; i < itemList.num; i++) {
		info_prog(gLang.rom.target[LANG_SEQ], i + 1, itemList.num);
		u32 addr;
		u8 fontNum;
		
		if (itemList.item[i] == NULL) {
			sqEntry = (AudioEntry) { 0 };
			Memfile_Write(&memLookUpTable, "\xFF\xFF", 2);
			Memfile_Write(&rom->mem.seqTbl, &sqEntry, 16);
			continue;
		}
		
		fs_set(itemList.item[i]);
		
		u32 med = 0;
		u32 seq = 0;
		char* confMed;
		char* confSeq;
		char* fseq;
		List flagList = List_New();
		List bankList = List_New();
		
		Memfile_Null(dataFile);
		Memfile_Null(config);
		Memfile_LoadStr(config, fs_item("config.toml"));
		confMed = Ini_GetStr(config, "medium_type");
		confSeq = Ini_GetStr(config, "sequence_player");
		
		if (!g64.audioOnly) {
			Ini_GetArr(config, "sequence_flags", &flagList);
			
			if (Ini_GetError())
				errr_align(gLang.err_fail, config->info.name);
		}
		
		for (;; med++) {
			if (med >= ArrCount(sMediumType))
				errr(gLang.audio.err_unk_medium, itemList.item[i],  x_fmt("index: %d", med));
			if (!strcmp(sMediumType[med], confMed))
				break;
		}
		for (;; seq++) {
			if (seq >= ArrCount(sSeqPlayerType))
				errr(gLang.audio.err_unk_seqplr, itemList.item[i],  x_fmt("index: %d", seq));
			if (!strcmp(sSeqPlayerType[seq], confSeq))
				break;
		}
		
		fseq = fs_find(".aseq");
		
		if (!g64.audioOnly) {
			seqFlag[i] = 0;
			for (int k = 0; k < flagList.num; k++) {
				foreach(j, sSeqFlagName) {
					if (!stricmp(flagList.item[k], sSeqFlagName[j]))
						seqFlag[i] |= (1) << j;
				}
			}
		}
		
		sqEntry.medium = med;
		sqEntry.seqPlayer = seq;
		sqEntry.audioTable1 = 0;
		sqEntry.audioTable2 = 0;
		sqEntry.numInst = 0;
		sqEntry.numDrum = 0;
		sqEntry.numSfx = 0;
		
		if (fseq) {
			Memfile_LoadBin(dataFile, fseq);
			addr = sequenceMem.seekPoint;
			Memfile_Append(&sequenceMem, dataFile);
			Memfile_Align(&sequenceMem, 16);
			sqEntry.romAddr = addr;
			sqEntry.size = dataFile->size;
		} else {
			sqEntry.romAddr = Ini_GetInt(config, "seq_pointer");
			sqEntry.size = 0;
		}
		
		SwapBE(sqEntry.romAddr);
		SwapBE(sqEntry.size);
		Memfile_Write(&rom->mem.seqTbl, &sqEntry, 16);
		
		u16 offset = memIndexTable.seekPoint;
		Memfile_Write(&memLookUpTable, &offset, 2);
		
		Ini_GetArr(config, "bank_id", &bankList);
		fontNum = bankList.num;
		Memfile_Write(&memIndexTable, &fontNum, 1);
		for (int j = 0; j < fontNum; j++) {
			u8 bankId = shex(bankList.item[j]);
			Memfile_Write(&memIndexTable, &bankId, 1);
		}
		
		List_Free(&flagList);
		List_Free(&bankList);
	}
	
	u16 add = memLookUpTable.seekPoint;
	
	for (int i = 0; i < itemList.num; i++) {
		if (memLookUpTable.cast.u16[i] != 0xFFFF)
			memLookUpTable.cast.u16[i] += add;
		SwapBE(memLookUpTable.cast.u16[i]);
	}
	Memfile_Append(&memLookUpTable, &memIndexTable);
	Memfile_Append(&rom->mem.seqFontTbl, &memLookUpTable);
	
	if (g64.audioOnly)
		Memfile_SaveBin(&sequenceMem, "sequences.bin");
	
	else
		rom->offset.segment.seqRom = Dma_WriteMemfile(rom, DMA_FIND_FREE, &sequenceMem, false);
	
	Memfile_Free(&memIndexTable);
	Memfile_Free(&memLookUpTable);
	Memfile_Free(&sequenceMem);
	List_Free(&itemList);
}

typedef struct {
	Memfile*    sample;
	Memfile*    wave;
	Memfile*    book;
	Memfile*    loop;
	const char* path;
	u32 i;
} SfxThread;

static mutex_t sMutexSfx;

static void SfxTable_Build(SfxThread* this) {
	Memfile config = Memfile_New();
	Sample* sample = (Sample*)&this->sample->cast.u8[sizeof(Sample) * this->i];
	AdpcmBook* book = NULL;
	AdpcmLoop* loop = NULL;
	u8* wave = NULL;
	f32* tuning;
	
	nested(void, HookVars, ()) {
		nested_var(book);
		nested_var(loop);
		nested_var(wave);
		
		nested(void, GetFile, (Memfile * this, const char* wildcard)) {
			const char* file = fs_find(wildcard);
			
			if (!file)
				errr_align(gLang.err_load, "%s%s", fs_item(""), wildcard);
			Memfile_WriteFile(this, file);
		};
		
		nested(void, HookVars_MemSeek, (void* ptr, u32 * segment, Memfile * mem)) {
			void** set = ptr;
			
			*set = &mem->cast.u8[mem->seekPoint];
			*segment = mem->seekPoint;
		};
		
		bool hasLoop = false;
		
		if (sys_stat(fs_find(".loopbook.bin")))
			hasLoop = true;
		
		HookVars_MemSeek(&book, &sample->book, this->book);
		GetFile(this->book, ".book.bin");
		Memfile_Align(this->book, 16);
		
		HookVars_MemSeek(&wave, &sample->sampleAddr, this->wave);
		GetFile(this->wave, ".vadpcm.bin");
		sample->data = this->wave->seekPoint - sample->sampleAddr;
		Memfile_Align(this->book, 16);
		
		u32 fill[4] = {};
		HookVars_MemSeek(&loop, &sample->loop, this->loop);
		Memfile_Write(this->loop, fill, sizeof(fill));
		if (hasLoop)
			GetFile(this->loop, ".loopbook.bin");
		Memfile_Align(this->book, 16);
	};
	
	fs_set(this->path);
	Memfile_LoadStr(&config, fs_item("config.toml"));
	
	memset(sample, 0, sizeof(*sample));
	
	pthread_mutex_lock(&sMutexSfx);
	HookVars(&book, &loop, &wave, this, sample);
	pthread_mutex_unlock(&sMutexSfx);
	
	sample->data |= Ini_GetInt(&config, "codec") << (32 - 4);
	sample->data |= Ini_GetInt(&config, "medium") << (32 - 6);
	sample->data |= Ini_GetInt(&config, "bitA") << (32 - 7);
	sample->data |= Ini_GetInt(&config, "bitB") << (32 - 8);
	SwapBE(sample->data);
	
	loop->start = Ini_GetInt(&config, "loop_start");
	loop->end = Ini_GetInt(&config, "loop_end");
	loop->count = Ini_GetInt(&config, "loop_count");
	tuning = (f32*)&loop->origSpls;
	*tuning = Ini_GetFloat(&config, "tuning");
	
	if (Ini_GetError())
		errr_align(gLang.err_fail, fs_item("config.toml"));
	
	SwapBE(loop->start)
	SwapBE(loop->end)
	SwapBE(loop->count)
	SwapBE(loop->origSpls)
	
	Memfile_Free(&config);
}

void Audio_BuildSfxTable(Rom* rom, Memfile* dataFile, Memfile* config) {
	List list = List_New();
	SfxThread* thd;
	Memfile sample = Memfile_New();
	Memfile wave = Memfile_New();
	Memfile book = Memfile_New();
	Memfile loop = Memfile_New();
	u32 offsetWave, offsetBook, offsetLoop;
	
	if (!sys_stat("rom/sound/sfx/")) return;
	List_Walk(&list, "rom/sound/sfx/", 0, LIST_FOLDERS | LIST_NO_DOT);
	if (list.num == 0) return;
	
	thd = alloc(sizeof(SfxThread) * list.num);
	
	Memfile_Alloc(&sample, MbToBin(10.0));
	Memfile_Alloc(&wave, MbToBin(8.0));
	Memfile_Alloc(&book, MbToBin(1.0));
	Memfile_Alloc(&loop, MbToBin(1.0));
	
	for (int i = 0; i < list.num; i++) {
		thd[i].sample = &sample;
		thd[i].wave = &wave;
		thd[i].book = &book;
		thd[i].loop = &loop;
		thd[i].path = list.item[i];
		thd[i].i = i;
		
		Parallel_Add(SfxTable_Build, &thd[i]);
	}
	
	gParallel_ProgMsg = gLang.rom.target[LANG_SFX];
	pthread_mutex_init(&sMutexSfx, 0);
	Parallel_Exec(g64.threadNum);
	pthread_mutex_destroy(&sMutexSfx);
	
	sample.seekPoint = sizeof(Sample) * list.num;
	offsetBook = sample.seekPoint;
	Memfile_Append(&sample, &book);
	
	offsetLoop = sample.seekPoint;
	Memfile_Append(&sample, &loop);
	
	offsetWave = sample.seekPoint;
	Memfile_Append(&sample, &wave);
	
	for (int i = 0; i < list.num; i++) {
		Sample* samples = sample.data;
		
		samples[i].book += offsetBook;
		samples[i].sampleAddr += offsetWave;
		samples[i].loop += offsetLoop;
		SwapBE(samples[i].book);
		SwapBE(samples[i].sampleAddr);
		SwapBE(samples[i].loop);
	}
	
	Dma_WriteMemfile(rom, 5, &sample, 0);
	
	Memfile_Free(&sample);
	List_Free(&list);
}

// # # # # # # # # # # # # # # # # # # # #
// # EXTRA                               #
// # # # # # # # # # # # # # # # # # # # #

void Audio_DeleteUnreferencedSamples(void) {
	List smpl_list = List_New();
	List font_list = List_New();
	List ref_list = List_New();
	
	Rom_ItemList(&smpl_list, "rom/sound/sample/", SORT_NO, LIST_FOLDERS);
	Rom_ItemList(&font_list, "rom/sound/soundfont/", SORT_NUMERICAL, LIST_FILES);
	List_Alloc(&ref_list, smpl_list.num);
	
	for (int i = 0; i < font_list.num; i++) {
		Toml font = Toml_New();
		int num;
		
		Toml_Load(&font, font_list.item[i]);
		
		num = Toml_ArrCount(&font, "inst.entry");
		for (int i = 0; i < num; i++) {
			if (!Toml_TabItemNum(&font, "inst.entry[%d]", i))
				continue;
			
			if (Toml_Var(&font, "inst.entry[%d].low.sample", i))
				List_Add(&ref_list, Toml_GetStr(&font, "inst.entry[%d].low.sample", i));
			if (Toml_Var(&font, "inst.entry[%d].prim.sample", i))
				List_Add(&ref_list, Toml_GetStr(&font, "inst.entry[%d].prim.sample", i));
			if (Toml_Var(&font, "inst.entry[%d].hi.sample", i))
				List_Add(&ref_list, Toml_GetStr(&font, "inst.entry[%d].hi.sample", i));
		}
		
		num = Toml_ArrCount(&font, "sfx.entry");
		for (int i = 0; i < num; i++) {
			if (!Toml_TabItemNum(&font, "sfx.entry[%d]", i))
				continue;
			if (Toml_Var(&font, "sfx.entry[%d].sample", i))
				List_Add(&ref_list, Toml_GetStr(&font, "sfx.entry[%d].sample", i));
		}
		
		num = Toml_ArrCount(&font, "drum.entry");
		for (int i = 0; i < num; i++) {
			if (!Toml_TabItemNum(&font, "drum.entry[%d]", i))
				continue;
			if (Toml_Var(&font, "drum.entry[%d].sample", i))
				List_Add(&ref_list, Toml_GetStr(&font, "drum.entry[%d].sample", i));
		}
		
		Toml_Free(&font);
	}
	
	osLog("sort");
	List_Sort(&ref_list);
	
	for (int i = 0; i < smpl_list.num; i++) {
		const char* a = x_strdup(smpl_list.item[i]);
		strend(a, "/")[0] = '\0';
		const char* filename = x_basename(a);
		
		osLog("check ref: %s", filename);
		
		for (int k = 0; k < ref_list.num; k++) {
			if (ref_list.item[k])
				if (streq(filename, ref_list.item[k]))
					delete(smpl_list.item[i]);
		}
	}
	
	for (int i = 0; i < smpl_list.num; i++) {
		if (!smpl_list.item[i]) continue;
		
		warn_align(gLang.rm, smpl_list.item[i]);
		if (sys_rmdir(smpl_list.item[i]))
			warn_align(gLang.err_rm, smpl_list.item[i]);
	}
	
	List_Free(&smpl_list);
	List_Free(&font_list);
	List_Free(&ref_list);
}
