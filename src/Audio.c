#include "z64rom.h"
#include "Tools.h"

char sBankFiles[1024 * 5][512];
s32 sBankNum;
SampleInfo sUnsortedSampleTbl[1024 * 5];
SampleInfo* sSortedSampleTbl[1024 * 5];
s32 sDumpID;
s32 sSortID;
char* sMediumType[] = {
	"ram",
	"unk",
	"cart",
	"ddrive"
};
char* sSeqPlayerType[] = {
	"sfx",
	"fanfare",
	"bgm",
	"demo"
};
struct {
	char name[128];
	u32  segment;
	u32  size;
	f32  tuninOverride;
	char dir[512];
} sSampleTbl[1024 * 2];
s32 sSampleTblNum;
const char* sInstSectionNames[3] = {
	"low",
	"prim",
	"hi",
};

f32 Audio_CalcReleaseRate(f32 a) {
	return 256.0 * 0.00130208 / a;
}

f32 Audio_GetReleaseRate(u8 i) {
#if 0
	f32 r = 0;
	
	switch (i) {
		case 255:
			r =  Audio_CalcReleaseRate(0.25f);
			break;
		case 254:
			r =  Audio_CalcReleaseRate(0.33f);
			break;
		case 253:
			r =  Audio_CalcReleaseRate(0.5f);
			break;
		case 252:
			r =  Audio_CalcReleaseRate(0.66f);
			break;
		case 251:
			r =  Audio_CalcReleaseRate(0.75f);
			break;
		case 128 ... 250:
			r =  Audio_CalcReleaseRate(251 - i);
			break;
		case 16 ... 127:
			r =  Audio_CalcReleaseRate(4 * (143 - i));
			break;
		case 1 ... 15:
			r =  Audio_CalcReleaseRate(60 * (23 - i));
			break;
	}
	
	return r / 1.3333333333;
#endif
	
	return (f32)i / 255;
}

u8 Audio_GetReleaseID(f32 r) {
#if 0
	s32 id = 255;
	f32 diff = FLT_MAX;
	
	for (s32 i = 0; i <= 255; i++) {
		f32 val = Audio_GetReleaseRate(i);
		f32 ndiff = Abs(r - val);
		
		if (ndiff < diff) {
			id = i;
			diff = ndiff;
		}
	}
	
	return id;
#endif
	
	return Clamp((s32)(r * 255), 0, 255);
}

// # # # # # # # # # # # # # # # # # # # #
// # CONFIG                              #
// # # # # # # # # # # # # # # # # # # # #

