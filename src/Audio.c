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

#if 0
char* sSeqModeType[] = {
	"default",
	"enemy",
	"still",
	"ignore"
};
#endif

#define __Config_Sample(wow, sampletype) \
	Config_WriteVar_Hex(# wow "_sample", ReadBE(sample->sampleAddr) + rom->offset.segment.smplRom + off); \
	Config_WriteVar_Flo(# wow "_tuning", *f); \
	if (sBankNum < 0) { printf("\a\n"); exit(1); /* "go intentionally bonkers" */ } \
	sUnsortedSampleTbl[sDumpID].tuning = *f; \
	sUnsortedSampleTbl[sDumpID].data = sample->data; \
	sUnsortedSampleTbl[sDumpID].sampleAddr = ReadBE(sample->sampleAddr) + rom->offset.segment.smplRom + off; \
	sUnsortedSampleTbl[sDumpID].loop = VirtualToSegmented(0x0, SegmentedToVirtual(0x1, ReadBE(sample->loop))); \
	sUnsortedSampleTbl[sDumpID++].book = VirtualToSegmented(0x0, SegmentedToVirtual(0x1, ReadBE(sample->book))); \
	Assert(sDumpID < 1024 * 5);

#define __Config_Sample_NULL(wow) \
	Config_WriteVar_Str(# wow "_sample", "NULL"); \
	Config_WriteVar_Str(# wow "_tuning", "NULL");

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
	Config_WriteTitle_Str("Instrument");
	Config_WriteVar_Int("loaded", instrument->loaded);
	Config_WriteVar_Int("split_lo", instrument->splitLo);
	Config_WriteVar_Int("split_hi", instrument->splitHi);
	
	Config_SPrintf("\n");
	Config_WriteTitle_Str("Envelope");
	Config_WriteVar_Int("attack_rate", ReadBE(env[0].rate));
	Config_WriteVar_Int("attack_level", ReadBE(env[0].level));
	Config_WriteVar_Int("hold_rate", ReadBE(env[1].rate));
	Config_WriteVar_Int("hold_level", ReadBE(env[1].level));
	Config_WriteVar_Int("decay_rate", ReadBE(env[2].rate));
	Config_WriteVar_Int("decay_level", ReadBE(env[2].level));
	Config_WriteVar_Int("decay2_rate", ReadBE(env[3].rate));
	Config_WriteVar_Int("decay2_level", ReadBE(env[3].level));
	Config_WriteVar_Int("release", instrument->release);
	
	Config_SPrintf("\n");
	if (instrument->lo.sample != 0) {
		sample = SegmentedToVirtual(0x1, ReadBE(instrument->lo.sample));
		val = ReadBE(instrument->lo.swap32);
		__Config_Sample(low, ins);
	} else {
		__Config_Sample_NULL(low);
	}
	
	Config_SPrintf("\n");
	if (instrument->prim.sample != 0) {
		sample = SegmentedToVirtual(0x1, ReadBE(instrument->prim.sample));
		val = ReadBE(instrument->prim.swap32);
		sUnsortedSampleTbl[sDumpID].isPrim = true;
		sUnsortedSampleTbl[sDumpID].splitLo = instrument->splitLo;
		sUnsortedSampleTbl[sDumpID].splitHi = instrument->splitHi;
		__Config_Sample(prim, ins);
	} else {
		__Config_Sample_NULL(prim);
	}
	
	Config_SPrintf("\n");
	if (instrument->hi.sample != 0) {
		sample = SegmentedToVirtual(0x1, ReadBE(instrument->hi.sample));
		val = ReadBE(instrument->hi.swap32);
		__Config_Sample(hi, ins);
	} else {
		__Config_Sample_NULL(hi);
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
	Config_WriteTitle_Str("Sfx");
	if (sfx->sample != 0) {
		Sample* sample = SegmentedToVirtual(0x1, ReadBE(sfx->sample));
		val = ReadBE(sfx->swap32);
		__Config_Sample(prim, sfx);
	} else {
		__Config_Sample_NULL(prim);
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
	Config_WriteTitle_Str("Drum");
	Config_WriteVar_Int("loaded", drum->loaded);
	Config_WriteVar_Int("pan", drum->pan);
	
	Config_SPrintf("\n");
	Config_WriteTitle_Str("Envelope");
	Config_WriteVar_Int("attack_rate", ReadBE(env[0].rate));
	Config_WriteVar_Int("attack_level", ReadBE(env[0].level));
	Config_WriteVar_Int("hold_rate", ReadBE(env[1].rate));
	Config_WriteVar_Int("hold_level", ReadBE(env[1].level));
	Config_WriteVar_Int("decay_rate", ReadBE(env[2].rate));
	Config_WriteVar_Int("decay_level", ReadBE(env[2].level));
	Config_WriteVar_Int("decay2_rate", ReadBE(env[3].rate));
	Config_WriteVar_Int("decay2_level", ReadBE(env[3].level));
	Config_WriteVar_Int("release", drum->release);
	
	Config_SPrintf("\n");
	Config_WriteTitle_Str("Sample");
	if (drum->sound.sample != 0) {
		Sample* sample = SegmentedToVirtual(0x1, ReadBE(drum->sound.sample));
		val = ReadBE(drum->sound.swap32);
		__Config_Sample(prim, drm);
	} else {
		__Config_Sample_NULL(prim);
	}
	
	MemFile_SaveFile_String(config, out);
	
	return 1;
}

static void Rom_Config_Sample(Rom* rom, MemFile* config, Sample* sample, char* name, char* out) {
	AdpcmLoop* loop = SegmentedToVirtual(0x0, sample->loop);
	
	MemFile_Reset(config);
	Config_WriteTitle_Str(name);
	Config_WriteVar_Int("codec", ReadBE(sample->data) >> (32 - 4));
	Config_WriteVar_Int("medium", (ReadBE(sample->data) >> (32 - 6)) & 2);
	Config_WriteVar_Int("bitA", (ReadBE(sample->data) >> (32 - 7)) & 1);
	Config_WriteVar_Int("bitB", (ReadBE(sample->data) >> (32 - 8)) & 1);
	
	Config_SPrintf("\n");
	Config_WriteTitle_Str("Loop");
	Config_WriteVar_Int("loop_start", ReadBE(loop->start));
	Config_WriteVar_Int("loop_end", ReadBE(loop->end));
	Config_WriteVar_Int("loop_count", ReadBE(loop->count));
	Config_WriteVar_Int("tail_end", ReadBE(loop->origSpls));
	
	MemFile_SaveFile_String(config, out);
}

/* / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / */

static void Rom_Dump_Samples_PatchWavFiles(MemFile* dataFile, MemFile* config) {
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
		
		{ NOTE(0, 5), 0, &gSampleInfo[413] }, // CongaOpen
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
	
	for (s32 i = 0; i < ArrayCount(info); i++) {
		printf_progress("Update Sample", i + 1, ArrayCount(info));
		char* file = Dir_File("%s/Sample.wav", info[i].info->name);
		
		MemFile_Reset(dataFile);
		MemFile_LoadFile(dataFile, file);
		instInfo = MemMem(dataFile->data, dataFile->dataSize, "inst", 4);
		smplInfo = MemMem(dataFile->data, dataFile->dataSize, "smpl", 4);
		
		/* basenote */ instInfo[8] = info[i].basenote;
		/* finetune */ instInfo[9] = info[i].finetune;
		/* basenote */ smplInfo[5] = info[i].basenote;
		/* finetune */ smplInfo[6] = info[i].finetune;
		MemFile_SaveFile(dataFile, file);
	}
#undef NOTE
}

void Rom_Dump_SoundFont(Rom* rom, MemFile* dataFile, MemFile* config) {
	AudioEntryHead* head = SegmentedToVirtual(0, rom->offset.table.fontTable);
	AudioEntryHead* sampHead = SegmentedToVirtual(0, rom->offset.table.sampleTable);
	AudioEntry* entry;
	u32 num = ReadBE(head->numEntries);
	SoundFont* bank;
	Instrument* instrument;
	Sound* sfx;
	u32 off = 0;
	
	Dir_Enter("soundfont/.vanilla/");
	for (s32 i = 0; i < num; i++) {
		printf_progress("SoundFont", i + 1, num);
		
		entry = &head->entries[i];
		
		bank = SegmentedToVirtual(0x0, ReadBE(entry->romAddr) + rom->offset.segment.fontRom);
		off = ReadBE(sampHead->entries[entry->audioTable1].romAddr);
		if (off & 0xF) {
			printf_warning("audioTable Segment %08X id %d", off, entry->audioTable1);
			off = off & 0xFFFFFFF0;
		}
		SetSegment(0x1, bank);
		
		Dir_Enter("0x%02X-%s/", i, gBankName_OoT[i]); {
			if (entry->numInst) {
				Dir_Enter("instrument/");
				
				for (s32 j = 0; j < entry->numInst; j++) {
					char* output = Dir_File("%d-Inst.cfg", j);
					
					if (bank->instruments[j] == 0)
						instrument = NULL;
					else
						instrument = SegmentedToVirtual(0x1, ReadBE(bank->instruments[j]));
					if (Rom_Config_Instrument(rom, config, instrument, "instrument", output, off)) {
						strcpy(sBankFiles[sBankNum++], output);
						Assert(sBankNum < 1024 * 5);
					}
				}
				
				Dir_Leave();
			}
			
			if (entry->numSfx) {
				Dir_Enter("sfx/");
				
				for (s32 j = 0; j < ReadBE(entry->numSfx); j++) {
					char* output = Dir_File("%d-Sfx.cfg", j);
					sfx = SegmentedToVirtual(0x1, ReadBE(bank->sfx));
					
					if (Rom_Config_Sfx(rom, config, &sfx[j], "Sound Effect", output, off)) {
						strcpy(sBankFiles[sBankNum++], output);
						Assert(sBankNum < 1024 * 5);
					}
				}
				
				Dir_Leave();
			}
			
			if (entry->numDrum) {
				Dir_Enter("drum/");
				
				for (s32 j = 0; j < entry->numDrum; j++) {
					char* output = Dir_File("%d-Drum.cfg", j);
					u32* wow = SegmentedToVirtual(0x1, ReadBE(bank->drums));
					
					if (Rom_Config_Drum(rom, config, wow[j], "Drum", output, off)) {
						strcpy(sBankFiles[sBankNum++], output);
						Assert(sBankNum < 1024 * 5);
					}
				}
				
				Dir_Leave();
			}
			
			MemFile_Reset(config);
			Config_WriteTitle_Str(gBankName_OoT[i]);
			
			MemFile_Printf(config, "# Sample Medium types [");
			for (s32 e = 0; e < ArrayCount(sMediumType); e++) {
				if (e != 0)
					MemFile_Printf(config, "/");
				MemFile_Printf(config, "%s", sMediumType[e]);
			}
			MemFile_Printf(config, "]\n");
			Config_WriteVar_Str("medium_type", sMediumType[entry->medium]);
			
			MemFile_Printf(config, "# Sequence Player types [");
			for (s32 e = 0; e < ArrayCount(sSeqPlayerType); e++) {
				if (e != 0)
					MemFile_Printf(config, "/");
				MemFile_Printf(config, "%s", sSeqPlayerType[e]);
			}
			MemFile_Printf(config, "]\n");
			Config_WriteVar_Str("sequence_player", sSeqPlayerType[entry->seqPlayer]);
			
			MemFile_SaveFile_String(config, Dir_File("config.cfg"));
			
			Dir_Leave();
		}
	}
	Dir_Leave();
	
	SetSegment(0x1, NULL);
}

void Rom_Dump_Sequences(Rom* rom, MemFile* dataFile, MemFile* config) {
	AudioEntryHead* head = SegmentedToVirtual(0x0, rom->offset.table.seqTable);
	u8* seqFontTable;
	u16* segFontOffTable;
	AudioEntry* entry;
	RomFile romFile;
	u32 num = ReadBE(head->numEntries);
	
	Dir_Enter("sequence/.vanilla/");  {
		SetSegment(0x1, SegmentedToVirtual(0x0, rom->offset.table.seqFontTbl));
		
		MemFile_Reset(config);
		for (s32 i = 0; i < num; i++) {
			Dir_Enter("0x%02X-%s/", i, gSequenceName_OoT[i]); {
				printf_progress("Sequence", i + 1, num);
				segFontOffTable = SegmentedToVirtual(0x0, rom->offset.table.seqFontTbl);
				entry = &head->entries[i];
				romFile.data = SegmentedToVirtual(0x0, ReadBE(entry->romAddr) + rom->offset.segment.seqRom);
				romFile.size = ReadBE(entry->size);
				
				MemFile_Reset(config);
				Config_WriteTitle_Str(gSequenceName_OoT[i]);
				seqFontTable = SegmentedToVirtual(0x1, ReadBE(segFontOffTable[i]));
				
				Config_WriteVar_Int("bank_num", ReadBE(seqFontTable[0]));
				for (s32 i = 0; i < ReadBE(seqFontTable[0]); i++) {
					char* title = Tmp_Printf("bank_id_%d", i);
					Config_WriteVar_Hex(title, ReadBE(seqFontTable[i + 1]) & 0xFF);
				}
				
				if (romFile.size != 0) {
					Rom_Extract(dataFile, romFile, Dir_File("%s.seq", gSequenceName_OoT[i]));
				} else {
					Config_WriteVar_Hex("seq_pointer", ReadBE(entry->romAddr));
				}
				
				Config_WriteTitle_Str("Entry Data");
				
				MemFile_Printf(config, "# Sample Medium types [");
				for (s32 e = 0; e < ArrayCount(sMediumType); e++) {
					if (e != 0)
						MemFile_Printf(config, "/");
					MemFile_Printf(config, "%s", sMediumType[e]);
				}
				MemFile_Printf(config, "]\n");
				Config_WriteVar_Str("medium_type", sMediumType[entry->medium]);
				
				MemFile_Printf(config, "# Sequence Player types [");
				for (s32 e = 0; e < ArrayCount(sSeqPlayerType); e++) {
					if (e != 0)
						MemFile_Printf(config, "/");
					MemFile_Printf(config, "%s", sSeqPlayerType[e]);
				}
				MemFile_Printf(config, "]\n");
				Config_WriteVar_Str("sequence_player", sSeqPlayerType[entry->seqPlayer]);
				
				MemFile_SaveFile_String(config, Dir_File("config.cfg"));
				
				Dir_Leave();
			}
		}
		
		SetSegment(0x1, NULL);
		
		Dir_Leave();
	}
}

typedef struct {
	const N64AudioInfo* sample;
	Rom*  rom;
	SampleInfo* tbl;
	char* path;
	u32   i;
} SampleDumpArg;

static void SampleDump_Thread(SampleDumpArg* arg) {
	const N64AudioInfo* sample = arg->sample;
	SampleInfo* tbl = arg->tbl;
	AdpcmLoop* loop;
	AdpcmBook* book;
	MemFile* dataFile;
	MemFile* config;
	RomFile rf;
	Rom* rom = arg->rom;
	char* name = sample->dublicate == NULL ? sample->name : sample->dublicate->name;
	u32 sampRate = sample->dublicate == NULL ? sample->sampleRate : sample->dublicate->sampleRate;
	char FILE_WAV[512];
	char FILE_VAD[512];
	char FILE_BOK[512];
	char FILE_LBK[512];
	char FILE_CFG[512];
	
	if (name == NULL)
		printf_error("Sample ID [%D] is missing name", arg->i);
	
	if (sampRate == 0)
		printf_error("Sample [%s] is missing samplerate", name);
	
	book = SegmentedToVirtual(0x0, tbl->book);
	loop = SegmentedToVirtual(0x0, tbl->loop);
	
	Sys_MakeDir("%s%s/", arg->path, name);
	snprintf(FILE_WAV, 512, "%s%s/Sample.wav", arg->path, name);
	snprintf(FILE_VAD, 512, "%s%s/sample.vadpcm.bin", arg->path, name);
	snprintf(FILE_BOK, 512, "%s%s/sample.book.bin", arg->path, name);
	snprintf(FILE_LBK, 512, "%s%s/sample.loopbook.bin", arg->path, name);
	snprintf(FILE_CFG, 512, "%s%s/config.cfg", arg->path, name);
	
	dataFile = Malloc(dataFile, sizeof(MemFile));
	config = Malloc(config, sizeof(MemFile));
	*dataFile = MemFile_Initialize();
	*config = MemFile_Initialize();
	MemFile_Malloc(dataFile, MbToBin(4.0));
	MemFile_Malloc(config, MbToBin(1.0));
	
	MemFile_Params(dataFile, MEM_REALLOC, true, MEM_END);
	
	rf.size = ReadBE(tbl->data) & 0x00FFFFFF;
	rf.data = SegmentedToVirtual(0x0, tbl->sampleAddr);
	Rom_Extract(dataFile, rf, FILE_VAD);
	
	rf.size = sizeof(s16) * 8 * ReadBE(book->order) * ReadBE(book->npredictors) + 8;
	rf.data = book;
	Rom_Extract(dataFile, rf, FILE_BOK);
	
	Rom_Config_Sample(rom, config, (Sample*)tbl, name, FILE_CFG);
	
	if (loop->count) {
		rf.size = 0x20;
		rf.data = SegmentedToVirtual(0x0, tbl->loop + 0x10);
		Rom_Extract(dataFile, rf, FILE_LBK);
	}
	
	if (gExtractAudio) {
		char cmd[2048];
		
		Tools_Command(
			cmd,
			z64audio,
			"--i %s "
			"--o %s "
			"--S "
			"--srate %d "
			"--tuning %f ",
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
		
		if (SysExe(cmd))
			printf_error("z64audio Failed!");
		
		MemFile_Reset(dataFile);
		s8* instInfo;
		
		if (MemFile_LoadFile(dataFile, FILE_WAV)) {
			printf_warning_align("Sample not found", "%s", FILE_WAV);
		}
		
		instInfo = StrStr(dataFile->data, "inst");
		
		if (instInfo) {
			Config_SPrintf("\n # Instrument Info\n");
			Config_WriteVar_Int("basenote", instInfo[8]);
			Config_WriteVar_Int("finetune", instInfo[9]);
			MemFile_SaveFile_String(config, FILE_CFG);
		} else {
			if (dataFile->dataSize == 0)
				printf_warning_align("Audio", "Empty File [%s]", FILE_WAV);
		}
	}
	
	MemFile_Free(dataFile);
	MemFile_Free(config);
	Free(dataFile);
	Free(config);
}

void Rom_Dump_Samples(Rom* rom, MemFile* dataFile, MemFile* config) {
	SampleInfo* smallest = sUnsortedSampleTbl;
	SampleInfo* largest = sUnsortedSampleTbl;
	RomFile rf;
	SampleInfo** tbl;
	AdpcmLoop* loop;
	AdpcmBook* book;
	char buff[16];
	char* name;
	u32 sampRate;
	
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
	
	Dir_Enter("sample/.vanilla/");  {
		s32 i = 0;
		SampleDumpArg arg[32];
		Thread thread[32];
		
		ThreadLock_Init();
		while (i < sSortID) {
			printf_progress("Sample", i + 1, sSortID);
			u32 target = Clamp(sSortID - i, 0, 16);
			
			for (s32 j = 0; j < target; j++) {
				arg[j].i = i + j;
				arg[j].rom = rom;
				arg[j].sample = &gSampleInfo[i + j];
				arg[j].tbl = tbl[i + j];
				arg[j].path = "rom/sound/sample/.vanilla/";
				
				ThreadLock_Create(&thread[j], SampleDump_Thread, &arg[j]);
			}
			
			if (gThreading)
				for (s32 j = 0; j < target; j++)
					ThreadLock_Join(&thread[j]);
			
			i += 16;
		}
		ThreadLock_Free();
		
		for (s32 j = 0; j < sBankNum; j++) {
			char* replacedName = NULL;
			printf_progress("Update SoundFont", j + 1, sBankNum);
			
			MemFile_Clear(config);
			MemFile_LoadFile_String(config, sBankFiles[j]);
			for (s32 i = 0; i < sSortID; i++) {
				name = gSampleInfo[i].dublicate == NULL ? gSampleInfo[i].name : gSampleInfo[i].dublicate->name;
				
				sprintf(buff, "0x%X", sSortedSampleTbl[i]->sampleAddr);
				if (String_Replace(config->data, buff, name)) {
					replacedName = name;
				}
			}
			
			config->dataSize = strlen(config->data);
			MemFile_SaveFile_String(config, sBankFiles[j]);
			
			// Rename SFX To their samples
			if (StrStr(sBankFiles[j], "-Sfx")) {
				char* tempName = Tmp_Printf("%s%d-%s.cfg", String_GetPath(sBankFiles[j]), String_GetInt(String_GetBasename(sBankFiles[j])), replacedName);
				
				renamer_remove(sBankFiles[j], tempName);
			}
			
			// Rename Inst to their primary sample
			if ((StrStr(sBankFiles[j], "-Inst") || StrStr(sBankFiles[j], "-Drum")) && (StrStr(config->data, "Inst_") || StrStr(config->data, "Perc_"))) {
				char instName[256] = { 0 };
				char* tempName;
				
				strcpy(instName, Config_GetString(config, "prim_sample"));
				String_Remove(instName, strlen("Inst_"));
				String_Replace(instName, "_Prim", "");
				String_Replace(instName, "Soft", "");
				String_Replace(instName, "Hard", "");
				String_Replace(instName, "Mute", "");
				String_Replace(instName, "Open", "");
				String_Replace(instName, "_Hi", "Var");
				
				if (instName[0] == 0)
					printf_error("String maniplation failed for instrument");
				
				tempName = Tmp_Printf("%s%d-%s.cfg", String_GetPath(sBankFiles[j]), String_GetInt(String_GetBasename(sBankFiles[j])), instName);
				
				renamer_remove(sBankFiles[j], tempName);
			}
		}
		
		if (gExtractAudio)
			Rom_Dump_Samples_PatchWavFiles(dataFile, config);
		
		Dir_Leave();
	}
}

/* / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / */

struct {
	char name[128];
	u32  segment;
	u32  size;
	f32  tuninOverride;
	char dir[512];
} sSampleTbl[1024 * 2];
s32 sSampleTblNum;

static s32 Audio_LoadFile(MemFile* dataFile, char* file) {
	char buf[512];
	char* smpl;
	
	Log("File: %s", file);
	
	strcpy(buf, file);
	String_Replace(buf, "*", "sample");
	smpl = Dir_File(buf);
	
	if (smpl && Sys_Stat(smpl)) {
		if (MemFile_LoadFile(dataFile, smpl))
			return 1;
	} else {
		smpl = Dir_File(file);
		
		if (smpl) {
			if (MemFile_LoadFile(dataFile, Dir_File(file)))
				return 1;
		} else
			return 1;
	}
	
	return 0;
}

void Rom_Build_SetAudioSegment(Rom* rom) {
	#define INST_ADDR(x, y) SegmentedToVirtual(0x0, ((x) - 0x7F588E60)), SegmentedToVirtual(0x0, ((y) - 0x7F588E60))
	#define RAM_ADDR rom->file.seekPoint + 0x7F588E60
	
	MemFile_Params(&rom->file, MEM_ALIGN, 16, MEM_END);
	MemFile_Seek(&rom->file, 0xB65C00);
	
	Mips64_SplitLoad(INST_ADDR(0x800E330C, 0x800E3310), MIPS_REG_A1, rom->offset.segment.seqRom);
	Mips64_SplitLoad(INST_ADDR(0x800E3320, 0x800E3324), MIPS_REG_A1, rom->offset.segment.fontRom);
	Mips64_SplitLoad(INST_ADDR(0x800E3334, 0x800E3338), MIPS_REG_A1, rom->offset.segment.smplRom);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32C4, 0x800E32D4), MIPS_REG_T1, RAM_ADDR);
	MemFile_Append(&rom->file, &rom->mem.seqTbl);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32C8, 0x800E32D8), MIPS_REG_T2, RAM_ADDR);
	MemFile_Append(&rom->file, &rom->mem.fontTbl);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32CC, 0x800E32DC), MIPS_REG_T3, RAM_ADDR);
	MemFile_Append(&rom->file, &rom->mem.sampleTbl);
	
	Mips64_SplitLoad(INST_ADDR(0x800E32D0, 0x800E32E0), MIPS_REG_T6, RAM_ADDR);
	MemFile_Append(&rom->file, &rom->mem.seqFontTbl);
	
	MemFile_Params(&rom->file, MEM_ALIGN, 0, MEM_END);
	
	if (rom->file.seekPoint - 0xB65C00 > 0x318C - 0x10) {
		printf_warning("AudioDebug_Draw overwriting exceeded. Might cause trouble...");
	}
	
#undef INST_ADDR
#undef RAM_ADDR
}

