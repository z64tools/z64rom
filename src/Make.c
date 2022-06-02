#include "z64rom.h"
#include "Make.h"

#define THREAD_NUM 42

static const char* sFlags;
static const char* sFlagsCode;
static const char* sFlagsLink;
static const char* sFlagsULibLink;
static volatile bool sMake = false;
u32 gThreading = true;

static char* Make_Wildcard(const char* path, const char* fmt, ...) {
	ItemList list = ItemList_Initialize();
	char* file = NULL;
	Time stat = 0;
	char buf[64];
	va_list va;
	
	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);
	
	ItemList_List(&list, path, 0, LIST_FILES | LIST_NO_DOT);
	for (s32 i = 0; i < list.num; i++) {
		if (StrEndCase(list.item[i], buf) && Sys_Stat(list.item[i]) > stat) {
			file = list.item[i];
			stat = Sys_Stat(file);
		}
	}
	
	if (file)
		file = HeapStrDup(file);
	
	ItemList_Free(&list);
	
	return file;
}

static void Make_Info(const char* tool, const char* target) {
	printf_lock("[M]: %s\n", Filename(target));
	sMake = 1;
}

static void Make_Run(char* cmd) {
	strcat(cmd, " 2>&1");
	char* msg = SysExeO(cmd);
	
	if (strlen(msg) > 1 && (StrStrCase(msg, "warning") || StrStrCase(msg, "error"))) {
		char* word = CopyWord(msg, 0);
		
		StrRep(msg, "warning", PRNT_PRPL "warning" PRNT_RSET);
		StrRep(msg, "error", PRNT_REDD "error" PRNT_RSET);
		StrRep(msg, word, HeapPrint(PRNT_YELW "%s" PRNT_RSET, word));
		printf_WinFix();
		printf_lock("%s", msg);
	}
	Free(msg);
}

// # # # # # # # # # # # # # # # # # # # #
// # MAKE_SOUND                          #
// # # # # # # # # # # # # # # # # # # # #

static ThreadFunc Sequence_Convert(MakeArg* targ) {
	ItemList* list;
	char cmd[512];
	char* toml;
	char* seq = NULL;
	char* midi = NULL;
	char* mus = NULL;
	u32 index = Value_Hex(PathSlot(targ->path, -1));
	
	Calloc(list, sizeof(ItemList));
	*list = ItemList_Initialize();
	
	if ((midi = Make_Wildcard(targ->path, ".mid"))) {
		toml = HeapPrint("%sconfig.toml", targ->path);
		seq = HeapPrint("%ssequence.aseq", targ->path);
		
		if (Sys_Stat(seq) > Sys_Stat(midi) && !gMakeForce)
			goto free;
		
		if (!Sys_Stat(toml)) {
			ItemList van = ItemList_Initialize();
			ItemList_List(&van, HeapPrint("rom/sound/sequence/%s/", gVanilla), 0, LIST_FOLDERS);
			
			if (Sys_Stat(van.item[index]))
				Sys_Copy(HeapPrint("%sconfig.toml", van.item[index]), toml);
			
			ItemList_Free(&van);
		}
		
		Tools_Command(cmd, seq64, "--in=\"%s\" --out=\"%s\" --abi=Zelda --pref=false --flstudio=true", midi, seq);
		Make_Run(cmd);
		Make_Info("seq64", midi);
		
		goto free;
	}
	
	if ((mus = Make_Wildcard(targ->path, ".mus"))) {
		toml = HeapPrint("%sconfig.toml", targ->path);
		seq = HeapPrint("%ssequence.aseq", targ->path);
		
		if (Sys_Stat(seq) > Sys_Stat(mus) && !gMakeForce)
			goto free;
		
		if (!Sys_Stat(toml)) {
			ItemList van = ItemList_Initialize();
			ItemList_List(&van, HeapPrint("rom/sound/sequence/%s/", gVanilla), 0, LIST_FOLDERS);
			
			if (Sys_Stat(van.item[index]))
				Sys_Copy(HeapPrint("%sconfig.toml", van.item[index]), toml);
			
			ItemList_Free(&van);
		}
		
		Tools_Command(cmd, seqas, "\"%s\" \"%s\"", mus, seq);
		Make_Run(cmd);
		Make_Info("seq-assembler", mus);
		
		goto free;
	}
	
free:
	ItemList_Free(list);
	Free(list);
}