#define __Config_Sample(config, wow, sampletype) \
	Config_WriteSection(config, # wow, NO_COMMENT); \
	Config_WriteHex(config, "sample", ReadBE(sample->sampleAddr) + rom->offset.segment.smplRom + off, NO_COMMENT); \
	Config_WriteFloat(config, "tuning", *f, NO_COMMENT); \
	if (sBankNum < 0) { printf("\a\n"); exit(1); /* "go intentionally bonkers" */ } \
	sUnsortedSampleTbl[sDumpID].tuning = *f; \
	sUnsortedSampleTbl[sDumpID].data = sample->data; \
	sUnsortedSampleTbl[sDumpID].sampleAddr = ReadBE(sample->sampleAddr) + rom->offset.segment.smplRom + off; \
	sUnsortedSampleTbl[sDumpID].loop = VirtualToSegmented(0x0, SegmentedToVirtual(0x1, ReadBE(sample->loop))); \
	sUnsortedSampleTbl[sDumpID++].book = VirtualToSegmented(0x0, SegmentedToVirtual(0x1, ReadBE(sample->book))); \
	Assert(sDumpID < 1024 * 5);

#define __Config_Sample_NULL(config, wow) \
	Config_WriteSection(config, # wow, NO_COMMENT); \
	Config_WriteStr(config, "sample", "NULL", false, NO_COMMENT); \
	Config_WriteStr(config, "tuning", "NULL", false, NO_COMMENT);

static void Rom_Config_Envelope(MemFile* config, Adsr* env) {
	ItemList listRate = ItemList_Initialize();
	ItemList listLevl = ItemList_Initialize();
	
	Config_Print(config, "\n");
	Config_WriteComment(config, "Envelope, values between 0.0 - 1.0");
	ItemList_Alloc(&listRate, 32, 512);
	ItemList_Alloc(&listLevl, 32, 512);
	
	for (s32 i = 0; ; i++) {
		if (ReadBE(env[i].rate) < 0)
			break;
		ItemList_AddItem(&listRate, xFmt("%.5f", (f32)ReadBE(env[i].rate) / __INT16_MAX__));
		ItemList_AddItem(&listLevl, xFmt("%.5f", (f32)ReadBE(env[i].level) / __INT16_MAX__));
	}
	Config_WriteArray(config, "env_rate", &listRate, NO_QUOTES, NO_COMMENT);
	Config_WriteArray(config, "env_level", &listLevl, NO_QUOTES, NO_COMMENT);
	ItemList_Free(&listRate);
	ItemList_Free(&listLevl);
}

static s32 Rom_Config_Instrument(Rom* rom, MemFile* config, Instrument* instrument, char* name, char* out, u32 off) {
	Adsr* env;
	Sample* sample;
	u32 val;
	f32* f = (f32*)&val;
	Instrument tempI = { .splitHi = 127 };
	Adsr tempE[4] = { { -1, 0 }, { 0 }, { 0 }, { 0 } };
	
	if (instrument == NULL) {
		env = tempE;
		instrument = &tempI;
	} else {
		env = SegmentedToVirtual(0x1, ReadBE(instrument->envelope));
	}
	
	if (instrument->lo.sample == 0 &&
		instrument->prim.sample == 0 &&
		instrument->hi.sample == 0) {
		return 0;
	}
	
	MemFile_Reset(config);
	Config_WriteComment(config, "Instrument");
	Config_WriteStr(config, "split_lo", Music_NoteWord(instrument->splitLo + 21), true, "Prim Start");
	Config_WriteStr(config, "split_hi", Music_NoteWord(instrument->splitHi + 21), true, "Prim End");
	Rom_Config_Envelope(config, env);
	Config_Print(config, "%-15s = %.4f\n", "release_rate", Audio_GetReleaseRate(instrument->release));
	
	Config_Print(config, "\n");
	if (instrument->lo.sample != 0) {
		sample = SegmentedToVirtual(0x1, ReadBE(instrument->lo.sample));
		val = ReadBE(instrument->lo.swap32);
		__Config_Sample(config, low, ins);
	} else {
		__Config_Sample_NULL(config, low);
	}
	
	if (instrument->prim.sample != 0) {
		sample = SegmentedToVirtual(0x1, ReadBE(instrument->prim.sample));
		val = ReadBE(instrument->prim.swap32);
		sUnsortedSampleTbl[sDumpID].isPrim = true;
		sUnsortedSampleTbl[sDumpID].splitLo = instrument->splitLo;
		sUnsortedSampleTbl[sDumpID].splitHi = instrument->splitHi;
		__Config_Sample(config, prim, ins);
	} else {
		__Config_Sample_NULL(config, prim);
	}
	
	if (instrument->hi.sample != 0) {
		sample = SegmentedToVirtual(0x1, ReadBE(instrument->hi.sample));
		val = ReadBE(instrument->hi.swap32);
		__Config_Sample(config, hi, ins);
	} else {
		__Config_Sample_NULL(config, hi);
	}
	
	MemFile_SaveFile_String(config, out);
	
	return 1;
}

static s32 Rom_Config_Sfx(Rom* rom, MemFile* config, Sound* sfx, char* name, char* out, u32 off) {
	u32 val;
	f32* f = (f32*)&val;
	
	if (sfx->sample == 0) {
		return 0;
	}
	
	MemFile_Reset(config);
	Config_WriteComment(config, "Sfx");
	if (sfx->sample != 0) {
		Sample* sample = SegmentedToVirtual(0x1, ReadBE(sfx->sample));
		val = ReadBE(sfx->swap32);
		__Config_Sample(config, prim, sfx);
	} else {
		__Config_Sample_NULL(config, prim);
	}
	MemFile_SaveFile_String(config, out);
	
	return 1;
}

static s32 Rom_Config_Drum(Rom* rom, MemFile* config, u32 drumSeg, char* name, char* out, u32 off) {
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
		drum = SegmentedToVirtual(0x1, ReadBE(drumSeg));
		env = SegmentedToVirtual(0x1, ReadBE(drum->envelope));
	}
	
	if (drum->sound.sample == 0) {
		return 0;
	}
	
	MemFile_Reset(config);
	Config_WriteComment(config, "Drum");
	Config_WriteInt(config, "pan", drum->pan, NO_COMMENT);
	Rom_Config_Envelope(config, env);
	Config_Print(config, "%-15s = %.4f\n", "release_rate", Audio_GetReleaseRate(drum->release));
	
	Config_Print(config, "\n");
	Config_WriteComment(config, "Sample");
	if (drum->sound.sample != 0) {
		Sample* sample = SegmentedToVirtual(0x1, ReadBE(drum->sound.sample));
		val = ReadBE(drum->sound.swap32);
		__Config_Sample(config, prim, drm);
	} else {
		__Config_Sample_NULL(config, prim);
	}
	
	MemFile_SaveFile_String(config, out);
	
	return 1;
}

static void Rom_Config_Sample(Rom* rom, MemFile* config, Sample* sample, char* name, char* out) {
	AdpcmLoop* loop = SegmentedToVirtual(0x0, sample->loop);
	
	MemFile_Reset(config);
	Config_WriteComment(config, name);
	Config_WriteInt(config, "codec", ReadBE(sample->data) >> (32 - 4), NO_COMMENT);
	Config_WriteInt(config, "medium", (ReadBE(sample->data) >> (32 - 6)) & 2, NO_COMMENT);
	Config_WriteInt(config, "bitA", (ReadBE(sample->data) >> (32 - 7)) & 1, NO_COMMENT);
	Config_WriteInt(config, "bitB", (ReadBE(sample->data) >> (32 - 8)) & 1, NO_COMMENT);
	
	Config_Print(config, "\n");
	Config_WriteComment(config, "Loop");
	Config_WriteInt(config, "loop_start", ReadBE(loop->start), NO_COMMENT);
	Config_WriteInt(config, "loop_end", ReadBE(loop->end), NO_COMMENT);
	Config_WriteInt(config, "loop_count", ReadBE(loop->count), NO_COMMENT);
	Config_WriteInt(config, "tail_end", ReadBE(loop->origSpls), NO_COMMENT);
	
	MemFile_SaveFile_String(config, out);
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

static void Audio_PatchWavFiles(MemFile* dataFile, MemFile* config) {
	#define NOTE(note, octave) (note + (12 * (octave)))
	u8* instInfo;
	u32* smplInfo;
	const struct {
		u8 basenote;
		s8 finetune;
		const N64AudioInfo* info;
	} info[] = {
		{ NOTE(0, 3), 0, &gSampleInfo[367] }, // LowPerc
		{ NOTE(1, 3), 0, &gSampleInfo[368] }, // Snare
		{ NOTE(3, 3), 0, &gSampleInfo[369] }, // SoftSnare
		{ NOTE(1, 4), 0, &gSampleInfo[370] }, // Cymbal
		{ NOTE(0, 6), 0, &gSampleInfo[371] }, // Timpani
		{ NOTE(5, 3), 0, &gSampleInfo[405] }, // Gong
		{ NOTE(0, 6), 0, &gSampleInfo[406] }, // WindChimes
		
		{ NOTE(0, 5), 0, &gSampleInfo[407] }, // CongaOpen
		{ NOTE(0, 5), 0, &gSampleInfo[414] }, // CongaSoft
		{ NOTE(8, 4), 0, &gSampleInfo[415] }, // CongaMute
		
		{ NOTE(5, 3), 0, &gSampleInfo[393] }, // LuteA
		{ NOTE(7, 3), 0, &gSampleInfo[394] }, // LuteB
		
		{ NOTE(11, 3), 0, &gSampleInfo[378] }, // Tambourine
		{ NOTE(0, 4), 0, &gSampleInfo[120] }, // Tambourine
		{ NOTE(2, 4), 0, &gSampleInfo[121] }, // Tambourine
		
		{ NOTE(0, 3), 0, &gSampleInfo[432] }, // Cajon
		{ NOTE(2, 3), 0, &gSampleInfo[433] }, // Cajon
	};
	
	foreach(i, info) {
		printf_progress("Update Sample", i + 1, ArrayCount(info));
		char* file = xFmt("rom/sound/sample/%s/%s/Sample.wav", gVanilla, info[i].info->name);
		
		MemFile_Reset(dataFile);
		MemFile_LoadFile(dataFile, file);
		instInfo = MemMem(dataFile->data, dataFile->size, "inst", 4);
		smplInfo = MemMem(dataFile->data, dataFile->size, "smpl", 4);
		
		/* basenote */ instInfo[8] = info[i].basenote;
		/* finetune */ instInfo[9] = info[i].finetune;
		/* basenote */ smplInfo[5] = info[i].basenote;
		/* finetune */ smplInfo[6] = info[i].finetune;
		MemFile_SaveFile(dataFile, file);
	}
#undef NOTE
}

void Audio_DumpSoundFont(Rom* rom, MemFile* dataFile, MemFile* config) {
	AudioEntryHead* head = SegmentedToVirtual(0, rom->offset.table.fontTable);
	AudioEntryHead* sampHead = SegmentedToVirtual(0, rom->offset.table.sampleTable);
	AudioEntry* entry;
	u32 num = ReadBE(head->numEntries);
	SoundFont* bank;
	Instrument* instrument;
	Sound* sfx;
	u32 off = 0;
	
	for (s32 i = 0; i < num; i++) {
		char* path = xFmt("rom/sound/soundfont/%s/0x%02X-%s/", gVanilla, i, gBankName_OoT[i]);
		printf_progress("SoundFont", i + 1, num);
		
		entry = &head->entries[i];
		
		bank = SegmentedToVirtual(0x0, ReadBE(entry->romAddr) + rom->offset.segment.fontRom);
		off = ReadBE(sampHead->entries[entry->audioTable1].romAddr);
		if (off & 0xF) {
			printf_warning("audioTable Segment %08X id %d", off, entry->audioTable1);
			off = off & 0xFFFFFFF0;
		}
		SetSegment(0x1, bank);
		
		if (entry->numInst) {
			for (s32 j = 0; j < entry->numInst; j++) {
				char* output = xFmt("%sinstrument/%d-Inst.cfg", path, j);
				Sys_MakeDir(Path(output));
				
				if (bank->instruments[j] == 0)
					instrument = NULL;
				else
					instrument = SegmentedToVirtual(0x1, ReadBE(bank->instruments[j]));
				if (Rom_Config_Instrument(rom, config, instrument, "Instrument", output, off)) {
					strcpy(sBankFiles[sBankNum++], output);
					Assert(sBankNum < 1024 * 5);
				}
			}
		}
		
		if (entry->numSfx) {
			for (s32 j = 0; j < ReadBE(entry->numSfx); j++) {
				char* output = xFmt("%ssfx/%d-Sfx.cfg", path, j);
				Sys_MakeDir(Path(output));
				
				sfx = SegmentedToVirtual(0x1, ReadBE(bank->sfx));
				
				if (Rom_Config_Sfx(rom, config, &sfx[j], "Sound Effect", output, off)) {
					strcpy(sBankFiles[sBankNum++], output);
					Assert(sBankNum < 1024 * 5);
				}
			}
		}
		
		if (entry->numDrum) {
			for (s32 j = 0; j < entry->numDrum; j++) {
				char* output = xFmt("%sdrum/%d-Drum.cfg", path, j);
				u32* wow = SegmentedToVirtual(0x1, ReadBE(bank->drums));
				
				Sys_MakeDir(Path(output));
				
				if (Rom_Config_Drum(rom, config, wow[j], "Drum", output, off)) {
					strcpy(sBankFiles[sBankNum++], output);
					Assert(sBankNum < 1024 * 5);
				}
			}
		}
		
		MemFile_Reset(config);
		Config_WriteComment(config, gBankName_OoT[i]);
		
		MemFile_Printf(config, "# Sample Medium types [");
		for (s32 e = 0; e < ArrayCount(sMediumType); e++) {
			if (e != 0)
				MemFile_Printf(config, "/");
			MemFile_Printf(config, "%s", sMediumType[e]);
		}
		MemFile_Printf(config, "]\n");
		
		Config_WriteStr(config, "medium_type", sMediumType[entry->medium], true, 0);
		
		MemFile_Printf(config, "# Sequence Player types [");
		for (s32 e = 0; e < ArrayCount(sSeqPlayerType); e++) {
			if (e != 0)
				MemFile_Printf(config, "/");
			MemFile_Printf(config, "%s", sSeqPlayerType[e]);
		}
		MemFile_Printf(config, "]\n");
		
		Config_WriteStr(config, "sequence_player", sSeqPlayerType[entry->seqPlayer], true, 0);
		
		MemFile_SaveFile_String(config, xFmt("%sconfig.cfg", path));
	}
	
	SetSegment(0x1, NULL);
}

void Audio_DumpSequence(Rom* rom, MemFile* dataFile, MemFile* config) {
	AudioEntryHead* head = SegmentedToVirtual(0x0, rom->offset.table.seqTable);
	u8* seqFlag = SegmentedToVirtual(0x0, 0xBA77F8);
	u8* seqFontTable;
	u16* segFontOffTable;
	AudioEntry* entry;
	RomFile romFile;
	u32 num = ReadBE(head->numEntries);
	
	SetSegment(0x1, SegmentedToVirtual(0x0, rom->offset.table.seqFontTbl));
	
	MemFile_Reset(config);
	for (s32 i = 0; i < num; i++) {
		char* path = xFmt("rom/sound/sequence/%s/0x%02X-%s/", gVanilla, i, gSequenceName_OoT[i]);
		ItemList bankList = ItemList_Initialize();
		ItemList flagList = ItemList_Initialize();
		u32 bankNum;
		u32 bankId;
		
		Sys_MakeDir(path);
		printf_progress("Sequence", i + 1, num);
		segFontOffTable = SegmentedToVirtual(0x0, rom->offset.table.seqFontTbl);
		entry = &head->entries[i];
		romFile.data = SegmentedToVirtual(0x0, ReadBE(entry->romAddr) + rom->offset.segment.seqRom);
		romFile.size = ReadBE(entry->size);
		
		MemFile_Reset(config);
		seqFontTable = SegmentedToVirtual(0x1, ReadBE(segFontOffTable[i]));
		
		ItemList_Alloc(&bankList, 8, 256);
		Config_WriteComment(config, gSequenceName_OoT[i]);
		bankNum = ReadBE(seqFontTable[0]);
		
		for (s32 i = 0; i < bankNum; i++) {
			bankId = (ReadBE(seqFontTable[i + 1]) & 0xFF);
			ItemList_AddItem(&bankList, xFmt("0x%02X", bankId));
		}
		
		Config_WriteArray(config, "bank_id", &bankList, false, 0);
		
		if (romFile.size != 0) {
			Rom_Extract(dataFile, romFile, xFmt("%s%s.aseq", path, gSequenceName_OoT[i]));
		} else {
			Config_WriteHex(config, "seq_pointer", ReadBE(entry->romAddr), "Sequence ID - Jumps into this sequence");
		}
		
		MemFile_Printf(config, "# Sample Medium types [");
		for (s32 e = 0; e < ArrayCount(sMediumType); e++) {
			if (e != 0)
				MemFile_Printf(config, "/");
			MemFile_Printf(config, "%s", sMediumType[e]);
		}
		MemFile_Printf(config, "]\n");
		
		Config_WriteStr(config, "medium_type", sMediumType[entry->medium], true, 0);
		
		MemFile_Printf(config, "# Sequence Player types [");
		for (s32 e = 0; e < ArrayCount(sSeqPlayerType); e++) {
			if (e != 0)
				MemFile_Printf(config, "/");
			MemFile_Printf(config, "%s", sSeqPlayerType[e]);
		}
		MemFile_Printf(config, "]\n");
		Config_WriteStr(config, "sequence_player", sSeqPlayerType[entry->seqPlayer], true, 0);
		
		ItemList_Alloc(&flagList, 10, 1024);
		
		for (s32 b = (1) << 0, n = 0; b < SEQFLAG_MAX; b = (b) << 1, n++) {
			if (seqFlag[i] & b)
				ItemList_AddItem(&flagList, sSeqFlagName[n]);
		}
		
		Config_Print(config, "# [ ");
		for (s32 e = 0; e < ArrayCount(sSeqFlagName); e++) {
			Config_Print(config, "%s", sSeqFlagName[e]);
			
			if (e + 1 < ArrayCount(sSeqFlagName))
				Config_Print(config, "/", sSeqFlagName[e]);
		}
		Config_Print(config, "]\n");
		
		Config_WriteArray(config, "sequence_flags", &flagList, QUOTES, NO_COMMENT);
		
		MemFile_SaveFile_String(config, xFmt("%sconfig.cfg", path));
		
		ItemList_Free(&bankList);
		ItemList_Free(&flagList);
	}
	
	SetSegment(0x1, NULL);
}

static void SampleDump_Thread(SampleDumpArg* arg) {
	const N64AudioInfo* sample = arg->sample;
	SampleInfo* tbl = arg->tbl;
	AdpcmLoop* loop;
	AdpcmBook* book;
	MemFile* memF;
	MemFile* memC;
	RomFile rf;
	Rom* rom = arg->rom;
	char* name = sample->name;
	u32 sampRate = sample->sampleRate;
	char* FILE_WAV;
	char* FILE_VAD;
	char* FILE_BOK;
	char* FILE_LBK;
	char* FILE_CFG;
	
	if (sample->dublicate) return;
	if (name == NULL) printf_error("Sample ID [%D] is missing name", arg->i);
	if (sampRate == 0) printf_error("Sample [%s] is missing samplerate", name);
	
	book = SegmentedToVirtual(0x0, tbl->book);
	loop = SegmentedToVirtual(0x0, tbl->loop);
	
	Sys_MakeDir("%s%s/", arg->path, name);
	asprintf(&FILE_WAV, "%s%s/Sample.wav", arg->path, name);
	asprintf(&FILE_VAD, "%s%s/sample.vadpcm.bin", arg->path, name);
	asprintf(&FILE_BOK, "%s%s/sample.book.bin", arg->path, name);
	asprintf(&FILE_LBK, "%s%s/sample.loopbook.bin", arg->path, name);
	asprintf(&FILE_CFG, "%s%s/config.cfg", arg->path, name);
	
	Malloc(memF, sizeof(MemFile));
	Malloc(memC, sizeof(MemFile));
	*memF = MemFile_Initialize();
	*memC = MemFile_Initialize();
	MemFile_Alloc(memF, MbToBin(1.0));
	MemFile_Alloc(memC, MbToBin(1.0));
	
	rf.size = ReadBE(tbl->data) & 0x00FFFFFF;
	rf.data = SegmentedToVirtual(0x0, tbl->sampleAddr);
	Rom_Extract(memF, rf, FILE_VAD);
	
	rf.size = sizeof(s16) * 8 * ReadBE(book->order) * ReadBE(book->npredictors) + 8;
	rf.data = book;
	Rom_Extract(memF, rf, FILE_BOK);
	
	Rom_Config_Sample(rom, memC, (Sample*)tbl, name, FILE_CFG);
	
	if (loop->count) {
		rf.size = 0x20;
		rf.data = SegmentedToVirtual(0x0, tbl->loop + 0x10);
		Rom_Extract(memF, rf, FILE_LBK);
	}
	
	MemFile_Free(memF);
	
	if (gDumpAudio) {
		char cmd[2048];
		s8* instInfo;
		
		Tools_Command(
			cmd,
			z64audio,
			"--i %s "
			"--o %s "
			"--S "
			"--srate %d "
			"--tuning %f "
			"--config-override",
			FILE_VAD,
			FILE_WAV,
			sampRate,
			tbl->tuning
		);
		
		if (tbl->isPrim && (tbl->splitHi != 127 || tbl->splitLo != 0)) {
			catprintf(cmd, "--split-hi %d", tbl->splitHi + 21);
			if (tbl->splitLo)
				catprintf(cmd, "--split-lo %d", tbl->splitLo + 21);
		}
		
		SysExe(cmd);
		
		if (MemFile_LoadFile(memF, FILE_WAV))
			printf_warning_align("Sample not found", "%s", FILE_WAV);
		
		instInfo = MemMem(memF->data, memF->size, "inst", 4);
		
		if (instInfo) {
			Config_Print(memC, "\n ");
			Config_WriteComment(memC, "Instrument Info");
			Config_WriteInt(memC, "basenote", instInfo[8], NO_COMMENT);
			Config_WriteInt(memC, "finetune", instInfo[9], NO_COMMENT);
			MemFile_SaveFile_String(memC, FILE_CFG);
		} else {
			if (memF->size == 0)
				printf_warning_align("Audio", "Empty File [%s]", FILE_WAV);
		}
		
		MemFile_Free(memF);
	}
	
	MemFile_Free(memC);
	Free(memF);
	Free(memC);
	Free(FILE_WAV);
	Free(FILE_VAD);
	Free(FILE_BOK);
	Free(FILE_LBK);
	Free(FILE_CFG);
}

extern u32 gThreadNum;

void Audio_DumpSampleTable(Rom* rom, MemFile* dataFile, MemFile* config) {
	SampleInfo* smallest = sUnsortedSampleTbl;
	SampleInfo* largest = sUnsortedSampleTbl;
	SampleInfo** tbl;
	char buff[16];
	char* name;
	
	for (s32 i = 0; i < sDumpID; i++) {
		if (smallest->sampleAddr > sUnsortedSampleTbl[i].sampleAddr) {
			smallest = &sUnsortedSampleTbl[i];
		}
		if (largest->sampleAddr < sUnsortedSampleTbl[i].sampleAddr) {
			largest = &sUnsortedSampleTbl[i];
		}
	}
	sSortedSampleTbl[sSortID++] = smallest;
	
	while (1) {
		smallest = largest;
		
		for (s32 i = 0; i < sDumpID; i++) {
			if (sUnsortedSampleTbl[i].sampleAddr < smallest->sampleAddr) {
				if (sUnsortedSampleTbl[i].sampleAddr > sSortedSampleTbl[sSortID - 1]->sampleAddr) {
					smallest = &sUnsortedSampleTbl[i];
				}
			}
		}
		sSortedSampleTbl[sSortID++] = smallest;
		if (smallest->sampleAddr == largest->sampleAddr)
			break;
	}
	
	tbl = sSortedSampleTbl;
	
	s32 i = 0;
	SampleDumpArg* arg;
	Thread* thread;
	
	Calloc(arg, sizeof(SampleDumpArg) * gThreadNum);
	Calloc(thread, sizeof(Thread) * gThreadNum);
	
	if (gThreading)
		ThreadLock_Init();
	
	while (i < sSortID) {
		u32 target = Clamp(sSortID - i, 0, gThreadNum);
		
		for (s32 j = 0; j < target; j++) {
			arg[j].i = i + j;
			arg[j].rom = rom;
			arg[j].sample = &gSampleInfo[i + j];
			arg[j].tbl = tbl[i + j];
			arg[j].path = xFmt("rom/sound/sample/%s/", gVanilla);
			
			if (gThreading)
				ThreadLock_Create(&thread[j], SampleDump_Thread, &arg[j]);
			
			else {
				SampleDump_Thread(&arg[j]);
				printf_progress("Sample", i + j + 1, sSortID);
			}
		}
		
		if (gThreading) {
			for (s32 j = 0; j < target; j++) {
				ThreadLock_Join(&thread[j]);
			}
			printf_progress("Sample", i + target, sSortID);
		}
		
		i += gThreadNum;
	}
	
	if (gThreading)
		ThreadLock_Free();
	
	for (s32 j = 0; j < sBankNum; j++) {
		char* replacedName = NULL;
		printf_progress("Update SoundFont", j + 1, sBankNum);
		
		MemFile_Clear(config);
		MemFile_LoadFile_String(config, sBankFiles[j]);
		for (s32 i = 0; i < sSortID; i++) {
			name = gSampleInfo[i].dublicate == NULL ? gSampleInfo[i].name : gSampleInfo[i].dublicate->name;
			
			sprintf(buff, "0x%X", sSortedSampleTbl[i]->sampleAddr);
			if (StrRep(config->data, buff, xFmt("\"%s\"", name))) {
				replacedName = name;
			}
		}
		
		config->size = strlen(config->data);
		MemFile_SaveFile_String(config, sBankFiles[j]);
		
		// Rename SFX To their samples
		if (StrStr(sBankFiles[j], "-Sfx")) {
			char* tempName = xFmt("%s%d-%s.cfg", Path(sBankFiles[j]), Value_Int(Basename(sBankFiles[j])), replacedName);
			
			Sys_Rename(sBankFiles[j], tempName);
		}
		
		// Rename Inst to their primary sample
		if ((StrStr(sBankFiles[j], "-Inst") || StrStr(sBankFiles[j], "-Drum")) && (StrStr(config->data, "Inst_") || StrStr(config->data, "Perc_"))) {
			char instName[256] = { 0 };
			char* tempName;
			char* var;
			
			Config_GotoSection("prim");
			var = Config_GetStr(config, "sample");
			Log("%s", var);
			
			Config_GotoSection(NULL);
			
			strcpy(instName, var);
			StrRem(instName, strlen("Inst_"));
			StrRep(instName, "_Prim", "");
			StrRep(instName, "Soft", "");
			StrRep(instName, "Hard", "");
			StrRep(instName, "Mute", "");
			StrRep(instName, "Open", "");
			StrRep(instName, "_Hi", "Var");
			
			if (instName[0] == 0)
				printf_error("String maniplation failed for instrument");
			
			tempName = xFmt("%s%d-%s.cfg", Path(sBankFiles[j]), Value_Int(Basename(sBankFiles[j])), instName);
			
			Sys_Rename(sBankFiles[j], tempName);
		}
	}
	
	if (gDumpAudio)
		Audio_PatchWavFiles(dataFile, config);
}

// # # # # # # # # # # # # # # # # # # # #
// # BUILD                               #
// # # # # # # # # # # # # # # # # # # # #

static s32 Audio_LoadFile(MemFile* dataFile, char* file) {
	char* buf;
	char* smpl;
	
	smpl = FileSys_File(file);
	
	if (smpl && Sys_Stat(smpl)) {
		if (MemFile_LoadFile(dataFile, smpl))
			return 1;
	} else {
		buf = xStrDup(file);
		StrRep(buf, "sample", "*");
		smpl = FileSys_FindFile(file);
		
		if (smpl) {
			if (MemFile_LoadFile(dataFile, smpl))
				return 1;
		} else
			return 1;
	}
	
	return 0;
}

void Audio_UpdateSegments(Rom* rom) {
	#define RAM_CODE ((u32)0x8001CE60)
	#define INST_ADDR(x, y) SegmentedToVirtual(SEG_CODE, ((x) - RAM_CODE)), SegmentedToVirtual(SEG_CODE, ((y) - RAM_CODE))
	#define RAM_ADDR rom->code.seekPoint + 0x8001CE60
	
	MemFile_Params(&rom->code, MEM_ALIGN, 16, MEM_END);
	MemFile_Seek(&rom->code, 0xD1C00 /* 0xB65C00 - RELOC_CODE */);
	
	Mips64_SplitLoad(INST_ADDR(0x800E330C, 0x800E3310), MIPS_REG_A1, rom->offset.segment.seqRom);
	Mips64_SplitLoad(INST_ADDR(0x800E3320, 0x800E3324), MIPS_REG_A1, rom->offset.segment.fontRom);
	Mips64_SplitLoad(INST_ADDR(0x800E3334, 0x800E3338), MIPS_REG_A1, rom->offset.segment.smplRom);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32C4, 0x800E32D4), MIPS_REG_T1, RAM_ADDR);
	MemFile_Append(&rom->code, &rom->mem.seqTbl);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32C8, 0x800E32D8), MIPS_REG_T2, RAM_ADDR);
	MemFile_Append(&rom->code, &rom->mem.fontTbl);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32CC, 0x800E32DC), MIPS_REG_T3, RAM_ADDR);
	MemFile_Append(&rom->code, &rom->mem.sampleTbl);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32D0, 0x800E32E0), MIPS_REG_T6, RAM_ADDR);
	MemFile_Append(&rom->code, &rom->mem.seqFontTbl);
	
	// Move Audio Heap
	Mips64_SplitLoad(INST_ADDR(0x800E3204, 0x800E3208), MIPS_REG_T9, 0x806C0000);
	
	MemFile_Params(&rom->code, MEM_ALIGN, 0, MEM_END);
	
	if (rom->code.seekPoint - 0xD1C00 > 0x318C - 0x10) {
		printf_warning("AudioDebug_Draw overwriting exceeded. Might cause trouble...");
	}
	
