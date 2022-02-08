#include "../lib/ExtLib.h"

char* align = "                ";

void Func_Map(MemFile* map, MemFile* out) {
	u32 lineCount = String_GetLineCount(map->str);
	char* linePtr = map->str;
	char* wow = NULL;
	
	for (s32 i = 0;; i++) {
		char* addr;
		char* name;
		
		if (i != 0) {
			while (*linePtr != '\n')
				linePtr++;
			linePtr++;
		}
		
		if (linePtr[0] != ' ' || linePtr[1] == '.')
			continue;
		if (!strncmp(linePtr, ".options", 8))
			break;
		if (strncmp(linePtr, align, strlen(align)))
			continue;
		
		name = String_GetWord(linePtr, 1);
		addr = String_GetWord(linePtr, 0);
		
		if (name[0] == '.' || name[0] == '0')
			continue;
		if (!str2cmp(name, "0x0000") ||
			!str2cmp(name, "_Rom") ||
			!str2cmp(name, "_ovl") ||
			!str2cmp(name, "_buffers") ||
			!str2cmp(name, "_message") ||
			!str2cmp(name, "_vr_") ||
			String_MemMem(name, "_room_") ||
			String_MemMem(name, "_scene")
		)
			continue;
		
		if (strlen(addr) <= 2)
			continue;
		
		if (!str2cmp(name, "FileChoose_")) {
			char* wordPtr = String_Word(linePtr, 1);
			
			if (str2cmp(wordPtr, "FileChoose_Init\n") &&
				str2cmp(wordPtr, "FileChoose_Destroy\n"))
				continue;
			
			if (!str2cmp(wordPtr, "FileChoose_Init\n")) {
				wow = String_MemMem(linePtr, "..ovl_kaleido_scope");
				
				if (wow)
					linePtr = wow;
			}
		} else if (!str2cmp(name, "Opening_")) {
			char* wordPtr = String_Word(linePtr, 1);
			
			if (str2cmp(wordPtr, "Opening_Init\n") &&
				str2cmp(wordPtr, "Opening_Destroy\n"))
				continue;
		} else if (!str2cmp(name, "Title_")) {
			char* wordPtr = String_Word(linePtr, 1);
			
			if (str2cmp(wordPtr, "Title_Init\n") &&
				str2cmp(wordPtr, "Title_Destroy\n"))
				continue;
		} else if (!str2cmp(name, "Select_")) {
			char* wordPtr = String_Word(linePtr, 1);
			
			if (str2cmp(wordPtr, "Select_Init\n") &&
				str2cmp(wordPtr, "Select_Destroy\n"))
				continue;
		}
		
		MemFile_Printf(out, "%s = 0x%08X;\n", name, (u32)String_GetInt(addr));
		
		if (!strcmp(name, "_softsprite_matrix_staticSegmentEnd"))
			break;
	}
}

