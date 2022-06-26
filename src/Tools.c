#include "z64rom.h"
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
const char* URL_Z64HDR_DOWNLOAD = "https://codeload.github.com/rankaisija64/z64hdr/zip/refs/heads/main-dev";
const char* URL_Z64HDR_UPDT_API = "https://api.github.com/repos/z64tools/z64hdr";
const char* sTools[] = {
#ifdef _WIN32
	"tools\\mips64-binutils\\bin\\mips64-gcc.exe",
	"tools\\mips64-binutils\\bin\\mips64-ld.exe",
	"tools\\mips64-binutils\\bin\\mips64-objdump.exe",
	"tools\\mips64-binutils\\bin\\mips64-objcopy.exe",
	"tools\\z64audio.exe",
	"tools\\z64convert.exe",
	"tools\\seq64_console.exe",
	"tools\\seqas.exe",
	"tools\\novl.exe",
	"tools\\wget.exe",
#else
	"tools/mips64-binutils/bin/mips64-gcc",
	"tools/mips64-binutils/bin/mips64-ld",
	"tools/mips64-binutils/bin/mips64-objdump",
	"tools/mips64-binutils/bin/mips64-objcopy",
	"tools/z64audio",
	"tools/z64convert",
	"tools/seq64_console",
	"tools/seqas",
	"tools/novl",
	"wget",
#endif
};
static char buffer[ArrayCount(sTools) + 1][256];
bool sAutoDownload = true;