#undef INST_ADDR
#undef RAM_ADDR
#undef FUNC_RAM
}

void Audio_BuildSampleTable(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList itemList = ItemList_Initialize();
	MemFile sample = MemFile_Initialize();
	AudioEntryHead head = { 0 };
	AudioEntry entry = { 0 };
	
	MemFile_Alloc(&sample, MbToBin(0.25));
	MemFile_Reset(dataFile);
	Rom_ItemList(&itemList, "rom/sound/sample/", SORT_NO, LIST_FOLDERS);
	MemFile_Params(dataFile, MEM_ALIGN, 16, MEM_END);
	
	for (s32 i = 0; i < itemList.num; i++) {
		printf_progress("Sample", i + 1, itemList.num);
		MemFile_Reset(config);
		MemFile_Reset(&sample);
		
		FileSys_Path(itemList.item[i]); {
			char* file = FileSys_File("sample.vadpcm.bin");
			char* cfg = FileSys_File("config.cfg");
			
			if (cfg == NULL)
				printf_error("Could not locate sample in [%s]", itemList.item[i]);
			if (file == NULL)
				printf_error("Could not locate sample in [%s]", itemList.item[i]);
			
			Log("smpl: %s", file);
			Log("cfg: %s", cfg);
			
			if (MemFile_LoadFile(&sample, file))
				printf_error_align("Failed to load file", "%s", file);
			
			if (MemFile_LoadFile_String(config, cfg))
				printf_error_align("Failed to load file", "%s", cfg);
			
			if (Config_Variable(config->str, "tuning"))
				sSampleTbl[sSampleTblNum].tuninOverride = Config_GetFloat(config, "tuning");
			
			sSampleTbl[sSampleTblNum].segment = dataFile->seekPoint;
			if (dataFile->seekPoint & 0xF)
				printf_error("Error: Samplebank alignment failed!");
			
			sSampleTbl[sSampleTblNum].size = sample.size;
			strcpy(sSampleTbl[sSampleTblNum].dir, FileSys_File(""));
			strcpy(sSampleTbl[sSampleTblNum].name, PathSlot(itemList.item[i], -1));
			StrRep(sSampleTbl[sSampleTblNum].name, "/", "\0");
			sSampleTblNum++;
			MemFile_Append(dataFile, &sample);
		}
	}
	
	head.numEntries = 1;
	SwapBE(head.numEntries);
	entry.romAddr = 0;
	entry.size = ReadBE(dataFile->size);
	entry.medium = 2;
	entry.seqPlayer = 4;
	MemFile_Write(&rom->mem.sampleTbl, &head, 16);
	MemFile_Write(&rom->mem.sampleTbl, &entry, 16);
	
	if (gAudioOnly)
		MemFile_SaveFile(dataFile, "samples.bin");
	
	else
		rom->offset.segment.smplRom = Dma_WriteEntry(rom, DMA_FIND_FREE, dataFile, false);
	MemFile_Params(dataFile, MEM_ALIGN, 0, MEM_END);
	
	MemFile_Free(&sample);
	ItemList_Free(&itemList);
}

