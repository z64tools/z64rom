#include "z64rom.h"
#include "Make.h"

extern u32 gThreadNum;

static const char* sGccBaseFlags;
static const char* sGccActorFlags;
static const char* sGccCodeFlags;
static const char* sGccKaleidoFlags;
static const char* sGccStateFlags;
static const char* sLinkerBaseFlags;
static const char* sLinkerCodeFlags;
static const char* sLinkerSceneFlags;
static const char* sLinkerULibFlags;
static volatile bool sMake = false;
static ItemList sDepList_uLibHeader;

static void Make_Info(const char* tool, const char* target) {
	printf_lock("[M]: %s\n", target);
	sMake = 1;
}

static void Make_Run(char* cmd) {
	strcat(cmd, " 2>&1");
	char* msg = SysExeO(cmd);
	
	if (strlen(msg) > 1 && (StrStrCase(msg, "warning") || StrStrCase(msg, "error"))) {
		char* word = CopyWord(msg, 0);
		
		StrRep(msg, "warning", PRNT_PRPL "warning" PRNT_RSET);
		StrRep(msg, "error", PRNT_REDD "error" PRNT_RSET);
		StrRep(msg, word, xFmt(PRNT_YELW "%s" PRNT_RSET, word));
		printf_WinFix();
		printf_lock("%s", msg);
	}
	Free(msg);
}

static void Make_Thread(Thread* thread, void (*func)(MakeArg*), void* arg) {
	if (gThreading)
		ThreadLock_Create(thread, func, arg);
	
	else
		func(arg);
}

// # # # # # # # # # # # # # # # # # # # #
// # Make_Object                         #
// # # # # # # # # # # # # # # # # # # # #

s32 sObjectCompile;

static ThreadFunc Object_Convert(MakeArg* arg) {
	char* cmd;
	char* in;
	char* out;
	char* cfg;
	char* header;
	char* linker;
	MemFile mem = MemFile_Initialize();
	bool isPlayas = false;
	char* mnf;
	
	FileSys_Path(arg->path);
	
	in = FileSys_FindFile("*.objex");
	mnf = FileSys_FindFile("*.mnf");
	cfg = FileSys_File("config.cfg");
	
	if (in == NULL)
		return;
	
	if (!Sys_Stat(cfg)) {
		MemFile m = MemFile_Initialize();
		
		MemFile_Alloc(&m, 0x256);
		
		Config_WriteInt(&m, "segment", 6, NO_COMMENT);
		Config_WriteFloat(&m, "scale", 100.0f, NO_COMMENT);
		
		MemFile_SaveFile_String(&m, cfg);
		MemFile_Free(&m);
	}
	
	out = xRep(xRep(in, ".objex", ".zobj"), "src/", "rom/");
	header = xFmt("src/lib_user/object/%s.h", xRep(PathSlot(in, -1), "/", ""));
	linker = xRep(xRep(header, "src/lib_user/", "include/"), ".h", ".ld");
	
	if (
		
		!gMakeForce
	   &&
		(
			Sys_Stat(out) > Sys_Stat(in) &&
			Sys_Stat(out) > Sys_Stat(in) &&
			Sys_Stat(out) > Sys_Stat(cfg) &&
			Sys_Stat(out) >= Sys_Stat(header) &&
			Sys_Stat(out) >= Sys_Stat(linker)
		)
	   &&
		(
			!mnf || (Sys_Stat(out) > Sys_Stat(mnf))
		)
	)
		return;
	
	Sys_MakeDir(Path(header));
	Sys_MakeDir(Path(linker));
	Sys_MakeDir(Path(out));
	
	MemFile_LoadFile_String(&mem, cfg);
	
	if (!Config_Variable(mem.str, "segment"))
		printf_error("No 'segment' in '%s'", arg->path);
	if (!Config_Variable(mem.str, "scale"))
		printf_error("No 'scale' in '%s'", arg->path);
	
	sObjectCompile = true;
	Malloc(cmd, 1024);
	Tools_Command(
		cmd,
		z64convert,
		"--silent "
		"--in %s "
		"--out %s "
		"--address 0x%08X "
		"--scale %ff "
		"--header %s "
		"--linker %s",
		in,
		out,
		(Config_GetInt(&mem, "segment") << 24),
		Config_GetFloat(&mem, "scale"),
		header,
		linker
	);
	
	if (StrStr(mem.str, "[playas]")) {
		isPlayas = true;
		catprintf(cmd, " --playas");
	}
	
	Make_Info("z64convert", out);
	Make_Run(cmd);
	
	if (isPlayas) {
		Config_GotoSection("playas");
		char* header = Config_GetStr(&mem, "header");
		char* patch = Config_GetStr(&mem, "patch");
		char* script = Config_GetStr(&mem, "script");
		char* bank = Config_GetStr(&mem, "bank");
		
		Tools_Command(
			cmd,
			z64playas,
			"--no-wait "
			"--silence "
			"--i %s "
			"--o %s "
			"--b %s "
			"--s %s "
			"--p %s "
			"--h %s "
			,
			out,
			out,
			FileSys_File(bank),
			FileSys_File(script),
			FileSys_File(patch),
			FileSys_File(header)
		);
		
		Make_Info("z64playas", out);
		Make_Run(cmd);
	}
	
	MemFile_Free(&mem);
	Free(cmd);
}

void Make_Object(void) {
	ItemList list = ItemList_Initialize();
	MakeArg* targ;
	Thread* thread;
	s32 i = 0;
	
	if (!Sys_Stat("src/object/"))
		return;
	
	Calloc(targ, sizeof(MakeArg) * gThreadNum);
	Calloc(thread, sizeof(Thread) * gThreadNum);
	
	ItemList_List(&list, "src/object/", 0, LIST_FOLDERS | LIST_NO_DOT);
	
	if (list.num == 0)
		goto free;
	
	if (gThreading)
		ThreadLock_Init();
	
	while (i < list.num) {
		u32 target = Clamp(list.num - i, 0, gThreadNum);
		
		for (s32 j = 0; j < target; j++) {
			targ[j].path = list.item[i + j];
			
			if (gThreading) {
				ThreadLock_Create(&thread[j], Object_Convert, &targ[j]);
			} else {
				Object_Convert(&targ[j]);
			}
		}
		
		if (gThreading)
			for (s32 j = 0; j < target; j++)
				ThreadLock_Join(&thread[j]);
		
		i += gThreadNum;
	}
	
	if (gThreading)
		ThreadLock_Free();
	
	if (sObjectCompile) {
		MemFile mem = MemFile_Initialize();
		MemFile mout = MemFile_Initialize();
		ItemList ldFiles = ItemList_Initialize();
		s32 crc = false;
		u8* digestA;
		u8* digestB;
		
		MemFile_Alloc(&mem, MbToBin(2));
		MemFile_Alloc(&mout, MbToBin(2));
		MemFile_Params(&mem, MEM_REALLOC, true, MEM_END);
		MemFile_Params(&mout, MEM_REALLOC, true, MEM_END);
		
		if (Sys_Stat("include/z_object_user.ld")) {
			MemFile_LoadFile_String(&mout, "include/z_object_user.ld");
			crc = true;
			digestA = Sys_Sha256(mout.data, mout.size);
			MemFile_Reset(&mout);
		}
		
		ItemList_SetFilter(&ldFiles, CONTAIN_END, ".ld");
		ItemList_List(&ldFiles, "include/object/", 0, LIST_FILES | LIST_NO_DOT);
		
		for (s32 j = 0; j < ldFiles.num; j++) {
			MemFile_LoadFile_String(&mem, ldFiles.item[j]);
			MemFile_Append(&mout, &mem);
			MemFile_Printf(&mout, "\n");
		}
		
		if (crc) {
			digestB = Sys_Sha256(mout.data, mout.size);
			if (memcmp(digestA, digestB, 32))
				MemFile_SaveFile_String(&mout, "include/z_object_user.ld");
			Free(digestA);
			Free(digestB);
		} else
			MemFile_SaveFile_String(&mout, "include/z_object_user.ld");
		
		ItemList_Free(&ldFiles);
		MemFile_Free(&mout);
		MemFile_Free(&mem);
	}
	
free:
	
	Free(targ);
	Free(thread);
	ItemList_Free(&list);
}