static void GetProtos(MemFile* temp, MemFile* out, MemFile* var, ItemList* srcList, bool comments) {
	char* fnc = Tmp_Alloc(0x4000);
	
	for (s32 i = 0; i < srcList->num; i++) {
		char* str;
		u32 lineCount;
		u32 writeTitle = 0;
		
		if (!strcmp(srcList->item[i], "audio_init_params.c"))
			continue;
		
		if (String_MemMemCase(srcList->item[i], ".cfg") || String_MemMemCase(srcList->item[i], ".h"))
			continue;
		
		MemFile_Reset(temp);
		MemFile_LoadFile_String(temp, Dir_File(srcList->item[i]));
		
		str = temp->str;
		lineCount = String_GetLineCount(str);
		
		for (s32 line = 0; line < lineCount; line++) {
			if (line != 0) {
				while (*str != '\n')
					str++;
				str++;
			}
			char* lineStr = String_GetLine(str, 0);
			
			if (str[0] == '#' || str[0] <= ' ' || str[0] == '}' || str[0] == '/')
				continue;
			
			s32 x;
			for (s32 x = 0;; x++) {
				if (str[x] == ';' || str[x] == '[') {
					x = -1;
					break;
				}
				if (str[x] == '\0' || str[x] == '\n')
					break;
			}
			
			if (x < 0)
				continue;
			
			if (String_MemMem(String_GetWord(lineStr, 1), "(")) {
				s32 s = 0;
				s32 panicEscape = 0;
				s32 d = -1;
				
				while (1) {
					if (str[s] == '(' && str[s + 1] == '*' && d == -1) {
						panicEscape = true;
						break;
					}
					if (str[s] == '[') {
						panicEscape = true;
						break;
					}
					if (str[s] == ')' && d == 0)
						break;
					fnc[s] = str[s];
					if (fnc[s] == '\n')
						fnc[s] = ' ';
					if (fnc[s] == '(')
						d++;
					if (fnc[s] == ')')
						d--;
					if (fnc[s] == '{') {
						s -= 2;
						break;
					}
					s++;
				}
				
				if (panicEscape)
					continue;
				
				fnc[s++] = ')';
				fnc[s++] = ';';
				fnc[s++] = '\0';
				fnc[s++] = '\0';
				
				for (s32 k = 0; k < s; k++) {
					if (fnc[k + 1] == ' ' && fnc[k] == ' ') {
						memmove(&fnc[k], &fnc[k + 1], strlen(&fnc[k + 1]) + 1);
						s--;
						k--;
					}
				}
				
				if (String_MemMem(fnc, "StorageChange") ||
					String_MemMem(fnc, "SoundFontData") ||
					String_MemMem(fnc, "RelocInfo") ||
					String_MemMem(fnc, "FreqLerp") ||
					String_MemMem(fnc, "arg3_800F") ||
					String_MemMem(fnc, "GfxMod")
				)
					continue;
				
				if (writeTitle++ == 0 && comments)
					MemFile_Printf(out, "// %s\n", srcList->item[i]);
				MemFile_Printf(out, "%s\n", fnc);
			}
		}
		
		if (comments && writeTitle)
			MemFile_Printf(out, "\n");
	}
	
	if (var == NULL) return;
	
	for (s32 i = 0; i < srcList->num; i++) {
		char* str;
		u32 lineCount;
		u32 writeTitle = 0;
		
		if (String_MemMemCase(srcList->item[i], ".cfg") || String_MemMemCase(srcList->item[i], ".h"))
			continue;
		
		MemFile_Reset(temp);
		MemFile_LoadFile_String(temp, Dir_File(srcList->item[i]));
		
		str = temp->str;
		lineCount = String_GetLineCount(str);
		
		for (s32 line = 0; line < lineCount; line++) {
			s32 breaker = 0;
			if (line != 0) {
				while (*str != '\n')
					str++;
				str++;
			}
			
			if (str[0] == '#' || str[0] <= ' ' || str[0] == '}' || str[0] == '/')
				continue;
			
			for (s32 e = 0;; e++) {
				if (str[e] == '(' || str[e] == '{' || str[e] == ':') {
					breaker = true;
					break;
				}
				if (str[e] == '=' || str[e] == ';') {
					break;
				}
			}
			
			if (breaker)
				continue;
			
			for (s32 e = 0;; e++) {
				if (str[e + 1] == '=' || str[e] == ';') {
					fnc[e++] = ';';
					fnc[e++] = '\0';
					break;
				}
				fnc[e] = str[e];
			}
			
			if (
				String_MemMem(fnc, "NatureAmbienceDataIO") ||
				String_MemMem(fnc, "FreqLerp") ||
				String_MemMem(fnc, "SfxPlayerState") ||
				String_MemMem(fnc, "OcarinaStick") ||
				String_MemMem(fnc, "SoundRequest") ||
				String_MemMem(fnc, "UnusedBankLerp") ||
				String_MemMem(fnc, "DoorLockInfo") ||
				String_MemMem(fnc, "TargetRangeParams") ||
				String_MemMem(fnc, "CameraMode") ||
				String_MemMem(fnc, "CameraSetting") ||
				String_MemMem(fnc, "HitInfo") ||
				String_MemMem(fnc, "ColChkBloodFunc") ||
				String_MemMem(fnc, "ColChkResetFunc") ||
				String_MemMem(fnc, "ColChkLineFunc") ||
				String_MemMem(fnc, "ColChkVsFunc") ||
				String_MemMem(fnc, "ColChkApplyFunc") ||
				String_MemMem(fnc, "DebugDispObject_DrawFunc") ||
				String_MemMem(fnc, "InputCombo") ||
				String_MemMem(fnc, "PrintTextBuffer") ||
				String_MemMem(fnc, "DrawItemTableEntry") ||
				String_MemMem(fnc, "CutsceneStateHandler") ||
				String_MemMem(fnc, "DebugDispObjectInfo") ||
				String_MemMem(fnc, "LightningBolt") ||
				String_MemMem(fnc, "LightsBuffer") ||
				String_MemMem(fnc, "MessageTableEntry") ||
				String_MemMem(fnc, "MapMarkDataOverlay") ||
				String_MemMem(fnc, "MapMarkInfo") ||
				String_MemMem(fnc, "typedef") ||
				String_MemMem(fnc, "NaviColor") ||
				String_MemMem(fnc, "sUnused") ||
				String_MemMem(fnc, "SsSramContext") ||
				String_MemMem(fnc, "SavePlayerData") ||
				String_MemMem(fnc, "OptionsMenuTextureInfo") ||
				String_MemMem(fnc, "__OSEventState") ||
				String_MemMem(fnc, "OSMgrArgs") ||
				String_MemMem(fnc, "BowStringData") ||
				String_MemMem(fnc, "TextTriggerEntry") ||
				String_MemMem(fnc, "RestrictionFlags") ||
				String_MemMem(fnc, "struct_8011FB48") ||
				String_MemMem(fnc, "Struct_8011FAF0") ||
				String_MemMem(fnc, "struct_80116130") ||
				String_MemMem(fnc, "Struct_8016E320") ||
				String_MemMem(fnc, "Struct_8012AF0C") ||
				String_MemMem(fnc, "gBuildMakeOption") ||
				String_MemMem(fnc, "gBuildDate") ||
				String_MemMem(fnc, "gBuildTeam")
			)
				continue;
			
			String_Replace(fnc, "static ", "");
			
			if (!String_MemMem(fnc, "extern "))
				MemFile_Printf(var, "extern %s\n", fnc);
			else
				MemFile_Printf(var, "%s\n", fnc);
		}
		
		if (comments && writeTitle)
			MemFile_Printf(out, "\n");
	}
}