void Rom_Build_SampleTable(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList itemList = ItemList_Initialize();
	MemFile sample = MemFile_Initialize();
	AudioEntryHead head = { 0 };
	AudioEntry entry = { 0 };
	
	MemFile_Malloc(&sample, MbToBin(0.25));
	MemFile_Reset(dataFile);
	Rom_ItemList(&itemList, SORT_NO, IS_DIR);
	MemFile_Params(dataFile, MEM_ALIGN, 16, MEM_END);
	
	for (s32 i = 0; i < itemList.num; i++) {
		printf_progress("Append Sample", i + 1, itemList.num);
		MemFile_Reset(config);
		MemFile_Reset(&sample);
		
		Dir_Enter(itemList.item[i]); {
			char* file = Dir_File("*.vadpcm.bin");
			char* cfg = Dir_File("config.cfg");
			f32 tuning;
			
			if (cfg == NULL)
				printf_error("Could not locate sample in [%s]", itemList.item[i]);
			if (file == NULL)
				printf_error("Could not locate sample in [%s]", itemList.item[i]);
			
			Log("smp: %s", file);
			Log("cfg: %s", cfg);
			
			if (MemFile_LoadFile(&sample, file))
				printf_error_align("Failed to load file", "%s", file);
			
			if (MemFile_LoadFile_String(config, cfg))
				printf_error_align("Failed to load file", "%s", cfg);
			
			Config_SuppressNext();
			tuning = Config_GetFloat(config, "tuning");
			
			if (tuning > 0.0f)
				sSampleTbl[sSampleTblNum].tuninOverride = tuning;
			
			sSampleTbl[sSampleTblNum].segment = dataFile->seekPoint;
			if (dataFile->seekPoint & 0xF)
				printf_error("Error: Samplebank alignment failed!");
			
			sSampleTbl[sSampleTblNum].size = sample.dataSize;
			strcpy(sSampleTbl[sSampleTblNum].dir, Dir_File(""));
			strcpy(sSampleTbl[sSampleTblNum].name, String_GetFolder(itemList.item[i], -1));
			String_Replace(sSampleTbl[sSampleTblNum].name, "/", "\0");
			sSampleTblNum++;
			MemFile_Append(dataFile, &sample);
			
			Dir_Leave();
		}
	}
	
	head.numEntries = 1;
	SwapBE(head.numEntries);
	entry.romAddr = 0;
	entry.size = ReadBE(dataFile->dataSize);
	entry.medium = 2;
	entry.seqPlayer = 4;
	MemFile_Write(&rom->mem.sampleTbl, &head, 16);
	MemFile_Write(&rom->mem.sampleTbl, &entry, 16);
	
	rom->offset.segment.smplRom = Dma_WriteEntry(rom, DMA_NO_ENTRY, dataFile, false);
	MemFile_Params(dataFile, MEM_ALIGN, 0, MEM_END);
	
	MemFile_Free(&sample);
	ItemList_Free(&itemList);
}