void Make_Sequence(void) {
	ItemList list = ItemList_Initialize();
	MakeArg targ[THREAD_NUM];
	Thread thread[THREAD_NUM];
	s32 i = 0;
	
	ItemList_List(&list, "rom/sound/sequence/", 0, LIST_FOLDERS | LIST_NO_DOT);
	
	if (list.num == 0)
		goto free;
	
	if (gThreading)
		ThreadLock_Init();
	
	while (i < list.num) {
		u32 target = Clamp(list.num - i, 0, THREAD_NUM);
		
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
		
		i += THREAD_NUM;
	}
	if (gThreading)
		ThreadLock_Free();
	
free:
	ItemList_Free(&list);
}

static ThreadFunc Sound_Convert(MakeArg* targ) {
	ItemList* list;
	char* vadpcm = NULL;
	char* audio = NULL;
	char* book = NULL;
	char* table = NULL;
	bool normalize = false;
	const char* fmt[] = {
		".wav",
		".aiff",
		".mp3"
	};
	
	Calloc(list, sizeof(ItemList));
	*list = ItemList_Initialize();
	
	ItemList_List(list, targ->path, 0, LIST_FILES | LIST_NO_DOT);
	
	for (s32 i = 0; i < list->num; i++) {
		if (StrStr(list->item[i], "normalize"))
			normalize = true;
		
		if (StrEndCase(list->item[i], ".book.bin"))
			book = list->item[i];
		
		if (StrEndCase(list->item[i], "/design.confg"))
			table = list->item[i];
		
		if (audio == NULL) {
			for (s32 j = 0; j < ArrayCount(fmt); j++) {
				if (StrEndCase(list->item[i], fmt[j])) {
					audio = list->item[i];
					break;
				}
			}
		} else {
			for (s32 j = 0; j < ArrayCount(fmt); j++) {
				if (StrEndCase(list->item[i], fmt[j]))
					if (Sys_Stat(audio) < Sys_Stat(list->item[i]))
						audio = list->item[i];
			}
		}
		
		if (vadpcm == NULL)
			if (StrEndCase(list->item[i], ".vadpcm.bin"))
				vadpcm = strdup(list->item[i]);
	}
	
	if (audio == NULL)
		goto free;
	
	if (vadpcm == NULL || (Sys_Stat(audio) > Sys_Stat(vadpcm)) || gMakeForce) {
		char command[2056];
		char* config;
		
		Malloc(config, 0x1024);
		
		strcpy(config, targ->path);
		StrRep(config, "rom/sound/sample/", HeapPrint("rom/sound/sample/%s/", gVanilla));
		strcat(config, "config.toml");
		
		if (vadpcm == NULL)
			vadpcm = HeapPrint("%ssample.bin", targ->path);
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
		
		if (Sys_Stat(config)) {
			MemFile mem = MemFile_Initialize();
			
			MemFile_LoadFile_String(&mem, config);
			catprintf(
				command,
				" --z64rom --basenote %d --finetune %d",
				Toml_GetInt(&mem, "basenote"),
				Toml_GetInt(&mem, "finetune")
			);
			MemFile_Free(&mem);
		}
		
		Make_Run(command);
		Make_Info("z64audio", audio);
	}
	
free:
	ItemList_Free(list);
	Free(list);
	Free(vadpcm);
}

void Make_Sound(void) {
	ItemList list = ItemList_Initialize();
	MakeArg targ[THREAD_NUM];
	Thread thread[THREAD_NUM];
	s32 i = 0;
	
	ItemList_List(&list, "rom/sound/sample/", 0, LIST_FOLDERS | LIST_NO_DOT);
	
	if (list.num == 0)
		goto free;
	
	if (gThreading)
		ThreadLock_Init();
	
	while (i < list.num) {
		u32 target = Clamp(list.num - i, 0, THREAD_NUM);
		
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
		
		i += THREAD_NUM;
	}
	if (gThreading)
		ThreadLock_Free();
	
free:
	ItemList_Free(&list);
}

// # # # # # # # # # # # # # # # # # # # #
// # MAKE_CODE                           #
// # # # # # # # # # # # # # # # # # # # #

