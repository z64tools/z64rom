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

s32 Tools_Validate(void) {
	for (s32 i = 0; i < ArrayCount(sTools); i++) {
#ifndef _WIN32
		if (StrStr(sTools[i], "wget"))
			continue;
#endif
		if (!Sys_Stat(sTools[i]))
			return 1;
	}
	
	return 0;
}

static s32 Tools_BinutilsSHA256(const char* zip) {
	MemFile code = MemFile_Initialize();
	MemFile arcmem = MemFile_Initialize();
	u8 buf[64];
	u8* hash;
	char* comp;
	
	MemFile_LoadFile(&code, "tools/binutils-sha256");
	MemFile_LoadFile(&arcmem, zip);
	
	hash = Sys_Sha256(arcmem.data, arcmem.dataSize);
	
#ifdef _WIN32
	comp = StrStr(code.str, "win32 = ");
#else
	comp = StrStr(code.str, "linux = ");
#endif
	comp += strlen("win32 = ");
	
	for (s32 i = 0; i < 32; i++) {
		char* a = &comp[i * 2];
		char b[3];
		
		sprintf(b, "%02X", hash[i]);
		
		if (strncmp(a, b, 2)) {
			printf("\n");
			printf_warning("Checksum Mismatch! Try Again? " PRNT_DGRY "[y/n]");
			if (printf_get_answer()) {
				printf("\n");
				MemFile_Free(&code);
				MemFile_Free(&arcmem);
				Sys_Delete(zip);
				
				return -1;
			}
			exit(1);
		}
	}
	
	MemFile_Free(&code);
	MemFile_Free(&arcmem);
	
	return 0;
}

static void Tools_Header() {
	const char* extract = "include/z64hdr-temp-update/";
	const char* zip = "include/z64hdr.zip";
	const char* download = "https://codeload.github.com/z64tools/z64hdr/zip/refs/heads/temp-update";
	char command[512];
	ItemList itemList = ItemList_Initialize();
	
	if (Sys_Stat("include/"))
		if (Sys_Delete_Recursive("include/"))
			printf_error("Could not delete [include/]");
	Sys_MakeDir("include/");
	
	cliprintf(command, Tools_Get("wget"), "%s -q --show-progress -O %s", download, zip);
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
}

void Tools_Update_Header(void) {
	char command[512];
	char* output;
	char* line;
	char* word;
	MemFile ver = MemFile_Initialize();
	
	printf_info_align("Validating", "z64hdr");
	Tools_Command(command, "wget", "%s -q -O -", sUrlHdr);
	output = Sys_CommandOut(command);
	
	line = StrStr(output, "\"pushed_at\":");
	
	if (line == NULL) {
		printf_warning("Could not retrieve update information of z64hdr...");
		
		goto free;
	}
	
	word = String_GetWord(line, 1);
	String_Replace(word, "\"", "");
	String_Replace(word, ",", "");
	
	if (!Sys_Stat("include/.version")) {
		Tools_Header();
		
		MemFile_Malloc(&ver, 0x800);
		MemFile_Write(&ver, word, strlen(word));
		MemFile_SaveFile_String(&ver, "include/.version");
		printf_info("" PRNT_CYAN "z64hdr" PRNT_RSET " has been installed succesfully!");
	} else {
		MemFile_LoadFile_String(&ver, "include/.version");
		
		if (!strcmp(ver.str, word)) {
			printf_info("" PRNT_CYAN "z64hdr" PRNT_RSET " is already up to date.");
			goto free;
		}
		
		Tools_Header();
		
		MemFile_Write(&ver, word, strlen(word));
		MemFile_SaveFile_String(&ver, "include/.version");
		printf_info("" PRNT_CYAN "z64hdr" PRNT_RSET " is up to date!");
	}
	
free:
	printf("\n");
	MemFile_Free(&ver);
}

static s32 zipExtract(const char* name, void* arg) {
	printf_prog_align("Extracting", name);
	
	return 0;
}

void Tools_Update_Binutils(void) {
	static u32 lastChoice;
	char command[512];
	char buffer[512] = { 0 };
	
#ifdef _WIN32
	char* zip = "tools/mips64-binutils-win32.zip";
	const char* url = "https://github.com/z64tools/z64rom/releases/download/z64rom_binutils/mips64-binutils-win32.zip";
#else
	char* zip = "tools/mips64-binutils-linux.zip";
	const char* url = "https://github.com/z64tools/z64rom/releases/download/z64rom_binutils/mips64-binutils-linux.zip";
#endif
	
	if (!Sys_Stat(zip)) {
		Sys_MakeDir("tools/mips64-binutils/");
		printf_warning("" PRNT_CYAN "mips64-binutils" PRNT_RSET " is required for some " PRNT_YELW "rom patches" PRNT_RSET ", and allows to make " PRNT_YELW "custom actors" PRNT_RSET " with z64rom.");
		printf_info("Download " PRNT_CYAN "mips64-binutils" PRNT_RSET " automatically?" PRNT_DGRY " [y/n]");
		
		if (!printf_get_answer()) {
			printf("\n");
manual:
			printf_warning("Download manually, drag and drop the file here and press enter.");
			printf_info("" PRNT_CYAN "https://github.com/z64tools/z64rom/releases/tag/z64rom_binutils");
			fgets(buffer, 511, stdin);
			
			String_Replace(buffer, "\n", "");
			String_Replace(buffer, "\"", "");
			zip = buffer;
			
			if (StrStr(buffer, "mips64-binutils") && Sys_Stat(buffer)) {
				goto sha;
			} else {
				printf("\n");
				printf_warning("Could not find " PRNT_YELW "%s " PRNT_RSET "Try again? " PRNT_DGRY "[y/n]", zip);
				if (printf_get_answer()) goto manual;
				else goto close;
			}
close:
			printf_warning("Could not SDFASDFADSf");
			SleepS(3);
			exit(0);
		}
		
		lastChoice = 1;
automatic:
		cliprintf(command, Tools_Get("wget"), "%s -q --show-progress -O %s", url, zip);
		if (Sys_Command(command)) {
			printf("\n");
			printf_warning("Automatic download failed...");
			goto manual;
		}
	}
	
sha:
	printf("\n");
	
	printf_info_align("Validating", "%s", zip);
	if (Tools_BinutilsSHA256(zip)) {
		if (lastChoice == 1) goto automatic;
		goto manual;
	}
	
	if (zip_extract(zip, "tools/mips64-binutils/", zipExtract, 0)) printf_error_align("zip_extract", "Failed");
	printf("\n");
	
	printf_info("" PRNT_CYAN "mips64-binutils" PRNT_RSET " have been installed succesfully!\n");
}

void Tools_Init(void) {
	if (!Sys_Stat(sTools[0]))
		Tools_Update_Binutils();
	Tools_Validate();
	if (!Sys_Stat("include/"))
		Tools_Update_Header();
}
