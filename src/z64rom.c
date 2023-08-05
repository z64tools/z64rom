#include "z64rom.h"
#include "z64make.h"
#include "package.h"
#include <ext_proc.h>
#include <ext_zip.h>
#include <ext_xm.h>

/*============================================================================*/

const char* gToolName = PRNT_CYAN "z64rom " PRNT_GRAY "1.5.8";
char gProjectConfig[32] = "z64project.toml";
StateZ g64 = {
	.buildID   = ROM_DEV,
	.threading = true,
};

/*============================================================================*/

#define ARG_TABLE(en, num, arg) en
enum {
#include "tbl_args.h"
	ARG_MAX
};

#define ARG_TABLE(en, num, arg) [en] = { arg, (char*)&gLang.help[en] }
const char* sArguments[][2] = {
#include "tbl_args.h"
};

#define ARG_TABLE(en, num, arg) num
const u32 sArgFolNum[] = {
#include "tbl_args.h"
};

/*============================================================================*/

#define PROJECT_TABLE(enum, path, old, fmt, default) enum
enum {
#include "tbl_project.h"
	PROJECT_ENUM_MAX
};

#define PROJECT_TABLE(enum, path, old, fmt, default) { path, old, fmt, default }
const char* sTomlProject[][4] = {
#include "tbl_project.h"
};

/*============================================================================*/

#define APPDATA_TABLE(enum, path, fmt, default) enum
enum {
#include "tbl_appdata.h"
	APPDATA_MAX
};

#define APPDATA_TABLE(enum, path, fmt, default) { path, fmt, default }
const char* sAppData[][3] = {
#include "tbl_appdata.h"
};

/*============================================================================*/

void Extra_WadInject(bool successMsg) {
	if (!sys_stat("tools/common-key.bin")) {
		Proc* p = Proc_New("tools/gzinject.exe -a genkey");
		
		Proc_SetState(p, PROC_THROW_ERROR | PROC_MUTE_STDOUT | PROC_OPEN_STDIN);
		Proc_SetPath(p, x_fmt("%stools/", sys_appdir()));
		Proc_Exec(p);
		Proc_Write(p, "45e\n");
		Proc_Join(p);
	}
	
	sys_setworkdir(x_fmt("%stools/", sys_appdir()));
	
	Proc* p =  Proc_New(
		"gzinject"
#ifdef _WIN32
			".exe"
#endif
	);
	char* buildrom = x_dirabs_f(sys_appdir(), g64.build[g64.buildID]);
	
	Proc_AddEach(p,
		"-a", "inject",
		"-w", x_dirabs_f(sys_appdir(), g64.baseWad),
		"-m", buildrom,
		"-p", "patches/NACE.gzi",
		"-p", "patches/gz_raphnet_remap.gzi",
		"-p", "patches/analog_substick.NACE.gzi",
		"-o", x_dirabs_f(sys_appdir(), x_fmt("%s%s.wad", g64.buildName, g64.suffix[g64.buildID])),
		"--verbose"
	);
	Proc_SetState(p, PROC_MUTE_STDOUT | PROC_THROW_ERROR);
	
	nested(char, patchMediaFmt, (const char* filename, const char letter)) {
		FILE* rom = fopen(filename, "rb+");
		char current;
		
		if (!rom) errr_align(gLang.err_load, filename);
		fseek(rom, 0x3B, SEEK_SET);
		fread(&current, 1, 1, rom);
		fseek(rom, 0x3B, SEEK_SET);
		fwrite(&letter, 1, 1, rom);
		fclose(rom);
		
		return current;
	};
	
	char old = patchMediaFmt(buildrom, 'C');
	Proc_Exec(p);
	
	int progress = 0;
	char* line;
	
	while ((line = Proc_ReadLine(p, READ_STDOUT)))
		info_fastprog(gLang.rom.target[LANG_INJECT],
			clamp_max((++progress) / 2.0f, 100.0f),
			100);
	info_prog_end();
	
	Proc_Join(p);
	patchMediaFmt(buildrom, old);
	
	if (successMsg)
		info(gLang.success);
}

s32 Extra_Project64(Rom* rom, bool doPlay) {
	char* project64;
	
	project64 = qxf(Toml_GetStr(&rom->toml, "project64"));
	
	if (strcmp(project64, "NULL") && sys_stat(project64) && doPlay) {
		bool hasPython;
		Proc* python3 = Proc_New("python3 --help");
		
		Proc_SetState(python3, PROC_MUTE);
		Proc_Exec(python3);
		hasPython = !Proc_Join(python3);
		
		Proc* log = NULL;
		Proc* emulator = Proc_New("%s %s", project64, g64.build[g64.buildID]);
		
		Proc_SetState(emulator, PROC_SYSTEM_EXE);
		
		if (hasPython) {
			log = Proc_New("python3 tools/n64log.py --quiet");
			Proc_Exec(log);
		}
		
		Proc_Exec(emulator);
		Proc_Join(emulator);
		
		Proc_Kill(log);
		
		exit(0);
	}
	
	return 0;
}

/*============================================================================*/

void RomContenList_Apply(void) {
	Memfile mem = Memfile_New();
	char* s;
	bool hello = false;
	
	nested(void, Hello, (void)) {
		nested_var(hello);
		
		if (!hello)
			info(gLang.main.rcl_title);
		hello = true;
	};
	
	Memfile_LoadStr(&mem, "rom/rcl.diff");
	
	s = mem.str;
	
	do {
		if (s[0] == '-') {
			char* file = x_cpyline(s + 2, 0);
			file = x_fmt(file, g64.vanilla);
			if (sys_stat(file)) {
				Hello();
				warn_align(gLang.rm, file);
				if (sys_isdir(file))
					sys_rmdir(file);
				else
					sys_rm(file);
			}
		}
		
	} while ((s = strline(s, 1)));
	
	Memfile_Free(&mem);
}

void RomContentList_Update(void) {
	Memfile mem = Memfile_New();
	char* s;
	bool changed = false;
	
	Memfile_LoadStr(&mem, "rom/rcl.diff");
	
	s = mem.str;
	
	do {
		char* file = x_cpyline(s + 2, 0);
		
		osLog("%s", file);
		if (!sys_stat(x_fmt(file, g64.vanilla))) {
			s[0] = '-';
			changed = true;
		}
		
	} while ((s = strline(s, 1)) && *s != '\0');
	
	if (changed)
		Memfile_SaveStr(&mem, "rom/rcl.diff");
	
	Memfile_Free(&mem);
}

void RomContentList_Create(void) {
	Memfile mem = Memfile_New();
	List list = List_New();
	
	struct {
		const char* path;
		u32 flags;
	} param[] = {
		{ "rom/actor/%s/",            LIST_FOLDERS            },
		{ "rom/effect/%s/",           LIST_FOLDERS            },
		{ "rom/object/%s/",           LIST_FOLDERS            },
		{ "rom/scene/%s/",            LIST_FOLDERS            },
		{ "rom/sound/sample/%s/",     LIST_FOLDERS            },
		{ "rom/sound/sequence/%s/",   LIST_FOLDERS            },
		{ "rom/sound/soundfont/%s/",  LIST_FILES              },
		{ "rom/system/animation/%s/", LIST_FOLDERS            },
		{ "rom/system/kaleido/%s/",   LIST_FOLDERS            },
		{ "rom/system/skybox/%s/",    LIST_FOLDERS            },
		{ "rom/system/state/%s/",     LIST_FOLDERS            },
		{ "rom/system/static/%s/",    LIST_FOLDERS            },
	};
	
	if (sys_stat("rom/rcl.diff")) {
		RomContenList_Apply();
		
		return;
	}
	
	Memfile_Alloc(&mem, MbToBin(2));
	
	for (int j = 0; j < ArrCount(param); j++) {
		List_Walk(&list, x_fmt(param[j].path, g64.vanilla), 0, param[j].flags | LIST_RELATIVE);
		
		for (int i = 0; i < list.num; i++) {
			Memfile_Fmt(&mem, "+ %s%s\n", param[j].path, list.item[i]);
		}
		
		List_Free(&list);
	}
	
	Memfile_SaveStr(&mem, "rom/rcl.diff");
}