// # # # # # # # # # # # # # # # # # # # #
// # Make_Sound                          #
// # # # # # # # # # # # # # # # # # # # #

static ThreadFunc Sequence_Convert(MakeArg* targ) {
	ItemList* list;
	char cmd[512];
	char* cfg;
	char* seq = NULL;
	char* midi = NULL;
	char* mus = NULL;
	u32 index = Value_Hex(PathSlot(targ->path, -1));
	
	Calloc(list, sizeof(ItemList));
	*list = ItemList_Initialize();
	
	FileSys_Path(targ->path);
	
	if ((midi = FileSys_FindFile(".mid"))) {
		MemFile midCfg = MemFile_Initialize();
		char* thisCfg = FileSys_File("config.cfg");
		
		cfg = xRep(FileSys_File("config.cfg"), "src/", "rom/");
		seq = xRep(FileSys_File("sequence.aseq"), "src/", "rom/");
		
		Sys_MakeDir(Path(seq));
		
		if (Sys_Stat(seq) > Sys_Stat(midi) &&
			Sys_Stat(cfg) > Sys_Stat(midi) &&
			(!thisCfg || Sys_Stat(seq) > Sys_Stat(thisCfg)) &&
			!gMakeForce)
			goto free;
		
		if (!Sys_Stat(thisCfg)) {
			ItemList van = ItemList_Initialize();
			ItemList_List(&van, xFmt("rom/sound/sequence/%s/", gVanilla), 0, LIST_FOLDERS);
			
			if (Sys_Stat(van.item[index]))
				Sys_Copy(xFmt("%sconfig.cfg", van.item[index]), thisCfg);
			
			ItemList_Free(&van);
		}
		
		MemFile_LoadFile_String(&midCfg, thisCfg);
		Sys_Copy(thisCfg, cfg);
		
		u8 masterVolume = 88;
		bool flStudio = false;
		bool loop = true;
		
		if (StrStr(midCfg.str, "[seq64]")) {
			Config_GotoSection("seq64");
			if (Config_Variable(midCfg.str, "master_volume"))
				masterVolume = Config_GetInt(&midCfg, "master_volume");
			
			if (Config_Variable(midCfg.str, "flstudio"))
				flStudio = Config_GetBool(&midCfg, "flstudio");
			
			if (Config_Variable(midCfg.str, "loop"))
				loop = Config_GetBool(&midCfg, "loop");
			Config_GotoSection("NULL");
		}
		
		MemFile_Free(&midCfg);
		
		if (Config_GetErrorState())
			printf_error("Missing values in config... Delete the current config to generate generic config in the next build!");
		
		Tools_Command(cmd, seq64, "--in=\"%s\" --out=\"%s\" --abi=Zelda --pref=false", midi, seq);
		
		strcat(cmd, xFmt(" --mastervol=0x%X", masterVolume));
		if (flStudio)
			strcat(cmd, " --flstudio=true");
		if (!loop)
			strcat(cmd, " --smartloop=false");
		
		Sys_MakeDir(Path(seq));
		Make_Run(cmd);
		Make_Info("seq64", seq);
		
		goto free;
	}
	
	if ((mus = FileSys_FindFile(".mus"))) {
		char* thisCfg = FileSys_File("config.cfg");
		
		cfg = xRep(FileSys_File("config.cfg"), "src/", "rom/");
		seq = xRep(FileSys_File("sequence.aseq"), "src/", "rom/");
		
		Sys_MakeDir(Path(seq));
		
		if (Sys_Stat(seq) > Sys_Stat(mus) && Sys_Stat(cfg) && !gMakeForce)
			goto free;
		
		if (!Sys_Stat(thisCfg)) {
			ItemList van = ItemList_Initialize();
			ItemList_List(&van, xFmt("rom/sound/sequence/%s/", gVanilla), 0, LIST_FOLDERS);
			
			if (Sys_Stat(van.item[index]))
				Sys_Copy(xFmt("%sconfig.cfg", van.item[index]), thisCfg);
			
			ItemList_Free(&van);
		}
		
		Sys_Copy(thisCfg, cfg);
		
		Tools_Command(cmd, seqas, "\"%s\" \"%s\"", mus, seq);
		Make_Run(cmd);
		Make_Info("seq-assembler", seq);
		
		goto free;
	}
	
free:
	ItemList_Free(list);
	Free(list);
}

void Make_Sequence(void) {
	ItemList list = ItemList_Initialize();
	MakeArg* targ;
	Thread* thread;
	s32 i = 0;
	
	if (!Sys_Stat("src/sound/sequence/"))
		return;
	
	Calloc(targ, sizeof(MakeArg) * gThreadNum);
	Calloc(thread, sizeof(Thread) * gThreadNum);
	
	ItemList_List(&list, "src/sound/sequence/", 0, LIST_FOLDERS | LIST_NO_DOT);
	
	if (list.num == 0)
		goto free;
	
	if (gThreading)
		ThreadLock_Init();
	
	while (i < list.num) {
		u32 target = Clamp(list.num - i, 0, gThreadNum);
		
		for (s32 j = 0; j < target; j++) {
			targ[j].path = list.item[i + j];
			
			if (gThreading) {
				ThreadLock_Create(&thread[j], Sequence_Convert, &targ[j]);
			} else {
				Sequence_Convert(&targ[j]);
			}
		}
		
		if (gThreading)
			for (s32 j = 0; j < target; j++)
				ThreadLock_Join(&thread[j]);
		
		i += gThreadNum;
	}
	if (gThreading)
		ThreadLock_Free();
	
free:
	
	Free(targ);
	Free(thread);
	ItemList_Free(&list);
}