static void Tools_CloseDialog(const char* toolName, bool askClose) {
	static u32 failCount = 0;
	const char* downMode[2] = {
		"automatic",
		"manual",
	};
	
	failCount++;
	
	if (failCount == 3) {
		printf("\n");
		
		printf_warning("Seems like download/installation has failed couple of times...");
		printf_info("Want to switch to " PRNT_REDD "%s mode" PRNT_RSET " and try again? Otherwise z64rom will close. " PRNT_DGRY "[y/n]", downMode[sAutoDownload]);
		if (Terminal_YesOrNo()) {
			sAutoDownload ^= 1;
			failCount = 0;
			
			return;
		}
	} else {
		if (sAutoDownload)
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

static s32 Tools_FileDialog(const char* output) {
	char* buffer;
	
	if (gAutoInstall == false) {
		if (!strcmp(ZIP_Z64HDR, output))
			buffer = StrUnq(gFile_z64hdr);
		else if (!strcmp(ZIP_BINUTIL, output))
			buffer = StrUnq(gFile_mips64);
		else
			buffer = StrUnq(Terminal_GetStr());
	} else {
		buffer = StrUnq(Terminal_GetStr());
	}
	
	if (buffer == NULL || !StrEnd(output, FileExtension(buffer))) {
		Terminal_ClearLines(3);
		printf_warning(
			"This does not seem to be valid " PRNT_REDD "%s " PRNT_RSET "file. Try again.",
			FileExtension(output)
		);
		
		return Tools_FileDialog(output);
	}
	
	Terminal_ClearLines(4);
	
	printf_info_align("Copying", PRNT_REDD "%s", Filename(buffer));
	if (Sys_Copy(buffer, output)) {
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

static s32 Tools_BinutilsSHA256(const char* zip, const char* shaFile) {
	MemFile code = MemFile_Initialize();
	MemFile arcmem = MemFile_Initialize();
	u8* hash;
	char* comp;
	
	if (MemFile_LoadFile_String(&code, shaFile)) printf_error("Could not open [%s]", shaFile);
	if (MemFile_LoadFile(&arcmem, zip)) printf_error("Could not open [%s]", zip);
	
	hash = Sys_Sha256(arcmem.data, arcmem.size);
	
#ifdef _WIN32
	comp = StrStr(code.str, "win32 = ");
	comp += strlen("win32 = ");
#else
	comp = StrStr(code.str, "linux = ");
	comp += strlen("linux = ");
#endif
	
	for (s32 i = 0; i < 32; i++) {
		char* a = &comp[i * 2];
		char b[3];
		
		sprintf(b, "%02X", hash[i]);
		
		if (strncmp(a, b, 2)) {
			Terminal_ClearLines(2);
			printf_warning("Checksum Mismatch in " PRNT_REDD "%s" PRNT_RSET "!", zip);
			Tools_CloseDialog(NULL, false);
			
			return 1;
		}
	}
	
	MemFile_Free(&code);
	MemFile_Free(&arcmem);
	
	return 0;
}

static s32 Tools_ZipExtractCallback(const char* name, void* arg) {
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

static void Tools_Clean(void) {
	if (Sys_Stat(ZIP_BINUTIL))
		Sys_Delete(ZIP_BINUTIL);
	if (Sys_Stat(ZIP_Z64HDR))
		Sys_Delete(ZIP_Z64HDR);
	if (Sys_Stat("include/")) {
		Sys_Rename("include/sequence.inc", "sequence.inc");
		Sys_Rename("include/ulib_linker.ld", "ulib_linker.ld");
		
		Sys_Delete_Recursive("include/");
		Sys_MakeDir("include/");
		Sys_Rename("sequence.inc", "include/sequence.inc");
		Sys_Rename("ulib_linker.ld", "include/ulib_linker.ld");
	}
	if (Sys_Stat("tools/mips64-binutils/"))
		Sys_Delete_Recursive("tools/mips64-binutils/");
}

static s32 Tools_ValidateTools_Required(void) {
	u32 fail = 0;
	
	const char* toolList[] = {
		"include/sequence.inc",
		"include/ulib_linker.ld",
		
		"tools/actor-object-deb.cfg",
		"tools/binutils-sha256",
		"tools/z64hdr-sha256",
		"tools/z64audio.cfg",
		"tools/important64.dll",
		
#ifdef _WIN32
		"tools/novl.exe",
		"tools/z64audio.exe",
		"tools/z64convert.exe",
		"tools/seq64_console.exe",
		"tools/seqas.exe",
		
		"tools/wget.exe",
#else
		"tools/novl",
		"tools/z64audio",
		"tools/z64convert",
		"tools/seq64_console",
		"tools/seqas",
#endif
	};
	
	for (s32 i = 0; i < ArrayCount(toolList); i++) {
		if (!Sys_Stat(toolList[i])) {
			fail = true;
			printf_toolinfo(gToolName, PRNT_REDD "Failure");
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

static s32 Tools_ValidateTools_Additional(void) {
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
					"export PATH=\"%s%stools/mips64-binutils/bin\" && "
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

void Tools_Install_mips64(void) {
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
		if (sAutoDownload == false) {
			printf_info("" PRNT_GRAY "https://github.com/z64tools/z64rom/releases/tag/z64rom_binutils");
			printf_info("Download manually, drag and drop the file here and press enter.");
			
			if (Tools_FileDialog(ZIP_BINUTIL)) {
				if (Terminal_YesOrNo())
					goto redo;
				
				Tools_CloseDialog("mips64-binutils", true);
				goto redo;
			}
		} else {
			Tools_Command(command, wget, "%s -q --no-hsts --show-progress -O %s", URL_BINUTIL_DOWNLOAD, ZIP_BINUTIL);
			if (SysExe(command)) {
				printf("\n");
				printf_warning("Failed to initialize download... Try again? Otherwise we'll do this manually. " PRNT_DGRY "[y/n]");
				if (Terminal_YesOrNo() == false)
					sAutoDownload = false;
				goto redo;
			}
			Terminal_ClearLines(2);
		}
	}
	
	printf_info_align("Validating", PRNT_REDD "%s", ZIP_BINUTIL);
	
	if (Tools_BinutilsSHA256(ZIP_BINUTIL, "tools/binutils-sha256"))
		goto redo;
	Terminal_ClearLines(2);
	
	Sys_Touch("tools/.failsafe");
	if (zip_extract(ZIP_BINUTIL, "tools/mips64-binutils/", Tools_ZipExtractCallback, 0)) printf_error_align("zip_extract", "Failed");
	Terminal_ClearLines(1);
	
#ifndef _WIN32
	SysExe("chmod -R u+x tools/mips64-binutils/*");
#endif
	
	Sys_Delete("tools/.failsafe");
	
	Terminal_ClearLines(2);
	printf_info("Installed successfully!\n");
}

void Tools_Install_z64hdr(s32 isUpdate) {
	const char* extract = "include/z64hdr-main-dev/";
	char command[512];
	ItemList itemList = ItemList_Initialize();
	
	if (isUpdate == true) {
		Sys_Delete_Recursive("include/z64hdr");
		Sys_Delete("include/z64hdr.zip");
	}
	
	Sys_MakeDir("include/");
	printf_info("" PRNT_BLUE "z64hdr " PRNT_RSET "is a collection of files allowing to interact");
	printf_info("with the game with source code, usually in C.\n");
	
redo:
	if (sAutoDownload == true) {
		Tools_Command(command, wget, "%s -q --show-progress -O %s", URL_Z64HDR_DOWNLOAD, ZIP_Z64HDR);
		if (SysExe(command)) {
			printf("\n");
			printf_warning("Failed to initialize download... Try again? Otherwise we'll do this manually. " PRNT_DGRY "[y/n]");
			if (Terminal_YesOrNo() == false)
				sAutoDownload = false;
			goto redo;
		}
		Terminal_ClearLines(2);
	} else {
		printf_info("" PRNT_GRAY "%s", URL_Z64HDR_DOWNLOAD);
		printf_info("Download manually, drag and drop the file here and press enter.");
		
		if (Tools_FileDialog(ZIP_Z64HDR)) {
			if (Terminal_YesOrNo())
				goto redo;
			
			Tools_CloseDialog("z64hdr", true);
			goto redo;
		}
	}
	
	Terminal_ClearLines(1);
	
	if (zip_extract(ZIP_Z64HDR, "include/", Tools_ZipExtractCallback, 0)) printf_error_align("zip_extract", "Failed");
	Terminal_ClearLines(1);
	
	ItemList_List(&itemList, extract, -1, LIST_FILES);
	
	Log("Move z64hdr files");
	for (s32 i = 0; i < itemList.num; i++) {
		char* input = itemList.item[i];
		char* output = strdup(input);
		
		StrRep(output, extract + strlen("include/"), "z64hdr/");
		Sys_MakeDir(Path(output));
		Sys_Rename(input, output);
		
		Free(output);
	}
	
	Log("Clean Temporal Extraction");
	if (Sys_Delete_Recursive(extract)) printf_error("Could not delete [%s]", extract);
	
	ItemList_Free(&itemList);
}

void Tools_CheckUpdates() {
	char* updateApi;
	char* url = "https://api.github.com/repos/z64tools/z64rom/releases/latest";
	char buffer[1024];
	char* tag;
	u32 sz = 0;
	u32 vnum[3] = { -1, -1, -1 };
	u32 cnum[3] = { -1, -1, -1 };
	u32 curVer, newVer;
	
	SysExe_IgnoreError();
	Tools_Command(buffer, wget, "%s -q -O -", url);
	updateApi = SysExeO(buffer);
	
	if (SysExe_GetError()) {
		Free(updateApi);
		
		return;
	}
	
	tag = StrStr(updateApi, "\"tag_name\": ");
	
	if (tag == NULL) goto error;
	
	tag += strlen("\"tag_name\": \"");
	
	while (tag[sz] != '\"') sz++;
	
	memset(buffer, 0, sz + 1);
	memcpy(buffer, tag, sz);
	
	sscanf(buffer, "z64rom_%d.%d.%d", &vnum[0], &vnum[1], &vnum[2]);
	sscanf(gToolName, "" PRNT_BLUE "z64rom " PRNT_GRAY "%d.%d.%d", &cnum[0], &cnum[1], &cnum[2]);
	
	for (s32 i = 0; i < 3; i++)
		if (vnum[i] == -1 || cnum[i] == -1)
			goto error;
	
	newVer = vnum[0] * 1000 + vnum[1] * 100 + vnum[2];
	curVer = cnum[0] * 1000 + cnum[1] * 100 + cnum[2];
	
	if (newVer > curVer) {
		printf_toolinfo(gToolName, "");
		Terminal_ClearLines(2);
		printf_warning("Update available [" PRNT_BLUE "%d.%d.%d" PRNT_RSET "]\n", vnum[0], vnum[1], vnum[2]);
	}
	
	Free(updateApi);
	
	return;
	
error:
	printf_warning("" PRNT_GRAY "Failed to retrieve update information from z64rom repo API :(\n");
	Free(updateApi);
}

s32 Tools_Init(void) {
	static MemFile dll;
	s32 setupFlag = 0;
	
	if (Tools_ValidateTools_Required()) {
		return -1;
	}
	
	if (Tools_ValidateTools_Additional() || !Sys_Stat("include/z64hdr/") || Sys_Stat("tools/.failsafe") || Sys_Stat("tools/.installing")) {
		setupFlag = 1;
		dll = MemFile_Initialize();
		MemFile_LoadFile(&dll, "tools/important64.dll");
		
		if (Sys_Stat("tools/.installing"))
			Tools_Clean();
		
		Sys_Touch("tools/.installing");
		
		if (gAutoInstall == -1) {
			printf_toolinfo(gToolName, PRNT_BLUE "Initialization Setup");
			printf_info("Play some chill music? " PRNT_DGRY "[y/n]");
			
			if (Terminal_YesOrNo())
				Sound_Xm_Play(
					dll.data,
					dll.size
				);
			Terminal_ClearLines(2);
			SleepF(0.2);
			printf_warning("Would you like to let z64rom handle installation automatically? " PRNT_DGRY "[y/n]");
			if (Terminal_YesOrNo()) {
				sAutoDownload = true;
			} else {
				sAutoDownload = false;
			}
			Terminal_ClearLines(2);
			SleepF(0.2);
		} else {
			sAutoDownload = gAutoInstall;
		}
		
		/* INIT */ {
			if (Tools_ValidateTools_Additional() || Sys_Stat("tools/.failsafe"))
				Tools_Install_mips64();
			if (!Sys_Stat("include/oot_mq_debug/z64hdr.h"))
				Tools_Install_z64hdr(false);
		}
		
		if (!Tools_ValidateTools_Additional() && Sys_Stat("include/z64hdr/")) {
			printf_info("All required tools have been installed successfully!");
			printf("\n");
		}
		Sys_Delete("tools/.installing");
	}
	
	Tools_Get(z64audio); // Init buffers
	
	return setupFlag;
}