/*============================================================================*/

static void Clean_RmDir(const char* item) {
	warn_align(gLang.rm, item);
	sys_rmdir(item);
}

void Clean_Make(void) {
	List list = List_New();
	
	List_Walk(&list, "rom/", -1, LIST_FILES);
	for (int i = 0; i < list.num; i++) {
		if (strend(list.item[i], ".o") || strend(list.item[i], ".elf")) {
			if (sys_stat(x_path(list.item[i])) && !strstr(list.item[i], ".entry"))
				Clean_RmDir(x_path(list.item[i]));
		}
	}
	List_Free(&list);
	
	List_Walk(&list, "rom/", -1, LIST_FOLDERS);
	for (int i = 0; i < list.num; i++) {
		if (striend(list.item[i], ".entry/"))
			Clean_RmDir(list.item[i]);
	}
	List_Free(&list);
	
	List_Walk(&list, "src/sound/sample/", 0, LIST_FOLDERS);
	for (int i = 0; i < list.num; i++) {
		memcpy(list.item[i], "rom/", 4);
		
		if (sys_stat(list.item[i]))
			Clean_RmDir(list.item[i]);
	}
	List_Free(&list);
	
	List_Walk(&list, "src/sound/sfx/", 0, LIST_FOLDERS);
	for (int i = 0; i < list.num; i++) {
		memcpy(list.item[i], "rom/", 4);
		
		if (sys_stat(list.item[i]))
			Clean_RmDir(list.item[i]);
	}
	List_Free(&list);
	
	List_Walk(&list, "src/object/", 0, LIST_FOLDERS);
	for (int i = 0; i < list.num; i++) {
		memcpy(list.item[i], "rom/", 4);
		
		if (sys_stat(list.item[i]))
			Clean_RmDir(list.item[i]);
	}
	List_Free(&list);
	
	const char* folders[] = {
		"src/lib_user/object/",
		"include/object/",
	};
	
	for (var_t i = 0; i < ArrCount(folders); i++) {
		const char* f = folders[i];
		
		if (sys_stat(f))
			Clean_RmDir(f);
	}
	
	osLog("Cleaning OK");
}

void Clean_Dump(void) {
	Clean_RmDir(x_fmt("rom/actor/%s/", g64.vanilla));
	Clean_RmDir(x_fmt("rom/effect/%s/", g64.vanilla));
	Clean_RmDir(x_fmt("rom/object/%s/", g64.vanilla));
	Clean_RmDir(x_fmt("rom/scene/%s/", g64.vanilla));
	Clean_RmDir(x_fmt("rom/sound/sample/%s/", g64.vanilla));
	Clean_RmDir(x_fmt("rom/sound/sequence/%s/", g64.vanilla));
	Clean_RmDir(x_fmt("rom/sound/soundfont/%s/", g64.vanilla));
	Clean_RmDir(x_fmt("rom/system/kaleido/%s/", g64.vanilla));
	Clean_RmDir(x_fmt("rom/system/skybox/%s/", g64.vanilla));
	Clean_RmDir(x_fmt("rom/system/state/%s/", g64.vanilla));
	Clean_RmDir(x_fmt("rom/system/static/%s/", g64.vanilla));
	
	Clean_RmDir("rom/lib_code/");
	Clean_RmDir("rom/lib_user/");
	Clean_RmDir("rom/");
	Clean_RmDir("include/object/");
	
	sys_rm(gProjectConfig);
	
	g64.cleanDump = true;
}

void Clean_Cache(void) {
	Clean_RmDir("rom/yaz-cache/");
}

/*============================================================================*/

typedef enum { Z_ITEM, Z_OLD_ITEM, Z_FMT, Z_DEF } ProjectItem;
#define PROJECT_ITEM(x) sTomlProject[x][Z_ITEM]