const char* sSFSampleName[3] = {
	"low_sample",
	"prim_sample",
	"hi_sample",
};
const char* sSFSampleTuning[3] = {
	"low_tuning",
	"prim_tuning",
	"hi_tuning",
};

static void SoundFont_Error_NotFound(const char* sampleName) {
	MemFile mem = MemFile_Initialize();
	
	Log("Dumping [audio_log]");
	MemFile_Malloc(&mem, 0x90000);
	
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
	
	if (!smpl[2] && inst->splitHi < 127) {
		printf_warning("split_hi fixed for [%s]", file);
		Config_Replace(mem, "split_hi", "127");
		
		mem->dataSize = strlen(mem->str);
		MemFile_SaveFile_String(mem, file);
		inst->splitHi = 127;
	}
}

static s32 SoundFont_Instrument_AssignNames(MemFile* mem, char** smplNam, MemFile* memBank) {
	u32 smplNum = 0;
	
	for (s32 soundID = 0; soundID < 3; soundID++) {
		smplNam[soundID] = Config_GetString(mem, sSFSampleName[soundID]);
		if (smplNam[soundID] == NULL) {
			continue;
		} else if (StrMtch(smplNam[soundID], "NULL")) {
			smplNam[soundID] = NULL;
			continue;
		}
		smplNum++;
	}
	
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
				
				if (inst->sound[soundID].tuning == 0)
					inst->sound[soundID].tuning = Config_GetFloat(mem, sSFSampleTuning[soundID]);
				
				SwapBE(inst->sound[soundID].swap32);
				smplID[soundID] = sampleID;
				break;
			}
		}
	}
}