static ThreadFunc Sound_Convert(MakeArg* targ) {
	char* vadpcm = NULL;
	char* audio = NULL;
	char* book = NULL;
	char* table = NULL;
	char* sampleCfg = NULL;
	char* vadCfg = NULL;
	char* vanillaCfg = NULL;
	MemFile cfgMem = MemFile_Initialize();
	
	FileSys_Path(targ->path);
	
	char* wav = FileSys_FindFile("*.wav");
	char* aiff = FileSys_FindFile("*.aiff");
	char* mp3 = FileSys_FindFile("*.mp3");
	
	// Get newest audio file
	if (Sys_Stat(wav) > (Sys_Stat(aiff)))
		audio = wav;
	else
		audio = aiff;
	if (Sys_Stat(audio) < Sys_Stat(mp3))
		audio = mp3;
	
	if (audio == NULL)
		goto free;
	
	FileSys_Path(xRep(targ->path, "src/", "rom/"));
	Sys_MakeDir(FileSys_File(""));
	
	vadpcm = FileSys_FindFile("*.vadpcm.bin");
	book = FileSys_FindFile("*.book.bin");
	sampleCfg = xRep(FileSys_File("config.cfg"), "rom/", "src/");
	vanillaCfg = xRep(FileSys_File("config.cfg"), "rom/sound/sample/", xFmt("rom/sound/sample/%s/", gVanilla));
	
	// Generate Sample Config
	bool normalize = true;
	bool inherit = false;
	bool halfPrecision = false;
	
	if (!Sys_Stat(sampleCfg)) {
		MemFile_Alloc(&cfgMem, 0x2000);
write:
		Config_WriteBool(&cfgMem, "normalize", normalize, "Maximum volume");
		Config_WriteBool(&cfgMem, "inherit_vanilla", inherit, "If vanilla config exists, inherit pitch from it");
		Config_WriteBool(&cfgMem, "half_precision", halfPrecision, "Rough compression");
		
		MemFile_SaveFile_String(&cfgMem, sampleCfg);
	} else {
		u32 rewrite = false;
		MemFile_LoadFile_String(&cfgMem, sampleCfg);
		
		if (StrStr(cfgMem.str, "[z64rom]")) {
			Config_GotoSection("z64rom");
			
			if (Config_Variable(cfgMem.str, "normalize"))
				normalize = Config_GetBool(&cfgMem, "normalize");
			
			if (Config_Variable(cfgMem.str, "inherit_vanilla"))
				inherit = Config_GetBool(&cfgMem, "inherit_vanilla");
			
			if (Config_Variable(cfgMem.str, "half_precision"))
				halfPrecision = Config_GetBool(&cfgMem, "half_precision");
			
			Config_GotoSection(NULL);
			rewrite = true;
		} else {
			if (Config_Variable(cfgMem.str, "normalize"))
				normalize = Config_GetBool(&cfgMem, "normalize");
			else rewrite = true;
			
			if (Config_Variable(cfgMem.str, "inherit_vanilla"))
				inherit = Config_GetBool(&cfgMem, "inherit_vanilla");
			else rewrite = true;
			
			if (Config_Variable(cfgMem.str, "half_precision"))
				halfPrecision = Config_GetBool(&cfgMem, "half_precision");
			else rewrite = true;
		}
		
		if (rewrite) {
			MemFile_Reset(&cfgMem);
			MemFile_Realloc(&cfgMem, cfgMem.memSize * 4);
			goto write;
		}
	}
	
	if (vadpcm == NULL || (Sys_Stat(sampleCfg) > Sys_Stat(vadpcm)) || (Sys_Stat(audio) > Sys_Stat(vadpcm)) || gMakeForce) {
		char command[2056];
		
		if (vadpcm == NULL)
			vadpcm = xFmt("%ssample.bin", xRep(targ->path, "src/", "rom/"));
		else
			StrRep(vadpcm, ".vadpcm", "");
		
		Log("Audio [%s] Vadpcm [%s]", audio, vadpcm);
		
		Tools_Command(command, z64audio, "-S --i \"%s\" --o \"%s\"", audio, vadpcm);
		if (table)
			catprintf(command, " --design \"%s\"", table);
		else if (book)
			catprintf(command, " --book \"%s\"", book);
		if (normalize)
			catprintf(command, " --m --n");
		if (halfPrecision)
			catprintf(command, " --half-precision");
		
		if (inherit && Sys_Stat(vanillaCfg)) {
			MemFile mem = MemFile_Initialize();
			
			Log("inherit [%s]", vanillaCfg);
			MemFile_LoadFile_String(&mem, vanillaCfg);
			catprintf(
				command,
				" --basenote %d --finetune %d",
				Config_GetInt(&mem, "basenote"),
				Config_GetInt(&mem, "finetune")
			);
			MemFile_Free(&mem);
		}
		
		Make_Run(command);
		Make_Info("z64audio", audio);
	}
	
free:
	MemFile_Free(&cfgMem);
}

void Make_Sound(void) {
	ItemList list = ItemList_Initialize();
	MakeArg* targ;
	Thread* thread;
	s32 i = 0;
	
	if (!Sys_Stat("src/sound/sample/"))
		return;
	
	Calloc(targ, sizeof(MakeArg) * gThreadNum);
	Calloc(thread, sizeof(Thread) * gThreadNum);
	
	ItemList_List(&list, "src/sound/sample/", 0, LIST_FOLDERS | LIST_NO_DOT);
	
	if (list.num == 0)
		goto free;
	
	if (gThreading)
		ThreadLock_Init();
	
	while (i < list.num) {
		u32 target = Clamp(list.num - i, 0, gThreadNum);
		
		for (s32 j = 0; j < target; j++) {
			targ[j].path = list.item[i + j];
			
			if (gThreading) {
				ThreadLock_Create(&thread[j], Sound_Convert, &targ[j]);
			} else {
				Sound_Convert(&targ[j]);
			}
		}
		
		if (gThreading)
			for (s32 j = 0; j < target; j++)
				ThreadLock_Join(&thread[j]);
		
		i += gThreadNum;
	}
	if (gThreading)
		ThreadLock_Free();
	
free:
	Free(targ);
	Free(thread);
	ItemList_Free(&list);
}

// # # # # # # # # # # # # # # # # # # # #
// # Callback_Helper                     #
// # # # # # # # # # # # # # # # # # # # #