static bool Project_UpdateSoundFont() {
	List list = List_New();
	date_t d = sys_timedate(sys_time());
	const char* tmp = strdup(x_fmt("%d.%d.%d-%d.%d", d.year, d.month, d.day, d.hour, d.minute));
	bool res = false;
	
	nested(void, Backup, (const char* file)) {
		const char* dest = x_fmt(".backup/%s/%s", tmp, file + strlen("rom/sound/"));
		
		sys_mkdir(x_path(dest));
		sys_cp(file, dest);
	};
	
	nested(void, Write, (const char* path, bool vanilla)) {
		List inst = List_New();
		List sfxx = List_New();
		List drum = List_New();
		Memfile config = Memfile_New();
		Toml* t = new(Toml);
		
		*t = Toml_New();
		fs_set(path);
		
		if (sys_stat(fs_item("instrument/")))
			List_Walk(&inst, fs_item("instrument/"), 0, LIST_FILES | LIST_NO_DOT);
		if (sys_stat(fs_item("sfx/")))
			List_Walk(&sfxx, fs_item("sfx/"), 0, LIST_FILES | LIST_NO_DOT);
		if (sys_stat(fs_item("drum/")))
			List_Walk(&drum, fs_item("drum/"), 0, LIST_FILES | LIST_NO_DOT);
		
		if (!sys_stat(fs_item("config.toml"))) {
			List_Free(&inst);
			List_Free(&sfxx);
			List_Free(&drum);
			
			warn_align(gLang.rm, path);
			sys_rmdir(path);
			
			return;
		}
		Memfile_LoadStr(&config, fs_item("config.toml"));
		
		if (!vanilla)
			Backup(config.info.name);
		
		Toml_SetVar(t, "#medium_type",     "\"ram\" / \"unk\"     / \"cart\" / \"ddrive\"");
		Toml_SetVar(t, "#sequence_player", "\"sfx\" / \"fanfare\" / \"bgm\"  / \"demo\"");
		
		Toml_SetVar(t, "medium_type", "\"%s\"", Ini_GetStr(&config, "medium_type"));
		Toml_SetVar(t, "sequence_player", "\"%s\"", Ini_GetStr(&config, "sequence_player"));
		
		for (var_t i = 0; i < inst.num; i++) {
			List envRte = List_New();
			List envLvl = List_New();
			const char* type[] = {
				"low", "prim", "hi"
			};
			
			s32 index = sint(x_filename(inst.item[i]));
			
			Memfile_LoadStr(&config, inst.item[i]);
			Ini_GetArr(&config, "env_rate", &envRte);
			Ini_GetArr(&config, "env_level", &envLvl);
			
			Toml_SetVar(t, x_fmt("inst.entry[%d].split_hi", index), "\"%s\"", Ini_GetStr(&config, "split_hi"));
			Toml_SetVar(t, x_fmt("inst.entry[%d].split_lo", index), "\"%s\"", Ini_GetStr(&config, "split_lo"));
			Toml_SetVar(t, x_fmt("inst.entry[%d].release_rate", index), "%f", Ini_GetFloat(&config, "release_rate"));
			
			for (var_t j = 0; j < envLvl.num; j++) {
				Toml_SetVar(t, x_fmt("inst.entry[%d].env_rate[%d]", index, j), "%s", envRte.item[j]);
				Toml_SetVar(t, x_fmt("inst.entry[%d].env_level[%d]", index, j), "%s", envLvl.item[j]);
			}
			
			for (var_t j = 0; j < ArrCount(type); j++)
				Toml_SetTab(t, "inst.entry[%d].%s", index, type[j]);
			
			for (var_t j = 0; j < ArrCount(type); j++) {
				const char* sample;
				f32 tuning;
				
				Ini_GotoTab(type[j]);
				if ((sample = Ini_GetStr(&config, "sample")) && strcmp(sample, "NULL")) {
					tuning = Ini_GetFloat(&config, "tuning");
					
					Toml_SetVar(t, x_fmt("inst.entry[%d].%s.sample", index, type[j]), "\"%s\"", sample);
					Toml_SetVar(t, x_fmt("inst.entry[%d].%s.tuning", index, type[j]), "%f", tuning);
				}
				Ini_GotoTab(NULL);
			}
			
			List_Free(&envLvl);
			List_Free(&envRte);
			
			if (!vanilla)
				Backup(inst.item[i]);
		}
		
		for (var_t i = 0; i < sfxx.num; i++) {
			const char* sample;
			f32 tuning;
			
			s32 index = sint(x_filename(sfxx.item[i]));
			
			Memfile_LoadStr(&config, sfxx.item[i]);
			Ini_GotoTab("prim");
			
			if ((sample = Ini_GetStr(&config, "sample")) && strcmp(sample, "NULL")) {
				tuning = Ini_GetFloat(&config, "tuning");
				
				Toml_SetVar(t, x_fmt("sfx.entry[%d].sample", index), "\"%s\"", sample);
				Toml_SetVar(t, x_fmt("sfx.entry[%d].tuning", index), "%f", tuning);
			}
			
			Ini_GotoTab(NULL);
			
			if (!vanilla)
				Backup(sfxx.item[i]);
		}
		
		for (var_t i = 0; i < drum.num; i++) {
			List envRte = List_New();
			List envLvl = List_New();
			const char* sample;
			f32 tuning;
			
			s32 index = sint(x_filename(drum.item[i]));
			
			Memfile_LoadStr(&config, drum.item[i]);
			
			Ini_GetArr(&config, "env_rate", &envRte);
			Ini_GetArr(&config, "env_level", &envLvl);
			
			Toml_SetVar(t, x_fmt("drum.entry[%d].pan", index), "%d", Ini_GetInt(&config, "pan"));
			Toml_SetVar(t, x_fmt("drum.entry[%d].release_rate", index), "%f", Ini_GetFloat(&config, "release_rate"));
			
			for (var_t j = 0; j < envLvl.num; j++) {
				Toml_SetVar(t, x_fmt("drum.entry[%d].env_rate[%d]", index, j), "%s", envRte.item[j]);
				Toml_SetVar(t, x_fmt("drum.entry[%d].env_level[%d]", index, j), "%s", envLvl.item[j]);
			}
			
			Ini_GotoTab("prim");
			if ((sample = Ini_GetStr(&config, "sample")) && strcmp(sample, "NULL")) {
				tuning = Ini_GetFloat(&config, "tuning");
				
				Toml_SetVar(t, x_fmt("drum.entry[%d].sample", index), "\"%s\"", sample);
				Toml_SetVar(t, x_fmt("drum.entry[%d].tuning", index), "%f", tuning);
			}
			Ini_GotoTab(NULL);
			
			List_Free(&envLvl);
			List_Free(&envRte);
			
			if (!vanilla)
				Backup(drum.item[i]);
		}
		
		warn_align(gLang.rm, path);
		sys_rmdir(path);
		
		if (vanilla)
			Toml_Save(t, x_fmt("rom/sound/soundfont/%s/%s.toml", g64.vanilla, x_rep(x_pathslot(path, -1), "/", "")));
		else
			Toml_Save(t, x_fmt("rom/sound/soundfont/%s.toml", x_rep(x_pathslot(path, -1), "/", "")));
		
		List_Free(&inst);
		List_Free(&sfxx);
		List_Free(&drum);
		Memfile_Free(&config);
		Toml_Free(t);
		delete(t);
	};
	
	if (sys_stat("rom/sound/soundfont")) {
		List_Walk(&list, x_fmt("rom/sound/soundfont/%s/", g64.vanilla), 0, LIST_FOLDERS | LIST_NO_DOT);
		for (var_t i = 0; i < list.num; i++)
			Write(list.item[i], true),
			res = true;
		List_Free(&list);
		
		List_Walk(&list, "rom/sound/soundfont/", 0, LIST_FOLDERS | LIST_NO_DOT);
		for (var_t i = 0; i < list.num; i++)
			Write(list.item[i], false);
		List_Free(&list);
		
		Memfile rcl = Memfile_New();
		Memfile new = Memfile_New();
		
		rcl.param.throwError = false;
		Memfile_LoadStr(&rcl, "rom/rcl.diff");
		
		if (rcl.data) {
			forline(line, rcl.str) {
				const char* xline;
				
				if (!line) continue;
				if (strlen(line) == 0) continue;
				
				if (strncmp(line + 2, "rom/sound/soundfont/%s/", 22)) {
					Memfile_Fmt(&new, "%s\n", x_cpyline(line, 0));
					
					continue;
				}
				
				xline = x_strndup(x_cpyline(line, 0), 261);
				
				if (strend(xline, "/"))
					strcpy(strend(xline, "/"), ".toml");
				
				Memfile_Fmt(&new, "%s\n", xline);
			}
			
			Memfile_SaveStr(&new, "rom/rcl.diff");
			Memfile_Free(&rcl);
			Memfile_Free(&new);
		}
	}
	
	delete(tmp);
	
	return res;
}

static bool Project_UpdateScenes() {
	List list = List_New();
	static const char* sTable[][2] = {
		{ "bottles",    "BOTTLES"         },
		{ "a_button",   "A_BUTTON"        },
		{ "b_button",   "B_BUTTON"        },
		{ "unused",     "UNUSED"          },
		
		{ "warp_song",  "WARP_SONG"       },
		{ "ocarina",    "OCARINA"         },
		{ "hookshot",   "HOOKSHOT"        },
		{ "trade_item", "TRADE_ITEM"      },
		
		{ "other",      "ALL"             },
		{ "din_nayru",  "DIN_NAYRU"       },
		{ "farores",    "FARORES_WIND"    },
		{ "sun_song",   "SUN_SONG"        },
	};
	
	List_Walk(&list, "rom/scene/", 1, LIST_FOLDERS);
	
	for (var_t i = 0; i < list.num; i++) {
		Memfile mem = Memfile_New();
		Toml toml = Toml_New();
		List flags = List_New();
		
		fs_set(list.item[i]);
		
		const char* config = fs_item("config.cfg");
		
		if (!sys_stat(config))
			goto cleanup;
		
		info("%s", config);
		
		Memfile_LoadStr(&mem, config);
		
		Toml_SetVar(&toml, "draw_func_index", "%d", Ini_GetInt(&mem, "scene_func_id"));
		
		for (var_t k = 0; k < ArrCount(sTable); k++)
			Toml_SetVar(&toml, x_fmt("enables.%s", sTable[k][0]), "true");
		
		if (!strstr(mem.str, "restriction_flags")) {
			Ini_GetArr(&mem, "restriction_flags", &flags);
			
			for (var_t k = 0; k < flags.num; k++)
				for (var_t l = 0; l < ArrCount(sTable); l++)
					if (!strcmp(flags.item[k], sTable[l][1]))
						Toml_SetVar(&toml, x_fmt("enables.%s", sTable[l][0]), "false");
		}
		
		Toml_Save(&toml, fs_item("config.toml"));
		sys_rm(fs_item("config.cfg"));
		cleanup:
		Memfile_Free(&mem);
		List_Free(&flags);
		Toml_Free(&toml);
	}
	
	return 0;
}