static void SoundFont_Read_Instrument(MemFile* mem, Instrument* inst) {
	inst->loaded = Config_GetInt(mem, "loaded");
	inst->splitLo = Config_GetInt(mem, "split_lo");
	inst->splitHi = Config_GetInt(mem, "split_hi");
	inst->release = Config_GetInt(mem, "release");
}

static void SoundFont_Read_Adsr(MemFile* mem, Adsr* adsr) {
	adsr[0].rate = Config_GetInt(mem, "attack_rate"); SwapBE(adsr[0].rate);
	adsr[0].level = Config_GetInt(mem, "attack_level"); SwapBE(adsr[0].level);
	adsr[1].rate = Config_GetInt(mem, "hold_rate"); SwapBE(adsr[1].rate);
	adsr[1].level = Config_GetInt(mem, "hold_level"); SwapBE(adsr[1].level);
	adsr[2].rate = Config_GetInt(mem, "decay_rate"); SwapBE(adsr[2].rate);
	adsr[2].level = Config_GetInt(mem, "decay_level"); SwapBE(adsr[2].level);
	adsr[3].rate = Config_GetInt(mem, "decay2_rate"); SwapBE(adsr[3].rate);
	adsr[3].level = Config_GetInt(mem, "decay2_level"); SwapBE(adsr[3].level);
}