static s32 Callback_Dependencies_PreGcc(const char* input, const char* output, const char* cfg, MemFile* make, ItemList* dep2) {
	if (Sys_Stat(cfg))
		MemFile_LoadFile_String(make, cfg);
	
	if (gMakeForce)
		return true;
	
	if (!Sys_Stat(output) || (Sys_Stat(input) > Sys_Stat(output)))
		return true;
	
	if (Sys_Stat(cfg)) {
		ItemList dep;
		ItemList listA = ItemList_Initialize();
		ItemList listB = ItemList_Initialize();
		
		if (Config_Variable(make->str, "dependencies"))
			Config_GetArray(make, "dependencies", &listA);
		
		Config_GotoSection(Basename(input));
		if (Config_Variable(make->str, "dependencies"))
			Config_GetArray(make, "dependencies", &listB);
		Config_GotoSection(NULL);
		
		ItemList_Combine(&dep, &listA, &listB);
		ItemList_Free(&listA);
		ItemList_Free(&listB);
		
		for (s32 i = 0; i < dep.num; i++) {
			if (Sys_Stat(xFmt("%s%s", Path(input), dep.item[i])) > Sys_Stat(output)) {
				ItemList_Free(&dep);
				
				return true;
			}
		}
		
		ItemList_Free(&dep);
	}
	
	if (dep2)
		for (s32 i = 0; i < dep2->num; i++)
			if (StrEnd(dep2->item[i], ".h"))
				if (Sys_Stat(dep2->item[i]) > Sys_Stat(output))
					return true;
	
	return false;
}

static s32 Callback_Dependencies_PreLd(const char* input, const char* output) {
	ItemList list = ItemList_Initialize();
	s32 ret = false;
	Time max;
	
	ItemList_Separated(&list, input, ' ');
	Log_ItemList(&list);   // Possibly something wrong?
	max = ItemList_StatMax(&list);
	
	if (!Sys_Stat(output) || max > (Sys_Stat(output)))
		ret = true;
	
	else if (Sys_Stat("include/z_lib_user.ld") > Sys_Stat(output))
		ret = true;
	
	else if (Sys_Stat("include/z_object_user.ld") > Sys_Stat(output))
		ret = true;
	
	ItemList_Free(&list);
	
	return ret;
}

static s32 Callback_Overlay_PreGcc(const char* input, const char* output, const char* cfg, MemFile* make) {
	if (Callback_Dependencies_PreGcc(input, output, cfg, make, &sDepList_uLibHeader))
		return CB_MAKE;
	
	return CB_BREAK;
}

// # # # # # # # # # # # # # # # # # # # #
// # Callback                            #
// # # # # # # # # # # # # # # # # # # # #

volatile f32 sTimeGCC, sTimeLD, sTimeZOVL, sTimeObjDump;
volatile u32 sCountGCC, sCountLD, sCountZOVL, sCounObjDump;

static s32 Callback_Kaleido(const char* input, MakeCallType type, const char* output, void* arg) {
	char* ovl;
	char* conf;
	
	if (type == PRE_GCC) {
		char* cfg = xFmt("%smake.cfg", Path(input));
		MemFile* make = arg;
		
		return Callback_Overlay_PreGcc(input, output, cfg, make);
	}
	
	if (type == PRE_LD) {
		if (Callback_Dependencies_PreLd(input, output))
			return CB_MAKE;
		
		return CB_BREAK;
	}
	
	if (type == POST_LD) {
		char* command = arg;
		char* info;
		char* dump;
		
		ovl = xFmt("%soverlay.zovl", Path(input));
		StrRep(ovl, "src/", "rom/");
		
		conf = xFmt("%sconfig.cfg", Path(input));
		StrRep(conf, "src/", "rom/");
		
		Tools_Command(command, mips64_objdump, "-t %s", input);
		dump = SysExeO(command); {
			MemFile newConf = MemFile_Initialize();
			u32 init, dest, updt, draw;
			char* str;
			
			if ((str = StrStr(dump, "__z64_init")) == NULL)
				printf_error("Could not find symbol [__z64_init] from [%s]", input);
			init = Value_Hex(LineHead(str, dump));
			
			if ((str = StrStr(dump, "__z64_dest")) == NULL)
				printf_error("Could not find symbol [__z64_dest] from [%s]", input);
			dest = Value_Hex(LineHead(str, dump));
			
			if ((str = StrStr(dump, "__z64_updt")) == NULL)
				printf_error("Could not find symbol [__z64_updt] from [%s]", input);
			updt = Value_Hex(LineHead(str, dump));
			
			if ((str = StrStr(dump, "__z64_draw")) == NULL)
				printf_error("Could not find symbol [__z64_draw] from [%s]", input);
			draw = Value_Hex(LineHead(str, dump));
			
			MemFile_Alloc(&newConf, 0x800);
			MemFile_Printf(&newConf, "# %s\n", Basename(input));
			Config_WriteHex(&newConf, "vram_addr", 0x80800000, NO_COMMENT);
			Config_WriteHex(&newConf, "init", init, NO_COMMENT);
			Config_WriteHex(&newConf, "dest", dest, NO_COMMENT);
			Config_WriteHex(&newConf, "updt", updt, NO_COMMENT);
			Config_WriteHex(&newConf, "draw", draw, NO_COMMENT);
			
			if (MemFile_SaveFile_String(&newConf, conf)) printf_error("Could not save [%s]", conf);
			
			MemFile_Free(&newConf);
		}
		
		info = PathSlot(input, -1);
		StrRep(info, "/", " ");
		
		Tools_Command(command, nOVL, "-v -c -s -A 0x80800000 -o %s %s", ovl, input);
		Time_Start(1);
		Make_Run(command);
		sTimeZOVL += Time_Get(1);
		sCountZOVL++;
		
		Free(dump);
		
		return 0;
	}
	
	return 0;
}

static s32 Callback_System(const char* input, MakeCallType type, const char* output, void* arg) {
	char* ovl = NULL;
	
	if (type == PRE_GCC) {
		char* cfg = xFmt("%smake.cfg", Path(input));
		MemFile* make = arg;
		
		return Callback_Overlay_PreGcc(input, output, cfg, make);
	}
	
	if (type == PRE_LD) {
		if (Callback_Dependencies_PreLd(input, output))
			return CB_MAKE;
		
		return CB_BREAK;
	}
	
	if (type == POST_LD) {
		char* command = arg;
		char* info;
		char* dump;
		
		ovl = xFmt("%soverlay.zovl", Path(input));
		StrRep(ovl, "src/", "rom/");
		
		info = PathSlot(input, -1);
		if (StrMtch(info, "0x"))
			StrRem(info, strlen("0x0000-"));
		StrRep(info, "/", " ");
		
		Tools_Command(command, nOVL, "-v -c -s -A 0x80800000 -o %s %s", ovl, input);
		Time_Start(1);
		Make_Run(command);
		sTimeZOVL += Time_Get(1);
		sCountZOVL++;
		
		Tools_Command(command, mips64_objdump, "-t %s", input);
		dump = SysExeO(command);
		
		char* config = xFmt("%sconfig.cfg", Path(input));
		MemFile mem = MemFile_Initialize();
		char* stateInit = LineHead(StrStr(dump, "__z64_init"), dump);
		char* stateDestroy = LineHead(StrStr(dump, "__z64_dest"), dump);
		
		if (stateInit == NULL)
			printf_error_align("No StateInit", "%s", input);
		if (stateDestroy == NULL)
			printf_error_align("No StateDestroy", "%s", input);
		
		MemFile_Alloc(&mem, 0x800);
		MemFile_Printf(&mem, "# %s\n\n", Basename(input));
		MemFile_Printf(&mem, "vram_addr = 0x80800000\n");
		MemFile_Printf(&mem, "init_func = 0x%.8s\n", stateInit);
		MemFile_Printf(&mem, "dest_func = 0x%.8s\n", stateDestroy);
		
		MemFile_SaveFile_String(&mem, config);
		MemFile_Free(&mem);
		
		Free(dump);
	}
	
	return 0;
}

