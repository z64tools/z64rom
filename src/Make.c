#include "z64rom.h"
#include "Make.h"

#define THREAD_NUM 42

const char* gFlags;
const char* gFlagsCode;
const char* gFlagsLink;
u32 gThreading = true;
volatile bool sMake = false;

typedef enum {
	PRE_GCC,
	POST_GCC,
	PRE_LD,
	POST_LD,
} PassType;

typedef enum {
	CB_BREAK = -1,
	CB_BUILD = 1
} CallbackReturn;

typedef s32 (* BinutilCallback)(const char*, PassType, void*, void*);

typedef struct ThreadArg {
	const char* path;
	const char* flag;
	ItemList*   itemList;
	void (* func)(struct ThreadArg*);
	BinutilCallback callback;
	u32 i;
} ThreadArg;

static void Make_Info(const char* tool, const char* target) {
	printf("[#]: %-16s %s\n", tool, String_GetFilename(target));
	sMake = 1;
}

// # # # # # # # # # # # # # # # # # # # #
// # MAKE_SOUND                          #
// # # # # # # # # # # # # # # # # # # # #

static void Sound_Convert(ThreadArg* targ) {
	if (StrEnd(targ->path, ".vanilla/"))
		return;
	
	ItemList* list;
	char* vadpcm = NULL;
	char* audio = NULL;
	const char* fmt[] = {
		".wav",
		".aiff",
		".mp3"
	};
	
	list = Calloc(list, sizeof(ItemList));
	*list = ItemList_Initialize();
	
	ItemList_List(list, targ->path, 0, LIST_FILES);
	
	for (s32 i = 0; i < list->num; i++) {
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
				vadpcm = list->item[i];
	}
	
	if (audio == NULL)
		goto free;
	
	if (vadpcm == NULL || Sys_Stat(audio) > Sys_Stat(vadpcm) || gMakeForce) {
		char command[512];
		Tools_Command(command, z64audio, "\"%s\"", audio);
		if (SysExe(command) > 0) printf_error_align("SysExe", "Failed");
		Make_Info("z64audio", audio);
	}
	
free:
	ItemList_Free(list);
	Free(list);
}

void Make_Sound(void) {
	ItemList list = ItemList_Initialize();
	ThreadArg targ[THREAD_NUM];
	Thread thread[THREAD_NUM];
	s32 i = 0;
	
	ItemList_List(&list, "rom/sound/sample/", 0, LIST_FOLDERS);
	
	if (list.num <= 1)
		goto free;
	
	if (gThreading)
		Thread_Init();
	while (i < list.num) {
		u32 target = Clamp(list.num - i, 0, THREAD_NUM);
		
		for (s32 j = 0; j < target; j++) {
			targ[j].path = list.item[i + j];
			
			if (gThreading) {
				Thread_Create(&thread[j], Sound_Convert, &targ[j]);
			} else {
				Sound_Convert(&targ[j]);
			}
		}
		
		if (gThreading)
			for (s32 j = 0; j < target; j++)
				Thread_Join(&thread[j]);
		
		i += THREAD_NUM;
	}
	if (gThreading)
		Thread_Free();
	
free:
	ItemList_Free(&list);
}

// # # # # # # # # # # # # # # # # # # # #
// # MAKE_CODE                           #
// # # # # # # # # # # # # # # # # # # # #

static s32 Overlay_ValidateMainFile(const char* input, char** oldInput) {
	ItemList list = ItemList_Initialize();
	char* newInput;
	char* sourcePath = String_GetPath(input);
	
	String_Replace(sourcePath, "rom/", "src/");
	ItemList_List(&list, sourcePath, 0, LIST_FILES);
	
	if (list.num > 2) {
		for (s32 h = 0; h < list.num; h++) {
			if (StrEnd(list.item[h], ".h")) {
				if (StrStr(input, String_GetBasename(list.item[h]))) {
					for (s32 c = 0; c < list.num; c++) {
						if (StrEnd(list.item[c], ".c")) {
							if (!strncmp(list.item[c], list.item[h], strlen(list.item[c]) - 2)) {
								ItemList_Free(&list);
								ItemList_List(&list, String_GetPath(input), 0, LIST_FILES);
								
								newInput = Tmp_Alloc(512);
								
								for (s32 i = 0; i < list.num; i++) {
									if (StrEnd(list.item[i], ".o")) {
										strcat(newInput, list.item[i]);
										if (i + 1 < list.num)
											strcat(newInput, " ");
									}
								}
								
								*oldInput = newInput;
								
								return 0;
							}
						}
					}
				}
			}
		}
	}
	
	return 1;
}

