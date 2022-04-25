#include "Make.h"

#define THREAD_NUM 32

volatile bool gMsgCompiled;
const char* gAppPath;
const char* gTools[] = {
#ifdef _WIN32
	"tools\\mips64-binutils-win32\\bin\\mips64-gcc.exe",
	"tools\\mips64-binutils-win32\\bin\\mips64-ld.exe",
	"tools\\mips64-binutils-win32\\bin\\mips64-objdump.exe",
	"tools\\mips64-binutils-win32\\bin\\mips64-objcopy.exe",
	"tools\\z64audio.exe",
	"tools\\novl.exe",
#else
	"tools/mips64-binutils/bin/mips64-gcc",
	"tools/mips64-binutils/bin/mips64-ld",
	"tools/mips64-binutils/bin/mips64-objdump",
	"tools/mips64-binutils/bin/mips64-objcopy",
	"tools/z64audio",
	"tools/novl",
#endif
};
const char* gFlags = "-c -Iinclude/z64hdr -Iinclude/z64hdr/include -Iinclude/ "
	"-Isrc/lib_user -G 0 -O1 -fno-reorder-blocks -std=gnu99 -march=vr4300 -mabi=32"
	" -mips3 -mno-explicit-relocs -mno-memcpy -mno-check-zero-division -Wall"
	" -Wno-builtin-declaration-mismatch -Wno-unused-variable";
const char* gFlagsCode = "-mno-gpopt -fomit-frame-pointer";

static const char* Make_Tool(const char* app) {
	static char buffer[6][256];
	static u8 init;
	
	if (!init) {
		init++;
		for (s32 i = 0; i < 6; i++)
			sprintf(
				buffer[i],
				"set PATH=%stools\\mips64-binutils-win32\\bin;%s && "
				"%s",
				
				gAppPath,
				"%PATH%",
				
				gTools[i]
			);
	}
	
	if (StrStr(app, "mips64-gcc"))
		return buffer[0];
	
	if (StrStr(app, "mips64-ld"))
		return buffer[1];
	
	if (StrStr(app, "mips64-objdump"))
		return buffer[2];
	
	if (StrStr(app, "mips64-objcopy"))
		return buffer[3];
	
	if (StrStr(app, "z64audio"))
		return buffer[4];
	
	if (StrStr(app, "novl"))
		return buffer[5];
	
	return NULL;
}

void Make_Compile_Object(const char* source, const char* output, const char* flags, u32 depNum, const char* dep[]) {
	char command[1024] = { 0 };
	
	if (!Sys_Stat(source))
		printf_error_align("Make_Compile_Object", "Source not found [%s]", source);
	
	// if (Sys_Stat(output) && Sys_Stat(output) >= Sys_Stat(source))
	// 	return;
	//
	// // Check if newer than output
	// if (depNum) {
	// 	for (s32 i = 0;; i++) {
	// 		if (Sys_Stat(dep[i]) > Sys_Stat(output))
	// 			break;
	// 		if (i == depNum - 1)
	// 			return;
	// 	}
	// }
	
	sprintf(
		command,
		"%s %s %s %s -o %s",
		Make_Tool("mips64-gcc"),
		gFlags,
		flags,
		source,
		output
	);
	
	if (!Sys_Stat(String_GetPath(output)))
		Sys_MakeDir(output);
	if (Sys_Command(command)) printf_error("Stopping");
	gMsgCompiled = true;
	
	Log("Compiled: [%s]", output);
}

void Make_Compile_Elf(const char* source, const char* output, const char* flags, u32 depNum, const char* dep[]) {
	MemFile entry = MemFile_Initialize();
	char command[1024] = { 0 };
	char entryDir[1024] = { 0 };
	
	if (!Sys_Stat(source))
		printf_error_align("Make_Compile_Elf", "Source not found [%s]", source);
	
	// if (Sys_Stat(output) && Sys_Stat(output) >= Sys_Stat(source))
	// 	return;
	//
	// // Check if newer than output
	// if (depNum) {
	// 	for (s32 i = 0;; i++) {
	// 		if (Sys_Stat(dep[i]) > Sys_Stat(output))
	// 			break;
	// 		if (i == depNum - 1)
	// 			return;
	// 	}
	// }
	
	strcpy(entryDir, String_GetPath(output));
	strcat(entryDir, ".entry/");
	strcat(entryDir, String_GetBasename(output));
	strcat(entryDir, "/entry.ld");
	printf_info("mkdir %s", String_GetPath(entryDir));
	Sys_MakeDir(String_GetPath(entryDir));
	
	MemFile_Malloc(&entry, 0x20);
	MemFile_Printf(&entry, "ENTRY_POINT = 0x%08X;\n", 0x80800000);
	MemFile_SaveFile(&entry, entryDir);
	MemFile_Free(&entry);
	
	sprintf(
		command,
		"%s -o %s %s -L%s %s",
		Make_Tool("mips64-ld"),
		output,
		source,
		String_GetPath(entryDir),
		flags
	);
	
	if (!Sys_Stat(String_GetPath(output)))
		Sys_MakeDir(output);
	if (Sys_Command(command)) printf_error("Stopping");
	Log("Compiled: [%s]", output);
}