static s32 Callback_Actor(const char* input, MakeCallType type, const char* output, void* arg) {
	char* ovl;
	char* conf;
	
	if (type == PRE_GCC) {
		char* cfg = xFmt("%smake.cfg", Path(input));
		MemFile* make = arg;
		
		return Callback_Overlay_PreGcc(input, output, cfg, make);
	}
	
	if (type == PRE_LD) {
		if (Callback_Dependencies_PreLd(input, output))
			return CB_MAKE;
		
		return CB_BREAK;
	}
	
	if (type == POST_LD) {
		char* command = arg;
		char* info;
		char* dump;
		
		ovl = xFmt("%soverlay.zovl", Path(input));
		StrRep(ovl, "src/", "rom/");
		
		conf = xFmt("%sconfig.cfg", Path(input));
		StrRep(conf, "src/", "rom/");
		
		Tools_Command(command, mips64_objdump, "-t %s", input);
		dump = SysExeO(command); {
			MemFile srcFile = MemFile_Initialize();
			MemFile newConf = MemFile_Initialize();
			char* sourceFolder = Path(input);
			char* temp;
			char* varName;
			ItemList list = ItemList_Initialize();
			
			StrRep(sourceFolder, "rom/", "src/");
			ItemList_List(&list, sourceFolder, -1, LIST_FILES | LIST_NO_DOT);
			
			MemFile_Alloc(&srcFile, MbToBin(1.0f));
			MemFile_Params(&srcFile, MEM_REALLOC, true, MEM_END);
			MemFile_Alloc(&newConf, MbToBin(1.0f));
			
			for (s32 i = 0; i < list.num; i++) {
				if (!StrEndCase(list.item[i], ".c"))
					continue;
				
				MemFile_Clear(&srcFile);
				if (MemFile_LoadFile_String(&srcFile, list.item[i]))
					printf_error("Could not open file [%s]", list.item[i]);
				
				temp = StrStr(srcFile.str, "\nActorInit ");
				if (temp)
					temp += strlen("\nActorInit ");
				
				else {
					temp = StrStr(srcFile.str, "\nconst ActorInit ");
					if (temp)
						temp += strlen("\nconst ActorInit ");
					
					else {
						temp = StrStr(srcFile.str, "ActorInit ");
						if (temp)
							temp += strlen("ActorInit ");
					}
				}
				
				if (temp)
					break;
			}
			
			if (temp == NULL)
				printf_error("Could not locate [ActorInit] from files in [%s]", sourceFolder);
			
			while (*temp <= ' ') temp++;
			
			varName = StrDupX(temp, 64);
			if (StrStr(varName, " ")) ((char*)StrStr(varName, " "))[0] = '\0';
			if (StrStr(varName, "=")) ((char*)StrStr(varName, "="))[0] = '\0';
			
			temp = LineHead(StrStrWhole(dump, varName), dump);
			
			MemFile_Printf(&newConf, "# %s\n", Basename(input));
			MemFile_Printf(&newConf, "alloc_type = 0\n");
			MemFile_Printf(&newConf, "vram_addr  = 0x80800000\n");
			MemFile_Printf(&newConf, "# %s\n", CopyLine(temp, 0));
			MemFile_Printf(&newConf, "init_vars  = 0x%.8s\n", temp);
			
			if (MemFile_SaveFile_String(&newConf, conf)) printf_error("Could not save [%s]", conf);
			
			ItemList_Free(&list);
			MemFile_Free(&srcFile);
			MemFile_Free(&newConf);
			Free(varName);
		}
		
		info = PathSlot(input, -1); StrRem(info, strlen("0x0000-")); StrRep(info, "/", " ");
		
		Tools_Command(command, nOVL, "-v -c -s -A 0x80800000 -o %s %s", ovl, input);
		Time_Start(1);
		Make_Run(command);
		sTimeZOVL += Time_Get(1);
		sCountZOVL++;
		
		Free(dump);
		
		return 0;
	}
	
	return 0;
}

static s32 Callback_Code(const char* input, MakeCallType type, const char* output, void* arg) {
	if (type == PRE_GCC) {
		char* cfg = xFmt("%smake.cfg", Path(input));
		MemFile* make = arg;
		
		if (Callback_Dependencies_PreGcc(input, output, cfg, make, &sDepList_uLibHeader))
			return CB_MAKE;
		
		return CB_BREAK;
	}
	
	if (type == POST_GCC)
		return 0;
	
	if (type == PRE_LD) {
		if (Callback_Dependencies_PreLd(input, output)) {
			u32* entryPoint = arg;
			MemFile mem = MemFile_Initialize();
			MemFile __config = MemFile_Initialize();
			MemFile* config = &__config;
			char* c = xAlloc(strlen(input) + 0x10);
			char* z64rom;
			char* z64ram;
			char* z64next;
			
			strcpy(c, input);
			if (!StrRep(c, "rom/", "src/")) goto error;
			if (!StrRep(c, ".o", ".c")) goto error;
			
			if (!Sys_Stat(c))
				StrRep(c, ".c", ".s");
			if (!Sys_Stat(c))
				goto error;
			
			MemFile_LoadFile_String(&mem, c);
			z64ram = StrStr(mem.str, "z64ram = ");
			z64rom = StrStr(mem.str, "z64rom = ");
			z64next = StrStr(mem.str, "z64next = ");
			
			if (z64ram == NULL) printf_error_align("No RAM Address:", "[%s]", Filename(c));
			if (z64rom == NULL) printf_error_align("No ROM Address:", "[%s]", Filename(c));
			
			z64ram += strlen("z64ram = ");
			z64rom += strlen("z64rom = ");
			if (z64next) z64next += strlen("z64next = ");
			
			MemFile_Alloc(config, 0x280);
			Config_WriteHex(config, "rom", Value_Hex(z64rom), NO_COMMENT);
			Config_WriteHex(config, "ram", Value_Hex(z64ram), NO_COMMENT);
			if (z64next) Config_WriteHex(config, "next", Value_Hex(z64next), NO_COMMENT);
			entryPoint[0] = Value_Hex(z64ram);
			
			strcpy(c, input);
			if (!StrRep(c, ".o", ".cfg")) goto error;
			
			MemFile_SaveFile_String(config, c);
			
			MemFile_Free(&mem);
			MemFile_Free(config);
			
			return CB_MAKE;
error:
			printf_error_align("StrRep", "[%s] -> [%s]", input, c);
		}
		
		return CB_BREAK;
	}
	
	if (type == POST_LD) {
		char* bin;
		char* command = arg;
		
		bin = xAlloc(strlen(input) + 8);
		strcpy(bin, input);
		StrRep(bin, ".elf", ".bin");
		
		Tools_Command(command, mips64_objcopy, "-R .MIPS.abiflags -O binary %s %s", input, bin);
		Make_Run(command);
	}
	
	return 0;
}