u32 Overlay_GetInit(void* overlay, u32 size) {
	u32* ptr = overlay;
	
	while (true) {
		if (size < sizeof(ActorInit))
			return 0;
		u32 match = 0;
		ActorInit* initData = (ActorInit*)ptr;
		
		if ((ReadBE(initData->init) & 0xFF000000) == 0x80000000) {
			if ((ReadBE(initData->update) & 0xFF000000) == 0x80000000) {
				if ((ReadBE(initData->destroy) & 0xFF000000) == 0x80000000 || initData->destroy == 0) {
					if ((ReadBE(initData->draw) & 0xFF000000) == 0x80000000 || initData->draw == 0) {
						match++;
					}
				}
			}
		}
		
		if (ReadBE(initData->id) <= ACTOR_ID_MAX && ReadBE(initData->objectId) <= OBJECT_ID_MAX)
			match++;
		
		if (initData->category <= ACTORCAT_CHEST)
			match++;
		
		if (!(ReadBE(initData->instanceSize) & 0xFF000000) && initData->instanceSize != 0 && (ReadBE(initData->instanceSize) % 4) == 0)
			match++;
		
		if (match == 4) {
			ActorInit* in = (void*)ptr;
			
			Log("id            %04X", ReadBE(in->id));
			Log("category      %04X", ReadBE(in->category));
			Log("flags         %04X", ReadBE(in->flags));
			Log("objectId      %04X", ReadBE(in->objectId));
			Log("instanceSize  %04X", ReadBE(in->instanceSize));
			Log("init          %04X", ReadBE(in->init));
			Log("destroy       %04X", ReadBE(in->destroy));
			Log("update        %04X", ReadBE(in->update));
			Log("draw          %04X", ReadBE(in->draw));
			break;
		}
		
		ptr++;
		size -= 4;
	}
	
	if (Overlay_GetInit(ptr + 1, size - 4))
		return (uPtr)ptr - (uPtr)overlay;
	
	return 0x80800000 + (uPtr)ptr - (uPtr)overlay;
}