static void SoundFont_Write_Adsr(MemFile* mem, Adsr* adsr, void32* setPtr) {
	if (!MemMemAlign(16, mem->data, mem->dataSize, adsr, 16)) {
		setPtr[0] = mem->seekPoint;
		MemFile_Write(mem, adsr, 16);
		MemFile_Align(mem, 16);
	} else {
		void* ptr = MemMemAlign(16, mem->data, mem->dataSize, adsr, 16);
		setPtr[0] = (uPtr)ptr - (uPtr)mem->data;
	}
}

static void SoundFont_Write_Sample(MemFile* dataFile, s32 sampleID, void32* setPtr, MemFile* memSample, MemFile* memBook, MemFile* memLoopBook, u32* sampleNum) {
	char* restoreDir = Tmp_String(Dir_Current());
	Sample smpl = { 0 };
	u32 loop[4 + 8];
	u32 loopSize = 4 * 4;
	
	if (sampleID < 0)
		return;
	
	Dir_Set(sSampleTbl[sampleID].dir);
	MemFile_Reset(dataFile);
	MemFile_LoadFile(dataFile, Dir_File("config.cfg"));
	
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
		if (Audio_LoadFile(dataFile, "*.loopbook.bin")) {
			printf_warning("" PRNT_REDD "[%s]" PRNT_RSET " has looppoints but could not find " PRNT_REDD "loopbook", sSampleTbl[sampleID].name);
			loop[0] = 0;
			WriteBE(loop[2], 0);
			loopSize = 0;
		} else {
			for (s32 i = 0; i < 8; i++) {
				loop[4 + i] = dataFile->cast.u32[i];
			}
			
			loopSize = 4 * (4 + 8);
		}
	}
	
	if (!MemMemAlign(0x10, memLoopBook->data, memLoopBook->dataSize, loop, loopSize)) {
		smpl.loop = memLoopBook->seekPoint;
		MemFile_Write(memLoopBook, loop, loopSize);
		MemFile_Align(memLoopBook, 16);
	} else {
		void* ptr = MemMemAlign(0x10, memLoopBook->data, memLoopBook->dataSize, loop, loopSize);
		smpl.loop = (uPtr)ptr - (uPtr)memLoopBook->data;
	}
	
	MemFile_Reset(dataFile);
	Audio_LoadFile(dataFile, "*.book.bin");
	if (!MemMemAlign(0x10, memBook->data, memBook->dataSize, dataFile->data, dataFile->dataSize)) {
		smpl.book = memBook->seekPoint;
		MemFile_Append(memBook, dataFile);
		MemFile_Align(memBook, 16);
	} else {
		void* ptr = MemMemAlign(0x10, memBook->data, memBook->dataSize, dataFile->data, dataFile->dataSize);
		smpl.book = (uPtr)ptr - (uPtr)memBook->data;
	}
	
	if (!MemMemAlign(0x10, memSample->data, memSample->dataSize, &smpl, sizeof(struct Sample))) {
		setPtr[0] = memSample->seekPoint;
		MemFile_Write(memSample, &smpl, sizeof(struct Sample));
		sampleNum[0]++;
	} else {
		void* ptr = MemMemAlign(0x10, memSample->data, memSample->dataSize, &smpl, sizeof(struct Sample));
		setPtr[0] = (uPtr)ptr - (uPtr)memSample->data;
	}
	Dir_Set(restoreDir);
}