// # # # # # # # # # # # # # # # # # # # #
// # Binutil                             #
// # # # # # # # # # # # # # # # # # # # #

static void Binutil_GCC(const char* source, const char* output, const char* flags, BinutilCallback callback) {
	char* command;
	MemFile make = MemFile_Initialize();
	s32 r = 0;
	
	if (callback) {
		switch ((r = callback(source, PRE_GCC, output, &make))) {
			case CB_BREAK:
				goto free;
			case CB_MAKE:
				goto build;
			default:
				printf_error("Unrecognized Callback Return [%d]", r);
		}
	} else {
		char* cfg = xFmt("%smake.cfg", Path(source));
		
		if (false == Callback_Dependencies_PreGcc(source, output, cfg, &make, &sDepList_uLibHeader))
			goto free;
	}
	
build:
	(void)0;
	char* newFlags = NULL;
	char* basename = Basename(source);
	
	Calloc(command, 2048);
	
	if (make.data) {
		char* var;
		
		if ((var = Config_Variable(make.str, "gcc_flags"))) {
			newFlags = StrDupX(Config_GetVariable(make.str, "gcc_flags"), 1024 * 2);
		}
		
		Config_GotoSection(basename);
		if ((var = Config_Variable(make.str, "gcc_flags"))) {
			if (!newFlags)
				newFlags = StrDupX(Config_GetVariable(make.str, "gcc_flags"), 1024 * 2);
			else
				catprintf(newFlags, " %s", Config_GetVariable(make.str, "gcc_flags"));
		}
		
		if (newFlags)
			flags = newFlags;
		
		Config_GotoSection(NULL);
	}
	
	Tools_Command(command, mips64_gcc, "%s %s %s -o %s", sGccBaseFlags, flags, source, output);
	Sys_MakeDir(output);
	
	Time_Start(1);
	Make_Run(command);
	Make_Info("GCC", output);
	sTimeGCC += Time_Get(1);
	sCountGCC++;
	
	if (callback)
		callback(output, POST_GCC, output, command);
	Free(command);
	if (newFlags)
		Free(newFlags);
	
free:
	MemFile_Free(&make);
}

static void Binutil_GCC_Assembly(const char* source, const char* output, BinutilCallback callback) {
	char* command;
	MemFile make = MemFile_Initialize();
	s32 r = 0;
	
	if (callback) {
		switch ((r = callback(source, PRE_GCC, output, &make))) {
			case CB_BREAK:
				goto free;
			case CB_MAKE:
				goto build;
			default:
				printf_error("Unrecognized Callback Return [%d]", r);
		}
	} else {
		char* cfg = xFmt("%smake.cfg", Path(source));
		
		if (false == Callback_Dependencies_PreGcc(source, output, cfg, &make, NULL))
			goto free;
	}
	
build:
	(void)0;
	
	Calloc(command, 2048);
	
	Tools_Command(command, mips64_gcc, "-c -x assembler-with-cpp -Wa,--no-pad-sections %s -o %s %s %s", source, output, sLinkerBaseFlags, sLinkerCodeFlags);
	StrRep(command, "--emit-relocs", "");
	Sys_MakeDir(output);
	
	Time_Start(1);
	Make_Run(command);
	Make_Info("ASM", output);
	sTimeGCC += Time_Get(1);
	sCountGCC++;
	
	if (callback)
		callback(output, POST_GCC, output, command);
	Free(command);
	
free:
	MemFile_Free(&make);
}

static void Binutil_LD(const char* source, const char* output, const char* flags, BinutilCallback callback) {
	MemFile entry = MemFile_Initialize();
	char* command;
	char entryDir[1024] = { 0 };
	u32 entryPoint = 0x80800000;
	s32 r = 0;
	
	if (callback) {
		switch ((r = callback(source, PRE_LD, (void*)output, &entryPoint))) {
			case CB_BREAK:
				return;
			case CB_MAKE:
				goto build;
			default:
				printf_error("Unrecognized Callback Return [%d]", r);
		}
	} else if (Sys_Stat(output) && Sys_Stat(output) >= Sys_Stat(source))
		return;
	
build:
	Calloc(command, 2048);
	
	strcpy(entryDir, Path(output));
	strcat(entryDir, ".entry/");
	strcat(entryDir, Basename(output));
	strcat(entryDir, "/entry.ld");
	Sys_MakeDir(Path(entryDir));
	
	MemFile_Alloc(&entry, 0x20);
	MemFile_Printf(&entry, "ENTRY_POINT = 0x%08X;\n", entryPoint);
	MemFile_SaveFile(&entry, entryDir);
	MemFile_Free(&entry);
	
	Tools_Command(command, mips64_ld, "-o %s %s -L%s %s %s", output, source, Path(entryDir), sLinkerBaseFlags, flags);
	Sys_MakeDir(output);
	
	Time_Start(1);
	Make_Run(command);
	sTimeLD += Time_Get(1);
	sCountLD++;
	Make_Info("LD", output);
	
	if (callback)
		callback(output, POST_LD, output, command);
	Free(command);
}

static void Binutil_ObjDump(char* cmd, const char* output) {
	Time_Start(1);
	MemFile linker = MemFile_Initialize();
	u32 lineNum = LineNum(cmd);
	char* txt = cmd;
	
	MemFile_Alloc(&linker, MbToBin(1));
	
	for (s32 i = 0; i < lineNum; i++) {
		char* line = CopyLine(txt, 0);
		char* word;
		
		// 0x80700000
		if (!MemMem(line, 3, "807", 3)) goto skip;
		if (!StrStr(line, " F ") && !StrStr(line, " O ")) goto skip;
		word = CopyWord(line, 5);
		
		if (word[0] == 's' && !islower(word[1])) goto skip;
		if (StrStr(word, "flag") && strlen(word) == 4) goto skip;
		if (StrStr(word, "segment") && strlen(word) == 7) goto skip;
		for (s32 j = 0; j < strlen(word); j++)
			if (word[j] == '.') goto skip;
		
		MemFile_Printf(&linker, "%-24s = ", word);
		word = CopyWord(line, 0);
		MemFile_Printf(&linker, "0x%s;\n", word);
skip:
		txt = Line(txt, 1);
	}
	sTimeObjDump += Time_Get(1);
	MemFile_SaveFile(&linker, "include/z_lib_user.ld");
	MemFile_Free(&linker);
	Free(cmd);
}

// # # # # # # # # # # # # # # # # # # # #
// # Thread                              #
// # # # # # # # # # # # # # # # # # # # #

