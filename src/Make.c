#include "Make.h"

#define THREAD_NUM 42

extern s32 gLogOutput;
const char* gFlags = "-c -Iinclude/z64hdr -Iinclude/z64hdr/include -Iinclude/ "
	"-Isrc/lib_user -G 0 -O1 -fno-reorder-blocks -std=gnu99 -march=vr4300 -mabi=32"
	" -mips3 -mno-explicit-relocs -mno-memcpy -mno-check-zero-division -Wall"
	" -Wno-builtin-declaration-mismatch -Wno-unused-variable";
const char* gFlagsCode = "-mno-gpopt -fomit-frame-pointer";
const char* gFlagsLink = "-Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/ -T z64hdr.ld -T objects.ld -T z_lib_user.ld --emit-relocs";
u32 gThreading = true;

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

typedef s32 (* BinutilCallback)(const char*, PassType, void*);

typedef struct ThreadArg {
	const char* path;
	const char* flag;
	ItemList*   itemList;
	void (* func)(struct ThreadArg*);
	BinutilCallback callback;
	u32 i;
} ThreadArg;

static s32 Callback_System(const char* input, PassType type, void* arg) {
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
		if (!Sys_Stat(ovl) || Sys_Stat(input) > Sys_Stat(ovl))
			return CB_BUILD;
		
		return 0;
	}
	
	if (type == POST_LD) {
		// char* config = Tmp_Printf("%sconfig.cfg", String_GetPath(input));
		char* command = arg;
		char* info;
		
		info = String_GetFolder(input, -1);
		String_Remove(info, strlen("0x0000-"));
		String_Replace(info, "/", " ");
		
		printf_info_align("novl", "%s", info);
		
		Tools_Command(command, "novl", "-v -c -A 0x80800000 -o %s %s", ovl, input);
		if (Sys_Command(command)) printf_error_align("Sys_Command", "Failed");
	}
	
	return 0;
}

static s32 Callback_Actor(const char* input, PassType type, void* arg) {
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
		if (!Sys_Stat(ovl) || Sys_Stat(input) > Sys_Stat(ovl))
			return CB_BUILD;
		
		return 0;
	}
	
	if (type == POST_LD) {
		// char* config = Tmp_Printf("%sconfig.cfg", String_GetPath(input));
		char* command = arg;
		char* info;
		char* sys;
		
		Tools_Command(command, "mips64-objdump", "-t %s", input);
		sys = Sys_CommandOut(command);
		
		info = String_GetFolder(input, -1); String_Remove(info, strlen("0x0000-")); String_Replace(info, "/", " ");
		printf_info_align("novl", "%s", info);
		
		Tools_Command(command, "novl", "-v -c -A 0x80800000 -o %s %s", ovl, input);
		if (Sys_Command(command)) printf_error_align("Sys_Command", "Failed");
		Free(sys);
		
		return 0;
	}
	
	return 0;
}

static s32 Callback_Code(const char* input, PassType type, void* arg) {
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
		
		Tools_Command(command, "mips64-objcopy", "-R .MIPS.abiflags -O binary %s %s", input, output);
		if (Sys_Command(command)) printf_error_align("Sys_Command", "Failed");
	}
	
	return 0;
}