void Rom_Build_SoundFont(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList itemList = ItemList_Initialize();
	MemFile soundFontMem = MemFile_Initialize();
	MemFile memBank = MemFile_Initialize();
	MemFile memBook = MemFile_Initialize();
	MemFile memLoopBook = MemFile_Initialize();
	MemFile memInst = MemFile_Initialize();
	MemFile memEnv = MemFile_Initialize();
	MemFile memSample = MemFile_Initialize();
	MemFile memSfx = MemFile_Initialize();
	MemFile memDrum = MemFile_Initialize();
	AudioEntryHead sfHead = { 0 };
	AudioEntry sfEntry = { 0 };
	
	MemFile_Malloc(&soundFontMem, MbToBin(2.00));
	MemFile_Malloc(&memBank, MbToBin(0.25));
	MemFile_Malloc(&memBook, MbToBin(0.25));
	MemFile_Malloc(&memLoopBook, MbToBin(0.25));
	MemFile_Malloc(&memInst, MbToBin(0.25));
	MemFile_Malloc(&memEnv, MbToBin(0.25));
	MemFile_Malloc(&memSample, MbToBin(0.25));
	MemFile_Malloc(&memSfx, MbToBin(0.25));
	MemFile_Malloc(&memDrum, MbToBin(0.25));
	
	Rom_ItemList(&itemList, SORT_NUMERICAL, IS_DIR);
	
	sfHead.numEntries = itemList.num;
	SwapBE(sfHead.numEntries);
	MemFile_Write(&rom->mem.fontTbl, &sfHead, 16);
	
	for (s32 i = 0; i < itemList.num; i++) {
		ItemList listInst = ItemList_Initialize();
		ItemList listSfx = ItemList_Initialize();
		ItemList listDrum = ItemList_Initialize();
		printf_progress("Build SoundFont", i + 1, itemList.num);
		MemFile_Reset(&memBank);
		MemFile_Reset(&memBook);
		MemFile_Reset(&memLoopBook);
		MemFile_Reset(&memInst);
		MemFile_Reset(&memEnv);
		MemFile_Reset(&memSample);
		MemFile_Reset(&memSfx);
		MemFile_Reset(&memDrum);
		MemFile_Reset(dataFile);
		MemFile_Reset(config);
		SetSegment(0x4, memBank.data);
		
		Dir_Enter(itemList.item[i]); {
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
			
			for (s32 y = 0; y < 3; y++) {
				if (Dir_Stat(dirs[y])) {
					Dir_Enter(dirs[y]);
					Dir_ItemList(lists[y], false);
					ItemList_NumericalSort(lists[y]);
					
					Dir_Leave();
				}
			}
			
			// DDDDDDDD SSSSSSSS Drum Segment | Sfx Segment
			MemFile_Write(&memBank, "\0\0\0\0\0\0\0\0", 8);
			
			for (s32 j = 0; j < listInst.num; j++) {
				Instrument instrument = { 0 };
				Adsr adsr[4];
				char* instFile = Dir_File("instrument/%s", listInst.item[j]);
				char* sampleName[3];
				s32 sampleIndex[3] = { -1, -1, -1 };
				
				// List does not have entry for j index
				if (listInst.item[j] == NULL) {
					u32 null = 0xFFFF; // Empty Instrument Entry
					MemFile_Write(&memBank, &null, sizeof(u32));
					continue;
				}
				
				MemFile_Reset(config);
				Log("Load File: %s", instFile);
				MemFile_LoadFile_String(config, instFile);
				
				if (SoundFont_Instrument_AssignNames(config, sampleName, &memBank)) continue;
				SoundFont_Instrument_AssignIndexes(config, sampleName, sampleIndex, &instrument);
				SoundFont_Read_Instrument(config, &instrument);
				SoundFont_Read_Adsr(config, adsr);
				SoundFont_Instrument_Validate(config, instFile, &instrument, sampleName);
				SoundFont_Write_Adsr(&memEnv, adsr, &instrument.envelope);
				
				for (s32 soundID = 0; soundID < 3; soundID++)
					SoundFont_Write_Sample(dataFile, sampleIndex[soundID], &instrument.sound[soundID].sample, &memSample, &memBook, &memLoopBook, &smplNum);
				
				SwapBE(memInst.seekPoint);
				MemFile_Write(&memBank, &memInst.seekPoint, sizeof(u32));
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
				MemFile_LoadFile_String(config, Dir_File("sfx/%s", listSfx.item[j]));
				prim = Config_GetString(config, "prim_sample");
				
				if (!strcmp(prim, "NULL")) {
					MemFile_Write(&memSfx, &sfx, sizeof(struct Sound));
					continue;
				}
				
				sfx.tuning = Config_GetFloat(config, "prim_tuning");
				idx = SoundFont_SmplID(prim);
				
				if (sSampleTbl[idx].tuninOverride > 0)
					sfx.tuning = sSampleTbl[idx].tuninOverride;
				SwapBE(sfx.swap32);
				SoundFont_Write_Sample(dataFile, idx, &sfx.sample, &memSample, &memBook, &memLoopBook, &smplNum);
				
				MemFile_Write(&memSfx, &sfx, sizeof(struct Sound));
			}
			
			for (s32 j = 0; j < listDrum.num; j++) {
				Drum drum = { 0 };
				Adsr adsr[4] = { 0 };
				char* currentConf = Tmp_String(Dir_File("drum/%s", listDrum.item[j]));
				char* prim;
				
				if (j == 0) {
					MemFile_Params(&memDrum, MEM_ALIGN, 0, MEM_END);
					memDrum.seekPoint += 4 * listDrum.num;
					memset(memDrum.data, 0, memDrum.seekPoint);
					MemFile_Align(&memDrum, 16);
					MemFile_Params(&memDrum, MEM_ALIGN, 16, MEM_END);
				}
				
				if (listDrum.item[j] == NULL) {
					memDrum.cast.u32[j] = 0;
					
					continue;
				}
				
				MemFile_Reset(config);
				MemFile_LoadFile_String(config, currentConf);
				prim = Tmp_String(Config_GetString(config, "prim_sample"));
				
				if (!memcmp(prim, "NULL", 4)) {
					memDrum.cast.u32[j] = 0;
					
					continue;
				} else {
					memDrum.cast.u32[j] = memDrum.seekPoint;
				}
				
				drum.sound.tuning = Config_GetFloat(config, "prim_tuning");
				drum.loaded = Config_GetInt(config, "loaded");
				drum.pan = Config_GetInt(config, "pan");
				drum.release = Config_GetInt(config, "release");
				SwapBE(drum.sound.swap32);
				
				SoundFont_Read_Adsr(config, adsr);
				SoundFont_Write_Adsr(&memEnv, adsr, &drum.envelope);
				SoundFont_Write_Sample(dataFile, SoundFont_SmplID(prim), &drum.sound.sample, &memSample, &memBook, &memLoopBook, &smplNum);
				
				MemFile_Write(&memDrum, &drum, sizeof(struct Drum));
				MemFile_Align(&memDrum, 16);
			}
			
			MemFile_Align(&memBank, 16);
			
			for (s32 j = 0; j < listInst.num; j++) {
				if (memBank.cast.u32[2 + j] == 0xFFFF) {
					continue;
				}
				SwapBE(memBank.cast.u32[2 + j]);
				memBank.cast.u32[2 + j] += memBank.seekPoint;
				SwapBE(memBank.cast.u32[2 + j]);
			}
			MemFile_Append(&memBank, &memInst); MemFile_Align(&memBank, 16);
			
			for (s32 j = 0; j < listInst.num; j++) {
				if (memBank.cast.u32[2 + j] == 0xFFFF) {
					continue;
				}
				Instrument* inst = SegmentedToVirtual(0x4, ReadBE(memBank.cast.u32[2 + j]));
				
				inst->envelope += memBank.seekPoint;
				SwapBE(inst->envelope);
			}
			
			for (s32 j = 0; j < listDrum.num; j++) {
				if (memDrum.cast.u32[j] == 0)
					continue;
				SetSegment(0x5, memDrum.data);
				Drum* drum = SegmentedToVirtual(0x5, memDrum.cast.u32[j]);
				
				drum->envelope += memBank.seekPoint;
				SwapBE(drum->envelope);
			}
			
			MemFile_Append(&memBank, &memEnv); MemFile_Align(&memBank, 16);
			
			for (s32 l = 0; l < smplNum; l++) {
				Sample* smpl = memSample.data;
				
				smpl[l].book += memBank.seekPoint;
				SwapBE(smpl[l].book);
			}
			
			MemFile_Append(&memBank, &memBook); MemFile_Align(&memBank, 16);
			
			for (s32 l = 0; l < smplNum; l++) {
				Sample* smpl = memSample.data;
				
				smpl[l].loop += memBank.seekPoint;
				SwapBE(smpl[l].loop);
			}
			
			MemFile_Append(&memBank, &memLoopBook); MemFile_Align(&memBank, 16);
			
			for (s32 j = 0; j < listInst.num; j++) {
				if (memBank.cast.u32[2 + j] == 0xFFFF) {
					memBank.cast.u32[2 + j] = 0;
					continue;
				}
				Instrument* inst = SegmentedToVirtual(0x4, ReadBE(memBank.cast.u32[2 + j]));
				
				if (inst->lo.swap32) {
					inst->lo.sample += memBank.seekPoint;
				}
				if (inst->prim.swap32) {
					inst->prim.sample += memBank.seekPoint;
				}
				if (inst->hi.swap32) {
					inst->hi.sample += memBank.seekPoint;
				}
				
				SwapBE(inst->lo.sample);
				SwapBE(inst->prim.sample);
				SwapBE(inst->hi.sample);
			}
			
			for (s32 j = 0; j < listSfx.num; j++) {
				Sound* sound = memSfx.data;
				
				sound[j].sample += memBank.seekPoint;
				SwapBE(sound[j].sample);
			}
			
			for (s32 j = 0; j < listDrum.num; j++) {
				if (memDrum.cast.u32[j] == 0)
					continue;
				SetSegment(0x5, memDrum.data);
				Drum* drum = SegmentedToVirtual(0x5, (memDrum.cast.u32[j]));
				
				drum->sound.sample += memBank.seekPoint;
				SwapBE(drum->sound.sample);
				SetSegment(0x5, NULL);
			}
			
			MemFile_Append(&memBank, &memSample); MemFile_Align(&memBank, 16);
			
			if (listSfx.num) {
				memBank.cast.u32[1] = ReadBE(memBank.seekPoint);
				MemFile_Append(&memBank, &memSfx); MemFile_Align(&memBank, 16);
			}
			
			if (listDrum.num) {
				for (s32 j = 0; j < listDrum.num; j++) {
					if (memDrum.cast.u32[j] == 0)
						continue;
					memDrum.cast.u32[j] += memBank.seekPoint;
					SwapBE(memDrum.cast.u32[j]);
				}
				memBank.cast.u32[0] = ReadBE(memBank.seekPoint);
				MemFile_Append(&memBank, &memDrum); MemFile_Align(&memBank, 16);
			}
			
			u32 seekPoint = soundFontMem.seekPoint;
			u32 med = 0;
			u32 seq = 0;
			char* confMed;
			char* confSeq;
			MemFile_Append(&soundFontMem, &memBank); MemFile_Align(&soundFontMem, 16);
			
			MemFile_Reset(config);
			MemFile_LoadFile_String(config, Dir_File("config.cfg"));
			confMed = Config_GetString(config, "medium_type");
			confSeq = Config_GetString(config, "sequence_player");
			
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
			
			sfEntry.romAddr = seekPoint;
			sfEntry.size = memBank.dataSize;
			sfEntry.medium = med;
			sfEntry.seqPlayer = seq;
			sfEntry.audioTable1 = 0;
			sfEntry.audioTable2 = -1;
			sfEntry.numInst = listInst.num;
			sfEntry.numDrum = listDrum.num;
			sfEntry.numSfx = listSfx.num;
			SwapBE(sfEntry.romAddr);
			SwapBE(sfEntry.size);
			SwapBE(sfEntry.numSfx);
			MemFile_Write(&rom->mem.fontTbl, &sfEntry, 16);
			
			Dir_Leave();
		}
		
		ItemList_Free(&listInst);
		ItemList_Free(&listSfx);
		ItemList_Free(&listDrum);
	}
	
	rom->offset.segment.fontRom = Dma_WriteEntry(rom, DMA_NO_ENTRY, &soundFontMem, false);
	
	MemFile_Free(&soundFontMem);
	MemFile_Free(&memBank);
	MemFile_Free(&memBook);
	MemFile_Free(&memLoopBook);
	MemFile_Free(&memInst);
	MemFile_Free(&memEnv);
	MemFile_Free(&memSample);
	MemFile_Free(&memSfx);
	MemFile_Free(&memDrum);
	
	ItemList_Free(&itemList);
}