static void SoundFont_Error_NotFound(const char* sampleName) {
	MemFile mem = MemFile_Initialize();
	
	Log("Dumping [audio_log]");
	MemFile_Alloc(&mem, 0x90000);
	
	for (s32 i = 0; i < sSampleTblNum; i++)
		MemFile_Printf(&mem, "%s\n", sSampleTbl[i].name);
	
	MemFile_SaveFile_String(&mem, "audio_log");
	MemFile_Free(&mem);
	
	Log("Check the latest loaded instrument in the logs above and compare it to [audio_log] to fix the name mismatch!");
	Log("You can also provide screenshot of this and [audio_log] to the developer.");
	printf_error("Could not find sample [%s]!", sampleName);
}

static s32 SoundFont_SmplID(const char* smpl) {
	s32 sampleID = 0;
	
	for (;; sampleID++) {
		if (sampleID == sSampleTblNum)
			SoundFont_Error_NotFound(smpl);
		if (!strcmp(sSampleTbl[sampleID].name, smpl))
			break;
	}
	
	return sampleID;
}

static void SoundFont_Instrument_Validate(MemFile* mem, const char* file, Instrument* inst, char** smpl) {
	u32 smplNum = 0;
	
	for (s32 i = 0; i < 3; i++)
		if (smpl[i] != NULL)
			smplNum++;
	
	if (inst->splitHi < inst->splitLo) {
		printf_error("Instrument [%s] [split_hi] is lower than [split_lo]");
	}
	
	if (smpl[1] && smpl[2] && smpl[0] && inst->splitHi == 127 && inst->splitLo == 0) {
		printf_warning("\aInstrument [%s] uses [prim_sample], [hi_sample] and [low_sample] but splits are set to [0, 127]", file);
		printf_warning("[hi_sample] and [low_sample] sounds are not going to play with these splits!");
	}
	
	if (smpl[1] && smpl[2] && inst->splitHi == 127) {
		printf_warning("\aInstrument [%s] uses [prim_sample] and [hi_sample] but split is set to highest value [127]", file);
		printf_warning("[hi_sample] sounds is not going to play with this splitting!");
	}
	
	if ((!smpl[2] && inst->splitHi < 127) || inst->splitHi > 127) {
		printf_warning("split_hi fixed for [%s]", file);
		Config_ReplaceVariable(mem, "split_hi", Music_NoteWord(127 + 21));
		
		mem->size = strlen(mem->str);
		MemFile_SaveFile_String(mem, file);
		inst->splitHi = 127;
	}
}