static bool Project_UpdateOverlays() {
	struct {
		const char* path;
		const char* in;
		const char* out;
	} target[] = {
		{ fmt("rom/actor/%s/",        g64.vanilla),        "actor.zovl",         "overlay.zovl"         },
		{ fmt("rom/effect/%s/",       g64.vanilla),        "effect.zovl",        "overlay.zovl"         },
		{ fmt("rom/system/state/%s/", g64.vanilla),        "state.zovl",         "overlay.zovl"         },
	};
	bool res = false;
	
	foreach(i, target) {
		List folder = List_New();
		
		if (!sys_stat(target[i].path))
			continue;
		
		List_Walk(&folder, target[i].path, 0, LIST_FOLDERS);
		
		for (int j = 0; j < folder.num; j++) {
			const char* in = x_fmt("%s%s", folder.item[j], target[i].in);
			const char* out = x_fmt("%s%s", folder.item[j], target[i].out);
			
			if (sys_stat(in))
				sys_mv(in, out),
				res = true;
		}
		
		List_Free(&folder);
		delete(target[i].path);
	}
	
	return res;
}

static bool Project_UpdateManifests() {
	List list = List_New();
	bool ret = false;
	
	if (!sys_stat("z64project.cfg"))
		return false;
	
	if (!sys_stat("src/object/"))
		return false;
	
	List_Walk(&list, "src/object/", 0, LIST_FOLDERS | LIST_RELATIVE);
	
	if (list.num) {
		List_SortSlot(&list, false);
		
		if (list.num < 0x16)
			return false;
		
		Memfile mem = Memfile_New();
		const char* file;
		
		for (var_t i = 0x14; i < 0x16; i++) {
			if (list.item[i]) {
				fs_set("src/object/%s", list.item[i]);
				file = fs_find("*.mnf");
				
				if (file) {
					warn_align(gLang.main.manifest_fix, file);
					Memfile_LoadStr(&mem, file);
					Memfile_Realloc(&mem, mem.memSize * 2);
					
					strrep(mem.str, "actor.zovl", "overlay.zovl");
					strrep(mem.str, "effect.zovl", "overlay.zovl");
					
					mem.size = strlen(mem.str);
					Memfile_SaveStr(&mem, file);
					ret = true;
				}
			}
		}
	}
	
	strrep(gProjectConfig, ".cfg", ".toml");
	
	return ret;
}

static bool Project_TomlUpdate() {
	List list = List_New();
	
	List_SetFilters(&list, CONTAIN_END, ".cfg");
	List_Walk(&list, "", -1, LIST_FILES);
	
	for (int i = 0; i < list.num; i++) {
		if (strstart(list.item[i], "patch/"))
			continue;
		if (strstart(list.item[i], "rom/scene/"))
			continue;
		if (streq(list.item[i], "tools/z64audio.cfg"))
			continue;
		if (streq(list.item[i], "tools/info.cfg"))
			continue;
		
		const char* toml = x_rep(list.item[i], ".cfg", ".toml");
		sys_mv(list.item[i], toml);
	}
	
	sys_mv("rom/system/entrance_table.toml", "src/entrance_table.toml");
	sys_mv("rom/system/vanilla.entrance_table.toml", "src/vanilla.entrance_table.toml");
	
	return false;
}

static u8 Audio_GetReleaseID(f32 r) {
	return clamp((s32)(r * 255), 0, 255);
}

static bool Project_UpdateEnvelopes() {
	List list = List_New();
	bool ssave = false;
	
	Rom_ItemList(&list, "rom/sound/soundfont/", 0, LIST_FILES | LIST_NO_DOT);
	
	if (list.num == 0) goto bailout;
	
	for (var_t i = 0; i < list.num; i++) {
		Toml toml = Toml_New();
		int num;
		bool save = false;
		
		Toml_Load(&toml, list.item[i]);
		
		const char* target[2] = {
			"inst", "drum"
		};
		
		for (var_t y = 0; y < ArrCount(target); y++) {
			num = Toml_ArrCount(&toml, "%s.entry", target[y]);
			
			for (var_t k = 0; k < num; k++) {
				int env = Toml_ArrCount(&toml, "%s.entry[%d].env_rate", target[y], k);
				
				if (!env) continue;
				if (TYPE_INT == Toml_VarType(&toml, "%s.entry[%d].env_rate[0]", target[y], k))
					continue;
				
				ssave = save = true;
				
				for (var_t e = 0; e < env; e++) {
					Toml_SetVar(&toml,
						x_fmt("%s.entry[%d].env_rate[%d]", target[y], k, e),
						"%d",
						(s16)(Toml_GetFloat(&toml, "%s.entry[%d].env_rate[%d]", target[y], k, e) * __INT16_MAX__) + 1
					);
					Toml_SetVar(&toml,
						x_fmt("%s.entry[%d].env_level[%d]", target[y], k, e),
						"%d",
						(s16)(Toml_GetFloat(&toml, "%s.entry[%d].env_level[%d]", target[y], k, e) * __INT16_MAX__) + 1
					);
				}
				
				Toml_SetVar(&toml,
					x_fmt("%s.entry[%d].env_rate[%d]", target[y], k, env),
					"%d",
					-1
				);
				Toml_SetVar(&toml,
					x_fmt("%s.entry[%d].env_level[%d]", target[y], k, env),
					"%d",
					0
				);
				Toml_SetVar(&toml,
					x_fmt("%s.entry[%d].release_rate", target[y], k),
					"%d",
					Audio_GetReleaseID(Toml_GetFloat(&toml, "%s.entry[%d].release_rate", target[y], k))
				);
			}
		}
		
		if (save)
			Toml_Save(&toml, list.item[i]);
		
		Toml_Free(&toml);
	}
	
	bailout:
	List_Free(&list);
	
	return ssave;
}

void Project_Write(Rom* rom) {
	Toml* t = &rom->toml;
	
	osAssert(t != NULL);
	
	for (var_t i = 0; i < PROJECT_ENUM_MAX; i++) {
		osLog("%d / %d", i, PROJECT_ENUM_MAX);
		if (sTomlProject[i][Z_OLD_ITEM] && Toml_Var(t, sTomlProject[i][Z_OLD_ITEM]) && strcmp(Toml_Var(t, sTomlProject[i][Z_OLD_ITEM]), "\"NULL\""))
			Toml_SetVar(t, sTomlProject[i][Z_ITEM], "%s", Toml_Var(t, sTomlProject[i][Z_OLD_ITEM]));
		
		else if (Toml_Var(t, sTomlProject[i][Z_ITEM]))
			Toml_SetVar(t, sTomlProject[i][Z_ITEM], "%s", Toml_Var(t, sTomlProject[i][Z_ITEM]));
		
		else
			Toml_SetVar(t, sTomlProject[i][Z_ITEM], sTomlProject[i][Z_FMT], sTomlProject[i][Z_DEF]);
		
		if (sTomlProject[i][Z_OLD_ITEM])
			Toml_RmVar(t, sTomlProject[i][Z_OLD_ITEM]);
	}
	
	const char* oldGcc[] = {
		"gcc_base_flags",
		"gcc_actor_flags",
		"gcc_code_flags",
		"gcc_kaleido_flags",
		"gcc_state_flags",
		"ld_base_flags",
		"ld_code_flags",
		"ld_scene_flags",
		"ld_ulib_flags",
	};
	
	for (var_t i = 0; i < ArrCount(oldGcc); i++)
		Toml_RmVar(t, oldGcc[i]);
}