static void Make_CodeThread_uLib(MakeArg* arg) {
	ItemList itemList = ItemList_Initialize();
	const char* elf = "rom/lib_user/z_lib_user.elf";
	const char* bin = "rom/lib_user/z_lib_user.bin";
	const char* ld = "include/z_lib_user.ld";
	char* inputList = NULL;
	char* command = NULL;
	u32 inputStrLen = 0;
	u32 breaker = true;
	
	ItemList_SetFilter(&itemList, CONTAIN_END, ".o");
	ItemList_List(&itemList, arg->path, -1, LIST_FILES | LIST_NO_DOT);
	
	if (!Sys_Stat(bin) || !Sys_Stat(ld))
		breaker = false;
	
	if (ItemList_StatMax(&itemList) > Sys_Stat(bin))
		breaker = false;
	
	if (breaker) {
		ItemList_Free(&itemList);
		
		return;
	}
	
	for (s32 i = 0; i < itemList.num; i++)
		inputStrLen += strlen(itemList.item[i]) + 4;
	
	Calloc(command, 2048);
	Calloc(inputList, inputStrLen);
	
	for (s32 i = 0; i < itemList.num; i++) {
		strcat(inputList, itemList.item[i]);
		strcat(inputList, " ");
	}
	
	if (true != false /* ELF */) {
		MemFile entry = MemFile_Initialize();
		
		MemFile_Alloc(&entry, 0x80);
		MemFile_Printf(&entry, "ENTRY_POINT = 0x80700000;\n");
		MemFile_SaveFile(&entry, "rom/lib_user/entry.ld");
		MemFile_Free(&entry);
		
		Tools_Command(command, mips64_ld, "-o %s %s -Lrom/lib_user/ %s %s", elf, inputList, sLinkerBaseFlags, arg->flag);
		Sys_MakeDir(elf);
		
		Make_Run(command);
	}
	if (true == true /* BIN */) {
		Tools_Command(command, mips64_objcopy, "-R .MIPS.abiflags -O binary %s %s", elf, bin);
		Make_Run(command);
	}
	if (false == false /* mips64_ld */) {
		Tools_Command(command, mips64_objdump, "-x -t %s", elf);
		Sys_MakeDir(ld);
		Binutil_ObjDump(SysExeO(command), ld);
	}
	
	Free(inputList);
	Free(command);
	ItemList_Free(&itemList);
	
	Make_Info("OBJDUMP", "include/z_code_lib.ld");
}

static void Make_CodeThread_GCC(MakeArg* arg) {
	char* output;
	char* input = arg->itemList->item[arg->i];
	
	if (StrStr(input, " "))
		printf_error("Build does not support whitespace characters! [%s]", input);
	
	if (StrEnd(input, ".c")) {
		output = StrDupX(input, strlen(input) + 10);
		strcpy(output, input);
		StrRep(output, ".c", ".o");
		StrRep(output, "src/", "rom/");
		
		Binutil_GCC(input, output, arg->flag, arg->callback);
		Free(output);
	} else if (StrEnd(input, ".s")) {
		output = StrDupX(input, strlen(input) + 10);
		StrRep(output, ".s", ".o");
		StrRep(output, "src/", "rom/");
		
		Binutil_GCC_Assembly(input, output, arg->callback);
		Free(output);
	}
}

static void Make_CodeThread_LD(MakeArg* arg) {
	char* output = NULL;
	char* input = arg->itemList->item[arg->i];
	ItemList* list = NULL;
	char* ninput = NULL;
	
	if (Sys_IsDir(input)) {
		u32 files = 0;
		if (StrEnd(input, xFmt("%s/", gVanilla)))
			return;
		
		Malloc(list, sizeof(ItemList));
		*list = ItemList_Initialize();
		
		ItemList_List(list, input, -1, LIST_FILES | LIST_NO_DOT);
		
		for (s32 i = 0; i < list->num; i++) {
			if (i == 0) {
				Calloc(ninput, 1024 * 8);
			}
			if (StrEndCase(list->item[i], ".o")) {
				if (StrStr(list->item[i], " "))
					printf_error("Build does not support whitespace characters! [%s]", list->item[i]);
				catprintf(ninput, "%s ", list->item[i]);
				files++;
			}
		}
		
		if (files == 0)
			goto free;
		
		input = ninput;
		
		Calloc(output, strlen(input));
		sprintf(output, "%sfile.elf", arg->itemList->item[arg->i]);
		StrRep(output, "src/", "rom/");
	} else {
		if (!StrEnd(input, ".o")) {
			goto free;
		}
		
		Calloc(output, strlen(input) + 10);
		strcpy(output, input);
		StrRep(output, ".o", ".elf");
		StrRep(output, "src/", "rom/");
	}
	
	Binutil_LD(input, output, arg->flag, arg->callback);
	
free:
	Free(output);
	Free(ninput);
	if (list)
		ItemList_Free(list);
	Free(list);
}

static ThreadFunc Make_CodeThread_File(MakeArg* arg) {
	s32 i = 0;
	Thread* thread;
	MakeArg* passArg;
	ItemList itemList = ItemList_Initialize();
	
	Calloc(thread, sizeof(Thread) * gThreadNum);
	Calloc(passArg, sizeof(MakeArg) * gThreadNum);
	
	if (!Sys_Stat(arg->path))
		return;
	
	ItemList_SetFilter(&itemList, CONTAIN_END, ".c", CONTAIN_END, ".s", CONTAIN_END, ".h", CONTAIN_END, ".o", CONTAIN_END, ".elf");
	ItemList_List(&itemList, arg->path, -1, LIST_FILES | LIST_NO_DOT);
	
	while (i < itemList.num) {
		u32 target = Clamp(itemList.num - i, 0, gThreadNum);
		
		for (s32 j = 0; j < target; j++) {
			memcpy(&passArg[j], arg, sizeof(MakeArg));
			passArg[j].itemList = &itemList;
			passArg[j].i = i + j;
			
			Make_Thread(&thread[j], arg->func, &passArg[j]);
		}
		
		if (gThreading)
			for (s32 j = 0; j < target; j++)
				ThreadLock_Join(&thread[j]);
		
		i += gThreadNum;
	}
	
	Free(thread);
	Free(passArg);
	ItemList_Free(&itemList);
}

static ThreadFunc Make_CodeThread_Folder(MakeArg* arg) {
	s32 i = 0;
	Thread* thread;
	MakeArg* passArg;
	ItemList itemList = ItemList_Initialize();
	
	Calloc(thread, sizeof(Thread) * gThreadNum);
	Calloc(passArg, sizeof(MakeArg) * gThreadNum);
	
	if (!Sys_Stat(arg->path))
		return;
	
	ItemList_SetFilter(&itemList, CONTAIN_END, ".c", CONTAIN_END, ".h", CONTAIN_END, ".o", CONTAIN_END, ".elf");
	ItemList_List(&itemList, arg->path, 0, LIST_FOLDERS | LIST_NO_DOT);
	
	while (i < itemList.num) {
		u32 target = Clamp(itemList.num - i, 0, gThreadNum);
		
		for (s32 j = 0; j < target; j++) {
			memcpy(&passArg[j], arg, sizeof(MakeArg));
			passArg[j].itemList = &itemList;
			passArg[j].i = i + j;
			
			Make_Thread(&thread[j], arg->func, &passArg[j]);
		}
		
		if (gThreading)
			for (s32 j = 0; j < target; j++)
				ThreadLock_Join(&thread[j]);
		
		i += gThreadNum;
	}
	
	Free(thread);
	Free(passArg);
	
	ItemList_Free(&itemList);
}

