#include "Tools.h"
#include <zip.h>

#ifdef _WIN32
const char* ZIP_BINUTIL = "tools/mips64-binutils-win32.zip";
const char* URL_BINUTIL_DOWNLOAD = "https://github.com/z64tools/z64rom/releases/download/z64rom_binutils/mips64-binutils-win32.zip";
#else
const char* ZIP_BINUTIL = "tools/mips64-binutils-linux.zip";
const char* URL_BINUTIL_DOWNLOAD = "https://github.com/z64tools/z64rom/releases/download/z64rom_binutils/mips64-binutils-linux.zip";
#endif

const char* ZIP_Z64HDR = "include/z64hdr.zip";
const char* URL_Z64HDR_DOWNLOAD = "https://codeload.github.com/z64tools/z64hdr/zip/refs/heads/temp-update";
const char* URL_Z64HDR_UPDT_API = "https://api.github.com/repos/z64tools/z64hdr";
const char* sTools[] = {
#ifdef _WIN32
	"tools\\mips64-binutils\\bin\\mips64-gcc.exe",
	"tools\\mips64-binutils\\bin\\mips64-ld.exe",
	"tools\\mips64-binutils\\bin\\mips64-objdump.exe",
	"tools\\mips64-binutils\\bin\\mips64-objcopy.exe",
	"tools\\z64audio.exe",
	"tools\\z64convert.exe",
	"tools\\novl.exe",
	"tools\\wget.exe",
#else
	"tools/mips64-binutils/bin/mips64-gcc",
	"tools/mips64-binutils/bin/mips64-ld",
	"tools/mips64-binutils/bin/mips64-objdump",
	"tools/mips64-binutils/bin/mips64-objcopy",
	"tools/z64audio",
	"tools/z64convert",
	"tools/novl",
	"wget",
#endif
};

bool gAutoDownload = true;

extern const char* gToolName;

static void Tools__CloseDialog(const char* toolName, bool askClose) {
	static u32 failCount = 0;
	const char* downMode[2] = {
		"automatic",
		"manual",
	};
	
	failCount++;
	
	if (failCount == 3) {
		printf("\n");
		
		printf_warning("Seems like download/installation has failed couple of times...");
		printf_info("Want to switch to " PRNT_REDD "%s mode" PRNT_RSET " and try again? Otherwise z64rom will close. " PRNT_DGRY "[y/n]", downMode[gAutoDownload]);
		if (Terminal_YesOrNo()) {
			gAutoDownload ^= 1;
			failCount = 0;
			
			return;
		}
	} else {
		if (gAutoDownload)
			return;
		printf("\n");
		if (askClose) {
			if (toolName)
				printf_warning("%s is required to use z64rom.", toolName);
			printf_info("Want to try again? Otherwise z64rom will close.");
			
			if (Terminal_YesOrNo())
				return;
		} else {
			printf_info("Please, try again!");
			
			return;
		}
	}
	
	exit(0);
}

static s32 Tools__FileDialog(const char* output) {
	char buffer[512];
	
	strcpy(buffer, Terminal_GetStr());
	
	String_Replace(buffer, "\n", "");
	String_Replace(buffer, "\"", "");
	
	if (!StrEnd(output, String_Extension(buffer))) {
		Terminal_ClearLines(3);
		printf_warning(
			"This does not seem to be valid " PRNT_REDD "%s " PRNT_RSET "file. Try again.",
			String_Extension(output)
		);
		
		return Tools__FileDialog(output);
	}
	
	Terminal_ClearLines(4);
	
	printf_info_align("Copying", PRNT_REDD "%s", String_GetFilename(buffer));
	if (Sys_Copy(buffer, output, false)) {
		Terminal_ClearLines(2);
		printf_warning(
			"Could not copy " PRNT_REDD "%s " PRNT_RSET "Try again? " PRNT_DGRY "[y/n]",
			buffer
		);
		
		return 1;
	}
	Terminal_ClearLines(2);
	
	return 0;
}

static s32 Tools__BinutilsSHA256(const char* zip, const char* shaFile) {
	MemFile code = MemFile_Initialize();
	MemFile arcmem = MemFile_Initialize();
	u8* hash;
	char* comp;
	
	MemFile_LoadFile(&code, shaFile);
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
			Terminal_ClearLines(2);
			printf_warning("Checksum Mismatch in " PRNT_REDD "%s" PRNT_RSET "!", zip);
			Tools__CloseDialog(NULL, false);
			
			return 1;
		}
	}
	
	MemFile_Free(&code);
	MemFile_Free(&arcmem);
	
	return 0;
}

static s32 Tools__ZipExtractCallback(const char* name, void* arg) {
	const char* filename = name;
	s32 i = strlen(filename);
	
redo:
	for (; i > 0; i--)
		if (filename[i - 1] == '/')
			break;
	filename = &filename[i];
	
	if (strlen(filename) <= 1) {
		i--;
		goto redo;
	}
	
	printf_prog_align("Extracting", filename, PRNT_REDD);
	
	return 0;
}