static s32 Callback_System(const char* input, MakeCallType type, void* arg, void* arg2) {
	char* ovl = NULL;
	
	if (type == PRE_GCC) {
		ovl = HeapPrint("%soverlay.zovl", Path(input));
		StrRep(ovl, "src/", "rom/");
		
		if ((!Sys_Stat(ovl) && Sys_Stat(input)) || Sys_Stat(input) > Sys_Stat(ovl))
			return CB_BUILD;
		
		return CB_BREAK;
	}
	
	if (type == PRE_LD) {
		ItemList list = ItemList_Initialize();
		ItemList_Separated(&list, input, ' ');
		
		if (!ItemList_StatMin(&list))
			return CB_BREAK;
		
		if (!Sys_Stat(arg2))
			return CB_BUILD;
		
		if (Sys_Stat(arg2) < ItemList_StatMax(&list))
			return CB_BUILD;
		
		return CB_BREAK;
	}
	
	if (type == POST_LD) {
		char* command = arg;
		char* info;
		char* dump;
		
		ovl = HeapPrint("%soverlay.zovl", Path(input));
		StrRep(ovl, "src/", "rom/");
		
		info = PathSlot(input, -1);
		if (StrMtch(info, "0x"))
			StrRem(info, strlen("0x0000-"));
		StrRep(info, "/", " ");
		
		Tools_Command(command, nOVL, "-v -c -s -A 0x80800000 -o %s %s", ovl, input);
		Make_Run(command);
		
		Tools_Command(command, mips64_objdump, "-t %s", input);
		dump = SysExeO(command); {
			char* config = HeapPrint("%sconfig.toml", Path(input));
			MemFile mem = MemFile_Initialize();
			char* _StateInit = LineHead(StrStr(dump, "_StateInit"));
			char* _StateDestroy = LineHead(StrStr(dump, "_StateDestroy"));
			
			if (_StateInit == NULL)
				printf_error_align("No StateInit", "%s", input);
			if (_StateDestroy == NULL)
				printf_error_align("No StateDestroy", "%s", input);
			
			MemFile_Malloc(&mem, 0x800);
			MemFile_Printf(&mem, "# %s\n\n", Basename(input));
			MemFile_Printf(&mem, "vram_addr = 0x80800000\n");
			MemFile_Printf(&mem, "init_func = 0x%.8s\n", _StateInit);
			MemFile_Printf(&mem, "dest_func = 0x%.8s\n", _StateDestroy);
			
			MemFile_SaveFile_String(&mem, config);
			MemFile_Free(&mem);
		}
		
		Free(dump);
	}
	
	return 0;
}

static s32 Callback_Actor(const char* input, MakeCallType type, void* arg, void* arg2) {
	char* ovl;
	char* conf;
	
	if (type == PRE_GCC) {
		ovl = HeapPrint("%soverlay.zovl", Path(input));
		StrRep(ovl, "src/", "rom/");
		
		if ((!Sys_Stat(ovl) && Sys_Stat(input)) || Sys_Stat(input) > Sys_Stat(ovl))
			return CB_BUILD;
		
		return CB_BREAK;
	}
	
	if (type == PRE_LD) {
		ItemList list = ItemList_Initialize();
		
		ItemList_Separated(&list, input, ' ');
		
		if (!ItemList_StatMin(&list))
			return CB_BREAK;
		
		if (!Sys_Stat(arg2))
			return CB_BUILD;
		
		if (Sys_Stat(arg2) < ItemList_StatMax(&list))
			return CB_BUILD;
		
		return CB_BREAK;
	}
	
	if (type == POST_LD) {
		char* command = arg;
		char* info;
		char* dump;
		
		ovl = HeapPrint("%soverlay.zovl", Path(input));
		StrRep(ovl, "src/", "rom/");
		
		conf = HeapPrint("%sconfig.toml", Path(input));
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
			
			MemFile_Malloc(&srcFile, MbToBin(1.0f));
			MemFile_Params(&srcFile, MEM_REALLOC, true, MEM_END);
			MemFile_Malloc(&newConf, MbToBin(1.0f));
			
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
						temp = StrStr(srcFile.str, " ActorInit ");
						if (temp)
							temp += strlen(" ActorInit ");
					}
				}
				
				if (temp)
					break;
			}
			
			if (temp == NULL)
				printf_error("Could not locate [ActorInit] from files in [%s]", sourceFolder);
			
			Malloc(varName, 64);
			strcpy(varName, CopyWord(temp, 0));
			StrRep(varName, "=", "");
			StrRep(varName, "[", "");
			StrIns(varName, " ");
			temp = LineHead(StrStr(dump, varName));
			
			MemFile_Printf(&newConf, "# %s\n", Basename(input));
			MemFile_Printf(&newConf, "alloc_type = 0\n");
			MemFile_Printf(&newConf, "vram_addr  = 0x80800000\n");
			MemFile_Printf(&newConf, "# %s\n", CopyLine(temp, 0));
			MemFile_Printf(&newConf, "init_vars  = 0x%.8s\n", temp);
			
			if (MemFile_SaveFile_String(&newConf, conf)) printf_error("Could not save [%s]", conf);
			
			MemFile_Free(&srcFile);
			MemFile_Free(&newConf);
			Free(varName);
		}
		
		info = PathSlot(input, -1); StrRem(info, strlen("0x0000-")); StrRep(info, "/", " ");
		
		Tools_Command(command, nOVL, "-v -c -s -A 0x80800000 -o %s %s", ovl, input);
		Make_Run(command);
		
		Free(dump);
		
		return 0;
	}
	
	return 0;
}