void Project_Reconfig(Rom* rom) {
	if (g64.reconfig) return;
	
	Project_Write(rom);
	g64.reconfig = true;
}

void Project_Read(Rom* rom) {
	const struct {
		const char*  item;
		const char** dchar;
		s8* dbool;
	} wow[] = {
		{ PROJECT_ITEM(BASEROM),       .dchar       = &g64.baseRom           },
		{ PROJECT_ITEM(BUILDROM),      .dchar       = &g64.buildName         },
		{ PROJECT_ITEM(VANILLA),       .dchar       = &g64.vanilla           },
		{ PROJECT_ITEM(BASEWAD),       .dchar       = &g64.baseWad           },
		{ PROJECT_ITEM(DOLPHIN),       .dchar       = &g64.dolphinDir        },
		
		{ PROJECT_ITEM(SUFFIX_REL),    .dchar       = &g64.suffix[0]         },
		{ PROJECT_ITEM(SUFFIX_DEV),    .dchar       = &g64.suffix[1]         },
		
		{ PROJECT_ITEM(CHECK_UPDATES), .dbool       = &g64.checkUpdates      },
		{ PROJECT_ITEM(YAZ_HEADER),    .dbool       = &g64.yazHeader         },
		
		{ PROJECT_ITEM(GCC_GCC),       .dchar       = &g64.gccFlags.gcc      },
		{ PROJECT_ITEM(GCC_MAIN),      .dchar       = &g64.gccFlags.main     },
		{ PROJECT_ITEM(GCC_ACTOR),     .dchar       = &g64.gccFlags.actor    },
		{ PROJECT_ITEM(GCC_CODE),      .dchar       = &g64.gccFlags.code     },
		{ PROJECT_ITEM(GCC_KALEIDO),   .dchar       = &g64.gccFlags.kaleido  },
		{ PROJECT_ITEM(GCC_STATE),     .dchar       = &g64.gccFlags.state    },
		
		{ PROJECT_ITEM(LD_BASE),       .dchar       = &g64.linkerFlags.base  },
		{ PROJECT_ITEM(LD_CODE),       .dchar       = &g64.linkerFlags.code  },
		{ PROJECT_ITEM(LD_SCENE),      .dchar       = &g64.linkerFlags.scene },
		{ PROJECT_ITEM(LD_ULIB),       .dchar       = &g64.linkerFlags.ulib  },
	};
	
	osLog("project read");
	if (sys_stat("z64project.cfg"))
		strrep(gProjectConfig, ".toml", ".cfg");
	osLog("swapped?");
	
	if (!rom->toml.data) {
		if (sys_stat(gProjectConfig)) {
			Toml_Load(&rom->toml, gProjectConfig);
			
			for (var_t i = 0; i < PROJECT_ENUM_MAX; i++) {
				if (!Toml_Var(&rom->toml, sTomlProject[i][Z_ITEM])) {
					warn(gLang.main.reconfig_force);
					Project_Reconfig(rom);
					info(gLang.main.reconfig_ok);
					break;
				}
			}
		} else {
			rom->toml = Toml_New();
			Project_Write(rom);
		}
	}
	
	for (var_t i = 0; i < ArrCount(wow); i++) {
		if (wow[i].dchar) {
			delete(*wow[i].dchar);
			*wow[i].dchar = Toml_GetStr(&rom->toml, wow[i].item);
		}
		if (wow[i].dbool)
			*wow[i].dbool = Toml_GetBool(&rom->toml, wow[i].item);
		
	}
	
	delete(g64.build[0], g64.build[1]);
	g64.build[0] = fmt("%s%s.z64", g64.buildName, g64.suffix[0]);
	g64.build[1] = fmt("%s%s.z64", g64.buildName, g64.suffix[1]);
}

/*============================================================================*/

const char* sAppDataFile;
const char* sProjectFolder;

#define APPDATA_ITEM(enum) x_fmt(sAppData[enum][0], sProjectFolder)

static void AppData_Read() {
	Toml* t = &g64.app_data;
	
	sAppDataFile = fmt("%s/z64rom/config.toml", sys_env(ENV_APPDATA));
	sProjectFolder = strdup(x_snakeify(sys_appdir()));
	
	sys_mkdir(x_path(sAppDataFile));
	
	if (sys_stat(sAppDataFile)) Toml_Load(t, sAppDataFile);
	else *t = Toml_New();
	
	for (var_t i = 0; i < APPDATA_MAX; i++) {
		if (!Toml_Var(t, APPDATA_ITEM(i)))
			Toml_SetVar(t, APPDATA_ITEM(i), sAppData[i][1], sAppData[i][2]);
	}
	
	Lang_Init(qxf(Toml_GetStr(&g64.app_data, APPDATA_ITEM(APPDATA_LANG))));
}

static void AppData_Save() {
	if (g64.app_data.changed)
		Toml_Save(&g64.app_data, sAppDataFile);
	delete(sAppDataFile, sProjectFolder);
}

static void AppData_Apply() {
	Toml* t = &g64.app_data;
	
	if (!g64.dump && g64.buildID != Toml_GetInt(t, APPDATA_ITEM(APPDATA_BUILD_TYPE))) {
		gForceCodeMake = true;
		g64.noMake = false;
		delete(g64.makeTarget);
	}
	
	if (g64.yazHeader != Toml_GetInt(t, APPDATA_ITEM(APPDATA_YAZ_HEADER)))
		Clean_Cache();
}

static s32 sNumSetRom;

static void ArgParse_SetPJ64(Rom* rom, const char* file) {
	Toml_SetVar(&rom->toml, "project64", "\"%s\"", x_strunq(file));
}

static void ArgParse_SetWad(Rom* rom, const char* file) {
	if (dir_isrel(file))
		file = dirabs_f(sys_appdir(), file);
	if (dir_isabs(file))
		file = dirrel_f(sys_appdir(), file);
	
	Toml_SetVar(&rom->toml, PROJECT_ITEM(BASEWAD), "\"%s\"", file);
}

static void ArgParse_SetInput(const char* file) {
	delete(g64.input);
	g64.input = strdup(file);
	sNumSetRom++;
}

static void ArgParse_Project(void) {
	g64.buildID = ROM_RELEASE;
}

const struct {
	const char* ext;
	union {
		void  (*zargfunc)(void);
		void  (*tmplfunc)(const char*);
		void  (*romfunc)(Rom*, const char*);
		void* set;
	};
	s32  type;
	Exit exit;
} sArgFileInfo[] = {
	{ ".mid",          .set          = Template_NewSequence, 1,         EXIT_PROMPT           },
	{ ".wav",          .set          = Template_NewSample,   1,         EXIT_PROMPT           },
	{ ".aiff",         .set          = Template_NewSample,   1,         EXIT_PROMPT           },
	{ ".mp3",          .set          = Template_NewSample,   1,         EXIT_PROMPT           },
	{ ".mid",          .set          = Template_NewSequence, 1,         EXIT_PROMPT           },
	
	{ ".z64",          .set          = ArgParse_SetInput,    1,         EXIT_CONTINUE         },
	{ ".wad",          .set          = ArgParse_SetWad,      2,         EXIT_PROMPT           },
	{ "project64.exe", .set          = ArgParse_SetPJ64,     2,         EXIT_PROMPT           },
	
	{ gProjectConfig,  .set          = ArgParse_Project,     0,         EXIT_CONTINUE         },
	
	{ ".zip",          .set          = Package_Load,         1,         EXIT_PROMPT           },
};