static void Tools__InstallHdr(s32 isUpdate) {
	const char* extract = "include/z64hdr-temp-update/";
	char command[512];
	ItemList itemList = ItemList_Initialize();
	
	if (isUpdate == false) {
		Sys_MakeDir("include/");
		printf_info("" PRNT_BLUE "z64hdr " PRNT_RSET "is a collection of files allowing to interact");
		printf_info("with the game with source code, usually in C.\n");
	}
	
	// if (Sys_Stat("include/"))
	// 	if (Sys_Delete_Recursive("include/"))
	// 		printf_error("Could not delete [include/]");
	
redo:
	if (gAutoDownload == true) {
		Tools_Command(command, wget, "%s -q --show-progress -O %s", URL_Z64HDR_DOWNLOAD, ZIP_Z64HDR);
		if (Sys_Command(command)) printf_error_align("Sys_Command", "Failed");
		Terminal_ClearLines(2);
	} else {
		printf_info("" PRNT_GRAY "%s", URL_Z64HDR_DOWNLOAD);
		printf_info("Download manually, drag and drop the file here and press enter.");
		
		if (Tools__FileDialog(ZIP_Z64HDR)) {
			if (Terminal_YesOrNo())
				goto redo;
			
			Tools__CloseDialog("z64hdr", true);
			goto redo;
		}
	}
	
	if (Tools__BinutilsSHA256(ZIP_Z64HDR, "tools/z64hdr-sha256"))
		goto redo;
	Terminal_ClearLines(1);
	
	if (zip_extract(ZIP_Z64HDR, "include/", Tools__ZipExtractCallback, 0)) printf_error_align("zip_extract", "Failed");
	Terminal_ClearLines(1);
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
	
	ItemList_Free(&itemList);
}

static char buffer[ArrayCount(sTools) + 1][256];

const char* Tools_Get(ToolIndex id) {
	static u8 init;
	
	if (!init) {
		init++;
		for (s32 i = 0; i < ArrayCount(sTools); i++)
			if (StrStr(sTools[i], "mips64")) {
				sprintf(
					buffer[i],
#ifdef _WIN32
					"set PATH=%stools\\mips64-binutils\\bin;%s && "
#else
					"export PATH=\"%s%stools\\mips64-binutils\\bin\" && "
#endif
					"%s",
					
#ifdef _WIN32
					Sys_AppDir(),
					"%PATH%",
#else
					"$PATH:",
					Sys_AppDir(),
#endif
					
					sTools[i]
				);
			} else {
				strcpy(buffer[i], sTools[i]);
			}
	}
	
	return buffer[id];
	
	return NULL;
}

s32 Tools_RegisterBlender(MemFile* mem) {
	char* blender = Config_GetString(mem, "blender_path");
	
	if (blender == NULL)
		return 0;
	
	strcpy(buffer[ArrayCount(sTools)], blender);
	
	return 1;
}

void Tools_Clean() {
	if (Sys_Stat(ZIP_BINUTIL))
		Sys_Delete(ZIP_BINUTIL);
	if (Sys_Stat(ZIP_Z64HDR))
		Sys_Delete(ZIP_Z64HDR);
	if (Sys_Stat("include/"))
		Sys_Delete_Recursive("include/");
	if (Sys_Stat("tools/mips64-binutils/"))
		Sys_Delete_Recursive("tools/mips64-binutils/");
}

s32 Tools_Validate_ReqrTools(void) {
	u32 fail = 0;
	
	const char* toolList[] = {
		"tools/binutils-sha256",
		"tools/z64hdr-sha256",
		"tools/z64audio.cfg",
		"tools/important64.dll",
#ifdef _WIN32
		"tools/novl.exe",
		"tools/z64audio.exe",
		"tools/z64convert.exe",
		
		"tools/wget.exe",
#else
		"tools/novl",
		"tools/z64audio",
		"tools/z64convert",
#endif
	};
	
	for (s32 i = 0; i < ArrayCount(toolList); i++) {
		if (!Sys_Stat(toolList[i])) {
			fail = true;
			printf_toolinfo(gToolName, PRNT_REDD "Failure\n\n");
			printf_warning_align("Missing Component", PRNT_REDD "%s", toolList[i]);
		}
	}
	
	if (fail) {
		printf("\n");
		printf_info("Get these files/tools from latest z64rom release: ");
		printf_info("" PRNT_BLUE "%s", "https://github.com/z64tools/z64rom/releases");
	}
	
	return fail;
}