static s32 Callback_Code(const char* input, MakeCallType type, void* arg, void* arg2) {
	if (type == PRE_GCC) {
		return 0;
	}
	if (type == POST_GCC) {
		return 0;
	}
	if (type == PRE_LD) {
		u32* entryPoint = arg;
		MemFile mem = MemFile_Initialize();
		MemFile __config = MemFile_Initialize();
		MemFile* config = &__config;
		char* c = HeapMalloc(strlen(input) + 0x10);
		char* z64rom;
		char* z64ram;
		char* z64next;
		
		strcpy(c, input);
		if (!StrRep(c, "rom/", "src/")) goto error;
		if (!StrRep(c, ".o", ".c")) goto error;
		
		MemFile_LoadFile_String(&mem, c);
		z64ram = StrStr(mem.str, "z64ram = ");
		z64rom = StrStr(mem.str, "z64rom = ");
		z64next = StrStr(mem.str, "z64next = ");
		
		if (z64ram == NULL) printf_error_align("No RAM Address:", "[%s]", Filename(c));
		if (z64rom == NULL) printf_error_align("No ROM Address:", "[%s]", Filename(c));
		
		z64ram += strlen("z64ram = ");
		z64rom += strlen("z64rom = ");
		if (z64next) z64next += strlen("z64next = ");
		
		MemFile_Malloc(config, 0x280);
		Toml_WriteHex(config, "rom", Value_Hex(z64rom), NO_COMMENT);
		Toml_WriteHex(config, "ram", Value_Hex(z64ram), NO_COMMENT);
		if (z64next) Toml_WriteHex(config, "next", Value_Hex(z64next), NO_COMMENT);
		entryPoint[0] = Value_Hex(z64ram);
		
		strcpy(c, input);
		if (!StrRep(c, ".o", ".toml")) goto error;
		
		MemFile_SaveFile_String(config, c);
		
		MemFile_Free(&mem);
		MemFile_Free(config);
		
		return 0;
error:
		printf_error_align("StrRep", "[%s] -> [%s]", input, c);
	}
	if (type == POST_LD) {
		char* output;
		char* command = arg;
		
		output = HeapMalloc(strlen(input) + 8);
		strcpy(output, input);
		StrRep(output, ".elf", ".bin");
		
		Tools_Command(command, mips64_objcopy, "-R .MIPS.abiflags -O binary %s %s", input, output);
		Make_Run(command);
	}
	
	return 0;
}