static void RomDiff(const char* file_a, const char* file_b) {
	typedef struct StructBE {
		u32 vstart;
		u32 vend;
		u32 __0;
		u32 __1;
	} RomFile;
	
	Memfile mem_a = Memfile_New();
	Memfile mem_b = Memfile_New();
	
	Memfile_LoadBin(&mem_a, file_a);
	Memfile_LoadBin(&mem_b, file_b);
	
	SegmentSet(0, mem_a.data);
	SegmentSet(1, mem_b.data);
	
	RomFile* dma_a = SegmentToVirtual(0, 0x00012F70);
	RomFile* dma_b = SegmentToVirtual(1, 0x00012F70);
	
	for (int dma_index = 0; dma_a->vend && dma_b->vend; dma_a++, dma_b++, dma_index++) {
		if (dma_a->vstart == -1U || dma_b->vstart == -1U)
			continue;
		if (dma_index < 3)
			continue;
		
		void* ptr_a = SegmentToVirtual(0, dma_a->vstart);
		void* ptr_b = SegmentToVirtual(1, dma_b->vstart);
		u32 segment_a = VirtualToSegment(0, ptr_a);
		u32 segment_b = VirtualToSegment(1, ptr_b);
		size_t size_a = dma_a->vend - dma_a->vstart;
		size_t size_b = dma_b->vend - dma_b->vstart;
		
		if (size_a != size_b)
			warn("size diff ID %X\n\tA: %08X %X\n\tB: %08X %X",
				dma_index, segment_a, size_a, segment_b, size_b);
		
		if (memcmp(ptr_a, ptr_b, Min(size_a, size_b))) {
			warn("data diff ID %X\n\tA: %08X %X\n\tB: %08X %X",
				dma_index, segment_a, size_a, segment_b, size_b);
			info_hex(0, ptr_a, 0x40, 0);
		}
	}
	
	Memfile_Free(&mem_a);
	Memfile_Free(&mem_b);
}

static Exit ArgParse_ValidateArguments(Rom* rom, const s32 narg, const char** arg) {
	s8* valid = qxf(new(s8[narg]));
	
	for (var_t i = 1; i < narg; i++) {
		if (valid[i] > 0)
			continue;
		
		for (var_t j = 0; j < ARG_MAX; j++) {
			const char* targ[] = { "app", arg[i], NULL };
			
			if (strarg(targ, sArguments[j][0])) {
				
				valid[i] = 1;
				for (var_t k = 0; k < sArgFolNum[j]; k++)
					valid[i + k + 1] = 1;
				break;
			}
		}
		
		if (!valid[i]) {
			for (var_t j = 0; j < ArrCount(sArgFileInfo); j++) {
				if (striend(arg[i], sArgFileInfo[j].ext)) {
					valid[i] = 1;
					break;
				}
			}
			
			if (!valid[i])
				if (sys_stat(arg[i]))
					valid[i] = -1;
		}
	}
	
	List arg_list = List_New();
	List file_list = List_New();
	
	for (var_t i = 1; i < narg; i++) {
		if (!valid[i]) {
			List_Add(&arg_list, arg[i]);
			valid[0] = -1;
		}
		if (valid[i] < 0) {
			List_Add(&file_list, arg[i]);
			valid[0] = -1;
		}
	}
	
	if (valid[0] < 0) {
		const char* args = List_Concat(&arg_list, " ");
		const char* files = List_Concat(&file_list, " ");
		
		if (args) warn_align(gLang.main.unk_arg, args);
		if (files) warn_align(gLang.main.unk_fmt, files);
		
		delete(args, files);
	}
	
	List_Free(&arg_list);
	List_Free(&file_list);
	
	return valid[0] < 0 ? EXIT_PROMPT : EXIT_CONTINUE;
}

static Exit ArgParse_ParseFileInputs(Rom* rom, const s32 narg, const char** arg) {
	Exit exit = EXIT_CONTINUE;
	
	for (var_t i = 1; i < narg; i++) {
		for (var_t j = 0; j < ArrCount(sArgFileInfo); j++) {
			if (striend(x_strunq(arg[i]), sArgFileInfo[j].ext)) {
				switch (sArgFileInfo[j].type) {
					case 0:
						sArgFileInfo[j].zargfunc();
						break;
						
					case 1:
						sArgFileInfo[j].tmplfunc(arg[i]);
						break;
						
					case 2:
						sArgFileInfo[j].romfunc(rom, arg[i]);
						break;
				}
				
				exit = Max(exit, sArgFileInfo[j].exit);
			}
		}
	}
	
	if (sNumSetRom > 1) {
		warn(gLang.main.err_too_many_roms);
		return EXIT_PROMPT;
	}
	
	return exit;
}

#include "z64elf.h"

