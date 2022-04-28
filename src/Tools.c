
#include "Tools.h"
#include <zip.h>

const char* sUrlHdr = "https://api.github.com/repos/z64tools/z64hdr";
const char* sTools[] = {
#ifdef _WIN32
	"tools\\mips64-binutils\\bin\\mips64-gcc.exe",
	"tools\\mips64-binutils\\bin\\mips64-ld.exe",
	"tools\\mips64-binutils\\bin\\mips64-objdump.exe",
	"tools\\mips64-binutils\\bin\\mips64-objcopy.exe",
	"tools\\z64audio.exe",
	"tools\\novl.exe",
	"tools\\wget.exe",
#else
	"tools/mips64-binutils/bin/mips64-gcc",
	"tools/mips64-binutils/bin/mips64-ld",
	"tools/mips64-binutils/bin/mips64-objdump",
	"tools/mips64-binutils/bin/mips64-objcopy",
	"tools/z64audio",
	"tools/novl",
	"wget",
#endif
};

const char* Tools_Get(const char* app) {
	static char buffer[ArrayCount(sTools)][256];
	static u8 init;
	
	if (!init) {
		init++;
		for (s32 i = 0; i < ArrayCount(sTools); i++)
			if (StrStr(sTools[i], "mips64")) {
				sprintf(
					buffer[i],
					"set PATH=%stools\\mips64-binutils\\bin;%s && "
					"%s",
					
					Sys_AppDir(),
					"%PATH%",
					
					sTools[i]
				);
			} else {
				strcpy(buffer[i], sTools[i]);
			}
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
	
	if (StrStr(app, "wget"))
		return buffer[6];
	
	return NULL;
}

static void Tools_Validate() {
	for (s32 i = 0; i < ArrayCount(sTools); i++) {
#ifndef _WIN32
		if (StrStr(sTools[i], "wget"))
			continue;
#endif
		if (!Sys_Stat(sTools[i]))
			printf_error_align("NOT_FOUND", "[ %s ]", sTools[i]);
	}
}

void Tools_Update_Header() {
	const char* extract = "include/z64hdr-temp-update/";
	const char* zip = "include/z64hdr.zip";
	const char* download = "https://codeload.github.com/z64tools/z64hdr/zip/refs/heads/temp-update";
	char command[512];
	ItemList itemList = ItemList_Initialize();
	
	if (Sys_Stat("include/"))
		if (Sys_Delete_Recursive("include/"))
			printf_error("Could not delete [include/]");
	Sys_MakeDir("include/");
	
	cliprintf(command, Tools_Get("wget"), "%s -q -O %s", download, zip);
	if (Sys_Command(command)) printf_error_align("Sys_Command", "Failed");
	if (zip_extract(zip, "include/", 0, 0)) printf_error_align("zip_extract", "Failed");
	ItemList_Recursive(&itemList, extract, NULL, PATH_RELATIVE);
	
	for (s32 i = 0; i < itemList.num; i++) {
		char* input = itemList.item[i];
		char* output = strdup(input);
		
		String_Replace(output, extract + strlen("include/"), "z64hdr/");
		Sys_MakeDir(String_GetPath(output));
		Sys_Rename(input, output);
		
		Free(output);
	}
	
	Sys_Touch("include/objects.ld");
	if (Sys_Delete_Recursive(extract)) printf_error("Could not delete [%s]", extract);
	Sys_Delete(zip);
	
	ItemList_Free(&itemList);
	printf_info_align("Updated", "z64hdr");
}

void Tools_Update(void) {
	char command[512];
	char* output;
	char* line;
	char* word;
	MemFile ver = MemFile_Initialize();
	
	Tools_Validate();
	
	Tools_Command(command, "wget", "%s -q -O -", sUrlHdr);
	output = Sys_CommandOut(command);
	
	line = StrStr(output, "\"pushed_at\":");
	
	if (line == NULL) {
		printf_warning_align("Updater", "Failed to check z64hdr update");
		
		return;
	}
	
	word = String_GetWord(line, 1);
	String_Replace(word, "\"", "");
	String_Replace(word, ",", "");
	
	if (!Sys_Stat("include/.version")) {
		Tools_Update_Header();
		
		MemFile_Malloc(&ver, 0x800);
		MemFile_Write(&ver, word, strlen(word));
		MemFile_SaveFile_String(&ver, "include/.version");
	} else {
		MemFile_LoadFile_String(&ver, "include/.version");
		
		if (!strcmp(ver.str, word))
			return;
		
		Tools_Update_Header();
		
		MemFile_Write(&ver, word, strlen(word));
		MemFile_SaveFile_String(&ver, "include/.version");
	}
}