static ThreadFunc Code_GCC(const char* source, const char* output, const char* flags, BinutilCallback callback) {
	char* command;
	
	if (callback) {
		switch (callback(source, PRE_GCC, NULL, NULL)) {
			case CB_BREAK:
				return;
			case 0:
				break;
			case CB_BUILD:
				goto build;
		}
	}
	
	if (Sys_Stat(output) && Sys_Stat(output) >= Sys_Stat(source) && !gMakeForce)
		return;
	
build:
	(void)0;
	char* newFlags = NULL;
	char* flagFile = HeapPrint("%sflags.toml", Path(source));
	
	Calloc(command, 2048);
	
	if (Sys_Stat(flagFile)) {
		MemFile mem = MemFile_Initialize();
		char* var;
		
		MemFile_LoadFile_String(&mem, flagFile);
		
		if ((var = Toml_Variable(mem.str, "flags"))) {
			Calloc(newFlags, 1024 * 2);
			
			strcpy(newFlags, flags);
			catprintf(newFlags, " %s", var);
			
			flags = newFlags;
		}
		
		if ((var = Toml_Variable(mem.str, Basename(source)))) {
			Calloc(newFlags, 1024 * 2);
			
			strcpy(newFlags, flags);
			catprintf(newFlags, " %s", var);
			
			flags = newFlags;
		}
		
		MemFile_Free(&mem);
	}
	
	Tools_Command(command, mips64_gcc, "%s %s %s -o %s", sFlags, flags, source, output);
	Sys_MakeDir(output);
	
	Make_Run(command);
	Make_Info("GCC", source);
	
	if (callback)
		callback(output, POST_GCC, command, NULL);
	Free(command);
	if (newFlags)
		Free(newFlags);
}

static ThreadFunc Code_LD(const char* source, const char* output, const char* flags, BinutilCallback callback) {
	MemFile entry = MemFile_Initialize();
	char* command;
	char entryDir[1024] = { 0 };
	u32 entryPoint = 0x80800000;
	
	if (callback) {
		switch (callback(source, PRE_LD, &entryPoint, (void*)output)) {
			case CB_BREAK:
				return;
			case 0:
				break;
			case CB_BUILD:
				goto build;
		}
	}
	
	if (Sys_Stat(output) && Sys_Stat(output) >= Sys_Stat(source))
		return;
	
build:
	Calloc(command, 2048);
	
	strcpy(entryDir, Path(output));
	strcat(entryDir, ".entry/");
	strcat(entryDir, Basename(output));
	strcat(entryDir, "/entry.ld");
	Sys_MakeDir(Path(entryDir));
	
	MemFile_Malloc(&entry, 0x20);
	MemFile_Printf(&entry, "ENTRY_POINT = 0x%08X;\n", entryPoint);
	MemFile_SaveFile(&entry, entryDir);
	MemFile_Free(&entry);
	
	Tools_Command(command, mips64_ld, "-o %s %s -L%s %s", output, source, Path(entryDir), flags);
	Sys_MakeDir(output);
	
	Make_Run(command);
	Make_Info("LD", source);
	
	if (callback)
		callback(output, POST_LD, command, NULL);
	Free(command);
}

static ThreadFunc Code_ObjDump(char* cmd, const char* output) {
	MemFile linker = MemFile_Initialize();
	u32 lineNum = LineNum(cmd);
	char* txt = cmd;
	
	MemFile_Malloc(&linker, MbToBin(1));
	
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
	
	MemFile_SaveFile(&linker, "include/z_lib_user.ld");
	MemFile_Free(&linker);
	Free(cmd);
}

static ThreadFunc Make_Linker_Thread(MakeArg* arg) {
	ItemList itemList = ItemList_Initialize();
	const char* elf = "rom/lib_user/z_lib_user.elf";
	const char* bin = "rom/lib_user/z_lib_user.bin";
	const char* ld = "include/z_lib_user.ld";
	char* inputList = NULL;
	char* command = NULL;
	u32 inputStrLen = 0;
	u32 breaker = true;
	
	ItemList_List(&itemList, arg->path, -1, LIST_FILES | LIST_NO_DOT);
	
	if (!Sys_Stat(bin))
		breaker = false;
	
	if (!Sys_Stat(ld))
		breaker = false;
	
	for (s32 i = 0; i < itemList.num; i++) {
		if (!StrEnd(itemList.item[i], ".o"))
			continue;
		
		if (breaker)
			if (Sys_Stat(bin) < Sys_Stat(itemList.item[i]))
				breaker = false;
		
		inputStrLen += strlen(itemList.item[i]) + 4;
	}
	
	if (breaker) {
		ItemList_Free(&itemList);
		
		return;
	}
	
	Calloc(command, 2048);
	Calloc(inputList, inputStrLen);
	
	for (s32 i = 0; i < itemList.num; i++) {
		if (!StrEnd(itemList.item[i], ".o"))
			continue;
		strcat(inputList, itemList.item[i]);
		strcat(inputList, " ");
	}
	
	if (true != false /* ELF */) {
		MemFile entry = MemFile_Initialize();
		
		MemFile_Malloc(&entry, 0x80);
		MemFile_Printf(&entry, "ENTRY_POINT = 0x80700000;\n");
		MemFile_SaveFile(&entry, "rom/lib_user/entry.ld");
		MemFile_Free(&entry);
		
		Tools_Command(command, mips64_ld, "-o %s %s %s", elf, inputList, arg->flag);
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
		Code_ObjDump(SysExeO(command), ld);
	}
	
	Free(inputList);
	Free(command);
	ItemList_Free(&itemList);
	
	Make_Info("OBJDUMP", "z_code_lib.ld");
}