s32 Tools_Validate_AddiTools(void) {
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

void Tools_Update_Header(void) {
	char command[512];
	char* output;
	char* line;
	char* word;
	MemFile ver = MemFile_Initialize();
	
	Tools_Command(command, wget, "%s -q -O -", URL_Z64HDR_UPDT_API);
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
		Tools__InstallHdr(false);
		
		MemFile_Malloc(&ver, 0x800);
		MemFile_Write(&ver, word, strlen(word));
		MemFile_SaveFile_String(&ver, "include/.version");
		Terminal_ClearLines(2);
		printf_info("Installed succesfully!");
	} else {
		MemFile_LoadFile_String(&ver, "include/.version");
		
		if (!strcmp(ver.str, word)) {
			printf_info("z64hdr is already up to date.");
			goto free;
		}
		
		Tools__InstallHdr(true);
		
		MemFile_Write(&ver, word, strlen(word));
		MemFile_SaveFile_String(&ver, "include/.version");
		printf_info("z64hdr is up to date!");
	}
	
free:
	printf("\n");
	MemFile_Free(&ver);
}

void Tools_Update_Binutils(void) {
	char command[512];
	
	if (Sys_Stat("tools/.failsafe")) {
		Sys_Delete_Recursive("tools/mips64-binutils/");
	}
	
	if (!Sys_Stat(sTools[0])) {
		printf_info(
			"" PRNT_BLUE "mips64-binutils" PRNT_RSET " is required for some " PRNT_REDD "rom patches" PRNT_RSET ", and "
		);
		printf_info("allows to make " PRNT_REDD "custom actors" PRNT_RSET " with z64rom.\n");
	}
	
	if (!Sys_Stat(ZIP_BINUTIL)) {
		Sys_MakeDir("tools/mips64-binutils/");
		
redo:
		if (gAutoDownload == false) {
			printf_info("" PRNT_GRAY "https://github.com/z64tools/z64rom/releases/tag/z64rom_binutils");
			printf_info("Download manually, drag and drop the file here and press enter.");
			
			if (Tools__FileDialog(ZIP_BINUTIL)) {
				if (Terminal_YesOrNo())
					goto redo;
				
				Tools__CloseDialog("mips64-binutils", true);
				goto redo;
			}
		} else {
			Tools_Command(command, wget, "%s -q --show-progress -O %s", URL_BINUTIL_DOWNLOAD, ZIP_BINUTIL);
			if (Sys_Command(command)) {
				printf("\n");
				printf_warning("Failed to initialize download... Try again? Otherwise we'll do this manually. " PRNT_DGRY "[y/n]");
				if (Terminal_YesOrNo())
					goto redo;
				goto redo;
			}
			Terminal_ClearLines(2);
		}
	}
	
	printf_info_align("Validating", PRNT_REDD "%s", ZIP_BINUTIL);
	
	if (Tools__BinutilsSHA256(ZIP_BINUTIL, "tools/binutils-sha256"))
		goto redo;
	Terminal_ClearLines(2);
	
	Sys_Touch("tools/.failsafe");
	if (zip_extract(ZIP_BINUTIL, "tools/mips64-binutils/", Tools__ZipExtractCallback, 0)) printf_error_align("zip_extract", "Failed");
	Terminal_ClearLines(1);
	Sys_Delete("tools/.failsafe");
	
	Terminal_ClearLines(2);
	printf_info("Installed succesfully!\n");
}

s32 Tools_Init(void) {
	static MemFile dll;
	s32 setupFlag = 0;
	
	if (Tools_Validate_ReqrTools()) {
		return -1;
	}
	
	if (Tools_Validate_AddiTools() || !Sys_Stat("include/z64hdr/") || Sys_Stat("tools/.failsafe") || Sys_Stat("tools/.installing")) {
		setupFlag = 1;
		dll = MemFile_Initialize();
		MemFile_LoadFile(&dll, "tools/important64.dll");
		
		if (Sys_Stat("tools/.installing"))
			Tools_Clean();
		
		Sys_Touch("tools/.installing");
		printf_toolinfo(gToolName, PRNT_BLUE "Initialization Setup\n\n");
		printf_info("Play some chill music? " PRNT_DGRY "[y/n]");
		
		if (Terminal_YesOrNo())
			Sound_Xm_Play(
				dll.data,
				dll.dataSize
			);
		Terminal_ClearLines(3);
		SleepF(0.2);
		
		printf_warning("Would you like to let z64rom handle installation automatically? " PRNT_DGRY "[y/n]");
		if (Terminal_YesOrNo()) {
			gAutoDownload = true;
		} else {
			gAutoDownload = false;
		}
		Terminal_ClearLines(3);
		SleepF(0.2);
		
		/* INIT */ {
			if (Tools_Validate_AddiTools() || Sys_Stat("tools/.failsafe"))
				Tools_Update_Binutils();
			if (!Sys_Stat("include/oot_mq_debug/z64hdr.h"))
				Tools_Update_Header();
		}
		
		if (!Tools_Validate_AddiTools() && Sys_Stat("include/z64hdr/")) {
			printf_info("All required tools have been installed succesfully!");
		}
		Sys_Delete("tools/.installing");
	}
	
	return setupFlag;
}