void Func_Proto(MemFile* temp, MemFile* out, MemFile* var) {
	MemFile_Printf(
		out,
		"#ifndef FUNCTIONS_H\n"
		"#define FUNCTIONS_H\n"
		"\n"
		"#include \"z64.h\"\n"
		"\n"
		"#include <common/intrinsics.h>\n\n"
	);
	MemFile_Printf(
		var,
		"#ifndef VARIABLES_H\n"
		"#define VARIABLES_H\n"
		"\n"
		"#include \"z64.h\"\n"
		"#include \"segment_symbols.h\"\n\n"
	);
	
	Dir_Enter("src/");
	Dir_Enter("code/"); {
		ItemList srcList;
		Dir_ItemList(&srcList, IS_FILE);
		GetProtos(temp, out, var, &srcList, true);
		
		Dir_Leave();
	}
	
	Dir_Enter("boot/"); {
		ItemList srcList;
		Dir_ItemList(&srcList, IS_FILE);
		GetProtos(temp, out, var, &srcList, true);
		
		Dir_Leave();
	}
	
	Dir_Enter("dmadata/"); {
		ItemList srcList;
		Dir_ItemList(&srcList, IS_FILE);
		GetProtos(temp, out, var, &srcList, true);
		
		Dir_Leave();
	}
	
	Dir_Enter("buffers/"); {
		ItemList srcList;
		Dir_ItemList(&srcList, IS_FILE);
		GetProtos(temp, out, var, &srcList, true);
		
		Dir_Leave();
	}
	
	Dir_Enter("libultra/"); {
		MemFile_Printf(out, "// LibUltra\n");
		
		Dir_Enter("gu/"); {
			ItemList srcList;
			Dir_ItemList(&srcList, IS_FILE);
			GetProtos(temp, out, var, &srcList, false);
			
			Dir_Leave();
		}
		
		Dir_Enter("io/"); {
			ItemList srcList;
			Dir_ItemList(&srcList, IS_FILE);
			GetProtos(temp, out, var, &srcList, false);
			
			Dir_Leave();
		}
		
		Dir_Enter("libc/"); {
			ItemList srcList;
			Dir_ItemList(&srcList, IS_FILE);
			GetProtos(temp, out, var, &srcList, false);
			
			Dir_Leave();
		}
		
		Dir_Enter("os/"); {
			ItemList srcList;
			Dir_ItemList(&srcList, IS_FILE);
			GetProtos(temp, out, var, &srcList, false);
			
			Dir_Leave();
		}
		
		Dir_Enter("rmon/"); {
			ItemList srcList;
			Dir_ItemList(&srcList, IS_FILE);
			GetProtos(temp, out, var, &srcList, false);
			
			Dir_Leave();
		}
		
		Dir_Leave();
	}
	
	Dir_Enter("overlays/gamestates/"); {
		ItemList stateList;
		
		Dir_ItemList(&stateList, IS_DIR);
		
		for (s32 i = 0; i < stateList.num; i++) {
			Dir_Enter("%s", stateList.item[i]); {
				ItemList srcList;
				Dir_ItemList(&srcList, IS_FILE);
				GetProtos(temp, out, NULL, &srcList, true);
				
				Dir_Leave();
			}
		}
		
		Dir_Leave();
	}
	
	Dir_Leave();
	Dir_Enter("include/"); {
		char* ptr;
		
		MemFile_Reset(temp);
		MemFile_LoadFile_String(temp, Dir_File("variables.h"));
		
		ptr = String_MemMem(temp->str, "extern");
		
		MemFile_Write(var, ptr, strlen(ptr));
		
		Dir_Leave();
	}
	
	MemFile_Printf(
		out,
		"\n#endif // FUNCTIONS_H\n"
	);
	Dir_Leave();
}