static ThreadFunc Make_Code_Thread_C(MakeArg* arg) {
	char* output;
	char* input = arg->itemList->item[arg->i];
	
	if (!StrEnd(input, ".c")) {
		return;
	}
	
	Calloc(output, strlen(input) + 10);
	strcpy(output, input);
	StrRep(output, ".c", ".o");
	StrRep(output, "src/", "rom/");
	
	Code_GCC(input, output, arg->flag, arg->callback);
	Free(output);
}

static ThreadFunc Make_Code_Thread_O(MakeArg* arg) {
	char* output = NULL;
	char* input = arg->itemList->item[arg->i];
	ItemList* list = NULL;
	char* ninput = NULL;
	
	if (Sys_IsDir(input)) {
		u32 files = 0;
		if (StrEnd(input, HeapPrint("%s/", gVanilla)))
			return;
		
		Malloc(list, sizeof(ItemList));
		*list = ItemList_Initialize();
		
		ItemList_List(list, input, -1, LIST_FILES | LIST_NO_DOT);
		
		for (s32 i = 0; i < list->num; i++) {
			if (i == 0) {
				Calloc(ninput, 1024 * 8);
			}
			if (StrEndCase(list->item[i], ".o")) {
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
	
	Code_LD(input, output, arg->flag, arg->callback);
	
free:
	Free(output);
	Free(ninput);
	if (list)
		ItemList_Free(list);
	Free(list);
}

static ThreadFunc Make_Code_Thread_Single(MakeArg* arg) {
	s32 i = 0;
	Thread thread[THREAD_NUM];
	MakeArg passArg[THREAD_NUM];
	ItemList itemList = ItemList_Initialize();
	
	if (!Sys_Stat(arg->path))
		return;
	
	ItemList_List(&itemList, arg->path, -1, LIST_FILES | LIST_NO_DOT);
	
	while (i < itemList.num) {
		u32 target = Clamp(itemList.num - i, 0, THREAD_NUM);
		
		for (s32 j = 0; j < target; j++) {
			memcpy(&passArg[j], arg, sizeof(MakeArg));
			passArg[j].itemList = &itemList;
			passArg[j].i = i + j;
			
			if (gThreading) {
				ThreadLock_Create(&thread[j], arg->func, &passArg[j]);
			} else {
				arg->func(&passArg[j]);
			}
		}
		
		if (gThreading)
			for (s32 j = 0; j < target; j++)
				ThreadLock_Join(&thread[j]);
		
		i += THREAD_NUM;
	}
	
	ItemList_Free(&itemList);
}

static ThreadFunc Make_Code_Thread_Folder(MakeArg* arg) {
	s32 i = 0;
	Thread thread[THREAD_NUM];
	MakeArg passArg[THREAD_NUM];
	ItemList itemList = ItemList_Initialize();
	
	if (!Sys_Stat(arg->path))
		return;
	
	ItemList_List(&itemList, arg->path, 0, LIST_FOLDERS | LIST_NO_DOT);
	
	while (i < itemList.num) {
		u32 target = Clamp(itemList.num - i, 0, THREAD_NUM);
		
		for (s32 j = 0; j < target; j++) {
			memcpy(&passArg[j], arg, sizeof(MakeArg));
			passArg[j].itemList = &itemList;
			passArg[j].i = i + j;
			
			if (gThreading) {
				ThreadLock_Create(&thread[j], arg->func, &passArg[j]);
			} else {
				arg->func(&passArg[j]);
			}
		}
		
		if (gThreading)
			for (s32 j = 0; j < target; j++)
				ThreadLock_Join(&thread[j]);
		
		i += THREAD_NUM;
	}
	
	ItemList_Free(&itemList);
}

void Make_Code(void) {
	const char* pathC[] = {
		"src/lib_user/",
		"src/lib_code/",
		"src/actor/",
		"src/effect/",
		"src/system/state/",
	};
	const char* pathO[] = {
		"rom/lib_user/",
		"rom/lib_code/",
		"rom/actor/",
		"rom/effect/",
		"rom/system/state/",
	};
	const char* flagObject[] = {
		sFlagsCode,
		sFlagsCode,
		"",
		"",
		"",
	};
	const char* flagLinker[] = {
		sFlagsULibLink,
		sFlagsLink,
		sFlagsLink,
		sFlagsLink,
		sFlagsLink,
	};
	BinutilCallback callback[] = {
		NULL,
		Callback_Code,
		Callback_Actor,
		NULL,
		Callback_System,
	};
	Thread thread[ArrayCount(pathC)];
	MakeArg args[ArrayCount(pathC)] = { 0 };
	
	if (gThreading)
		ThreadLock_Init();
	
	foreach(set, pathC) {
		args[set].path = pathC[set];
		args[set].flag = flagObject[set];
		args[set].func = Make_Code_Thread_C;
		args[set].callback = callback[set];
		
		if (gThreading)
			ThreadLock_Create(&thread[set], Make_Code_Thread_Single, &args[set]);
		
		else
			Make_Code_Thread_Single(&args[set]);
	}
	
	if (gThreading)
		ThreadLock_Join(&thread[0]);
	memset(&args[0], 0, sizeof(args[0]));
	args[0].path = pathO[0];
	args[0].flag = flagLinker[0];
	
	if (gThreading)
		ThreadLock_Create(&thread[0], Make_Linker_Thread, &args[0]);
	
	else
		Make_Linker_Thread(&args[0]);
	
	foreach(set, pathC) {
		if (gThreading)
			ThreadLock_Join(&thread[set]);
		
		if (set == 0)
			continue;
		
		memset(&args[set], 0, sizeof(args[set]));
		args[set].path = pathO[set];
		args[set].func = Make_Code_Thread_O;
		args[set].callback = callback[set];
		args[set].flag = flagLinker[set];
		
		// Process as folders if Actor, System or Effect
		if (set >= 2) {
			if (gThreading)
				ThreadLock_Create(&thread[set], Make_Code_Thread_Folder, &args[set]);
			
			else
				Make_Code_Thread_Folder(&args[set]);
		} else {
			if (gThreading)
				ThreadLock_Create(&thread[set], Make_Code_Thread_Single, &args[set]);
			
			else
				Make_Code_Thread_Single(&args[set]);
		}
	}
	
	if (gThreading)
		foreach(set, pathC)
		ThreadLock_Join(&thread[set]);
	
	if (gThreading)
		ThreadLock_Free();
}

// # # # # # # # # # # # # # # # # # # # #
// # MAKE                                #
// # # # # # # # # # # # # # # # # # # # #

void Make(Rom* rom, s32 message) {
	sFlags = Toml_GetStr(&rom->config, "mips64_gcc_flags");
	sFlagsCode = Toml_GetStr(&rom->config, "mips64_gcc_flags_code");
	sFlagsLink = Toml_GetStr(&rom->config, "mips64_ld_flags");
	sFlagsULibLink = Toml_GetStr(&rom->config, "ulib_ld_flags");
	
	if (!sFlags || !sFlagsCode || !sFlagsLink || !sFlagsULibLink)
		printf_error("Missing a required flags from [z64project.toml]. Do a redump by drag'n'dropping your baserom on z64rom.");
	
	if (gBuildTarget)
		sFlags = HeapPrint("%s -DDEV_BUILD", sFlags);
	
	if (!sFlags || !sFlagsCode || !sFlagsLink)
		printf_error("[z64project.toml] is missing mips64 flags! Please, do fresh dump!");
	
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	
	if (gMakeTarget) {
		if (StrStrCase(gMakeTarget, "audio")) {
			Make_Sound();
			Make_Sequence();
		}
		if (StrStrCase(gMakeTarget, "code"))
			Make_Code();
	} else {
		Make_Sequence();
		Make_Sound();
		Make_Code();
	}
	
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	setvbuf(stderr, NULL, _IONBF, BUFSIZ);
	
	printf_WinFix();
	
	if (sMake && message)
		printf_info("Make OK");
}