static s32 Callback_System(const char* input, PassType type, void* arg, void* arg2) {
	char* ovl;
	
	if (type == POST_GCC) return 0;
	
	ovl = Tmp_Printf("%sactor.zovl", String_GetPath(input));
	String_Replace(ovl, "src/", "rom/");
	
	if (type == PRE_GCC) {
		if (!Sys_Stat(ovl) || Sys_Stat(input) > Sys_Stat(ovl))
			return CB_BUILD;
		
		return 0;
	}
	
	if (type == PRE_LD) {
		if (Overlay_ValidateMainFile(input, arg2)) {
			return CB_BREAK;
		}
		
		if (!Sys_Stat(ovl) || Sys_Stat(input) > Sys_Stat(ovl))
			return CB_BUILD;
		
		return 0;
	}
	
	if (type == POST_LD) {
		char* command = arg;
		char* info;
		char* dump;
		
		info = String_GetFolder(input, -1);
		String_Remove(info, strlen("0x0000-"));
		String_Replace(info, "/", " ");
		
		Log("novl %s", info);
		
		Tools_Command(command, nOVL, "-v -c -s -A 0x80800000 -o %s %s", ovl, input);
		if (SysExe(command) > 0) printf_error_align("SysExe", "Failed");
		
		Tools_Command(command, mips64_objdump, "-t %s", input);
		dump = SysExeO(command); {
			char* config = Tmp_Printf("%sconfig.cfg", String_GetPath(input));
			MemFile mem = MemFile_Initialize();
			char* _StateInit = String_LineHead(StrStr(dump, "_StateInit"));
			char* _StateDestroy = String_LineHead(StrStr(dump, "_StateDestroy"));
			
			if (_StateInit == NULL)
				printf_error_align("No StateInit", "%s", input);
			if (_StateDestroy == NULL)
				printf_error_align("No StateDestroy", "%s", input);
			
			MemFile_Malloc(&mem, 0x800);
			MemFile_Printf(&mem, "# %s\n\n", String_GetBasename(input));
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

static s32 Callback_Actor(const char* input, PassType type, void* arg, void* arg2) {
	char* ovl;
	char* conf;
	
	if (type == POST_GCC) return 0;
	
	ovl = Tmp_Printf("%sactor.zovl", String_GetPath(input));
	String_Replace(ovl, "src/", "rom/");
	
	conf = Tmp_Printf("%sconfig.cfg", String_GetPath(input));
	String_Replace(ovl, "src/", "rom/");
	
	if (type == PRE_GCC) {
		if (!Sys_Stat(ovl) || !Sys_Stat(conf) || Sys_Stat(input) > Sys_Stat(ovl))
			return CB_BUILD;
		
		return 0;
	}
	
	if (type == PRE_LD) {
		if (!Sys_Stat(ovl) || !Sys_Stat(conf) || Sys_Stat(input) > Sys_Stat(ovl))
			return CB_BUILD;
		
		return 0;
	}
	
	if (type == POST_LD) {
		char* command = arg;
		char* info;
		char* dump;
		
		info = String_GetFolder(input, -1); String_Remove(info, strlen("0x0000-")); String_Replace(info, "/", " ");
		Log("novl %s", info);
		
		Tools_Command(command, nOVL, "-v -c -s -A 0x80800000 -o %s %s", ovl, input);
		if (SysExe(command) > 0) printf_error_align("SysExe", "Failed");
		
		Tools_Command(command, mips64_objdump, "-t %s", input);
		dump = SysExeO(command); {
			MemFile mC = MemFile_Initialize();
			MemFile mConf = MemFile_Initialize();
			char* inC = strdup(input);
			char* temp;
			char* varName;
			
			String_Replace(inC, "rom/", "src/");
			String_Replace(inC, ".elf", ".c");
			
			MemFile_Malloc(&mConf, 0x80);
			if (MemFile_LoadFile(&mC, inC)) printf_error("Could not open [%s]", inC);
			
			temp = StrStr(mC.str, "\nActorInit ");
			if (temp)
				temp += strlen("\nActorInit ");
			else {
				temp = StrStr(mC.str, "\nconst ActorInit ");
				if (temp)
					temp += strlen("\nconst ActorInit ");
				else {
					temp = StrStr(mC.str, " ActorInit ");
					if (temp)
						temp += strlen(" ActorInit ");
					else {
						printf_WinFix();
						printf_error_align("ActorInit", "Could not locate ActorInit in [%s]", inC);
					}
				}
			}
			
			varName = Malloc(varName, 64);
			strcpy(varName, String_GetWord(temp, 0));
			String_Replace(varName, "=", "");
			String_Replace(varName, "[", "");
			String_Insert(varName, " ");
			temp = String_LineHead(StrStr(dump, varName));
			
			MemFile_Printf(&mConf, "# %s\n", String_GetBasename(input));
			MemFile_Printf(&mConf, "alloc_type = 0\n");
			MemFile_Printf(&mConf, "vram_addr  = 0x80800000\n");
			MemFile_Printf(&mConf, "# %s\n", String_GetLine(temp, 0));
			MemFile_Printf(&mConf, "init_vars  = 0x%.8s\n", temp);
			
			if (MemFile_SaveFile_String(&mConf, conf)) printf_error("Could not save [%s]", conf);
			
			MemFile_Free(&mC);
			MemFile_Free(&mConf);
			Free(inC);
			Free(varName);
		}
		
		Free(dump);
		
		return 0;
	}
	
	return 0;
}

static s32 Callback_Code(const char* input, PassType type, void* arg, void* arg2) {
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
		char* c = Tmp_Alloc(strlen(input) + 0x10);
		char* z64rom;
		char* z64ram;
		
		strcpy(c, input);
		if (!String_Replace(c, "rom/", "src/")) goto error;
		if (!String_Replace(c, ".o", ".c")) goto error;
		
		MemFile_LoadFile_String(&mem, c);
		z64ram = StrStr(mem.str, "z64ram = ");
		z64rom = StrStr(mem.str, "z64rom = ");
		
		if (z64ram == NULL) printf_error_align("No RAM Address:", "[%s]", String_GetFilename(c));
		if (z64rom == NULL) printf_error_align("No ROM Address:", "[%s]", String_GetFilename(c));
		
		z64ram += strlen("z64ram = ");
		z64rom += strlen("z64rom = ");
		
		MemFile_Malloc(config, 0x280);
		Config_WriteVar_Hex("rom", String_GetHexInt(z64rom));
		Config_WriteVar_Hex("ram", String_GetHexInt(z64ram));
		entryPoint[0] = String_GetHexInt(z64ram);
		
		strcpy(c, input);
		if (!String_Replace(c, ".o", ".cfg")) goto error;
		
		MemFile_SaveFile_String(config, c);
		
		MemFile_Free(&mem);
		MemFile_Free(config);
		
		return 0;
error:
		printf_error_align("String_Replace", "[%s] -> [%s]", input, c);
	}
	if (type == POST_LD) {
		char* output;
		char* command = arg;
		
		output = Tmp_Alloc(strlen(input) + 8);
		strcpy(output, input);
		String_Replace(output, ".elf", ".bin");
		
		Tools_Command(command, mips64_objcopy, "-R .MIPS.abiflags -O binary %s %s", input, output);
		if (SysExe(command) > 0) printf_error_align("SysExe", "Failed");
	}
	
	return 0;
}

void Code_GCC(const char* source, const char* output, const char* flags, BinutilCallback callback) {
	char* command;
	
	if (!Sys_Stat(source))
		printf_error_align("Code_GCC", "Source not found [%s]", source);
	
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
	command = Calloc(command, 2048);
	
	Tools_Command(command, mips64_gcc, "%s %s %s -o %s", gFlags, flags, source, output);
	if (!Sys_Stat(String_GetPath(output)))
		Sys_MakeDir(output);
	if (SysExe(command) > 0) printf_error_align("SysExe", "Failed");
	
	Make_Info("mips64-gcc", source);
	
	if (callback)
		callback(output, POST_GCC, command, NULL);
	Free(command);
}

void Code_LD(const char* source, const char* output, const char* flags, BinutilCallback callback) {
	MemFile entry = MemFile_Initialize();
	char* command;
	char entryDir[1024] = { 0 };
	u32 entryPoint = 0x80800000;
	
	if (!Sys_Stat(source))
		printf_error_align("Code_LD", "Source not found [%s]", source);
	
	if (callback) {
		switch (callback(source, PRE_LD, &entryPoint, &source)) {
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
	command = Calloc(command, 2048);
	
	strcpy(entryDir, String_GetPath(output));
	strcat(entryDir, ".entry/");
	strcat(entryDir, String_GetBasename(output));
	strcat(entryDir, "/entry.ld");
	Sys_MakeDir(String_GetPath(entryDir));
	
	MemFile_Malloc(&entry, 0x20);
	MemFile_Printf(&entry, "ENTRY_POINT = 0x%08X;\n", entryPoint);
	MemFile_SaveFile(&entry, entryDir);
	MemFile_Free(&entry);
	
	Tools_Command(command, mips64_ld, "-o %s %s -L%s %s", output, source, String_GetPath(entryDir), flags);
	if (!Sys_Stat(String_GetPath(output)))
		Sys_MakeDir(output);
	if (SysExe(command) > 0) printf_error_align("SysExe", "Failed");
	
	Make_Info("mips64-ld", source);
	
	if (callback)
		callback(output, POST_LD, command, NULL);
	Free(command);
}

void Code_ObjDump(char* cmd, const char* output) {
	MemFile linker = MemFile_Initialize();
	u32 lineNum = String_GetLineCount(cmd);
	char* txt = cmd;
	
	MemFile_Malloc(&linker, 0x9999);
	
	for (s32 i = 0; i < lineNum; i++) {
		char* line = String_GetLine(txt, 0);
		char* word;
		
		if (!MemMem(line, 3, "806", 3)) goto skip;
		if (!StrStr(line, " F ") && !StrStr(line, " O ")) goto skip;
		word = String_GetWord(line, 5);
		
		if (word[0] == 's' && !islower(word[1])) goto skip;
		if (StrStr(word, "flag") && strlen(word) == 4) goto skip;
		if (StrStr(word, "segment") && strlen(word) == 7) goto skip;
		for (s32 j = 0; j < strlen(word); j++)
			if (isdigit(word[j])) goto skip;
		
		MemFile_Printf(&linker, "%-24s = ", word);
		word = String_GetWord(line, 0);
		MemFile_Printf(&linker, "0x%s;\n", word);
skip:
		txt = String_Line(txt, 1);
	}
	
	MemFile_SaveFile(&linker, "include/z_lib_user.ld");
	MemFile_Free(&linker);
	Free(cmd);
}

static void Make_Linker_Thread(ThreadArg* arg) {
	ItemList itemList = ItemList_Initialize();
	const char* elf = "rom/lib_user/z_lib_user.elf";
	const char* bin = "rom/lib_user/z_lib_user.bin";
	const char* ld = "include/z_lib_user.ld";
	char* inputList = NULL;
	char* command = NULL;
	u32 inputStrLen = 0;
	u32 breaker = true;
	
	ItemList_List(&itemList, arg->path, -1, LIST_FILES);
	
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
	
	command = Calloc(command, 2048);
	inputList = Calloc(inputList, inputStrLen);
	
	for (s32 i = 0; i < itemList.num; i++) {
		if (!StrEnd(itemList.item[i], ".o"))
			continue;
		strcat(inputList, itemList.item[i]);
		strcat(inputList, " ");
	}
	
	if (true != false /* ELF */) {
		MemFile entry = MemFile_Initialize();
		
		MemFile_Malloc(&entry, 0x80);
		MemFile_Printf(&entry, "ENTRY_POINT = 0x80600000;\n");
		MemFile_SaveFile(&entry, "rom/lib_user/entry.ld");
		MemFile_Free(&entry);
		
		Tools_Command(command, mips64_ld, "-o %s %s %s", elf, inputList, arg->flag);
		if (!Sys_Stat(String_GetPath(elf)))
			Sys_MakeDir(elf);
		if (SysExe(command) > 0) printf_error_align("SysExe", "Failed");
		Log("Compiled: [%s]", elf);
	}
	if (true == true /* BIN */) {
		Tools_Command(command, mips64_objcopy, "-R .MIPS.abiflags -O binary %s %s", elf, bin);
		if (SysExe(command) > 0) printf_error_align("SysExe", "Failed");
		Log("Compiled: [%s]", bin);
	}
	if (false == false /* mips64_ld */) {
		Tools_Command(command, mips64_objdump, "-x -t %s", elf);
		if (!Sys_Stat(String_GetPath(ld)))
			Sys_MakeDir(ld);
		Code_ObjDump(SysExeO(command), ld);
		Log("Compiled: [%s]", ld);
	}
	
	Free(inputList);
	Free(command);
	ItemList_Free(&itemList);
	
	Make_Info("mips64-objdump", "z_code_lib.ld");
}

static void Make_Code_Thread_Elf(ThreadArg* arg) {
	char* output;
	char* input = arg->itemList->item[arg->i];
	
	if (!StrEnd(input, ".o")) {
		Log("Skipped " PRNT_DGRY "[%s]", input);
		
		return;
	}
	
	output = Calloc(output, strlen(input) + 10);
	strcpy(output, input);
	String_Replace(output, ".o", ".elf");
	String_Replace(output, "src/", "rom/");
	
	Code_LD(input, output, arg->flag, arg->callback);
	Free(output);
}

static void Make_Code_Thread_Obj(ThreadArg* arg) {
	char* output;
	char* input = arg->itemList->item[arg->i];
	
	if (!StrEnd(input, ".c")) {
		Log("Skipped " PRNT_DGRY "[%s]", input);
		
		return;
	}
	
	output = Calloc(output, strlen(input) + 10);
	strcpy(output, input);
	String_Replace(output, ".c", ".o");
	String_Replace(output, "src/", "rom/");
	
	Code_GCC(input, output, arg->flag, arg->callback);
	Free(output);
}

void Make_Code_Thread(ThreadArg* arg) {
	s32 i = 0;
	Thread thread[THREAD_NUM];
	ThreadArg passArg[THREAD_NUM];
	ItemList itemList = ItemList_Initialize();
	
	if (!Sys_Stat(arg->path)) {
		Log("Path not found [%s]", arg->path);
		
		return;
	}
	
	ItemList_List(&itemList, arg->path, -1, LIST_FILES);
	
	while (i < itemList.num) {
		u32 target = Clamp(itemList.num - i, 0, THREAD_NUM);
		
		for (s32 j = 0; j < target; j++) {
			memcpy(&passArg[j], arg, sizeof(ThreadArg));
			passArg[j].itemList = &itemList;
			passArg[j].i = i + j;
			
			if (gThreading) {
				Thread_Create(&thread[j], arg->func, &passArg[j]);
			} else {
				arg->func(&passArg[j]);
			}
		}
		
		if (gThreading)
			for (s32 j = 0; j < target; j++)
				Thread_Join(&thread[j]);
		
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
		"src/system/",
	};
	const char* pathO[] = {
		"rom/lib_user/",
		"rom/lib_code/",
		"rom/actor/",
		"rom/effect/",
		"rom/system/",
	};
	const char* flagObject[] = {
		gFlagsCode,
		gFlagsCode,
		"",
		"",
		"",
	};
	const char* flagLinker[] = {
		"-Lrom/lib_user -Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/ -T z64hdr.ld -T objects.ld --emit-relocs",
		gFlagsLink,
		gFlagsLink,
		gFlagsLink,
		gFlagsLink,
	};
	BinutilCallback callback[] = {
		NULL,
		Callback_Code,
		Callback_Actor,
		NULL,
		Callback_System,
	};
	Thread thread[ArrayCount(pathC)];
	ThreadArg args[ArrayCount(pathC)] = { 0 };
	
	if (gThreading)
		Thread_Init();
	
	for (s32 set = 0; set < ArrayCount(pathC); set++) {
		args[set].path = pathC[set];
		args[set].flag = flagObject[set];
		args[set].func = Make_Code_Thread_Obj;
		args[set].callback = callback[set];
		if (gThreading) {
			Thread_Create(&thread[set], Make_Code_Thread, &args[set]);
		} else {
			Make_Code_Thread(&args[set]);
		}
	}
	
	if (gThreading)
		Thread_Join(&thread[0]);
	memset(&args[0], 0, sizeof(args[0]));
	args[0].path = pathO[0];
	args[0].flag = flagLinker[0];
	
	if (gThreading) {
		Thread_Create(&thread[0], Make_Linker_Thread, &args[0]);
	} else {
		Make_Linker_Thread(&args[0]);
	}
	
	for (s32 set = 0; set < ArrayCount(pathC); set++) {
		if (gThreading)
			Thread_Join(&thread[set]);
		
		if (set == 0)
			continue;
		
		memset(&args[set], 0, sizeof(args[set]));
		args[set].path = pathO[set];
		args[set].func = Make_Code_Thread_Elf;
		args[set].callback = callback[set];
		args[set].flag = flagLinker[set];
		if (gThreading) {
			Thread_Create(&thread[set], Make_Code_Thread, &args[set]);
		} else {
			Make_Code_Thread(&args[set]);
		}
	}
	
	if (gThreading)
		for (s32 set = 1; set < ArrayCount(pathC); set++)
			Thread_Join(&thread[set]);
	
	if (gThreading)
		Thread_Free();
}

// # # # # # # # # # # # # # # # # # # # #
// # MAKE                                #
// # # # # # # # # # # # # # # # # # # # #

void Make(Rom* rom) {
	gFlags = Config_GetString(&rom->config, "mips64_gcc_flags");
	gFlagsCode = Config_GetString(&rom->config, "mips64_gcc_flags_code");
	gFlagsLink = Config_GetString(&rom->config, "mips64_ld_flags");
	
	if (gDevBuild)
		gFlags = Tmp_Printf("%s -DDEV_BUILD", gFlags);
	
	Tools_Get(z64audio);
	
	if (!gFlags || !gFlagsCode || !gFlagsLink)
		printf_error("[z64project.cfg] is missing mips64 flags! Please, do fresh dump!");
	
	if (gMakeTarget) {
		if (StrStrCase(gMakeTarget, "sound"))
			Make_Sound();
		if (StrStrCase(gMakeTarget, "code"))
			Make_Code();
	} else {
		Make_Sound();
		Make_Code();
	}
	
	printf_WinFix();
	
	if (sMake)
		printf_info_align("Make", "OK\n");
}