static s32 SoundFont_Instrument_AssignNames(MemFile* mem, char** smplNam, MemFile* memBank) {
	u32 smplNum = 0;
	
	for (s32 soundID = 0; soundID < 3; soundID++) {
		Config_GotoSection(sInstSectionNames[soundID]);
		smplNam[soundID] = Config_GetStr(mem, "sample");
		if (smplNam[soundID] == NULL) {
			continue;
		} else if (StrMtch(smplNam[soundID], "NULL")) {
			smplNam[soundID] = NULL;
			continue;
		}
		smplNum++;
	}
	
	Config_GotoSection(NULL);
	
	if (smplNum == 0) {
		u32 null = 0xFFFF; // Empty Instrument Entry
		MemFile_Write(memBank, &null, sizeof(u32));
		
		return 1;
	}
	
	return 0;
}

static void SoundFont_Instrument_AssignIndexes(MemFile* mem, char** smplNam, s32* smplID, Instrument* inst) {
	for (s32 soundID = 0; soundID < 3; soundID++) {
		if (smplNam[soundID] == NULL)
			continue;
		
		for (s32 sampleID = 0;; sampleID++) {
			if (sampleID >= sSampleTblNum)
				SoundFont_Error_NotFound(smplNam[soundID]);
			
			if (!strcmp(sSampleTbl[sampleID].name, smplNam[soundID])) {
				inst->sound[soundID].tuning = sSampleTbl[sampleID].tuninOverride;
				
				if (inst->sound[soundID].tuning == 0) {
					Config_GotoSection(sInstSectionNames[soundID]);
					inst->sound[soundID].tuning = Config_GetFloat(mem, "tuning");
					
					Config_GotoSection(NULL);
				}
				
				SwapBE(inst->sound[soundID].swap32);
				smplID[soundID] = sampleID;
				break;
			}
		}
	}
}

static void SoundFont_Read_Instrument(MemFile* mem, Instrument* inst) {
	inst->loaded = 0;
	inst->splitLo = Music_NoteIndex(Config_GetStr(mem, "split_lo")) - 21;
	inst->splitHi = Music_NoteIndex(Config_GetStr(mem, "split_hi")) - 21;
	inst->splitLo = ClampMin(inst->splitLo, 0);
	inst->splitHi = ClampMin(inst->splitHi, 0);
	inst->release = Audio_GetReleaseID(Config_GetFloat(mem, "release_rate"));
}

static void SoundFont_Read_Adsr(MemFile* mem, Adsr* adsr) {
	ItemList listRate = ItemList_Initialize();
	ItemList listLevl = ItemList_Initialize();
	s32 i = 0;
	
	Config_GetArray(mem, "env_rate", &listRate);
	Config_GetArray(mem, "env_level", &listLevl);
	
	if (listRate.num != listLevl.num)
		printf_error("env_rate & env_level array num mismatch in [%s]", mem->info.name);
	
	if (listRate.num > 3)
		printf_warning("z64rom supports only 3 envelopes currently [%s]", mem->info.name);
	
	for (; i < ClampMax(listRate.num, 3); i++) {
		adsr[i].rate = Clamp(ceilf(Value_Float(listRate.item[i]) * __INT16_MAX__), 0, __INT16_MAX__);
		adsr[i].level = Clamp(ceilf(Value_Float(listLevl.item[i]) * __INT16_MAX__), 0, __INT16_MAX__);
		SwapBE(adsr[i].rate);
		SwapBE(adsr[i].level);
	}
	
	adsr[i].rate = -1;
	adsr[i].level = 0;
	SwapBE(adsr[i].rate);
	SwapBE(adsr[i].level);
	
	ItemList_Free(&listRate);
	ItemList_Free(&listLevl);
}

static void SoundFont_Write_Adsr(MemFile* mem, Adsr* adsr, void32* setPtr) {
	if (!MemMemAlign(16, mem->data, mem->size, adsr, 16)) {
		setPtr[0] = mem->seekPoint;
		MemFile_Write(mem, adsr, 16);
		MemFile_Align(mem, 16);
	} else {
		void* ptr = MemMemAlign(16, mem->data, mem->size, adsr, 16);
		setPtr[0] = (uptr)ptr - (uptr)mem->data;
	}
}