void Rom_Build_Sequence(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList itemList = ItemList_Initialize();
	MemFile memIndexTable = MemFile_Initialize();
	MemFile memLookUpTable = MemFile_Initialize();
	MemFile sequenceMem = MemFile_Initialize();
	AudioEntryHead sqHead = { 0 };
	AudioEntry sqEntry = { 0 };
	
	MemFile_Malloc(&memIndexTable, 0x800);
	MemFile_Malloc(&memLookUpTable, 0x800);
	MemFile_Malloc(&sequenceMem, MbToBin(1.0));
	Rom_ItemList(&itemList, SORT_NUMERICAL, IS_DIR);
	
	sqHead.numEntries = itemList.num;
	SwapBE(sqHead.numEntries);
	MemFile_Write(&rom->mem.seqTbl, &sqHead, 16);
	
	for (s32 i = 0; i < itemList.num; i++) {
		printf_progress("Append Sequences", i + 1, itemList.num);
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
		
		Dir_Enter(itemList.item[i]); {
			u32 med = 0;
			u32 seq = 0;
			char* confMed;
			char* confSeq;
			
			MemFile_Reset(dataFile);
			MemFile_Reset(config);
			MemFile_LoadFile_String(config, Dir_File("config.cfg"));
			confMed = Config_GetString(config, "medium_type");
			confSeq = Config_GetString(config, "sequence_player");
			
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
			
			sqEntry.medium = med;
			sqEntry.seqPlayer = seq;
			sqEntry.audioTable1 = 0;
			sqEntry.audioTable2 = 0;
			sqEntry.numInst = 0;
			sqEntry.numDrum = 0;
			sqEntry.numSfx = 0;
			
			if (Dir_File("*.seq")) {
				MemFile_LoadFile(dataFile, Dir_File("*.seq"));
				addr = sequenceMem.seekPoint;
				MemFile_Append(&sequenceMem, dataFile);
				MemFile_Align(&sequenceMem, 16);
				sqEntry.romAddr = addr;
				sqEntry.size = dataFile->dataSize;
			} else {
				sqEntry.romAddr = Config_GetInt(config, "seq_pointer");
				sqEntry.size = 0;
			}
			
			SwapBE(sqEntry.romAddr);
			SwapBE(sqEntry.size);
			MemFile_Write(&rom->mem.seqTbl, &sqEntry, 16);
			
			u16 offset = memIndexTable.seekPoint;
			MemFile_Write(&memLookUpTable, &offset, 2);
			fontNum = Config_GetInt(config, "bank_num");
			MemFile_Write(&memIndexTable, &fontNum, 1);
			for (s32 j = 0; j < fontNum; j++) {
				char* title = Tmp_Printf("bank_id_%d", j);
				u8 bankId = Config_GetInt(config, title);
				MemFile_Write(&memIndexTable, &bankId, 1);
			}
			
			Dir_Leave();
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
	
	rom->offset.segment.seqRom = Dma_WriteEntry(rom, DMA_NO_ENTRY, &sequenceMem, false);
	
	MemFile_Free(&memIndexTable);
	MemFile_Free(&memLookUpTable);
	MemFile_Free(&sequenceMem);
}