// # # # # # # # # # # # # # # # # # # # #
// # Make                                #
// # # # # # # # # # # # # # # # # # # # #

void Make_Code(void) {
	const struct {
		const char* src;
		const char* rom;
		const char* gccFlag;
		const char* ldFlag;
		BinutilCallback callback;
		s32 multiFileProcess;
	} param[] = {
		{
			.src = "src/lib_user/",
			.rom = "rom/lib_user/",
			.gccFlag = sGccCodeFlags,
			.ldFlag = sLinkerULibFlags,
			.callback = NULL,
			.multiFileProcess = false,
		}, {
			.src = "src/lib_code/",
			.rom = "rom/lib_code/",
			.gccFlag = sGccCodeFlags,
			.ldFlag = sLinkerCodeFlags,
			.callback = Callback_Code,
			.multiFileProcess = false,
		}, {
			.src = "src/actor/",
			.rom = "rom/actor/",
			.gccFlag = sGccActorFlags,
			.ldFlag = sLinkerCodeFlags,
			.callback = Callback_Actor,
			.multiFileProcess = true,
		}, {
			.src = "src/effect/",
			.rom = "rom/effect/",
			.gccFlag = sGccActorFlags,
			.ldFlag = sLinkerCodeFlags,
			.callback = NULL,
			.multiFileProcess = true,
		}, {
			.src = "src/system/state/",
			.rom = "rom/system/state/",
			.gccFlag = sGccStateFlags,
			.ldFlag = sLinkerCodeFlags,
			.callback = Callback_System,
			.multiFileProcess = true,
		}, {
			.src = "src/system/kaleido/",
			.rom = "rom/system/kaleido/",
			.gccFlag = sGccKaleidoFlags,
			.ldFlag = sLinkerCodeFlags,
			.callback = Callback_Kaleido,
			.multiFileProcess = true,
		}
	};
	Thread thread[ArrayCount(param)];
	MakeArg args[ArrayCount(param)] = { 0 };
	
	ItemList_SetFilter(&sDepList_uLibHeader, CONTAIN_END, ".h", FILTER_WORD, "object");
	ItemList_List(&sDepList_uLibHeader, "src/lib_user/", -1, LIST_FILES | LIST_NO_DOT);
	
	if (gThreading)
		ThreadLock_Init();
	
	foreach(set, param) {
		args[set].path = param[set].src;
		args[set].flag = param[set].gccFlag;
		args[set].func = Make_CodeThread_GCC;
		args[set].callback = param[set].callback;
		
		Make_Thread(&thread[set], Make_CodeThread_File, &args[set]);
	}
	
	if (gThreading)
		ThreadLock_Join(&thread[0]);
	memset(&args[0], 0, sizeof(args[0]));
	args[0].path = param[0].rom;
	args[0].flag = param[0].ldFlag;
	
	Make_Thread(&thread[0], Make_CodeThread_uLib, &args[0]);
	
	foreach(set, param) {
		if (gThreading)
			ThreadLock_Join(&thread[set]);
		
		if (set == 0)
			continue;
		
		memset(&args[set], 0, sizeof(args[set]));
		args[set].path = param[set].rom;
		args[set].func = Make_CodeThread_LD;
		args[set].callback = param[set].callback;
		args[set].flag = param[set].ldFlag;
		
		if (param[set].multiFileProcess == true)
			Make_Thread(&thread[set], Make_CodeThread_Folder, &args[set]);
		
		else
			Make_Thread(&thread[set], Make_CodeThread_File, &args[set]);
	}
	
	if (gThreading) {
		foreach(set, param) {
			ThreadLock_Join(&thread[set]);
		}
		ThreadLock_Free();
	}
	
	ItemList_Free(&sDepList_uLibHeader);
}

void Make(Rom* rom, s32 message) {
	sGccBaseFlags = qFree(StrDup(Config_GetStr(&rom->config, "gcc_base_flags")));
	sGccActorFlags = qFree(StrDup(Config_GetStr(&rom->config, "gcc_actor_flags")));
	sGccCodeFlags = qFree(StrDup(Config_GetStr(&rom->config, "gcc_code_flags")));
	sGccKaleidoFlags = qFree(StrDup(Config_GetStr(&rom->config, "gcc_kaleido_flags")));
	sGccStateFlags = qFree(StrDup(Config_GetStr(&rom->config, "gcc_state_flags")));
	
	sLinkerBaseFlags = qFree(StrDup(Config_GetStr(&rom->config, "ld_base_flags")));
	sLinkerCodeFlags = qFree(StrDup(Config_GetStr(&rom->config, "ld_code_flags")));
	sLinkerSceneFlags = qFree(StrDup(Config_GetStr(&rom->config, "ld_scene_flags")));
	sLinkerULibFlags = qFree(StrDup(Config_GetStr(&rom->config, "ld_ulib_flags")));
	
	if (Config_GetErrorState()) {
		printf("\n");
		printf_warning("Seems like your z64project is missing some required variables.");
		printf_warning("Run [z64rom --reconfig] to regenerates gcc and ld flags.");
		printf_getchar("Press enter to exit.");
		exit(1);
	}
	
	if (gBuildTarget == ROM_DEV)
		sGccBaseFlags = xFmt("%s -DDEV_BUILD", sGccBaseFlags);
	
	setvbuf(stdout, NULL, _IONBF, 0);
	
	if (gMakeTarget) {
		if (StrStrCase(gMakeTarget, "object")) {
			Make_Object();
		}
		if (StrStrCase(gMakeTarget, "audio")) {
			Make_Sound();
			Make_Sequence();
		}
		if (StrStrCase(gMakeTarget, "code"))
			Make_Code();
	} else {
		Make_Object();
		Make_Sequence();
		Make_Sound();
		Make_Code();
	}
	
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	setvbuf(stderr, NULL, _IONBF, BUFSIZ);
	printf_WinFix();
	
	if (sMake && message)
		printf_info("Make OK");
	
	if (gPrintInfo) {
		printf_info("Average Build Times:");
		printf("GCC:  " PRNT_YELW "%.4f" PRNT_RSET "s\n", sTimeGCC / sCountGCC);
		printf("LD:   " PRNT_YELW "%.4f" PRNT_RSET "s\n", sTimeLD / sCountLD);
		printf("OD:   " PRNT_YELW "%.4f" PRNT_RSET "s\n", sTimeObjDump);
		printf("ZOVL: " PRNT_YELW "%.4f" PRNT_RSET "s\n", sTimeZOVL / sCountZOVL);
	}
}