ItemList sItemListLib;

static void Compile_Library_Obj(u32 i) {
	char output[512];
	char* input = sItemListLib.item[i];
	
	if (input[strlen(input) - 1] != 'c' || input[strlen(input) - 2] != '.')
		return;
	
	Log("Input [%s]", input);
	
	strcpy(output, input);
	String_Replace(output, ".c", ".o");
	String_Replace(output, "src/", "rom/");
	
	Make_Compile_Object(input, output, gFlagsCode, 0, NULL);
}

static void Compile_Library_Ld(char* cmd) {
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
		MemFile_Printf(&linker, "%s;\n", word);
skip:
		txt = String_Line(txt, 1);
	}
	
	MemFile_SaveFile(&linker, "include/z_lib_user.ld");
	MemFile_Free(&linker);
	Free(cmd);
}

void Make_Library() {
	s32 i = 0;
	Thread thread[THREAD_NUM];
	char cli[512] = { 0 };
	
	gMsgCompiled = false;
	if (!Sys_Stat("src/lib_user/")) {
		Log("src/lib_user/ not found");
		
		return;
	}
	
	ItemList_Recursive(&sItemListLib, "src/lib_user/", NULL, PATH_RELATIVE);
	
	while (i < sItemListLib.num) {
		u32 target = Clamp(sItemListLib.num - i, 0, THREAD_NUM);
		
		for (s32 j = 0; j < target; j++) {
			Thread_Create(&thread[j], Compile_Library_Obj, (void*)(i + j));
		}
		
		for (s32 j = 0; j < target; j++)
			Thread_Join(&thread[j]);
		
		i += THREAD_NUM;
	}
	
	if (gMsgCompiled == false) {
		Log("Nothing Compiled");
		
		return;
	}
	
	if (Sys_Stat("rom/lib_user/entry.ld") == false) {
		MemFile entry = MemFile_Initialize();
		
		MemFile_Malloc(&entry, 0x160);
		MemFile_Printf(&entry, "ENTRY_POINT = 0x80600000;\n");
		if (MemFile_SaveFile(&entry, "rom/lib_user/entry.ld"))
			printf_error("Could not save [rom/lib_user/entry.ld]");
		MemFile_Free(&entry);
	}
	
	strcpy(cli, Make_Tool("mips64-ld"));
	strcat(cli, " -o rom/lib_user/z_lib_user.elf ");
	for (i = 0; i < sItemListLib.num; i++) {
		char output[256];
		if (!MemMem(sItemListLib.item[i], strlen(sItemListLib.item[i]) + 1, ".c\0", 3))
			continue;
		
		strcpy(output, sItemListLib.item[i]);
		String_Replace(output, ".c", ".o ");
		String_Replace(output, "src/", "rom/");
		
		strcat(cli, output);
	}
	strcat(
		cli,
		"-Lrom/lib_user "
		"-Linclude/z64hdr/oot_mq_debug/ "
		"-Linclude/z64hdr/common/ "
		"-Linclude/ "
		"-T z64hdr.ld "
		// "-T objects.ld "
		"--emit-relocs"
	);
	Sys_Command(cli);
	
	strcpy(cli, Make_Tool("mips64-objcopy"));
	strcat(cli, " -R .MIPS.abiflags -O binary rom/lib_user/z_lib_user.elf rom/lib_user/z_lib_user.bin");
	Sys_Command(cli);
	
	strcpy(cli, Make_Tool("mips64-objdump"));
	strcat(cli, " -x -t rom/lib_user/z_lib_user.elf");
	
	Compile_Library_Ld(Sys_CommandGet(cli));
	ItemList_Free(&sItemListLib);
}