static void SoundFont_Write_Sample(MemFile* dataFile, s32 sampleID, void32* setPtr, MemFile* memSample, MemFile* memBook, MemFile* memLoopBook, u32* sampleNum) {
	char* restoreDir = xStrDup(FileSys_File(""));
	Sample smpl = { 0 };
	u32 loop[4 + 8];
	u32 loopSize = 4 * 4;
	
	if (sampleID < 0)
		return;
	
	FileSys_Path(sSampleTbl[sampleID].dir);
	MemFile_Reset(dataFile);
	MemFile_LoadFile(dataFile, FileSys_File("config.cfg"));
	
	smpl.sampleAddr = ReadBE(sSampleTbl[sampleID].segment);
	smpl.data = sSampleTbl[sampleID].size;
	smpl.data |= Config_GetInt(dataFile, "codec") << (32 - 4);
	smpl.data |= Config_GetInt(dataFile, "medium") << (32 - 6);
	smpl.data |= Config_GetInt(dataFile, "bitA") << (32 - 7);
	smpl.data |= Config_GetInt(dataFile, "bitB") << (32 - 8);
	SwapBE(smpl.data);
	
	loop[0] = Config_GetInt(dataFile, "loop_start");
	loop[1] = Config_GetInt(dataFile, "loop_end");
	loop[2] = Config_GetInt(dataFile, "loop_count");
	loop[3] = Config_GetInt(dataFile, "tail_end");
	SwapBE(loop[0]);
	SwapBE(loop[1]);
	SwapBE(loop[2]);
	SwapBE(loop[3]);
	
	if (loop[2]) {
		MemFile_Reset(dataFile);
		if (Audio_LoadFile(dataFile, "sample.loopbook.bin")) {
			printf_warning("" PRNT_REDD "[%s]" PRNT_RSET " has looppoints but could not find " PRNT_REDD "loopbook", sSampleTbl[sampleID].name);
			loop[0] = 0;
			loop[2] = 0;
			loopSize = 0;
		} else {
			for (s32 i = 0; i < 8; i++) {
				loop[4 + i] = dataFile->cast.u32[i];
			}
			
			loopSize = 4 * (4 + 8);
		}
	}
	
	if (!MemMemAlign(16, memLoopBook->data, memLoopBook->size, loop, loopSize)) {
		smpl.loop = memLoopBook->seekPoint;
		MemFile_Write(memLoopBook, loop, loopSize);
		MemFile_Align(memLoopBook, 16);
	} else {
		void* ptr = MemMemAlign(16, memLoopBook->data, memLoopBook->size, loop, loopSize);
		smpl.loop = (uptr)ptr - (uptr)memLoopBook->data;
	}
	
	MemFile_Reset(dataFile);
	Audio_LoadFile(dataFile, "sample.book.bin");
	if (!MemMemAlign(16, memBook->data, memBook->size, dataFile->data, dataFile->size)) {
		smpl.book = memBook->seekPoint;
		MemFile_Append(memBook, dataFile);
		MemFile_Align(memBook, 16);
	} else {
		void* ptr = MemMemAlign(16, memBook->data, memBook->size, dataFile->data, dataFile->size);
		smpl.book = (uptr)ptr - (uptr)memBook->data;
	}
	
	if (!MemMemAlign(16, memSample->data, memSample->size, &smpl, sizeof(struct Sample))) {
		setPtr[0] = memSample->seekPoint;
		MemFile_Write(memSample, &smpl, sizeof(struct Sample));
		sampleNum[0]++;
	} else {
		void* ptr = MemMemAlign(16, memSample->data, memSample->size, &smpl, sizeof(struct Sample));
		setPtr[0] = (uptr)ptr - (uptr)memSample->data;
	}
	FileSys_Path(restoreDir);
}

typedef struct {
	u32 i;
	u32 ti;
	ItemList*   itemList;
	AudioEntry* sfEntry;
	MemFile memBank;
	u32 numInst;
	u32 numDrum;
	u32 numSfx;
} FontThread;