Exit ArgParse(Rom* rom, const s32 narg, const char** arg) {
	Exit (*func[])(Rom*, const s32, const char**) = {
		ArgParse_ValidateArguments,
		ArgParse_ParseFileInputs,
	};
	int i = 0;
	
	if ((i = strarg(arg, "diff"))) {
		RomDiff(arg[i], arg[i + 1]);
		
		return EXIT_INSTANT;
	}
	
	foreach(i, func) {
		Exit pass;
		
		if ((pass = func[i](rom, narg, arg)))
			return pass;
	}
	
	for (var_t a = 0; a < ARG_MAX; a++) {
		Memfile mem = Memfile_New();
		var_t i = 0;
		if (!(i = strarg(arg, sArguments[a][0])))
			continue;
		
		osLog("%s", sArguments[a][0]);
		
		switch (a) {
			case ARG_DEV_ELF_SYM:
				Memfile_LoadBin(&mem, arg[i]);
				Elf64* elf = Elf64_New(mem.data);
				info("%s: %s\nsym: %08X", x_filename(arg[i]), arg[i + 1],
					Elf64_FindSym(elf, arg[i + 1]));
				
				return EXIT_INSTANT;
				
			case ARG_LANG_SET:
				(void)0;
				List list = List_New();
				
				List_SetFilters(&list, CONTAIN_START, "lang_", CONTAIN_END, ".toml");
				List_Walk(&list, "tools/", 0, LIST_FILES | LIST_RELATIVE);
				
				for (var_t k = 0; k < list.num; k++) {
					if (!strcmp(x_fmt("%s.toml", arg[i]), list.item[k])) {
						Toml_SetVar(&g64.app_data, APPDATA_ITEM(APPDATA_LANG), "\"%s\"", arg[i]);
						info(gLang.info_lang_set, arg[i]);
						
						return EXIT_PROMPT;
					}
				}
				
				errr(gLang.err_lang_set, arg[i]);
				
				break;
				
			case ARG_THREADS:
				g64.threadNum = clamp(sint(arg[i]), 1, 128);
				break;
				
			case ARG_NEW_ACTOR:
				Template_NewActor(arg[i], arg[i + 1], arg[i + 2]);
				
				return EXIT_INSTANT;
				
			case ARG_NEW_EFFECT:
				Template_NewEffect(arg[i], arg[i + 1]);
				
				return EXIT_INSTANT;
				
			case ARG_SYM:
				GetSymInfo(arg[i]);
				
				return EXIT_INSTANT;
				
			case ARG_MIGRATE:
				(void)0;
				const char* mode = arg[i];
				Migrate(mode, arg[i + 1]);
				
				return EXIT_INSTANT;
				
			case ARG_PACK:
				Package_Pack();
				
				return EXIT_INSTANT;
				
			case ARG_RECONFIG:
				if (sys_stat("z64project.cfg"))
					strrep(gProjectConfig, ".toml", ".cfg");
				Project_Reconfig(rom);
				Project_Read(rom);
				
				if (g64.baseRom[0] != '\0') {
					osLog("Project_TomlUpdate();"); Project_TomlUpdate();
					osLog("Project_UpdateOverlays();"); Project_UpdateOverlays();
					osLog("Project_UpdateScenes();"); Project_UpdateScenes();
					osLog("Project_UpdateSoundFont();"); Project_UpdateSoundFont();
					osLog("Project_UpdateManifests();"); Project_UpdateManifests();
					osLog("Project_UpdateEnvelopes();"); Project_UpdateEnvelopes();
					Clean_Make();
				}
				
				info(gLang.main.reconfig_ok);
				
				return EXIT_INSTANT;
				
			case ARG_REINSTALL:
				sys_touch("tools/.installing");
				break;
				
			case ARG_CLEAR_PROJECT:
				Clean_Dump();
				cli_clearln(2);
				
				return EXIT_INSTANT;
				
			case ARG_CLEAR_CACHE:
				Clean_Cache();
				cli_clearln(2);
				
				return EXIT_INSTANT;
				
			case ARG_CLEAN_SAMPLES:
				Audio_DeleteUnreferencedSamples();
				
				return EXIT_PROMPT;
				
			case ARG_CLEAN_BETA:
				Rom_DeleteUnusedContent();
				return EXIT_PROMPT;
				break;
				
			case ARG_AUTO_INSTALL:
				g64.autoInstall = sbool(arg[i]);
				break;
				
			case ARG_FILE_Z64HDR:
				g64.file.z64hdr = strdup(arg[i]);
				if (!sys_stat(g64.file.z64hdr))
					errr_align(gLang.err_load, g64.file.z64hdr);
				break;
				
			case ARG_FILE_MIPS64:
				g64.file.gcc64 = strdup(arg[i]);
				if (!sys_stat(g64.file.gcc64))
					errr_align(gLang.err_load, g64.file.gcc64);
				break;
				
			case ARG_VANILLA:
				g64.vanilla = qxf(strdup(x_strunq(arg[i])));
				
				if (*g64.vanilla != '.')
					errr(gLang.main.err_custom_vanilla);
				break;
				
			case ARG_TARGET:
				g64.makeTarget = arg[i];
				break;
				
			case ARG_INFO:
				g64.info = true;
				break;
				
			case ARG_YAZ:
				g64.compress = true;
				break;
				
			case ARG_RELEASE:
				g64.buildID = ROM_RELEASE;
				break;
				
			case ARG_FORCE:
				g64.makeForce = true;
				break;
				
			case ARG_FORCE_CODE:
				gForceCodeMake = true;
				break;
				
			case ARG_CLEAN:
				Clean_Make();
				
				return EXIT_INSTANT;
				
			case ARG_NO_THREADING:
				g64.threading = false;
				g64.threadNum = 1;
				break;
				
			case ARG_UNK_AUDIO:
				g64.audioUnk = true;
				break;
				
			case ARG_AUDIO_ONLY:
				g64.audioOnly = true;
				
				if (strarg(arg, sArguments[ARG_UNK_AUDIO][0]))
					g64.audioUnk = true;
				
				if ((i = strarg(arg, "dump"))) {
					g64.dump = true;
					osLog("Dump Rom [%s]", arg[i]);
					Rom_New(rom, arg[i]);
					
					if ((i = strarg(arg, sArguments[ARG_AO_SEQFNT_TBL][0])))
						rom->offset.table.seqFontTbl = shex(arg[i]);
					if ((i = strarg(arg, sArguments[ARG_AO_SEQ_TBL][0])))
						rom->offset.table.seqTable = shex(arg[i]);
					if ((i = strarg(arg, sArguments[ARG_AO_FONT_TBL][0])))
						rom->offset.table.fontTable = shex(arg[i]);
					if ((i = strarg(arg, sArguments[ARG_AO_SAMPLE_TBL][0])))
						rom->offset.table.sampleTable = shex(arg[i]);
					
					if ((i = strarg(arg, sArguments[ARG_AO_SEQ_DATA][0])))
						rom->offset.segment.seqRom = shex(arg[i]);
					if ((i = strarg(arg, sArguments[ARG_AO_FONT_DATA][0])))
						rom->offset.segment.fontRom = shex(arg[i]);
					if ((i = strarg(arg, sArguments[ARG_AO_SAMPLE_DATA][0])))
						rom->offset.segment.smplRom = shex(arg[i]);
					
					AudioOnly_Dump(rom);
				} else if (strarg(arg, "build")) {
					rom->mem.sampleTbl = Memfile_New();
					rom->mem.fontTbl = Memfile_New();
					rom->mem.seqTbl = Memfile_New();
					rom->mem.seqFontTbl = Memfile_New();
					Memfile_Alloc(&rom->mem.sampleTbl, MbToBin(0.1));
					Memfile_Alloc(&rom->mem.fontTbl, MbToBin(0.1));
					Memfile_Alloc(&rom->mem.seqTbl, MbToBin(0.1));
					Memfile_Alloc(&rom->mem.seqFontTbl, MbToBin(0.1));
					
					g64.makeTarget = "sound";
					
					if (!g64.noMake) {
						Make_Sound();
						Make_Sequence();
					}
					AudioOnly_Build(rom);
				}
				
				info(gLang.success);
				
				exit(EXIT_SUCCESS);
				return EXIT_INSTANT;
				
			case ARG_CCDEFINE: {
				break;
			}
			
			case ARG_NO_MAKE:
				g64.noMake = true;
				break;
				
			case ARG_MAKE_ONLY:
				g64.makeOnly = true;
				break;
				
			case ARG_INJECT_VC:
				Extra_WadInject(true);
				
				return EXIT_INSTANT;
				
			case ARG_BUILD_VC:
				g64.compress = true;
				g64.buildVC = true;
				break;
				
			case ARG_INSTANT_SCENE:
				sys_touch("src/system/state/0x02-BootTitle/BootTitle.c");
				sys_touch("src/system/state/0x04-Opening/Opening.c");
				g64.instant.use = true;
				
				for (int j = 0; j < 4; j++)
					if (!arg[i + j])
						errr("%s", *((char**)sArguments[i][1]));
				
				g64.instant.scene = sint(arg[i]);
				osLog("arg0: %02X", g64.instant.scene);
				g64.instant.spawn = sint(arg[i + 1]);
				osLog("arg1: %02X", g64.instant.spawn);
				g64.instant.header = sint(arg[i + 2]);
				osLog("arg2: %02X", g64.instant.header);
				g64.instant.age = sint(arg[i + 3]);
				osLog("arg3: %02X", g64.instant.age);
				
				break;
				
			case ARG_NO_WAIT:
				g64.noWait = true;
				break;
				
			case ARG_UPDATE:
				Tools_InstallHeader(true);
				return EXIT_INSTANT;
				
			case ARG_UPGRADE:
				sys_exed(x_fmt("tools\\z64upgrade.exe --version %s", gToolName + strlen("" PRNT_BLUE "z64rom " PRNT_GRAY)));
				
				return EXIT_INSTANT;
				
			case ARG_NO_PLAY:
				g64.noPlay = true;
				break;
				
			case ARG_CHILL:
				Chill();
				g64.chill = true;
				break;
				
			case ARG_MM_IMPORT:
				MajorasMaskImport(true);
				
				return EXIT_INSTANT;
				
			case ARG_HELP:
				for (int i = 0; i < ARG_MAX; i++)
					printf("--%-16s %s\n" PRNT_RSET, sArguments[i][0], *((char**)sArguments[i][1]));
				
				return EXIT_PROMPT;
				
			case ARG_ROM_CONTENT_LIST:
				RomContentList_Update();
				
				return EXIT_INSTANT;
				
			case ARG_POST_UPDATE:
				(void)0;
				Zip zip = {};
				
				Zip_Load(&zip, "update.zip", ZIP_READ);
				Zip_ReadByName(&zip, "tools/z64upgrade.exe", &mem);
				
				mem.param.throwError = false;
				while (Memfile_SaveBin(&mem, "tools/z64upgrade.exe")) {
					warn(gLang.main.warn_update_retry);
					sys_sleep(2);
				}
				
				Zip_Free(&zip);
				Memfile_Free(&mem);
				
				sys_rm("update.zip");
				
				return EXIT_INSTANT;
				
			case ARG_DEV_PACK_RELEASE:
				(void)0;
				Proc* exe = Proc_New("tools\\z64upgrade.exe --pack --version %s", gToolName + strlen("" PRNT_BLUE "z64rom " PRNT_GRAY));
				
				Proc_Exec(exe);
				Proc_Join(exe);
				
				return EXIT_INSTANT;
				
			case ARG_YAZ_NO_CACHE:
				g64.noCache = true;
				break;
				
			default:
				break;
		}
	}
	
	return EXIT_CONTINUE;
}