ItemList sItemListCode;

static void Compile_Code_Obj(u32 i) {
	char output[512];
	char* input = sItemListCode.item[i];
	
	if (input[strlen(input)] != 'c' || input[strlen(input) - 1] != '.')
		return;
	
	strcpy(output, input);
	String_Replace(output, ".c", ".o");
	String_Replace(output, "src/", "rom/");
	
	Make_Compile_Object(input, output, gFlagsCode, 0, NULL);
}

void Make_Code_Obj() {
	s32 i = 0;
	Thread thread[THREAD_NUM];
	
	gMsgCompiled = false;
	if (!Sys_Stat("src/lib_code/")) {
		Log("src/lib_code/ not found");
		
		return;
	}
	ItemList_Recursive(&sItemListCode, "src/lib_code/", NULL, PATH_RELATIVE);
	
	while (i < sItemListCode.num) {
		u32 target = Clamp(sItemListCode.num - i, 0, THREAD_NUM);
		
		for (s32 j = 0; j < target; j++)
			Thread_Create(&thread[j], Compile_Code_Obj, (void*)(i + j));
		
		for (s32 j = 0; j < target; j++)
			Thread_Join(&thread[j]);
		
		i += THREAD_NUM;
	}
	
	ItemList_Free(&sItemListCode);
}

static void Compile_Code_Elf(u32 i) {
	char output[512];
	char* input = sItemListCode.item[i];
	
	if (input[strlen(input)] != 'o' || input[strlen(input) - 1] != '.')
		return;
	
	strcpy(output, input);
	String_Replace(output, ".o", ".elf");
	
	Make_Compile_Elf(input, output, "-Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/ -T z64hdr.ld -T z_lib_user.ld --emit-relocs", 0, NULL);
}

void Make_Code_Elf() {
	s32 i = 0;
	Thread thread[THREAD_NUM];
	
	gMsgCompiled = false;
	if (!Sys_Stat("src/lib_code/")) {
		Log("src/lib_code/ not found");
		
		return;
	}
	ItemList_Recursive(&sItemListCode, "src/lib_code/", NULL, PATH_RELATIVE);
	
	while (i < sItemListCode.num) {
		u32 target = Clamp(sItemListCode.num - i, 0, THREAD_NUM);
		
		for (s32 j = 0; j < target; j++)
			Thread_Create(&thread[j], Compile_Code_Elf, (void*)(i + j));
		
		for (s32 j = 0; j < target; j++)
			Thread_Join(&thread[j]);
		
		i += THREAD_NUM;
	}
	
	ItemList_Free(&sItemListCode);
}

ItemList sItemListActor;

static void Compile_Actor_Obj(u32 i) {
	char output[512];
	char* input = sItemListActor.item[i];
	
	if (input[strlen(input)] != 'c' || input[strlen(input) - 1] != '.')
		return;
	
	strcpy(output, input);
	String_Replace(output, ".c", ".o");
	String_Replace(output, "src/", "rom/");
	
	Make_Compile_Object(input, output, "", 0, NULL);
}

void Make_Actor_Obj() {
	s32 i = 0;
	Thread thread[THREAD_NUM];
	
	gMsgCompiled = false;
	if (!Sys_Stat("src/actor/")) {
		Log("src/actor/ not found");
		
		return;
	}
	ItemList_Recursive(&sItemListCode, "src/actor/", NULL, PATH_RELATIVE);
	
	while (i < sItemListActor.num) {
		u32 target = Clamp(sItemListActor.num - i, 0, THREAD_NUM);
		
		for (s32 j = 0; j < target; j++)
			Thread_Create(&thread[j], Compile_Actor_Obj, (void*)(i + j));
		
		for (s32 j = 0; j < target; j++)
			Thread_Join(&thread[j]);
		
		i += THREAD_NUM;
	}
	
	ItemList_Free(&sItemListActor);
}

void Make(void) {
	Thread thread[2];
	
	gAppPath = Sys_AppDir();
	
	Make_Library();
	Thread_Create(&thread[0], Make_Code_Obj, NULL);
	Thread_Create(&thread[1], Make_Actor_Obj, NULL);
	Thread_Join(&thread[0]);
	Thread_Join(&thread[1]);
	
	Make_Code_Elf();
	
	printf_info("Compiled");
	
	exit(0);
}