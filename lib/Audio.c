#include "z64rom.h"

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
		instInfo = Lib_MemMem(dataFile->data, dataFile->dataSize, "inst", 4);
		smplInfo = Lib_MemMem(dataFile->data, dataFile->dataSize, "smpl", 4);
		
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
		
		Dir_Enter("0x%02X-%s/", i, gBankName[i]); {
			if (entry->numInst) {
				Dir_Enter("instrument/");
				
				for (s32 j = 0; j < entry->numInst; j++) {
					char* output = Dir_File("%d-Inst.cfg", j);
					
					#ifndef NDEBUG
						if (gPrintfSuppress == PSL_DEBUG)
							printf_progress("inst", j + 1, entry->numInst);
					#endif
					
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
					
					#ifndef NDEBUG
						if (gPrintfSuppress == PSL_DEBUG)
							printf_progress("sfx", j + 1, ReadBE(entry->numSfx));
					#endif
					
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
					
					#ifndef NDEBUG
						if (gPrintfSuppress == PSL_DEBUG)
							printf_progress("drum", j + 1, entry->numDrum);
					#endif
					
					if (Rom_Config_Drum(rom, config, wow[j], "Drum", output, off)) {
						strcpy(sBankFiles[sBankNum++], output);
						Assert(sBankNum < 1024 * 5);
					}
				}
				
				Dir_Leave();
			}
			
			MemFile_Reset(config);
			Config_WriteTitle_Str(gBankName[i]);
			
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
			Dir_Enter("0x%02X-%s/", i, gSequenceName[i]); {
				printf_progress("Sequence", i + 1, num);
				segFontOffTable = SegmentedToVirtual(0x0, rom->offset.table.seqFontTbl);
				entry = &head->entries[i];
				romFile.data = SegmentedToVirtual(0x0, ReadBE(entry->romAddr) + rom->offset.segment.seqRom);
				romFile.size = ReadBE(entry->size);
				
				MemFile_Reset(config);
				Config_WriteTitle_Str(gSequenceName[i]);
				seqFontTable = SegmentedToVirtual(0x1, ReadBE(segFontOffTable[i]));
				
				Config_WriteVar_Int("bank_num", ReadBE(seqFontTable[0]));
				for (s32 i = 0; i < ReadBE(seqFontTable[0]); i++) {
					char* title = tprintf("bank_id_%d", i);
					Config_WriteVar_Hex(title, ReadBE(seqFontTable[i + 1]) & 0xFF);
				}
				
				if (romFile.size != 0) {
					Rom_Extract(dataFile, romFile, Dir_File("%s.seq", gSequenceName[i]));
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
	char sysbuf[256 * 2];
	
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
		for (s32 i = 0; i < sSortID; i++) {
			printf_progress("Sample", i + 1, sSortID);
			name = gSampleInfo[i].dublicate == NULL ? gSampleInfo[i].name : gSampleInfo[i].dublicate->name;
			sampRate = gSampleInfo[i].dublicate == NULL ? gSampleInfo[i].sampleRate : gSampleInfo[i].dublicate->sampleRate;
			
			if (gGenericNames) {
				name = tprintf("Sample_%03d", i);
				sampRate = 16000;
			}
			
			if (name == NULL)
				printf_error("Sample ID [%D] is missing name", i);
			
			if (sampRate == 0)
				printf_error("Sample [%s] is missing samplerate", name);
			
			Dir_Enter("%s/", name); {
				book = SegmentedToVirtual(0x0, tbl[i]->book);
				loop = SegmentedToVirtual(0x0, tbl[i]->loop);
				
				rf.size = ReadBE(tbl[i]->data) & 0x00FFFFFF;
				rf.data = SegmentedToVirtual(0x0, tbl[i]->sampleAddr);
				Rom_Extract(dataFile, rf, Dir_File("sample.vadpcm.bin"));
				
				rf.size = sizeof(s16) * 8 * ReadBE(book->order) * ReadBE(book->npredictors) + 8;
				rf.data = book;
				Rom_Extract(dataFile, rf, Dir_File("sample.book.bin"));
				
				Rom_Config_Sample(rom, config, (Sample*)tbl[i], name, Dir_File("config.cfg"));
				
				if (loop->count) {
					rf.size = 0x20;
					rf.data = SegmentedToVirtual(0x0, tbl[i]->loop + 0x10);
					Rom_Extract(dataFile, rf, Dir_File("sample.loopbook.bin"));
				}
				
				if (gExtractAudio) {
					#ifndef _WIN32
						strcpy(sysbuf, "./tools/z64audio --i ");
					#else
						strcpy(sysbuf, "tools\\z64audio.exe --i ");
					#endif
					strcat(sysbuf, Dir_File("sample.vadpcm.bin"));
					strcat(sysbuf, " --o ");
					strcat(sysbuf, Dir_File("Sample.wav"));
					
					#ifndef NDEBUG
						if (gPrintfSuppress == PSL_DEBUG)
							strcat(sysbuf, " --D");
						else
							strcat(sysbuf, " --S");
					#else
						strcat(sysbuf, " --S");
					#endif
					
					strcat(sysbuf, " --srate ");
					strcat(sysbuf, tprintf("%d", sampRate));
					strcat(sysbuf, " --tuning ");
					strcat(sysbuf, tprintf("%f", tbl[i]->tuning));
					
					if (tbl[i]->isPrim && (tbl[i]->splitHi != 127 || tbl[i]->splitLo != 0)) {
						strcat(sysbuf, " --split-hi ");
						strcat(sysbuf, tprintf("%d", tbl[i]->splitHi + 21));
						if (tbl[i]->splitLo) {
							strcat(sysbuf, " --split-lo ");
							strcat(sysbuf, tprintf("%d", tbl[i]->splitLo + 21));
						}
					}
					
					if (system(sysbuf))
						printf_error(sysbuf);
					
					MemFile_Reset(dataFile);
					s8* instInfo;
					
					if (MemFile_LoadFile(dataFile, Dir_File("Sample.wav"))) {
						printf_warning_align("Sample not found", "%s", Dir_File("Sample.wav"));
					}
					
					instInfo = Lib_MemMem(dataFile->data, dataFile->dataSize, "inst", 4);
					
					if (instInfo) {
						Config_SPrintf("\n # Instrument Info\n");
						Config_WriteVar_Int("basenote", instInfo[8]);
						Config_WriteVar_Int("finetune", instInfo[9]);
						MemFile_SaveFile_String(config, Dir_File("config.cfg"));
					} else {
						if (dataFile->dataSize == 0)
							printf_warning_align("Audio", "Empty File [%s]", Dir_File("Sample.wav"));
					}
				}
				
				Dir_Leave();
			}
		}
		
		for (s32 j = 0; j < sBankNum; j++) {
			char* replacedName = NULL;
			printf_progress("Update SoundFont", j + 1, sBankNum);
			
			MemFile_Clear(config);
			MemFile_LoadFile_String(config, sBankFiles[j]);
			for (s32 i = 0; i < sSortID; i++) {
				name = gSampleInfo[i].dublicate == NULL ? gSampleInfo[i].name : gSampleInfo[i].dublicate->name;
				if (gGenericNames) {
					name = tprintf("Sample_%03d", i);
					sampRate = 16000;
				}
				
				sprintf(buff, "0x%X", sSortedSampleTbl[i]->sampleAddr);
				if (String_Replace(config->data, buff, name)) {
					replacedName = name;
				}
			}
			
			config->dataSize = strlen(config->data);
			MemFile_SaveFile_String(config, sBankFiles[j]);
			
			// Rename SFX To their samples
			if (String_MemMem(sBankFiles[j], "-Sfx")) {
				char* tempName = tprintf("%s%d-%s.cfg", String_GetPath(sBankFiles[j]), String_GetInt(String_GetBasename(sBankFiles[j])), replacedName);
				
				renamer_remove(sBankFiles[j], tempName);
			}
			
			// Rename Inst to their primary sample
			if ((String_MemMem(sBankFiles[j], "-Inst") || String_MemMem(sBankFiles[j], "-Drum")) && (String_MemMem(config->data, "Inst_") || String_MemMem(config->data, "Perc_"))) {
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
				
				tempName = tprintf("%s%d-%s.cfg", String_GetPath(sBankFiles[j]), String_GetInt(String_GetBasename(sBankFiles[j])), instName);
				
				renamer_remove(sBankFiles[j], tempName);
			}
		}
		
		if (gExtractAudio && !gGenericNames)
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

static void Audio_LoadFile(MemFile* dataFile, char* file) {
	char buf[512];
	
	strcpy(buf, file);
	String_Replace(buf, "*", "sample");
	if (Stat(Dir_File(buf))) {
		if (MemFile_LoadFile(dataFile, Dir_File(buf))) printf_error("Exiting...");
	} else {
		if (MemFile_LoadFile(dataFile, Dir_File(file))) printf_error("Exiting...");
	}
}

void Rom_Build_SetAudioSegment(Rom* rom) {
	#define INST_ADDR(x, y) SegmentedToVirtual(0x0, ((x) - 0x7F588E60)), SegmentedToVirtual(0x0, ((y) - 0x7F588E60))
	#define RAM_ADDR rom->file.seekPoint + 0x7F588E60
	
	MemFile_Params(&rom->file, MEM_ALIGN, 16, MEM_END);
	MemFile_Seek(&rom->file, 0x00B65B70);
	
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
	#undef INST_ADDR
	#undef RAM_ADDR
}

void Rom_Build_SampleTable(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList itemList;
	MemFile sample = MemFile_Initialize();
	AudioEntryHead head = { 0 };
	AudioEntry entry = { 0 };
	
	MemFile_Malloc(&sample, MbToBin(0.25));
	MemFile_Reset(dataFile);
	Rom_ItemList(&itemList, false);
	MemFile_Params(dataFile, MEM_ALIGN, 16, MEM_REALLOC, true, MEM_END);
	
	for (s32 i = 0; i < itemList.num; i++) {
		printf_progress("Append Sample", i + 1, itemList.num);
		MemFile_Reset(config);
		MemFile_Reset(&sample);
		
		Dir_Enter(itemList.item[i]); {
			char* file = Dir_File("*.vadpcm.bin");
			char* cfg = Dir_File("config.cfg");
			f32 tuning;
			
			if (file == NULL)
				printf_error("Could not locate sample in [%s]", itemList.item[i]);
			
			if (MemFile_LoadFile(&sample, file))
				printf_error_align("Failed to load file", "%s", file);
			
			if (MemFile_LoadFile(config, cfg))
				printf_error_align("Failed to load file", "%s", cfg);
			
			Config_SuppressNext();
			tuning = Config_GetFloat(config, "tuning");
			
			if (tuning != 0) {
				sSampleTbl[sSampleTblNum].tuninOverride = tuning;
			}
			
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
	
	rom->offset.segment.smplRom = Dma_WriteEntry(rom, DMA_NO_ENTRY, dataFile);
	MemFile_Free(&sample);
	MemFile_Params(dataFile, MEM_ALIGN, 0, MEM_REALLOC, 0, MEM_END);
}

void Rom_Build_SoundFont(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList itemList;
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
	
	MemFile soundFontMem = MemFile_Initialize();
	
	MemFile_Malloc(&soundFontMem, MbToBin(2.00));
	
	MemFile_Malloc(&memBank, MbToBin(0.25));
	MemFile_Malloc(&memBook, MbToBin(0.25));
	MemFile_Malloc(&memLoopBook, MbToBin(0.25));
	MemFile_Malloc(&memInst, MbToBin(0.25));
	MemFile_Malloc(&memEnv, MbToBin(0.25));
	MemFile_Malloc(&memSample, MbToBin(0.25));
	MemFile_Malloc(&memSfx, MbToBin(0.25));
	MemFile_Malloc(&memDrum, MbToBin(0.25));
	
	Rom_ItemList(&itemList, true);
	
	sfHead.numEntries = itemList.num;
	SwapBE(sfHead.numEntries);
	MemFile_Write(&rom->mem.fontTbl, &sfHead, 16);
	
	for (s32 i = 0; i < itemList.num; i++) {
		ItemList listInst = { 0 };
		ItemList listSfx = { 0 };
		ItemList listDrum = { 0 };
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
			
			if (Dir_Stat("instrument/")) {
				Dir_Enter("instrument/");
				Dir_ItemList(&listInst, false);
				ItemList_NumericalSort(&listInst);
				
				Dir_Leave();
			}
			
			if (Dir_Stat("sfx/")) {
				Dir_Enter("sfx/");
				Dir_ItemList(&listSfx, false);
				ItemList_NumericalSort(&listSfx);
				
				Dir_Leave();
			}
			
			if (Dir_Stat("drum/")) {
				Dir_Enter("drum/");
				Dir_ItemList(&listDrum, false);
				ItemList_NumericalSort(&listDrum);
				
				Dir_Leave();
			}
			
			// DDDDDDDD SSSSSSSS Drum Segment | Sfx Segment
			MemFile_Write(&memBank, "\0\0\0\0\0\0\0\0", 8);
			
			for (s32 j = 0; j < listInst.num; j++) {
				char* restoreDir = Graph_GenStr(Dir_Current());
				Instrument instrument = { 0 };
				Adsr confEnv[4];
				char* sampleName[3];
				s32 sampleIndex[3] = { -1, -1, -1 };
				char* confSample[3] = {
					"low_sample",
					"prim_sample",
					"hi_sample",
				};
				char* confTuning[3] = {
					"low_tuning",
					"prim_tuning",
					"hi_tuning",
				};
				
				// List does not have entry for j index
				if (listInst.item[j] == NULL) {
					u32 null = 0xFFFF; // Empty Instrument Entry
					MemFile_Write(&memBank, &null, sizeof(u32));
					continue;
				}
				
				MemFile_Reset(config);
				MemFile_LoadFile_String(config, Dir_File("instrument/%s", listInst.item[j]));
				
				for (s32 soundID = 0; soundID < 3; soundID++) {
					sampleName[soundID] = Config_GetString(config, confSample[soundID]);
					if (!memcmp(sampleName[soundID], "NULL", 4)) {
						sampleName[soundID] = NULL;
					}
				}
				
				if (sampleName[0] == NULL && sampleName[1] == NULL && sampleName[2] == NULL) {
					u32 null = 0xFFFF; // Empty Instrument Entry
					MemFile_Write(&memBank, &null, sizeof(u32));
					
					continue;
				}
				
				for (s32 soundID = 0; soundID < 3; soundID++) {
					if (sampleName[soundID] == NULL)
						continue;
					
					for (s32 sampleID = 0;; sampleID++) {
						if (sampleID == sSampleTblNum)
							printf_error("Could not locate sample [%s]", sampleName[soundID]);
						
						if (!strcmp(sSampleTbl[sampleID].name, sampleName[soundID])) {
							instrument.sound[soundID].tuning = sSampleTbl[sampleID].tuninOverride;
							
							if (instrument.sound[soundID].tuning == 0)
								instrument.sound[soundID].tuning = Config_GetFloat(config, confTuning[soundID]);
							
							SwapBE(instrument.sound[soundID].swap32);
							sampleName[soundID] = NULL;
							sampleIndex[soundID] = sampleID;
							break;
						}
					}
				}
				
				instrument.loaded = Config_GetInt(config, "loaded");
				instrument.splitLo = Config_GetInt(config, "split_lo");
				instrument.splitHi = Config_GetInt(config, "split_hi");
				instrument.release = Config_GetInt(config, "release");
				confEnv[0].rate = Config_GetInt(config, "attack_rate"); SwapBE(confEnv[0].rate);
				confEnv[0].level = Config_GetInt(config, "attack_level"); SwapBE(confEnv[0].level);
				confEnv[1].rate = Config_GetInt(config, "hold_rate"); SwapBE(confEnv[1].rate);
				confEnv[1].level = Config_GetInt(config, "hold_level"); SwapBE(confEnv[1].level);
				confEnv[2].rate = Config_GetInt(config, "decay_rate"); SwapBE(confEnv[2].rate);
				confEnv[2].level = Config_GetInt(config, "decay_level"); SwapBE(confEnv[2].level);
				confEnv[3].rate = Config_GetInt(config, "decay2_rate"); SwapBE(confEnv[3].rate);
				confEnv[3].level = Config_GetInt(config, "decay2_level"); SwapBE(confEnv[3].level);
				
				if (!Lib_MemMem16(memEnv.data, memEnv.dataSize, confEnv, 0x10)) {
					instrument.envelope = memEnv.seekPoint;
					MemFile_Write(&memEnv, confEnv, 0x10);
					MemFile_Align(&memEnv, 16);
				} else {
					void* ptr = Lib_MemMem16(memEnv.data, memEnv.dataSize, confEnv, 0x10);
					instrument.envelope = (uPtr)ptr - (uPtr)memEnv.data;
				}
				
				for (s32 soundID = 0; soundID < 3; soundID++) {
					Sample smpl = { 0 };
					s32 sampleID = sampleIndex[soundID];
					u32 loop[4 + 8];
					u32 loopSize = 4 * 4;
					
					if (sampleID < 0)
						continue;
					
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
						Audio_LoadFile(dataFile, "*.loopbook.bin");
						for (s32 i = 0; i < 8; i++) {
							loop[4 + i] = dataFile->cast.u32[i];
						}
						
						loopSize = 4 * (4 + 8);
					}
					
					if (!Lib_MemMem16(memLoopBook.data, memLoopBook.dataSize, loop, loopSize)) {
						smpl.loop = memLoopBook.seekPoint;
						MemFile_Write(&memLoopBook, loop, loopSize);
						MemFile_Align(&memLoopBook, 16);
					} else {
						void* ptr = Lib_MemMem16(memLoopBook.data, memLoopBook.dataSize, loop, loopSize);
						smpl.loop = (uPtr)ptr - (uPtr)memLoopBook.data;
					}
					
					MemFile_Reset(dataFile);
					Audio_LoadFile(dataFile, "*.book.bin");
					if (!Lib_MemMem16(memBook.data, memBook.dataSize, dataFile->data, dataFile->dataSize)) {
						smpl.book = memBook.seekPoint;
						MemFile_Append(&memBook, dataFile);
						MemFile_Align(&memBook, 16);
					} else {
						void* ptr = Lib_MemMem16(memBook.data, memBook.dataSize, dataFile->data, dataFile->dataSize);
						smpl.book = (uPtr)ptr - (uPtr)memBook.data;
					}
					
					if (!Lib_MemMem16(memSample.data, memSample.dataSize, &smpl, sizeof(struct Sample))) {
						instrument.sound[soundID].sample = memSample.seekPoint;
						MemFile_Write(&memSample, &smpl, sizeof(struct Sample));
						smplNum++;
					} else {
						void* ptr = Lib_MemMem16(memSample.data, memSample.dataSize, &smpl, sizeof(struct Sample));
						instrument.sound[soundID].sample = (uPtr)ptr - (uPtr)memSample.data;
					}
					Dir_Set(restoreDir);
				}
				
				SwapBE(memInst.seekPoint);
				MemFile_Write(&memBank, &memInst.seekPoint, sizeof(u32));
				SwapBE(memInst.seekPoint);
				
				MemFile_Write(&memInst, &instrument, sizeof(struct Instrument));
			}
			
			for (s32 j = 0; j < listSfx.num; j++) {
				char* restoreDir = Graph_GenStr(Dir_Current());
				Sound sfx = { 0 };
				char* prim;
				
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
				
				/* SAMPLE */ {
					char* sample = prim;
					Sample smpl = { 0 };
					u32 loop[4 + 8];
					u32 loopSize = 4 * 4;
					
					s32 l = 0;
					for (;; l++) {
						if (l == sSampleTblNum)
							printf_error("Could not locate sample [%s]", sample);
						if (!strcmp(sSampleTbl[l].name, sample))
							break;
					}
					
					Dir_Set(sSampleTbl[l].dir);
					
					MemFile_Reset(config);
					MemFile_LoadFile_String(config, Dir_File("config.cfg"));
					
					if (sSampleTbl[l].tuninOverride > 0)
						sfx.tuning = sSampleTbl[l].tuninOverride;
					SwapBE(sfx.swap32);
					
					smpl.sampleAddr = ReadBE(sSampleTbl[l].segment);
					smpl.data = sSampleTbl[l].size;
					smpl.data |= Config_GetInt(config, "codec") << (32 - 4);
					smpl.data |= Config_GetInt(config, "medium") << (32 - 6);
					smpl.data |= Config_GetInt(config, "bitA") << (32 - 7);
					smpl.data |= Config_GetInt(config, "bitB") << (32 - 8);
					SwapBE(smpl.data);
					
					loop[0] = Config_GetInt(config, "loop_start");
					loop[1] = Config_GetInt(config, "loop_end");
					loop[2] = Config_GetInt(config, "loop_count");
					loop[3] = Config_GetInt(config, "tail_end");
					SwapBE(loop[0]);
					SwapBE(loop[1]);
					SwapBE(loop[2]);
					SwapBE(loop[3]);
					if (loop[2]) {
						MemFile_Reset(dataFile);
						Audio_LoadFile(dataFile, "*.loopbook.bin");
						for (s32 i = 0; i < 8; i++) {
							loop[4 + i] = dataFile->cast.u32[i];
						}
						
						loopSize = 4 * (4 + 8);
					}
					
					if (!Lib_MemMem16(memLoopBook.data, memLoopBook.dataSize, loop, loopSize)) {
						smpl.loop = memLoopBook.seekPoint;
						MemFile_Write(&memLoopBook, loop, loopSize);
						MemFile_Align(&memLoopBook, 16);
					} else {
						void* ptr = Lib_MemMem16(memLoopBook.data, memLoopBook.dataSize, loop, loopSize);
						smpl.loop = (uPtr)ptr - (uPtr)memLoopBook.data;
					}
					
					MemFile_Reset(dataFile);
					Audio_LoadFile(dataFile, "*.book.bin");
					if (!Lib_MemMem16(memBook.data, memBook.dataSize, dataFile->data, dataFile->dataSize)) {
						smpl.book = memBook.seekPoint;
						MemFile_Append(&memBook, dataFile);
						MemFile_Align(&memBook, 16);
					} else {
						void* ptr = Lib_MemMem16(memBook.data, memBook.dataSize, dataFile->data, dataFile->dataSize);
						smpl.book = (uPtr)ptr - (uPtr)memBook.data;
					}
					
					if (!Lib_MemMem16(memSample.data, memSample.dataSize, &smpl, sizeof(struct Sample))) {
						sfx.sample = memSample.seekPoint;
						MemFile_Write(&memSample, &smpl, sizeof(struct Sample));
						smplNum++;
					} else {
						void* ptr = Lib_MemMem16(memSample.data, memSample.dataSize, &smpl, sizeof(struct Sample));
						sfx.sample = (uPtr)ptr - (uPtr)memSample.data;
					}
				} Dir_Set(restoreDir);
				
				MemFile_Write(&memSfx, &sfx, sizeof(struct Sound));
			}
			
			for (s32 j = 0; j < listDrum.num; j++) {
				char* restoreDir = Graph_GenStr(Dir_Current());
				Drum drum = { 0 };
				Adsr confEnv[4] = { 0 };
				char* currentConf = Graph_GenStr(Dir_File("drum/%s", listDrum.item[j]));
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
				prim = Graph_GenStr(Config_GetString(config, "prim_sample"));
				
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
				
				confEnv[0].rate = Config_GetInt(config, "attack_rate"); SwapBE(confEnv[0].rate);
				confEnv[0].level = Config_GetInt(config, "attack_level"); SwapBE(confEnv[0].level);
				confEnv[1].rate = Config_GetInt(config, "hold_rate"); SwapBE(confEnv[1].rate);
				confEnv[1].level = Config_GetInt(config, "hold_level"); SwapBE(confEnv[1].level);
				confEnv[2].rate = Config_GetInt(config, "decay_rate"); SwapBE(confEnv[2].rate);
				confEnv[2].level = Config_GetInt(config, "decay_level"); SwapBE(confEnv[2].level);
				confEnv[3].rate = Config_GetInt(config, "decay2_rate"); SwapBE(confEnv[3].rate);
				confEnv[3].level = Config_GetInt(config, "decay2_level"); SwapBE(confEnv[3].level);
				
				if (!Lib_MemMem16(memEnv.data, memEnv.dataSize, confEnv, 0x10)) {
					drum.envelope = memEnv.seekPoint;
					MemFile_Write(&memEnv, confEnv, 0x10);
					MemFile_Align(&memEnv, 16);
				} else {
					void* ptr = Lib_MemMem16(memEnv.data, memEnv.dataSize, confEnv, 0x10);
					drum.envelope = (uPtr)ptr - (uPtr)memEnv.data;
				}
				
				/* SAMPLE */ {
					char* sample = prim;
					Sample smpl = { 0 };
					s32 sampleID = 0;
					u32 loop[4 + 8];
					u32 loopSize = 4 * 4;
					
					for (;; sampleID++) {
						if (sampleID == sSampleTblNum)
							printf_error("Could not locate sample [%s]", sample);
						if (!strcmp(sSampleTbl[sampleID].name, sample))
							break;
					}
					
					Dir_Set(sSampleTbl[sampleID].dir);
					MemFile_Reset(config);
					MemFile_LoadFile_String(config, Dir_File("config.cfg"));
					
					smpl.sampleAddr = ReadBE(sSampleTbl[sampleID].segment);
					smpl.data = sSampleTbl[sampleID].size;
					smpl.data |= Config_GetInt(config, "codec") << (32 - 4);
					smpl.data |= Config_GetInt(config, "medium") << (32 - 6);
					smpl.data |= Config_GetInt(config, "bitA") << (32 - 7);
					smpl.data |= Config_GetInt(config, "bitB") << (32 - 8);
					SwapBE(smpl.data);
					
					loop[0] = Config_GetInt(config, "loop_start");
					loop[1] = Config_GetInt(config, "loop_end");
					loop[2] = Config_GetInt(config, "loop_count");
					loop[3] = Config_GetInt(config, "tail_end");
					SwapBE(loop[0]);
					SwapBE(loop[1]);
					SwapBE(loop[2]);
					SwapBE(loop[3]);
					if (loop[2]) {
						MemFile_Reset(dataFile);
						Audio_LoadFile(dataFile, "*.loopbook.bin");
						for (s32 i = 0; i < 8; i++) {
							loop[4 + i] = dataFile->cast.u32[i];
						}
						
						loopSize = 4 * (4 + 8);
					}
					
					if (!Lib_MemMem16(memLoopBook.data, memLoopBook.dataSize, loop, loopSize)) {
						smpl.loop = memLoopBook.seekPoint;
						MemFile_Write(&memLoopBook, loop, loopSize);
						MemFile_Align(&memLoopBook, 16);
					} else {
						void* ptr = Lib_MemMem16(memLoopBook.data, memLoopBook.dataSize, loop, loopSize);
						smpl.loop = (uPtr)ptr - (uPtr)memLoopBook.data;
					}
					
					MemFile_Reset(dataFile);
					Audio_LoadFile(dataFile, "*.book.bin");
					smpl.book = memBook.seekPoint;
					if (!Lib_MemMem16(memBook.data, memBook.dataSize, dataFile->data, dataFile->dataSize)) {
						MemFile_Append(&memBook, dataFile);
						MemFile_Align(&memBook, 16);
					} else {
						void* ptr = Lib_MemMem16(memBook.data, memBook.dataSize, dataFile->data, dataFile->dataSize);
						smpl.book = (uPtr)ptr - (uPtr)memBook.data;
					}
					
					if (!Lib_MemMem16(memSample.data, memSample.dataSize, &smpl, sizeof(struct Sample))) {
						drum.sound.sample = memSample.seekPoint;
						MemFile_Write(&memSample, &smpl, sizeof(struct Sample));
						smplNum++;
					} else {
						void* ptr = Lib_MemMem16(memSample.data, memSample.dataSize, &smpl, sizeof(struct Sample));
						drum.sound.sample = (uPtr)ptr - (uPtr)memSample.data;
					}
					
					MemFile_Reset(config);
					MemFile_LoadFile_String(config, currentConf);
				} Dir_Set(restoreDir);
				
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
	}
	
	rom->offset.segment.fontRom = Dma_WriteEntry(rom, DMA_NO_ENTRY, &soundFontMem);
	
	MemFile_Free(&soundFontMem);
	
	MemFile_Free(&memBank);
	MemFile_Free(&memBook);
	MemFile_Free(&memInst);
	MemFile_Free(&memEnv);
	MemFile_Free(&memSample);
	MemFile_Free(&memSfx);
	MemFile_Free(&memDrum);
}

void Rom_Build_Sequence(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList itemList;
	MemFile memIndexTable = MemFile_Initialize();
	MemFile memLookUpTable = MemFile_Initialize();
	MemFile sequenceMem = MemFile_Initialize();
	AudioEntryHead sqHead = { 0 };
	AudioEntry sqEntry = { 0 };
	
	MemFile_Malloc(&memIndexTable, 0x1C0);
	MemFile_Malloc(&memLookUpTable, 0x1C0);
	MemFile_Malloc(&sequenceMem, MbToBin(1.0));
	Rom_ItemList(&itemList, true);
	
	sqHead.numEntries = itemList.num;
	SwapBE(sqHead.numEntries);
	MemFile_Write(&rom->mem.seqTbl, &sqHead, 16);
	
	for (s32 i = 0; i < itemList.num; i++) {
		printf_progress("Append Sequences", i + 1, itemList.num);
		u32 addr;
		u8 fontNum;
		
		// Skip "hardcoded" entries
		if (i == 0x7F || i == 0x80)
			continue;
		
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
				char* title = tprintf("bank_id_%d", j);
				u8 bankId = Config_GetInt(config, title);
				MemFile_Write(&memIndexTable, &bankId, 1);
			}
			
			Dir_Leave();
		}
	}
	
	u16 add = memLookUpTable.seekPoint;
	
	for (s32 i = 0; i < itemList.num; i++) {
		memLookUpTable.cast.u16[i] += add;
		SwapBE(memLookUpTable.cast.u16[i]);
	}
	MemFile_Append(&memLookUpTable, &memIndexTable);
	MemFile_Append(&rom->mem.seqFontTbl, &memLookUpTable);
	
	rom->offset.segment.seqRom = Dma_WriteEntry(rom, DMA_NO_ENTRY, &sequenceMem);
	
	MemFile_Free(&memIndexTable);
	MemFile_Free(&memLookUpTable);
	MemFile_Free(&sequenceMem);
}