void Make_Code_GCC(const char* source, const char* output, const char* flags, BinutilCallback callback) {
	char* command;
	
	if (!Sys_Stat(source))
		printf_error_align("Make_Code_GCC", "Source not found [%s]", source);
	
	if (callback) {
		switch (callback(source, PRE_GCC, NULL)) {
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
	command = Calloc(0, 2048);
	
	Tools_Command(command, "mips64-gcc", "%s %s %s -o %s", gFlags, flags, source, output);
	if (!Sys_Stat(String_GetPath(output)))
		Sys_MakeDir(output);
	if (Sys_Command(command)) printf_error_align("Sys_Command", "Failed");
	
	printf_info_align("mips64-gcc", "%s", String_GetFilename(source));
	
	if (callback)
		callback(output, POST_GCC, command);
	Free(command);
}

void Make_Code_LD(const char* source, const char* output, const char* flags, BinutilCallback callback) {
	MemFile entry = MemFile_Initialize();
	char* command;
	char entryDir[1024] = { 0 };
	u32 entryPoint = 0x80800000;
	
	if (!Sys_Stat(source))
		printf_error_align("Make_Code_LD", "Source not found [%s]", source);
	
	if (callback) {
		switch (callback(source, PRE_LD, &entryPoint)) {
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
	command = Calloc(0, 2048);
	
	strcpy(entryDir, String_GetPath(output));
	strcat(entryDir, ".entry/");
	strcat(entryDir, String_GetBasename(output));
	strcat(entryDir, "/entry.ld");
	Sys_MakeDir(String_GetPath(entryDir));
	
	MemFile_Malloc(&entry, 0x20);
	MemFile_Printf(&entry, "ENTRY_POINT = 0x%08X;\n", entryPoint);
	MemFile_SaveFile(&entry, entryDir);
	MemFile_Free(&entry);
	
	Tools_Command(command, "mips64-ld", "-o %s %s -L%s %s", output, source, String_GetPath(entryDir), flags);
	if (!Sys_Stat(String_GetPath(output)))
		Sys_MakeDir(output);
	if (Sys_Command(command)) printf_error_align("Sys_Command", "Failed");
	
	printf_info_align("mips64-ld", "%s", String_GetFilename(source));
	
	if (callback)
		callback(output, POST_LD, command);
	Free(command);
}

void Make_Code_ObjDump(char* cmd, const char* output) {
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
	
	ItemList_Recursive(&itemList, arg->path, NULL, PATH_RELATIVE);
	
	if (!Sys_Stat(bin))
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
		
		Tools_Command(command, "mips64-ld", "-o %s %s %s", elf, inputList, arg->flag);
		if (!Sys_Stat(String_GetPath(elf)))
			Sys_MakeDir(elf);
		if (Sys_Command(command)) printf_error_align("Sys_Command", "Failed");
		Log("Compiled: [%s]", elf);
	}
	if (true == true /* BIN */) {
		Tools_Command(command, "mips64-objcopy", "-R .MIPS.abiflags -O binary %s %s", elf, bin);
		if (Sys_Command(command)) printf_error_align("Sys_Command", "Failed");
		Log("Compiled: [%s]", bin);
	}
	if (false == false /* LD */) {
		Tools_Command(command, "mips64-objdump", "-x -t %s", elf);
		if (!Sys_Stat(String_GetPath(ld)))
			Sys_MakeDir(ld);
		Make_Code_ObjDump(Sys_CommandOut(command), ld);
		Log("Compiled: [%s]", ld);
	}
	
	Free(inputList);
	Free(command);
	
	printf_info_align("mips64-objdump", "" PRNT_CYAN "z_code_lib.ld");
}

static void Make_Code_Thread_Elf(ThreadArg* arg) {
	char* output;
	char* input = arg->itemList->item[arg->i];
	
	if (!StrEnd(input, ".o")) {
		Log("Skipped " PRNT_DGRY "[%s]", input);
		
		return;
	}
	
	output = Calloc(0, strlen(input) + 10);
	strcpy(output, input);
	String_Replace(output, ".o", ".elf");
	String_Replace(output, "src/", "rom/");
	
	Make_Code_LD(input, output, arg->flag, arg->callback);
	Free(output);
}

static void Make_Code_Thread_Obj(ThreadArg* arg) {
	char* output;
	char* input = arg->itemList->item[arg->i];
	
	if (!StrEnd(input, ".c")) {
		Log("Skipped " PRNT_DGRY "[%s]", input);
		
		return;
	}
	
	output = Calloc(0, strlen(input) + 10);
	strcpy(output, input);
	String_Replace(output, ".c", ".o");
	String_Replace(output, "src/", "rom/");
	
	Make_Code_GCC(input, output, arg->flag, arg->callback);
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
	
	ItemList_Recursive(&itemList, arg->path, NULL, PATH_RELATIVE);
	
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
		// "src/system/",
	};
	const char* pathO[] = {
		"rom/lib_user/",
		"rom/lib_code/",
		"rom/actor/",
		"rom/effect/",
		// "rom/system/",
	};
	const char* flagObject[] = {
		gFlagsCode,
		gFlagsCode,
		"",
		"",
		// "",
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
		// Callback_System,
	};
	Thread thread[ArrayCount(pathC)];
	ThreadArg args[ArrayCount(pathC)] = { 0 };
	
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
	
	Thread_Free();
}

void Make(void) {
	Tools_Update();
	
	Make_Code();
	
	printf_info_align("Make", "OK");
}