void Audio_ThreadBuildFont(FontThread* ft) {
	MemFile memBook, memLoopBook, memInst, memEnv, memSample, memSfx, memDrum;
	MemFile* dataFile;
	MemFile* config;
	MemFile* memBank = &ft->memBank;
	u32 i = ft->i;
	ItemList* itemList = ft->itemList;
	
	Calloc(dataFile, sizeof(MemFile));
	Calloc(config, sizeof(MemFile));
	
	MemFile_Alloc(dataFile, MbToBin(0.25));
	MemFile_Alloc(config, MbToBin(0.25));
	
	MemFile_Alloc(&memBook, MbToBin(0.25));
	MemFile_Alloc(&memLoopBook, MbToBin(0.25));
	MemFile_Alloc(&memInst, MbToBin(0.25));
	MemFile_Alloc(&memEnv, MbToBin(0.25));
	MemFile_Alloc(&memSample, MbToBin(0.25));
	MemFile_Alloc(&memSfx, MbToBin(0.25));
	MemFile_Alloc(&memDrum, MbToBin(0.25));
	
	ItemList listInst = ItemList_Initialize();
	ItemList listSfx = ItemList_Initialize();
	ItemList listDrum = ItemList_Initialize();
	u32 smplNum = 0;
	char* dirs[] = {
		"instrument/",
		"sfx/",
		"drum/",
	};
	ItemList* lists[] = {
		&listInst,
		&listSfx,
		&listDrum
	};
	
	MemFile_Reset(dataFile);
	MemFile_Reset(config);
	SetSegment(0x1 + ft->ti, memBank->data);
	
	if (itemList->item[i] == NULL) {
		ft->sfEntry->romAddr = 0;
		ft->sfEntry->size = 0;
		ft->sfEntry->medium = 0;
		ft->sfEntry->seqPlayer = 0;
		ft->sfEntry->audioTable1 = -1;
		ft->sfEntry->audioTable2 = -1;
		ft->sfEntry->numInst = 0;
		ft->sfEntry->numDrum = 0;
		ft->sfEntry->numSfx = 0;
		
		return;
	}
	
	FileSys_Path("%s", itemList->item[i]);
	
	for (s32 y = 0; y < 3; y++) {
		char* path = FileSys_File(dirs[y]);
		
		if (Sys_Stat(path)) {
			ItemList_List(lists[y], path, -1, LIST_FILES | LIST_NO_DOT | LIST_RELATIVE);
			ItemList_NumericalSort(lists[y]);
		}
	}
	
	// DDDDDDDD SSSSSSSS Drum Segment | Sfx Segment
	MemFile_Write(memBank, "\0\0\0\0\0\0\0\0", 8);
	
	for (s32 j = 0; j < listInst.num; j++) {
		Instrument instrument = { 0 };
		Adsr adsr[4] = { 0 };
		char* instFile = FileSys_File("instrument/%s", listInst.item[j]);
		char* sampleName[3];
		s32 sampleIndex[3] = { -1, -1, -1 };
		
		// List does not have entry for j index
		if (listInst.item[j] == NULL) {
			u32 null = 0xFFFF; // Empty Instrument Entry
			MemFile_Write(memBank, &null, sizeof(u32));
			continue;
		}
		
		MemFile_Reset(config);
		Log("Load File: %s", instFile);
		MemFile_LoadFile_String(config, instFile);
		
		if (SoundFont_Instrument_AssignNames(config, sampleName, memBank)) continue;
		SoundFont_Instrument_AssignIndexes(config, sampleName, sampleIndex, &instrument);
		SoundFont_Read_Instrument(config, &instrument);
		SoundFont_Read_Adsr(config, adsr);
		SoundFont_Instrument_Validate(config, instFile, &instrument, sampleName);
		SoundFont_Write_Adsr(&memEnv, adsr, &instrument.envelope);
		
		for (s32 soundID = 0; soundID < 3; soundID++)
			SoundFont_Write_Sample(dataFile, sampleIndex[soundID], &instrument.sound[soundID].sample, &memSample, &memBook, &memLoopBook, &smplNum);
		
		SwapBE(memInst.seekPoint);
		MemFile_Write(memBank, &memInst.seekPoint, sizeof(u32));
		SwapBE(memInst.seekPoint);
		
		MemFile_Write(&memInst, &instrument, sizeof(struct Instrument));
	}
	
	for (s32 j = 0; j < listSfx.num; j++) {
		Sound sfx = { 0 };
		char* prim;
		s32 idx;
		
		if (listSfx.item[j] == NULL) {
			MemFile_Write(&memSfx, &sfx, sizeof(struct Sound));
			continue;
		}
		
		MemFile_Reset(config);
		MemFile_LoadFile_String(config, FileSys_File("sfx/%s", listSfx.item[j]));
		
		Config_GotoSection("prim");
		prim = Config_GetStr(config, "sample");
		
		if (!strcmp(prim, "NULL")) {
			MemFile_Write(&memSfx, &sfx, sizeof(struct Sound));
			continue;
		}
		
		sfx.tuning = Config_GetFloat(config, "tuning");
		idx = SoundFont_SmplID(prim);
		
		Config_GotoSection(NULL);
		
		if (sSampleTbl[idx].tuninOverride > 0)
			sfx.tuning = sSampleTbl[idx].tuninOverride;
		SwapBE(sfx.swap32);
		SoundFont_Write_Sample(dataFile, idx, &sfx.sample, &memSample, &memBook, &memLoopBook, &smplNum);
		
		MemFile_Write(&memSfx, &sfx, sizeof(struct Sound));
	}
	
	for (s32 j = 0; j < listDrum.num; j++) {
		Drum drum = { 0 };
		Adsr adsr[4] = { 0 };
		char* prim;
		
		if (j == 0) {
			memDrum.seekPoint += 4 * listDrum.num;
			memset(memDrum.data, 0, memDrum.seekPoint);
			MemFile_Align(&memDrum, 16);
		}
		
		if (listDrum.item[j] == NULL) {
			memDrum.cast.u32[j] = 0;
			
			continue;
		}
		
		MemFile_Reset(config);
		MemFile_LoadFile_String(config, FileSys_File("drum/%s", listDrum.item[j]));
		
		Config_GotoSection("prim");
		prim = Config_GetStr(config, "sample");
		
		if (!memcmp(prim, "NULL", 4)) {
			memDrum.cast.u32[j] = 0;
			
			continue;
		} else {
			memDrum.cast.u32[j] = memDrum.seekPoint;
		}
		
		drum.sound.tuning = Config_GetFloat(config, "tuning");
		
		Config_GotoSection(NULL);
		
		drum.loaded = 0;
		drum.pan = Config_GetInt(config, "pan");
		drum.release = Audio_GetReleaseID(Config_GetFloat(config, "release_rate"));
		SwapBE(drum.sound.swap32);
		
		SoundFont_Read_Adsr(config, adsr);
		SoundFont_Write_Adsr(&memEnv, adsr, &drum.envelope);
		SoundFont_Write_Sample(dataFile, SoundFont_SmplID(prim), &drum.sound.sample, &memSample, &memBook, &memLoopBook, &smplNum);
		
		MemFile_Write(&memDrum, &drum, sizeof(struct Drum));
		MemFile_Align(&memDrum, 16);
	}
	
	MemFile_Align(memBank, 16);
	
	for (s32 j = 0; j < listInst.num; j++) {
		if (memBank->cast.u32[2 + j] == 0xFFFF) {
			continue;
		}
		SwapBE(memBank->cast.u32[2 + j]);
		memBank->cast.u32[2 + j] += memBank->seekPoint;
		SwapBE(memBank->cast.u32[2 + j]);
	}
	MemFile_Append(memBank, &memInst); MemFile_Align(memBank, 16);
	
	for (s32 j = 0; j < listInst.num; j++) {
		if (memBank->cast.u32[2 + j] == 0xFFFF) {
			continue;
		}
		Instrument* inst = SegmentedToVirtual(0x1 + ft->ti, ReadBE(memBank->cast.u32[2 + j]));
		
		inst->envelope += memBank->seekPoint;
		SwapBE(inst->envelope);
	}
	
	for (s32 j = 0; j < listDrum.num; j++) {
		if (memDrum.cast.u32[j] == 0)
			continue;
		SetSegment(0x80 + ft->ti, memDrum.data);
		Drum* drum = SegmentedToVirtual(0x80 + ft->ti, memDrum.cast.u32[j]);
		
		drum->envelope += memBank->seekPoint;
		SwapBE(drum->envelope);
	}
	
	MemFile_Append(memBank, &memEnv); MemFile_Align(memBank, 16);
	
	for (s32 l = 0; l < smplNum; l++) {
		Sample* smpl = memSample.data;
		
		smpl[l].book += memBank->seekPoint;
		SwapBE(smpl[l].book);
	}
	
	MemFile_Append(memBank, &memBook); MemFile_Align(memBank, 16);
	
	for (s32 l = 0; l < smplNum; l++) {
		Sample* smpl = memSample.data;
		
		smpl[l].loop += memBank->seekPoint;
		SwapBE(smpl[l].loop);
	}
	
	MemFile_Append(memBank, &memLoopBook); MemFile_Align(memBank, 16);
	
	for (s32 j = 0; j < listInst.num; j++) {
		if (memBank->cast.u32[2 + j] == 0xFFFF) {
			memBank->cast.u32[2 + j] = 0;
			continue;
		}
		Instrument* inst = SegmentedToVirtual(0x1 + ft->ti, ReadBE(memBank->cast.u32[2 + j]));
		
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
	
	for (s32 j = 0; j < listSfx.num; j++) {
		Sound* sound = memSfx.data;
		
		sound[j].sample += memBank->seekPoint;
		SwapBE(sound[j].sample);
	}
	
	for (s32 j = 0; j < listDrum.num; j++) {
		if (memDrum.cast.u32[j] == 0)
			continue;
		SetSegment(0x80 + ft->ti, memDrum.data);
		Drum* drum = SegmentedToVirtual(0x80 + ft->ti, (memDrum.cast.u32[j]));
		
		drum->sound.sample += memBank->seekPoint;
		SwapBE(drum->sound.sample);
		SetSegment(0x80 + ft->ti, NULL);
	}
	
	MemFile_Append(memBank, &memSample); MemFile_Align(memBank, 16);
	
	if (listSfx.num) {
		memBank->cast.u32[1] = ReadBE(memBank->seekPoint);
		MemFile_Append(memBank, &memSfx); MemFile_Align(memBank, 16);
	}
	
	if (listDrum.num) {
		for (s32 j = 0; j < listDrum.num; j++) {
			if (memDrum.cast.u32[j] == 0)
				continue;
			memDrum.cast.u32[j] += memBank->seekPoint;
			SwapBE(memDrum.cast.u32[j]);
		}
		memBank->cast.u32[0] = ReadBE(memBank->seekPoint);
		MemFile_Append(memBank, &memDrum); MemFile_Align(memBank, 16);
	}
	
	ft->numInst = listInst.num;
	ft->numDrum = listDrum.num;
	ft->numSfx = listSfx.num;
	
	ItemList_Free(&listInst);
	ItemList_Free(&listSfx);
	ItemList_Free(&listDrum);
	MemFile_Free(&memBook);
	MemFile_Free(&memLoopBook);
	MemFile_Free(&memInst);
	MemFile_Free(&memEnv);
	MemFile_Free(&memSample);
	MemFile_Free(&memSfx);
	MemFile_Free(&memDrum);
	
	MemFile_Free(dataFile);
	MemFile_Free(config);
	FileSys_Free();
	Free(dataFile);
	Free(config);
}

void Audio_BuildSoundFont(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList itemList = ItemList_Initialize();
	MemFile soundFontMem = MemFile_Initialize();
	AudioEntryHead sfHead = { 0 };
	AudioEntry* sfEntry;
	s32 i = 0;
	u32 override = 1;
	
	MemFile_Alloc(&soundFontMem, MbToBin(2.00));
	Rom_ItemList(&itemList, "rom/sound/soundfont/", SORT_NUMERICAL, LIST_FOLDERS);
	
	sfHead.numEntries = itemList.num;
	SwapBE(sfHead.numEntries);
	MemFile_Write(&rom->mem.fontTbl, &sfHead, 16);
	
	Calloc(sfEntry, sizeof(AudioEntry) * itemList.num);
	
	ThreadLock_Init();
	
	if (gThreading) {
		override = 82;
	}
	
	while (i < itemList.num) {
		FontThread thread[82];
		Thread thd[82];
		u32 target = Clamp(itemList.num - i, 0, override);
		
		for (s32 j = 0; j < target; j++) {
			MemFile_Alloc(&thread[j].memBank, MbToBin(0.25));
			thread[j].itemList = &itemList;
			thread[j].sfEntry = &sfEntry[i + j];
			thread[j].i = i + j;
			thread[j].ti = j;
			
			ThreadLock_Create(&thd[j], Audio_ThreadBuildFont, &thread[j]);
		}
		
		for (s32 j = 0; j < target; j++) {
			ThreadLock_Join(&thd[j]);
			u32 romAddr = soundFontMem.seekPoint;
			u32 med = 0;
			u32 seq = 0;
			char* confMed;
			char* confSeq;
			MemFile_Append(&soundFontMem, &thread[j].memBank);
			MemFile_Align(&soundFontMem, 16);
			
			MemFile_Reset(config);
			MemFile_LoadFile_String(config, xFmt("%sconfig.cfg", itemList.item[i + j]));
			confMed = Config_GetStr(config, "medium_type");
			confSeq = Config_GetStr(config, "sequence_player");
			
			for (;;) {
				if (med >= ArrayCount(sMediumType))
					printf_error("medium_type not recognized for SoundFont [0x%02X]", i);
				if (strcasecmp(sMediumType[med], confMed) == 0)
					break;
				med++;
			}
			for (;;) {
				if (seq >= ArrayCount(sSeqPlayerType))
					printf_error("sequence_player not recognized for SoundFont [0x%02X]", i);
				if (strcasecmp(sSeqPlayerType[seq], confSeq) == 0)
					break;
				seq++;
			}
			
			sfEntry[i + j].romAddr = romAddr;
			sfEntry[i + j].size = thread[j].memBank.size;
			sfEntry[i + j].medium = med;
			sfEntry[i + j].seqPlayer = seq;
			sfEntry[i + j].audioTable1 = 0;
			sfEntry[i + j].audioTable2 = -1;
			sfEntry[i + j].numInst = thread[j].numInst;
			sfEntry[i + j].numDrum = thread[j].numDrum;
			sfEntry[i + j].numSfx = thread[j].numSfx;
			SwapBE(sfEntry[i + j].romAddr);
			SwapBE(sfEntry[i + j].size);
			SwapBE(sfEntry[i + j].numSfx);
			MemFile_Write(&rom->mem.fontTbl, &sfEntry[i + j], 16);
			MemFile_Free(&thread[j].memBank);
			
			printf_progress("SoundFont", i + j + 1, itemList.num);
		}
		
		i += override;
	}
	
	ThreadLock_Free();
	
	if (gAudioOnly)
		MemFile_SaveFile(&soundFontMem, "soundfonts.bin");
	
	else
		rom->offset.segment.fontRom = Dma_WriteEntry(rom, DMA_FIND_FREE, &soundFontMem, false);
	
	MemFile_Free(&soundFontMem);
	ItemList_Free(&itemList);
	Free(sfEntry);
}

void Audio_BuildSequence(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList itemList = ItemList_Initialize();
	MemFile memIndexTable = MemFile_Initialize();
	MemFile memLookUpTable = MemFile_Initialize();
	MemFile sequenceMem = MemFile_Initialize();
	AudioEntryHead sqHead = { 0 };
	AudioEntry sqEntry = { 0 };
	u8* seqFlag = SegmentedToVirtual(SEG_CODE, 0xBA77F8 - RELOC_CODE);
	
	MemFile_Alloc(&memIndexTable, 0x800);
	MemFile_Alloc(&memLookUpTable, 0x800);
	MemFile_Alloc(&sequenceMem, MbToBin(1.0));
	Rom_ItemList(&itemList, "rom/sound/sequence/", SORT_NUMERICAL, LIST_FOLDERS);
	
	sqHead.numEntries = itemList.num;
	SwapBE(sqHead.numEntries);
	MemFile_Write(&rom->mem.seqTbl, &sqHead, 16);
	
	for (s32 i = 0; i < itemList.num; i++) {
		printf_progress("Sequences", i + 1, itemList.num);
		u32 addr;
		u8 fontNum;
		
		// Skip "hardcoded" entries
		if (i == 0x7F || i == 0x80) {
			sqEntry = (AudioEntry) { 0 };
			MemFile_Write(&memLookUpTable, "\xFF\xFF", 2);
			MemFile_Write(&rom->mem.seqTbl, &sqEntry, 16);
			continue;
		}
		
		if (itemList.item[i] == NULL) {
			sqEntry = (AudioEntry) { 0 };
			MemFile_Write(&memLookUpTable, "\xFF\xFF", 2);
			MemFile_Write(&rom->mem.seqTbl, &sqEntry, 16);
			continue;
		}
		
		FileSys_Path(itemList.item[i]); {
			u32 med = 0;
			u32 seq = 0;
			char* confMed;
			char* confSeq;
			char* fseq;
			ItemList flagList = ItemList_Initialize();
			ItemList bankList = ItemList_Initialize();
			
			MemFile_Reset(dataFile);
			MemFile_Reset(config);
			MemFile_LoadFile_String(config, FileSys_File("config.cfg"));
			confMed = Config_GetStr(config, "medium_type");
			confSeq = Config_GetStr(config, "sequence_player");
			Config_GetArray(config, "sequence_flags", &flagList);
			
			if (Config_GetErrorState())
				printf_error("File: [%s]", config->info.name);
			
			for (;; med++) {
				if (med >= ArrayCount(sMediumType))
					printf_error("medium_type not recognized for Sequence [0x%02X]", i);
				if (!strcmp(sMediumType[med], confMed))
					break;
			}
			for (;; seq++) {
				if (seq >= ArrayCount(sSeqPlayerType))
					printf_error("sequence_player not recognized for Sequence [0x%02X]", i);
				if (!strcmp(sSeqPlayerType[seq], confSeq))
					break;
			}
			
			fseq = FileSys_FindFile(".aseq");
			
			if (!gAudioOnly) {
				seqFlag[i] = 0;
				forlist(k, flagList) {
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
				MemFile_LoadFile(dataFile, fseq);
				addr = sequenceMem.seekPoint;
				MemFile_Append(&sequenceMem, dataFile);
				MemFile_Align(&sequenceMem, 16);
				sqEntry.romAddr = addr;
				sqEntry.size = dataFile->size;
			} else {
				sqEntry.romAddr = Config_GetInt(config, "seq_pointer");
				sqEntry.size = 0;
			}
			
			SwapBE(sqEntry.romAddr);
			SwapBE(sqEntry.size);
			MemFile_Write(&rom->mem.seqTbl, &sqEntry, 16);
			
			u16 offset = memIndexTable.seekPoint;
			MemFile_Write(&memLookUpTable, &offset, 2);
			
			Config_GetArray(config, "bank_id", &bankList);
			fontNum = bankList.num;
			MemFile_Write(&memIndexTable, &fontNum, 1);
			for (s32 j = 0; j < fontNum; j++) {
				u8 bankId = Value_Hex(bankList.item[j]);
				MemFile_Write(&memIndexTable, &bankId, 1);
			}
			
			ItemList_Free(&flagList);
			ItemList_Free(&bankList);
		}
	}
	
	u16 add = memLookUpTable.seekPoint;
	
	for (s32 i = 0; i < itemList.num; i++) {
		if (memLookUpTable.cast.u16[i] != 0xFFFF)
			memLookUpTable.cast.u16[i] += add;
		SwapBE(memLookUpTable.cast.u16[i]);
	}
	MemFile_Append(&memLookUpTable, &memIndexTable);
	MemFile_Append(&rom->mem.seqFontTbl, &memLookUpTable);
	
	if (gAudioOnly)
		MemFile_SaveFile(&sequenceMem, "sequences.bin");
	
	else
		rom->offset.segment.seqRom = Dma_WriteEntry(rom, DMA_FIND_FREE, &sequenceMem, false);
	
	MemFile_Free(&memIndexTable);
	MemFile_Free(&memLookUpTable);
	MemFile_Free(&sequenceMem);
	ItemList_Free(&itemList);
}

// # # # # # # # # # # # # # # # # # # # #
// # EXTRA                               #
// # # # # # # # # # # # # # # # # # # # #

void Audio_DeleteUnreferencedSamples(void) {
	ItemList sampleList = ItemList_Initialize();
	ItemList bankList = ItemList_Initialize();
	MemFile mem = MemFile_Initialize();
	const char* bankPath[] = {
		"instrument/",
		"drum/",
		"sfx/",
	};
	
	MemFile_Alloc(&mem, 0x1024);
	Rom_ItemList(&sampleList, "rom/sound/sample/", SORT_NO, LIST_FOLDERS);
	Rom_ItemList(&bankList, "rom/sound/soundfont/", SORT_NUMERICAL, LIST_FOLDERS);
	
	forlist(i, bankList) {
		if (bankList.item[i] == NULL)
			continue;
		
		FileSys_Path("%s", bankList.item[i]);
		
		foreach(j, bankPath) {
			if (!Sys_Stat(FileSys_File(bankPath[j])))
				continue;
			ItemList list = ItemList_Initialize();
			
			ItemList_List(&list, FileSys_File(bankPath[j]), 0, LIST_FILES);
			
			forlist(k, list) {
				if (!StrEndCase(list.item[k], ".cfg"))
					continue;
				
				MemFile_LoadFile_String(&mem, list.item[k]);
				
				forlist(l, sampleList) {
					char* word;
					
					if (sampleList.item[l] == NULL)
						continue;
					
					word = PathSlot(sampleList.item[l], -1);
					StrRep(word, "/", "");
					
					if (!StrStr(mem.str, word))
						continue;
					
					sampleList.item[l] = NULL;
				}
			}
			
			ItemList_Free(&list);
		}
	}
	
	forlist(i, sampleList) {
		if (sampleList.item[i] == NULL)
			continue;
		
		printf_warning("rm [%s]", sampleList.item[i]);
		Sys_Delete_Recursive(sampleList.item[i]);
	}
	
	MemFile_Free(&mem);
	ItemList_Free(&sampleList);
	ItemList_Free(&bankList);
}