/*============================================================================*/

void Chill(void) {
	extern DataFile gSongA, gSongB, gSongC, gSongD;
	extern DataFile gSongE, gSongF, gSongG;
	const DataFile* songList[] = {
		&gSongA,
		&gSongB,
		&gSongC,
		&gSongD,
		&gSongE,
		&gSongF,
		&gSongG,
	};
	const f32 volume[] = {
		0.75f,
		0.80f,
		0.38f,
		0.58f,
		0.42f,
		0.75f,
		0.75f,
	};
	
	static bool sWeChilling;
	
	if (!sWeChilling) {
		int c = ArrCount(songList);
		int id = (randf() * 753.167f);
		
		FastTracker_SetVolume(volume[id % c]);
		FastTracker_Play(
			songList[id % c]->data,
			songList[id % c]->size);
	}
	sWeChilling = true;
	g64.chill = true;
}

/*============================================================================*/

static void Main_SearchRoms() {
	List list = List_New();
	
	List_SetFilters(&list, CONTAIN_END, ".z64", CONTAIN_END, ".Z64");
	List_Walk(&list, "", 0, LIST_FILES);
	
	if (list.num) {
		const char* str;
		var_t index = 0;
		
		info(gLang.main.search_found);
		for (var_t i = 0; i < list.num; i++)
			printf("    " PRNT_GRAY "%d" PRNT_RSET ": "PRNT_YELW "%s"PRNT_RSET "\n", i, list.item[i]);
		
		info_nl();
		info(gLang.main.search_index);
		
		str = cli_gets();
		if (str && isdigit(str[0])) {
			index = sint(str);
			
			if (index >= 0 && index < list.num)
				g64.input = strdup(list.item[index]);
		}
		
		cli_clearln(list.num + 5);
	}
}

#define Z64_NO_INPUT    (!g64.input && g64.baseRom[0] == '\0')
#define Z64_NEW_BASEROM (g64.input)

f64 getval(void* udata, int id) {
	f32* tbl = udata;
	
	return tbl[id];
}

#include "z64elf.h"

int Scene_GetHeaderNum(void* segment);

s32 main(int narg, const char** arg) {
	Rom* rom = new(Rom);
	Toml* t = &rom->toml;
	
	g64.threadNum = clamp(sys_getcorenum() * 1.5f, 1, 128);
	g64.workDir = strdup(sys_workdir());
	sys_setworkdir(sys_appdir());
	
	info_title(gToolName, NULL);
	
	AppData_Read();
	Project_Read(rom);
	
	osLog("arg_parse");
	switch (ArgParse(rom, narg, arg)) {
		case EXIT_INSTANT: goto exit;
		case EXIT_PROMPT: goto prompt;
		default: break;
	}
	
	switch (Tools_Init()) {
		case EXIT_INSTANT: goto exit;
		case EXIT_PROMPT: goto prompt;
		default:
			if (g64.checkUpdates)
				Tools_CheckUpdates();
	}
	
	if (Z64_NO_INPUT) {
		Main_SearchRoms();
	}
	
	if (Z64_NEW_BASEROM) {
		const char* filename = x_filename(g64.input);
		
		if (!sys_stat(filename))
			sys_cp(g64.input, filename);
		
		Toml_SetVar(t, PROJECT_ITEM(BASEROM), "\"%s\"", filename);
		g64.baseRom = Toml_GetStr(t, PROJECT_ITEM(BASEROM));
		g64.dump = true;
		
	} else if (g64.baseRom[0] == '\0')
		goto prompt;
	
	AppData_Apply();
	Rom_New(rom, g64.baseRom);
	
	if (g64.info) {
		info("State:"
			"\nthreads: " PRNT_BLUE "%d" PRNT_RSET
			"\nbuild_id: " PRNT_BLUE "%d" PRNT_RSET
			"\ndump: " PRNT_BLUE "%B" PRNT_RSET
			"\nmake force: " PRNT_BLUE "%B" PRNT_RSET
			"\nmake only: " PRNT_BLUE "%B" PRNT_RSET
			"\ncompress: " PRNT_BLUE "%B" PRNT_RSET
			"\nno cache: " PRNT_BLUE "%B" PRNT_RSET
			"\nreconfig: " PRNT_BLUE "%B" PRNT_RSET
			,
			g64.threadNum,
			g64.buildID,
			g64.dump,
			g64.makeForce,
			g64.makeOnly,
			g64.compress,
			g64.noCache,
			g64.reconfig
		);
	}
	
	if (g64.dump) {
		Rom_Dump(rom);
		RomContentList_Create();
		
		Toml_SetVar(&g64.app_data, APPDATA_ITEM(APPDATA_BUILD_TYPE), "%d", -1);
	} else {
		if (!g64.noMake)
			Make(rom, true);
		if (g64.compress && !g64.noCache && !g64.makeOnly)
			Rom_Compress();
		if (!g64.makeOnly)
			Rom_Build(rom);
		
		if (!g64.noMake)
			Toml_SetVar(&g64.app_data, APPDATA_ITEM(APPDATA_BUILD_TYPE), "%d", g64.buildID);
		Toml_SetVar(&g64.app_data, APPDATA_ITEM(APPDATA_YAZ_HEADER), "%d", g64.yazHeader);
	}
	
	if (g64.buildVC)
		Extra_WadInject(false);
	
	info(gLang.success);
	
	prompt:
#ifdef _WIN32
		if (!g64.noWait)
			info_getc(gLang.press_enter);
#endif
	exit:
	
	if (rom->toml.changed && !g64.cleanDump && g64.baseRom[0])
		Toml_Save(&rom->toml, gProjectConfig);
	AppData_Save();
	
	FastTracker_Stop();
	Rom_Free(rom);
	Toml_Free(&g64.app_data);
	delete(rom);
	delete(g64.suffix[0], g64.suffix[1], g64.build[0], g64.build[1]);
	delete(g64.baseRom, g64.buildName, g64.baseWad, g64.vanilla, g64.input);
	delete(g64.file.z64hdr, g64.file.gcc64);
	delete(g64.gccFlags.gcc, g64.gccFlags.actor, g64.gccFlags.code, g64.gccFlags.kaleido, g64.gccFlags.state);
	delete(g64.linkerFlags.base, g64.linkerFlags.code, g64.linkerFlags.scene, g64.linkerFlags.ulib);
	delete(g64.workDir, g64.makeTarget);
	
	return 0;
}