int main(int argc, char* argv[]) {
	MemFile tmp = MemFile_Initialize();
	
	MemFile map = MemFile_Initialize();
	MemFile var = MemFile_Initialize();
	MemFile func = MemFile_Initialize();
	u32 parArg = 0;
	
	printf_SetPrefix("");
	printf_WinFix();
	
	MemFile_Malloc(&tmp, MbToBin(16.0));
	
	MemFile_Malloc(&map, MbToBin(16.0));
	MemFile_Malloc(&var, MbToBin(16.0));
	MemFile_Malloc(&func, MbToBin(16.0));
	
	if (Lib_ParseArguments(argv, "--decomp", &parArg)) {
		Dir_Set(argv[parArg]);
		MemFile_LoadFile_String(&tmp, Dir_File("build/z64.map"));
	} else {
		printf_error("Provide [--decomp]");
	}
	
	if (!Lib_ParseArguments(argv, "--out", &parArg)) {
		printf_error("Provide [--out]");
	}
	
	Func_Map(&tmp, &map);
	Func_Proto(&tmp, &func, &var);
	
	Dir_Set(argv[parArg]);
	MemFile_SaveFile_String(&map, Dir_File("oot_mq_debug/syms.ld"));
	MemFile_SaveFile_String(&var, Dir_File("include/variables.h"));
	MemFile_SaveFile_String(&func, Dir_File("include/functions.h"));
	
	printf_info("Save File [%s]", Dir_File("oot_mq_debug/syms.ld"));
	printf_info("Save File [%s]", Dir_File("include/variables.h"));
	printf_info("Save File [%s]", Dir_File("include/functions.h"));
	
	MemFile_Free(&map);
	MemFile_Free(&tmp);
	MemFile_Free(&func);
	MemFile_Free(&var);
}