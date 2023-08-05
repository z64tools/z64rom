#include "z64rom.h"
#include "z64make.h"
#include <ext_proc.h>
#include "z64elf.h"

static volatile bool sMake = false;
static volatile bool sMakeUserLibrary = false;

typedef struct LinkerInfo {
	const char* symbol;
	u32 address;
} LinkerInfo;

static LinkerInfo* gLinkerInfo;
static u32 gLinkerNum;
bool gForceCodeMake;

const char* gLinkerFile_User = "include/z_lib_user.ld";
const char* gLinkerFile_Object = "include/z_object_user.ld";
const char* gLinkerFile_TempUser = "include/.tmp.z_lib_user.ld";
const char* gLinkerFile_TempObject = "include/.tmp.z_object_user.ld";

static void LinkerInfo_RegisterDiff(const char* new, const char* oldFile) {
	Memfile mem = Memfile_New();
	u32 newNum = 0;
	u32 oldNum = 0;
	LinkerInfo* linkerNew = NULL;
	LinkerInfo* linkerPrev = NULL;
	
	Memfile_LoadStr(&mem, oldFile);
	
	forline(line, new) {
		LinkerInfo* info;
		
		if (strnlen(line, 3) < 2)
			continue;
		if (line[0] == '\n')
			continue;
		if (!strstr(line, "="))
			continue;
		
		linkerNew = realloc(linkerNew, sizeof(LinkerInfo) * (++newNum ));
		info = &linkerNew[newNum - 1];
		
		info->symbol = strndup(line, strcspn(line, " ="));
		info->address = shex(line + strcspn(line, "=") + 2);
	}
	
	forline(line, mem.str) {
		LinkerInfo* info;
		
		if (strnlen(line, 3) < 2)
			continue;
		if (line[0] == '\n')
			continue;
		
		linkerPrev = realloc(linkerPrev, sizeof(LinkerInfo) * ++oldNum );
		info = &linkerPrev[oldNum - 1];
		
		info->symbol = strndup(line, strcspn(line, " ="));
		info->address = shex(line + strcspn(line, "=") + 2);
	}
	
	for (int i = 0; i < newNum; i++) {
		LinkerInfo* info = NULL;
		bool found = false;
		
		for (int j = 0; j < oldNum; j++) {
			if (strcmp(linkerNew[i].symbol, linkerPrev[j].symbol))
				continue;
			
			found = true;
			
			if (linkerNew[i].address == linkerPrev[j].address)
				break;
			
			gLinkerInfo = realloc(gLinkerInfo, sizeof(LinkerInfo) * ++gLinkerNum);
			info = &gLinkerInfo[gLinkerNum - 1];
			info->symbol = strdup(linkerNew[i].symbol);
			
			break;
		}
		
		if (found)
			goto free;
		
		gLinkerInfo = realloc(gLinkerInfo, sizeof(LinkerInfo) * ++gLinkerNum);
		info = &gLinkerInfo[gLinkerNum - 1];
		info->symbol = strdup(linkerNew[i].symbol);
		
		free:
		delete(linkerNew[i].symbol);
	}
	
	for (int i = 0; i < oldNum; i++)
		delete(linkerPrev[i].symbol);
	
	delete(linkerNew, linkerPrev);
	Memfile_Free(&mem);
}

static void LinkerInfo_Free(void) {
	if (gLinkerInfo) {
		for (int i = 0; i < gLinkerNum; i++) {
#if 0
				warn("%s", gLinkerInfo[i].symbol);
#endif
			delete(gLinkerInfo[i].symbol);
		}
		
		delete(gLinkerInfo);
	}
}

static void Make_Info(const char* tool, const char* msg, const char* target) {
	if (msg) {
		if (!strcmp(tool, "gcc")) {
			char* modmsg = strndup(msg, strlen(msg) * 3);
			
			strinsat(modmsg + strcspn(modmsg, ":"), PRNT_RSET);
			strinsat(modmsg, PRNT_CYAN);
			char* nl = strline(modmsg, 1);
			strinsat(nl + strcspn(nl, ":"), PRNT_RSET);
			strinsat(nl, PRNT_CYAN);
			
			if (strrep(modmsg, "warning:", PRNT_PRPL "warning:" PRNT_RSET)) {
				strrep(modmsg, "[-", "[" PRNT_PRPL "-");
				strrep(modmsg, "]", PRNT_RSET "]");
				
			} else if (strrep(modmsg, "error:", PRNT_REDD "error:" PRNT_RSET)) {
				strrep(modmsg, "[-", "[" PRNT_REDD "-");
				strrep(modmsg, "]", PRNT_RSET "]");
			}
			
			info_volatile("\n%s\n", modmsg);
			
			delete(modmsg);
			
		} else
			info_volatile("\n%s\n", msg);
		
	} else
		info_volatile("" PRNT_PRPL "M" PRNT_GRAY ": " PRNT_RSET "%s" PRNT_RSET "\n", target);
	
	sMake = 1;
	delete(msg);
}

static void Make_Run(char* cmd) {
	strcat(cmd, " 2>&1");
	char* msg = sys_exes(cmd);
	
	if (strlen(msg) > 1 && (stristr(msg, "warning") || stristr(msg, "error")))
		info_volatile("\n%s\n", msg);
	
	delete(msg);
}

static const char* Make_EnvironmentPath(void) {
	static char* envPath;
	
	if (!envPath) {
		envPath = qxf(fmt( "PATH=%stools/mips64-binutils/bin", sys_appdir() ));
		// strrep(envPath, "/", "\\");
	}
	
	return envPath;
}

static const char* Make_TempPath(void) {
	static char* tempPath;
	
	if (!tempPath) {
		tempPath = qxf(fmt( "TMPDIR=%s", sys_env(ENV_TEMP) ));
		// strrep(envPath, "/", "\\");
	}
	
	return tempPath;
}

// # # # # # # # # # # # # # # # # # # # #
// # Make_SpawnCutsceneTable             #
// # # # # # # # # # # # # # # # # # # # #

void Make_SpawnCutsceneTable(void) {
	const char* txt[] = {
		"#ifndef SPAWN_CUTSCENE_TABLE_H\n"
		"#define SPAWN_CUTSCENE_TABLE_H\n"
		"#define LINK_AGE_BOTH 2\n"
		"\n"
		"typedef enum {\n"
		"    FLAG_SWITCH = 0,\n"
		"    FLAG_CHEST,\n"
		"    FLAG_EVENTCHKINF,\n"
		"    FLAG_COLLECTIBLE,\n"
		"} FlagType;\n"
		"\n"
		"union {\n"
		"    struct {\n"
		"        u32 header        : 4;\n"
		"        u32 type          : 2;\n"
		"        u32 age           : 2;\n"
		"        u32 flag          : 8;\n"
		"        u32 nextIsSegment : 1;\n"
		"        u32 spawn         : 7;\n"
		"        u32 scene         : 8;\n"
		"    };\n"
		"    void* segment;\n"
		"} sSpawnCutsceneTable[] = {\n"
		"    ",
		
		"\n};\n"
		"\n"
		"#endif"
	};
	
	Toml toml = {};
	Memfile mem = Memfile_New();
	const char* input = "src/spawn_cutscene_table.toml";
	const char* output = "src/lib_user/library/SpawnCutsceneTable";
	const char* dep = "src/lib_user/library/Cutscene.c";
	
	Memfile_Alloc(&mem, MbToBin(1.0));
	
	if (!sys_stat(input))
		errr_align(gLang.err_missing, input);
	
	if (sys_stat(input) <= sys_stat(output))
		goto free;
	
	Make_Info("Make", NULL, "spawn_cutscene_table.toml");
	Toml_Load(&toml, input);
	
	var_t count = Toml_ArrCount(&toml, "spawn_cutscene_entry");
	
	Memfile_Write(&mem, txt[0], strlen(txt[0]));
	
	for (var_t i = 0; i < count; i++) {
		const char* v;
		const char* requiredVar[] = {
			"spawn_cutscene_entry[%d].scene_id",
			"spawn_cutscene_entry[%d].spawn_id",
			"spawn_cutscene_entry[%d].age",
			"spawn_cutscene_entry[%d].flag_type",
			"spawn_cutscene_entry[%d].flag_id",
		};
		bool useHeader = false;
		
		Memfile_Fmt(&mem, "{\n");
		
		/* ERROR CHECK */ {
			for (var_t j = 0; j < ArrCount(requiredVar); j++)
				if (!Toml_Var(&toml, requiredVar[j], i))
					errr(gLang.make.err_sct_missing_var, i, requiredVar[j] + strlen("spawn_cutscene_entry[%d]."));
			
			if (!Toml_Var(&toml, "spawn_cutscene_entry[%d].segment", i) &&
				!Toml_Var(&toml, "spawn_cutscene_entry[%d].header", i))
				errr(gLang.make.err_sct_missing_sh, i);
		}
		
		if ((v = Toml_Var(&toml, "spawn_cutscene_entry[%d].header", i))) {
			useHeader = true;
			Memfile_Fmt(&mem, "        .header = %s,\n", v);
		} else
			Memfile_Fmt(&mem, "        .nextIsSegment = true,\n", v);
		
		v = Toml_Var(&toml, "spawn_cutscene_entry[%d].flag_id", i);
		Memfile_Fmt(&mem, "        .flag = %s,\n", v);
		
		v = Toml_Var(&toml, "spawn_cutscene_entry[%d].flag_type", i);
		if (!strcmp(v, "\"switch\""))
			Memfile_Fmt(&mem, "        .type = FLAG_SWITCH,\n");
		else if (!strcmp(v, "\"event_chk_inf\""))
			Memfile_Fmt(&mem, "        .type = FLAG_EVENTCHKINF,\n");
		else if (!strcmp(v, "\"chest\""))
			Memfile_Fmt(&mem, "        .type = FLAG_CHEST,\n");
		else if (!strcmp(v, "\"collectible\""))
			Memfile_Fmt(&mem, "        .type = FLAG_COLLECTIBLE,\n");
		else
			errr(gLang.make.err_sct_variable, i, "flag_type", v);
		
		v = Toml_Var(&toml, "spawn_cutscene_entry[%d].age", i);
		if (!strcmp(v, "\"adult\""))
			Memfile_Fmt(&mem, "        .age = LINK_AGE_ADULT,\n");
		else if (!strcmp(v, "\"child\""))
			Memfile_Fmt(&mem, "        .age = LINK_AGE_CHILD,\n");
		else if (!strcmp(v, "\"both\""))
			Memfile_Fmt(&mem, "        .age = LINK_AGE_BOTH,\n");
		else
			errr(gLang.make.err_sct_variable, i, "age", v);
		
		v = Toml_Var(&toml, "spawn_cutscene_entry[%d].scene_id", i);
		Memfile_Fmt(&mem, "        .scene = %s,\n", v);
		
		v = Toml_Var(&toml, "spawn_cutscene_entry[%d].spawn_id", i);
		Memfile_Fmt(&mem, "        .spawn = %s,\n", v);
		
		Memfile_Fmt(&mem, "    }, ");
		
		if (!useHeader) {
			Memfile_Fmt(&mem, "{\n");
			v = Toml_Var(&toml, "spawn_cutscene_entry[%d].segment", i);
			Memfile_Fmt(&mem, "        .segment = (void*)%s,\n", v);
			Memfile_Fmt(&mem, "    }, ");
		}
	}
	
	Memfile_Write(&mem, txt[1], strlen(txt[1]));
	
	Memfile_SaveStr(&mem, output);
	sys_touch(dep);
	
	free:
	Memfile_Free(&mem);
	Toml_Free(&toml);
}

// # # # # # # # # # # # # # # # # # # # #
// # Make_Object                         #
// # # # # # # # # # # # # # # # # # # # #

static ThreadFunc Object_Convert(const char* path) {
	Memfile mem = Memfile_New();
	bool isPlayas = false;
	
	fs_set(path);
	
	char* in = fs_find("*.objex");
	
	if (in == NULL)
		return;
	
	char* cfg = fs_item("config.toml");
	
	if (!sys_stat(cfg)) {
		FILE* m = FOPEN(cfg, "w");
		
		fprintf(m, "%-15s = %d\n", "segment", 6);
		fprintf(m, "%-15s = %d\n", "scale", 100);
		fclose(m);
	}
	
	char* out = x_rep(x_fmt("%sobject.zobj", x_path(in)), "src/", "rom/");
	char* header = x_fmt("include/object/%s.h", x_rep(x_pathslot(in, -1), "/", ""));
	char* linker = x_rep(header, ".h", ".ld");
	char* mnf = fs_find("*.mnf");
	char* sheader = x_rep(header, ".h", ".swap_h");
	
	if (!g64.makeForce
		&&
		(
			sys_stat(out) > sys_stat(in) &&
			sys_stat(out) > sys_stat(in) &&
			sys_stat(out) > sys_stat(cfg) &&
			sys_stat(out) >= sys_stat(header) &&
			sys_stat(out) >= sys_stat(linker)
		)
		&&
		(
			!mnf || (sys_stat(out) > sys_stat(mnf))
		))
		return;
	
	sys_mkdir(x_path(header));
	sys_mkdir(x_path(linker));
	sys_mkdir(x_path(out));
	
	Memfile_LoadStr(&mem, cfg);
	
	if (!Ini_Var(mem.str, "segment"))
		errr(gLang.make.err_missing_item, "segment", path);
	if (!Ini_Var(mem.str, "scale"))
		errr(gLang.make.err_missing_item, "scale", path);
	
	Hash hashNew = HashNew();
	Hash hashOld = HashNew();
	
	if (sys_stat(header)) {
		hashOld = HashFile(header);
		sys_mv(header, sheader);
	}
	
	Proc* exe = Proc_New(
		"%s "
		"--silent "
		"--in %s "
		"--out %s "
		"--address 0x%08X "
		"--scale %g "
		"--header %s "
		"--linker %s"
		,
		Tools_Get(z64convert),
		in,
		out,
		(Ini_GetInt(&mem, "segment") << 24),
		Ini_GetFloat(&mem, "scale"),
		header,
		linker
	);
	
	if (strstr(mem.str, "[playas]")) {
		isPlayas = true;
		Proc_AddArg(exe, "--playas");
	}
	
	Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE);
	Proc_Exec(exe);
	char* msg = Proc_Read(exe, READ_STDERR);
	Proc_Join(exe);
	
	if (hashOld.hashed) {
		hashNew = HashFile(header);
		
		if (!HashCmp(&hashNew, &hashOld)) {
			sys_rm(header);
			sys_mv(sheader, header);
		} else
			sys_rm(sheader);
	}
	
	Make_Info("gcc", msg, out);
	
	if (isPlayas) {
		Ini_GotoTab("playas");
		char* header = Ini_GetStr(&mem, "header");
		char* patch = Ini_GetStr(&mem, "patch");
		char* script = Ini_GetStr(&mem, "script");
		char* bank = Ini_GetStr(&mem, "bank");
		char* source = x_fmt("%splayas_output.zobj", x_path(x_rep(out, "rom/", "src/")));
		
		sys_cp(out, source);
		
		exe = Proc_New(
			"%s "
			"--no-wait "
			"--silence "
			"--i %s "
			"--o %s "
			"--b %s "
			"--s %s "
			"--p %s "
			"--h %s "
			,
			Tools_Get(z64playas),
			source,
			out,
			fs_item(bank),
			fs_item(script),
			fs_item(patch),
			fs_item(header)
		);
		
		Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE);
		Proc_Exec(exe);
		char* msg = Proc_Read(exe, READ_STDERR);
		Proc_Join(exe);
		
		Make_Info("gcc", msg, out);
	}
	
	Memfile_Free(&mem);
}

void Make_Object(void) {
	List list = List_New();
	MakeArg* arg = NULL;
	
	if (!sys_stat("src/object/"))
		return;
	
	List_Walk(&list, "src/object/", 0, LIST_FOLDERS | LIST_NO_DOT);
	
	if (list.num == 0)
		goto free;
	
	arg = calloc(sizeof(MakeArg) * list.num);
	
	for (int i = 0; i < list.num; i++)
		Parallel_Add(Object_Convert, list.item[i]);
	Parallel_Exec(g64.threadNum);
	
	Memfile mem = Memfile_New();
	Memfile newObjLinker = Memfile_New();
	List ldFiles = List_New();
	Hash hashOld = {};
	Hash hashNew = {};
	
	Memfile_Alloc(&mem, MbToBin(2));
	Memfile_Alloc(&newObjLinker, MbToBin(2));
	
	if (sys_stat(gLinkerFile_Object)) {
		Memfile_LoadStr(&mem, gLinkerFile_Object);
		hashOld = HashMem(mem.data, mem.size);
		Memfile_Null(&mem);
	}
	
	List_SetFilters(&ldFiles, CONTAIN_END, ".ld");
	sys_mkdir("include/object/");
	List_Walk(&ldFiles, "include/object/", 0, LIST_FILES | LIST_NO_DOT);
	
	for (int j = 0; j < ldFiles.num; j++) {
		Memfile_LoadStr(&mem, ldFiles.item[j]);
		Memfile_Append(&newObjLinker, &mem);
		Memfile_Fmt(&newObjLinker, "\n");
	}
	
	hashNew = HashMem(newObjLinker.data, newObjLinker.size);
	
	if (hashOld.hashed && HashCmp(&hashOld, &hashNew)) {
		LinkerInfo_RegisterDiff(newObjLinker.data, gLinkerFile_Object);
		Memfile_SaveStr(&newObjLinker, gLinkerFile_Object);
	} else
		Memfile_SaveStr(&newObjLinker, gLinkerFile_Object);
	
	List_Free(&ldFiles);
	Memfile_Free(&newObjLinker);
	Memfile_Free(&mem);
	
	free:
	if (!sys_stat(gLinkerFile_Object)) {
		FILE* f = FOPEN(gLinkerFile_Object, "w");
		fclose(f);
	}
	
	delete(arg);
	List_Free(&list);
}

// # # # # # # # # # # # # # # # # # # # #
// # Make_Sound                          #
// # # # # # # # # # # # # # # # # # # # #

static ThreadFunc Sequence_Convert(const char* file) {
	Memfile* mem = new(Memfile);
	u32 index = shex(x_pathslot(file, -1));
	u8 masterVolume = 88;
	char* outCfg;
	char* inCfg;
	char* output = NULL;
	char* srcFile = NULL;
	bool flStudio = false;
	bool loop = true;
	
	fs_set(file);
	
	srcFile = fs_find(".mid");
	if (!srcFile) srcFile = fs_find(".mus");
	if (!srcFile) goto free;
	
	inCfg = fs_item("config.toml");
	outCfg = x_rep(fs_item("config.toml"), "src/", "rom/");
	output = x_rep(fs_item("sequence.aseq"), "src/", "rom/");
	
	if (
		!g64.makeForce &&
		sys_stat(inCfg) &&
		sys_stat(output) > sys_stat(srcFile) &&
		sys_stat(outCfg) > sys_stat(inCfg)
	)
		goto free;
	
	sys_mkdir(x_path(output));
	
	// Copy config from vanilla directory if it doesn't exist!
	if (!sys_stat(inCfg)) {
		List van = List_New();
		List_Walk(&van, x_fmt("rom/sound/sequence/%s/", g64.vanilla), 0, LIST_FOLDERS);
		
		if (sys_stat(van.item[index]))
			sys_cp(x_fmt("%sconfig.toml", van.item[index]), inCfg);
		
		List_Free(&van);
	}
	
	Memfile_LoadStr(mem, inCfg);
	
	if (strend(srcFile, ".mid")) {
		if (strstr(mem->str, "[seq64]")) {
			Ini_GotoTab("seq64");
			if (Ini_Var(mem->str, "master_volume"))
				masterVolume = Ini_GetInt(mem, "master_volume");
			
			if (Ini_Var(mem->str, "flstudio"))
				flStudio = Ini_GetBool(mem, "flstudio");
			
			if (Ini_Var(mem->str, "loop"))
				loop = Ini_GetBool(mem, "loop");
			Ini_GotoTab("NULL");
		} else {
			Memfile_Seek(mem, MEMFILE_SEEK_END);
			
			Ini_WriteTab(mem, "seq64", NO_COMMENT);
			Ini_Fmt(mem, "\t"); Ini_WriteInt(mem, "master_volume", masterVolume, NO_COMMENT);
			Ini_Fmt(mem, "\t"); Ini_WriteBool(mem, "flstudio", flStudio, NO_COMMENT);
			Ini_Fmt(mem, "\t"); Ini_WriteBool(mem, "loop", loop, NO_COMMENT);
			
			Memfile_SaveStr(mem, inCfg);
		}
		
		if (Ini_GetError())
			errr_align(gLang.err_fail, inCfg);
		
		sys_cp(inCfg, outCfg);
		Memfile_Free(mem);
		
		Proc* exe = Proc_New(
			"%s "
			"--in=\"%s\" "
			"--out=\"%s\" "
			"--abi=Zelda "
			"--pref=false "
			"--mastervol=0x%X",
			Tools_Get(seq64),
			srcFile, output,
			masterVolume
		);
		
		if (flStudio)
			Proc_AddArg(exe, "--flstudio=true");
		if (!loop)
			Proc_AddArg(exe, "--smartloop=false");
		
		sys_mkdir(x_path(output));
		Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE);
		Proc_Exec(exe);
		char* msg = Proc_Read(exe, READ_STDERR);
		Proc_Join(exe);
		
		Make_Info("gcc", msg, output);
	} else if (strend(srcFile, ".mus")) {
		Proc* exe = Proc_New(
			"%s "
			"%s %s",
			Tools_Get(seqas),
			srcFile,
			output
		);
		
		sys_cp(inCfg, outCfg);
		Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE);
		Proc_Exec(exe);
		char* msg = Proc_Read(exe, READ_STDERR);
		Proc_Join(exe);
		
		Make_Info("gcc", msg, output);
	}
	
	free:
	Memfile_Free(mem);
	delete(mem);
}

void Make_Sequence(void) {
	List list = List_New();
	
	if (!sys_stat("src/sound/sequence/"))
		return;
	
	List_Walk(&list, "src/sound/sequence/", 0, LIST_FOLDERS | LIST_NO_DOT);
	
	if (list.num == 0)
		goto free;
	
	for (int i = 0; i < list.num; i++) {
		Parallel_Add(Sequence_Convert, list.item[i]);
	}
	
	Parallel_Exec(g64.threadNum);
	
	free:
	List_Free(&list);
}

static bool sSoundMade = false;

static ThreadFunc Sound_Convert(const char* path) {
	char* vadpcm = NULL;
	char* audio = NULL;
	char* book = NULL;
	char* table = NULL;
	char* vanillaCfg = NULL;
	Memfile cfgMem = Memfile_New();
	
	fs_set(path);
	
	char* config = fs_item("config.toml");
	
	audio = fs_find("*.wav");
	if (!audio)
		audio = fs_find("*.mp3");
	if (!audio)
		audio = fs_find("*aiff");
	
	if (audio == NULL)
		goto free;
	
	fs_set(x_rep(path, "src/", "rom/"));
	sys_mkdir(fs_item(""));
	
	vadpcm = fs_find("*.vadpcm.bin");
	book = fs_find("*.book.bin");
	vanillaCfg = x_rep(fs_item("config.toml"), "rom/sound/sample/", x_fmt("rom/sound/sample/%s/", g64.vanilla));
	
	// Generate Sample Config
	bool normalize = true;
	bool inherit = false;
	bool halfPrecision = false;
	
	if (!sys_stat(config)) {
		Memfile_Alloc(&cfgMem, 0x2000);
		write:
		Ini_WriteBool(&cfgMem, "normalize", normalize, "Maximum volume");
		Ini_WriteBool(&cfgMem, "inherit_vanilla", inherit, "If vanilla config exists, inherit pitch from it");
		Ini_WriteBool(&cfgMem, "half_precision", halfPrecision, "Rough compression");
		
		Memfile_SaveStr(&cfgMem, config);
	} else {
		u32 rewrite = false;
		Memfile_LoadStr(&cfgMem, config);
		
		if (Ini_Var(cfgMem.str, "normalize"))
			normalize = Ini_GetBool(&cfgMem, "normalize");
		else rewrite = true;
		
		if (Ini_Var(cfgMem.str, "inherit_vanilla"))
			inherit = Ini_GetBool(&cfgMem, "inherit_vanilla");
		else rewrite = true;
		
		if (Ini_Var(cfgMem.str, "half_precision"))
			halfPrecision = Ini_GetBool(&cfgMem, "half_precision");
		else rewrite = true;
		
		if (rewrite) {
			Memfile_Null(&cfgMem);
			Memfile_Realloc(&cfgMem, cfgMem.memSize * 4);
			goto write;
		}
	}
	
	if (vadpcm == NULL || (sys_stat(config) > sys_stat(vadpcm)) || (sys_stat(audio) > sys_stat(vadpcm)) || g64.makeForce) {
		if (vadpcm == NULL)
			vadpcm = x_fmt("%ssample.bin", x_rep(path, "src/", "rom/"));
		else
			strrep(vadpcm, ".vadpcm", "");
		
		osLog("Audio [%s] Vadpcm [%s]", audio, vadpcm);
		
		Proc* proc = Proc_New("%s -S", Tools_Get(z64audio));
		
		Proc_AddEach(proc, "--i", audio, "--o", vadpcm);
		
		if (table)
			Proc_AddEach(proc, "--design", table);
		else if (book)
			Proc_AddEach(proc, "--book", book);
		if (normalize)
			Proc_AddEach(proc, "--m", "--n");
		if (halfPrecision)
			Proc_AddEach(proc, "--half-precision");
		
		if (inherit && sys_stat(vanillaCfg)) {
			Toml toml = Toml_New();
			
			osLog("inherit [%s]", vanillaCfg);
			Toml_Load(&toml, vanillaCfg);
			
			Proc_AddEach(proc,
				"--basenote", Toml_Var(&toml, "basenote"),
				"--finetune", Toml_Var(&toml, "finetune"));
			Toml_Free(&toml);
		}
		
		Proc_Exec(proc);
		Proc_Join(proc);
		Make_Info("z64audio", NULL, audio);
		
		if (strstr(audio, "/sound/sfx/"))
			sSoundMade = true;
	}
	
	free:
	Memfile_Free(&cfgMem);
}

void Make_SfxOnly(void) {
	List listB = List_New();
	
	if (sys_stat("src/sound/sfx/"))
		List_Walk(&listB, "src/sound/sfx/", 0, LIST_FOLDERS | LIST_NO_DOT);
	
	for (int i = 0; i < listB.num; i++) {
		Parallel_Add(Sound_Convert, listB.item[i]);
	}
	
	if (listB.num)
		Parallel_Exec(g64.threadNum);
	
	List_Free(&listB);
}

void Make_Sound(void) {
	List listA = List_New();
	List listB = List_New();
	
	if (sys_stat("src/sound/sample/"))
		List_Walk(&listA, "src/sound/sample/", 0, LIST_FOLDERS | LIST_NO_DOT);
	if (sys_stat("src/sound/sfx/"))
		List_Walk(&listB, "src/sound/sfx/", 0, LIST_FOLDERS | LIST_NO_DOT);
	
	for (int i = 0; i < listA.num; i++) {
		Parallel_Add(Sound_Convert, listA.item[i]);
	}
	
	for (int i = 0; i < listB.num; i++) {
		Parallel_Add(Sound_Convert, listB.item[i]);
	}
	
	if (listA.num || listB.num)
		Parallel_Exec(g64.threadNum);
	
	List_Free(&listA);
	List_Free(&listB);
}

// # # # # # # # # # # # # # # # # # # # #
// # Make_Fast64                         #
// # # # # # # # # # # # # # # # # # # # #

static const char* Fast64_OutputFile(const char* input, const char* format) {
	char* output = strndup(x_path(input), strlen(input) * 2);
	
	strrep(output, "src/", "rom/");
	if (format) strcat(output, ".build/");
	
	if (!sys_stat(output))
		sys_mkdir(output);
	
	if (strstr(input, "scene.c")) {
		strcat(output, "scene");
		
		if (!format)
			strcat(output, ".zscene");
	} else {
		strcat(output, strstr(x_basename(input), "room_"));
		
		if (!format)
			strcat(output, ".zroom");
	}
	
	if (format)
		strcat(output, format);
	
	return output;
}

static u32 Fast64_EntryAddr(const char* input) {
	if (!strcmp(input, "scene"))
		return 0x02000000;
	
	return 0x03000000;
}

static const char* Fast64_EntryFile(const char* input) {
	char* type = strend(input, "scene.o") ? "scene" : "room";
	char* entry = x_fmt("%s.entry/%s/entry.ld", x_path(input), type);
	FILE* f = NULL;
	
	if (!sys_stat(entry)) {
		sys_mkdir(x_path(entry));
		osAssert( (f = fopen(entry, "w")) != NULL);
		fprintf(f, "ENTRY_POINT = 0x%08X;\n", Fast64_EntryAddr(type));
		fclose(f);
	}
	
	return entry;
}

static ThreadFunc Fast64_Compile(const char* input) {
	const char* object = Fast64_OutputFile(input, ".o");
	const char* elf = Fast64_OutputFile(input, ".elf");
	const char* output = Fast64_OutputFile(input, NULL);
	const char* entry = Fast64_EntryFile(object);
	char* msg = NULL;
	
	if (sys_stat(output) > sys_stat(input) && !g64.makeForce)
		goto end;
	
	Proc* exe = Proc_New(
		"%s %s "
		"-c -std=gnu99 -march=vr4300 -mabi=32 -mips3 -mno-explicit-relocs -mno-memcpy -mno-check-zero-division -fno-common -fno-toplevel-reorder -fno-zero-initialized-in-bss "
		"-Wno-missing-braces -Wno-builtin-declaration-mismatch "
		"-Iinclude/z64hdr -Iinclude/z64hdr/include -Iinclude/z64hdr/oot_mq_debug "
		"-o %s",
		Tools_Get(mips64_gcc), input,
		object
	);
	
	Proc_SetEnv(exe, Make_EnvironmentPath());
	Proc_SetEnv(exe, Make_TempPath());
	Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE_STDERR);
	Proc_Exec(exe);
	msg = Proc_Read(exe, READ_STDERR);
	Proc_Join(exe);
	Make_Info("gcc", msg, object);
	delete(msg);
	
	exe = Proc_New(
		"%s %s "
		"--unresolved-symbols=ignore-all %s -L%s %s "
		"-o %s",
		Tools_Get(mips64_ld), object,
		g64.linkerFlags.base, x_path(entry), g64.linkerFlags.scene,
		elf
	);
	Proc_SetEnv(exe, Make_EnvironmentPath());
	Proc_SetEnv(exe, Make_TempPath());
	Proc_SetState(exe, PROC_THROW_ERROR);
	Proc_Exec(exe);
	Proc_Join(exe);
	Make_Info("ld", msg, elf);
	
	exe = Proc_New("%s -R .MIPS.abiflags -O binary %s %s", Tools_Get(mips64_objcopy), elf, output);
	Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE);
	Proc_Exec(exe);
	msg = Proc_Read(exe, READ_STDOUT);
	Proc_Join(exe);
	Make_Info("gcc", msg, output);
	
	end:
	delete(msg, object, output, input, elf);
}

void Make_Fast64(void) {
	List paths = List_New();
	
	List_Walk(&paths, "src/scene/", 0, LIST_FOLDERS | LIST_NO_DOT);
	
	if (paths.num == 0)
		goto free;
	
	for (int i = 0; i < paths.num; i++) {
		List files = List_New();
		
		List_Walk(&files, paths.item[i], 0, LIST_FILES | LIST_NO_DOT);
		
		for (int j = 0; j < files.num; j++) {
			if (!strend(files.item[j], ".c"))
				continue;
			
			Parallel_Add(Fast64_Compile, strdup(files.item[j]));
		}
		
		List_Free(&files);
	}
	
	Parallel_Exec(g64.threadNum);
	
	free:
	List_Free(&paths);
}

// # # # # # # # # # # # # # # # # # # # #
// # Callback_Helper                     #
// # # # # # # # # # # # # # # # # # # # #

static char* Source_GetDepFile(const char* input) {
	if (!strend(input, ".c")) return NULL;
	char* dpfile = alloc(strlen(input) * 2);
	
	strcpy(dpfile, input);
	memcpy(dpfile, "rom/", 4);
	strcpy(strend(dpfile, ".c"), ".dep");
	
	return dpfile;
}

static char* Object_GetSourceFile(const char* str) {
	char* new = x_strndup(str, strlen(str) + 10);
	char* ext = striend(new, ".o");
	
	if (!ext) return NULL;
	
	memcpy(new, "src/", 4);
	memcpy(ext, ".c", 2);
	osLog("Stat %s", new);
	
	if (!sys_stat(new))
		memcpy(ext, ".s", 2);
	
	osLog("Stat %s", new);
	if (!sys_stat(new))
		return NULL;
	
	return new;
}

static s32 Source_StatDeps(const char* input, const char* output, const char* config) {
	const char* dep;
	Memfile mem = Memfile_New();
	bool r = false;
	
	if (sys_stat(config) > sys_stat(output))
		return true;
	
	if (g64.makeForce || gForceCodeMake)
		return true;
	
	if (!sys_stat(output) || (sys_stat(input) > sys_stat(output)))
		return true;
	
	if ((dep = Source_GetDepFile(input)) && sys_stat(dep)) {
		Memfile_LoadStr(&mem, dep);
		
		forline(line, mem.str) {
			if (sys_stat(x_cpyline(line, 0)) > sys_stat(output)) {
				r = true;
				break;
			}
		}
		
		Memfile_Free(&mem);
	}
	delete(dep);
	
	if (!r && sys_stat(config) && strend(config, "make.toml")) {
		Toml toml = Toml_New();
		char* basename = strdup(x_basename(output));
		
		Toml_Load(&toml, config);
		
		if (Toml_Var(&toml, "dep[0]")) {
			int num = Toml_ArrCount(&toml, "dep");
			
			for (int i = 0; i < num && !r; i++) {
				const char* f = Toml_GetStr(&toml, "dep[%d]", i);
				
				if (sys_stat(f) > sys_stat(output))
					r = true;
			}
		}
		
		if (Toml_Var(&toml, "%s.dep[0]", basename)) {
			int num = Toml_ArrCount(&toml, "%s.dep", basename);
			
			for (int i = 0; i < num && !r; i++) {
				const char* f = Toml_GetStr(&toml, "%s.dep[%d]", basename, i);
				
				if (sys_stat(f) > sys_stat(output))
					r = true;
			}
		}
		
		Toml_Free(&toml);
		delete(basename);
	}
	
	return r;
}

static s32 Object_StatDeps(const char* input, const char* output, const char* config) {
	List list = List_New();
	s32 ret = false;
	time_t max;
	
	List_Tokenize(&list, input, ' ');
	max = List_StatMax(&list);
	
	if (!sys_stat(output) || max > (sys_stat(output)))
		ret = true;
	else {
		if (!config)
			config = x_fmt("%sconfig.toml", x_path(output));
		
		if (!sys_stat(config))
			ret = true;
		
		if (!ret && gLinkerNum) {
			for (int i = 0; i < list.num; i++) {
				Elf64* elf = Elf64_Load(list.item[i]);
				
				for (int j = 0; j < gLinkerNum; j++) {
					if (Elf64_FindSym(elf, gLinkerInfo[j].symbol) == 0) {
						Elf64_Free(elf);
						List_Free(&list);
						
						return true;
					}
				}
				
				Elf64_Free(elf);
			}
		}
	}
	
	List_Free(&list);
	
	return ret;
}

// # # # # # # # # # # # # # # # # # # # #
// # Callback                            #
// # # # # # # # # # # # # # # # # # # # #

volatile f64 sTimeCallbackKaleido;
volatile f64 sTimeCallbackSystem;
volatile f64 sTimeCallbackOverlay;
volatile f64 sTimeCallbackCode;

static s32 Callback_Kaleido(const char* input, MakeCallType type, const char* output, void* arg) {
	char* ovl;
	char* conf;
	
	if (type == PRE_GCC) {
		char* config = arg;
		
		if (Source_StatDeps(input, output, config))
			return CB_MAKE;
		
		return CB_BREAK;
	}
	
	if (type == PRE_LD) {
		if (Object_StatDeps(input, output, NULL))
			return CB_MAKE;
		
		return CB_BREAK;
	}
	
	if (type == POST_LD) {
		time_start(1);
		char* info;
		
		ovl = x_fmt("%soverlay.zovl", x_path(input));
		strrep(ovl, "src/", "rom/");
		
		Proc* exe = Proc_New("%s -v -c -s -A 0x80800000 -o %s %s", Tools_Get(nOVL), ovl, input);
		Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE_STDOUT);
		Proc_Exec(exe);
		
		Elf64* elf = Elf64_Load(input);
		
		u32 sym[4];
		if (( sym[0] = Elf64_FindSym(elf, "__z64_init") ) == NULL_SYM) errr(gLang.make.err_missing_item, "__z64_init", input);
		if (( sym[1] = Elf64_FindSym(elf, "__z64_dest") ) == NULL_SYM) errr(gLang.make.err_missing_item, "__z64_dest", input);
		if (( sym[2] = Elf64_FindSym(elf, "__z64_updt") ) == NULL_SYM) errr(gLang.make.err_missing_item, "__z64_updt", input);
		if (( sym[3] = Elf64_FindSym(elf, "__z64_draw") ) == NULL_SYM) errr(gLang.make.err_missing_item, "__z64_draw", input);
		Elf64_Free(elf);
		
		conf = x_rep(x_fmt("%sconfig.toml", x_path(input)), "src/", "rom/");
		FILE* f = FOPEN(conf, "w");
		
		fprintf(f, "vram_addr = 0x%08X\n", 0x80800000);
		fprintf(f, "init      = 0x%08X\n", sym[0]);
		fprintf(f, "dest      = 0x%08X\n", sym[1]);
		fprintf(f, "updt      = 0x%08X\n", sym[2]);
		fprintf(f, "draw      = 0x%08X\n", sym[3]);
		fclose(f);
		
		info = x_pathslot(input, -1);
		strrep(info, "/", " ");
		
		Proc_Join(exe);
		sTimeCallbackKaleido += time_get(1);
		
		return 0;
	}
	
	return 0;
}

static s32 Callback_System(const char* input, MakeCallType type, const char* output, void* arg) {
	char* ovl = NULL;
	
	if (type == PRE_GCC) {
		char* config = arg;
		
		if (Source_StatDeps(input, output, config))
			return CB_MAKE;
		
		return CB_BREAK;
	}
	
	if (type == PRE_LD) {
		
		if (Object_StatDeps(input, output, NULL))
			return CB_MAKE;
		
		return CB_BREAK;
	}
	
	if (type == POST_LD) {
		time_start(1);
		char* info;
		FILE* f = FOPEN(x_fmt("%sconfig.toml", x_path(input)), "w");
		
		ovl = x_fmt("%soverlay.zovl", x_path(input));
		strrep(ovl, "src/", "rom/");
		
		Proc* exe = Proc_New("%s -v -c -s -A 0x80800000 -o %s %s", Tools_Get(nOVL), ovl, input);
		Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE_STDOUT);
		Proc_Exec(exe);
		
		info = x_pathslot(input, -1);
		if (!strncmp(info, "0x", 2))
			strrem(info, strlen("0x0000-"));
		strrep(info, "/", " ");
		
		Elf64* elf = Elf64_Load(input);
		
		u32 sym[2];
		if (( sym[0] = Elf64_FindSym(elf, "__z64_init") ) == NULL_SYM) errr(gLang.make.err_missing_item, "__z64_init", input);
		if (( sym[1] = Elf64_FindSym(elf, "__z64_dest") ) == NULL_SYM) errr(gLang.make.err_missing_item, "__z64_dest", input);
		Elf64_Free(elf);
		
		fprintf(f, "# %s\n\n", x_basename(input));
		fprintf(f, "vram_addr = 0x80800000\n");
		fprintf(f, "init_func = 0x%08X\n", sym[0]);
		fprintf(f, "dest_func = 0x%08X\n", sym[1]);
		fclose(f);
		
		Proc_Join(exe);
		
		sTimeCallbackSystem += time_get(0);
	}
	
	return 0;
}

static s32 Callback_Overlay(const char* input, MakeCallType type, const char* output, void* arg) {
	char* ovl;
	char* conf;
	
	if (type == PRE_GCC) {
		char* config = arg;
		
		if (Source_StatDeps(input, output, config))
			return CB_MAKE;
		
		return CB_BREAK;
	}
	
	if (type == PRE_LD) {
		if (Object_StatDeps(input, output, NULL))
			return CB_MAKE;
		
		return CB_BREAK;
	}
	
	if (type == POST_LD) {
		time_start(1);
		char* sourceFolder = x_path(input);
		Memfile mem = Memfile_New();
		List list = List_New();
		char* temp;
		char* varName = NULL;
		const char* constructType;
		
		if (strstr(input, "/actor/"))
			constructType = "ActorInit";
		else
			constructType = "EffectSsInit";
		
		ovl = x_fmt("%soverlay.zovl", x_path(input));
		strrep(ovl, "src/", "rom/");
		
		Proc* exe = Proc_New("%s -v -c -s -A 0x80800000 -o %s %s", Tools_Get(nOVL), ovl, input);
		Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE_STDOUT);
		Proc_Exec(exe);
		
		strrep(sourceFolder, "rom/", "src/");
		List_Walk(&list, sourceFolder, -1, LIST_FILES | LIST_NO_DOT);
		
		for (int i = 0; i < list.num; i++) {
			if (!striend(list.item[i], ".c"))
				continue;
			
			Memfile_LoadStr(&mem, list.item[i]);
			
			temp = strwstr(mem.str, constructType);
			if (temp) {
				temp += strlen(constructType);
				break;
			}
		}
		
		if (temp == NULL)
			errr(gLang.make.err_missing_item, constructType, sourceFolder);
		
		temp += strspn(temp, " ");
		temp = strndup(temp, strcspn(temp, " ="));
		
		Elf64* elf = Elf64_Load(output);
		u32 addr = Elf64_FindSym(elf, temp);
		Elf64_Free(elf);
		
		if (addr == NULL_SYM)
			errr(gLang.make.err_missing_item, temp, sourceFolder);
		
		conf = x_fmt("%sconfig.toml", x_path(input));
		strrep(conf, "src/", "rom/");
		
		FILE* cfg = FOPEN(conf, "w");
		fprintf(cfg, "# %s\n", x_basename(input));
		fprintf(cfg, "alloc_type = 0\n");
		fprintf(cfg, "vram_addr  = 0x80800000\n");
		fprintf(cfg, "init_vars  = 0x%08X\n", addr);
		fclose(cfg);
		
		delete(temp, varName);
		List_Free(&list);
		Memfile_Free(&mem);
		
		Proc_Join(exe);
		sTimeCallbackOverlay += time_get(1);
		
		return 0;
	}
	
	return 0;
}

static s32 Callback_Code(const char* input, MakeCallType type, const char* output, void* arg) {
	if (type == PRE_GCC) {
		char* config = arg;
		
		if (Source_StatDeps(input, output, config))
			return CB_MAKE;
		
		return CB_BREAK;
	}
	
	if (type == POST_GCC)
		return 0;
	
	if (type == PRE_LD) {
		time_start(1);
		const char* cnfname = rep(output, ".elf", ".toml");
		
		if (Object_StatDeps(input, output, cnfname)) {
			u32* entryPoint = arg;
			Memfile mem = Memfile_New();
			Memfile __config = Memfile_New();
			Memfile* config = &__config;
			char* c = Object_GetSourceFile(input);
			char* z64rom;
			char* z64ram;
			char* z64next;
			
			Memfile_LoadStr(&mem, c);
			z64ram = strstr(mem.str, "z64ram = ");
			z64rom = strstr(mem.str, "z64rom = ");
			z64next = strstr(mem.str, "z64next = ");
			
			if (z64ram == NULL) errr(gLang.make.err_missing_item, "z64ram", x_filename(c));
			if (z64rom == NULL) errr(gLang.make.err_missing_item, "z64rom", x_filename(c));
			
			z64ram += strlen("z64ram = ");
			z64rom += strlen("z64rom = ");
			if (z64next) z64next += strlen("z64next = ");
			
			Memfile_Alloc(config, 0x280);
			Ini_WriteHex(config, "rom", shex(z64rom), NO_COMMENT);
			Ini_WriteHex(config, "ram", shex(z64ram), NO_COMMENT);
			if (z64next) Ini_WriteHex(config, "next", shex(z64next), NO_COMMENT);
			entryPoint[0] = shex(z64ram);
			
			osLog(".elf to .toml: %s", output);
			Memfile_SaveStr(config, cnfname);
			
			Memfile_Free(&mem);
			Memfile_Free(config);
			delete(cnfname);
			
			return CB_MAKE;
		}
		
		delete(cnfname);
		sTimeCallbackCode += time_get(1);
		
		return CB_BREAK;
	}
	
	if (type == POST_LD) {
		char* bin;
		
		bin = x_alloc(strlen(input) + 8);
		strcpy(bin, input);
		strrep(bin, ".elf", ".bin");
		
		Proc* exe = Proc_New("%s -R .MIPS.abiflags -O binary %s %s", Tools_Get(mips64_objcopy), input, bin);
		Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE);
		Proc_Exec(exe);
		char* msg = Proc_Read(exe, READ_STDOUT);
		Proc_Join(exe);
		
		delete(msg);
	}
	
	return 0;
}

// # # # # # # # # # # # # # # # # # # # #
// # Binutil                             #
// # # # # # # # # # # # # # # # # # # # #

static void Binutil_Compiler_WriteDepFile(const char* source, const char* flags) {
	Proc* gcc;
	char* msg;
	char* dep;
	FILE* f;
	
	if ((dep = Source_GetDepFile(source)) == NULL)
		return;
	
	gcc = Proc_New("%s %s %s %s -M", Tools_Get(mips64_gcc), g64.gccFlags.main, flags, source);
	Proc_SetEnv(gcc, Make_EnvironmentPath());
	Proc_SetEnv(gcc, Make_TempPath());
	Proc_SetState(gcc, PROC_THROW_ERROR | PROC_MUTE_STDOUT);
	Proc_Exec(gcc);
	msg = Proc_Read(gcc, READ_STDOUT);
	
	strrep(msg, "\x0D\x0A", "\n");
	strrep(msg, " \\\n", "");
	strrep(msg, " ", "\n");
	char* msg_line = strline(msg, 2);
	memmove(msg, msg_line, strlen(msg_line) + 1);
	
	sys_mkdir(x_path(dep));
	if ((f = fopen(dep, "w")) == NULL)
		errr_align(gLang.err_create, dep);
	
	forline(line, msg) {
		if (strstart(line, "include/z64hdr/"))
			continue;
		fwrite(line, 1, linelen(line), f);
		fprintf(f, "\n");
	}
	
	fclose(f);
	
	Proc_Join(gcc);
	delete(msg, dep);
}

static void Binutil_Compiler(const char* source, const char* output, const char* flags, BinutilCallback callback) {
	s32 r = 0;
	char* config = x_fmt("%smake.toml", x_path(source));
	
	if (callback) {
		switch ((r = callback(source, PRE_GCC, output, config))) {
			case CB_BREAK:
				return;
			case CB_MAKE:
				break;
			default:
				errr_align(gLang.err_fail, "compiler %d", r);
		}
	} else {
		if (false == Source_StatDeps(source, output, config))
			return;
	}
	
	char* nflags = NULL;
	Proc* gcc = NULL;
	char* msg = NULL;
	
	sys_mkdir(output);
	
	if (sys_stat(config)) {
		Toml toml = Toml_New();
		const char* basename = x_basename(source);
		Toml_Load(&toml, config);
		
		if (Toml_Var(&toml, "gcc_flags")) {
			delete(nflags);
			nflags = Toml_GetStr(&toml, "gcc_flags");
			flags = x_fmt("%s %s", nflags, flags);
		}
		
		if (Toml_Var(&toml, "%s.gcc_flags", basename)) {
			delete(nflags);
			nflags = Toml_GetStr(&toml, "%s.gcc_flags", basename);
			flags = x_fmt("%s %s", nflags, flags);
		}
	}
	
	if (!nflags)
		flags = x_fmt("%s %s", g64.gccFlags.gcc, flags);
	else
		delete(nflags);
	
	Binutil_Compiler_WriteDepFile(source, flags);
	
	gcc = Proc_New("%s %s %s %s -o %s", Tools_Get(mips64_gcc), flags, g64.gccFlags.main, source, output);
	
	Proc_SetEnv(gcc, Make_EnvironmentPath());
	Proc_SetEnv(gcc, Make_TempPath());
	Proc_SetState(gcc, PROC_THROW_ERROR | PROC_MUTE_STDERR);
	Proc_Exec(gcc);
	msg = Proc_Read(gcc, READ_STDERR);
	Proc_Join(gcc);
	
	Make_Info("gcc", msg, output);
	
	if (callback)
		callback(output, POST_GCC, output, NULL);
}

static void Binutil_AsmCompiler(const char* source, const char* output, BinutilCallback callback) {
	s32 r = 0;
	
	if (callback) {
		switch ((r = callback(source, PRE_GCC, output, NULL))) {
			case CB_BREAK:
				return;
			case CB_MAKE:
				goto build;
			default:
				errr_align(gLang.err_fail, "asm %d", r);
		}
	} else {
		char* cfg = x_fmt("%smake.toml", x_path(source));
		
		if (false == Source_StatDeps(source, output, cfg))
			return;
	}
	
	build:
	(void)0;
	
	Proc* exe = Proc_New("%s -c -x assembler-with-cpp -Wa,--no-pad-sections %s -o %s %s %s", Tools_Get(mips64_gcc), source, output, x_rep(g64.linkerFlags.base, "--emit-relocs", ""), x_rep(g64.linkerFlags.code, "--emit-relocs", ""));
	
	sys_mkdir(output);
	Proc_SetEnv(exe, Make_EnvironmentPath());
	Proc_SetEnv(exe, Make_TempPath());
	Proc_SetState(exe, PROC_THROW_ERROR | PROC_MUTE_STDERR);
	Proc_Exec(exe);
	char* msg = Proc_Read(exe, READ_STDERR);
	Proc_Join(exe);
	
	Make_Info("gcc", msg, output);
	
	if (callback)
		callback(output, POST_GCC, output, NULL);
}

static inline bool Binutil_Linker_StatSource(const char* input, const char* output) {
	List list = List_New();
	bool del = false;
	u8* rm;
	u8* cmp;
	
	List_Tokenize2(&list, input, ' ');
	
	rm = calloc(sizeof(u8) * list.num);
	cmp = alloc(sizeof(u8) * list.num);
	
	memset(cmp, 0xFF, sizeof(u8) * list.num);
	
	for (int i = 0; i < list.num; i++) {
		if (!Object_GetSourceFile(list.item[i])) {
			if (!sys_rm(list.item[i]))
				info_volatile("" PRNT_REDD "R" PRNT_GRAY ": " PRNT_RSET "%s\n", list.item[i]);
			
			del = true;
			rm[i] = 0xFF;
		}
	}
	
	if (del) {
		info_volatile("" PRNT_REDD "R" PRNT_GRAY ": " PRNT_RSET "%s\n", output);
		
		if (!memcmp(rm, cmp, sizeof(u8) * list.num)) {
			if (
				strstr(output, "/actor/") ||
				strstr(output, "/effect/") ||
				strstr(output, "/system/")
			) {
				if (!sys_rmdir(x_path(list.item[0])))
					info_volatile("" PRNT_REDD "R" PRNT_GRAY ": " PRNT_RSET "%s\n", x_path(list.item[0]));
			}
			
		}
	}
	
	List_Free(&list);
	delete(cmp, rm);
	
	return !del;
}

static void Binutil_Linker(const char* source, const char* output, const char* flags, BinutilCallback callback) {
	Memfile entry = Memfile_New();
	char entryDir[1024] = { 0 };
	u32 entryPoint = 0x80800000;
	s32 r = 0;
	
	if (!Binutil_Linker_StatSource(source, output))
		return;
	
	if (callback) {
		switch ((r = callback(source, PRE_LD, (void*)output, &entryPoint))) {
			case CB_BREAK:
				return;
			case CB_MAKE:
				break;
			default:
				errr_align(gLang.err_fail, "linker %d", r);
		}
	} else if (!Object_StatDeps(source, output, NULL))
		return;
	
	strcpy(entryDir, x_path(output));
	strcat(entryDir, ".entry/");
	strcat(entryDir, x_basename(output));
	strcat(entryDir, "/entry.ld");
	sys_mkdir(x_path(entryDir));
	
	Memfile_Alloc(&entry, 0x20);
	Memfile_Fmt(&entry, "ENTRY_POINT = 0x%08X;\n", entryPoint);
	Memfile_SaveBin(&entry, entryDir);
	Memfile_Free(&entry);
	
	Proc* exe = Proc_New("%s -o %s %s -L%s %s %s", Tools_Get(mips64_ld), output, source, x_path(entryDir), g64.linkerFlags.base, flags);
	
	sys_mkdir(output);
	Proc_SetEnv(exe, Make_EnvironmentPath());
	Proc_SetEnv(exe, Make_TempPath());
	Proc_SetState(exe, PROC_THROW_ERROR);
	Proc_Exec(exe);
	Proc_Join(exe);
	
	Make_Info("ld", NULL, output);
	
	if (callback)
		callback(output, POST_LD, output, NULL);
}

// # # # # # # # # # # # # # # # # # # # #
// # Thread                              #
// # # # # # # # # # # # # # # # # # # # #

static void UserLibrary_ExtractSymbols(char* cmd, const char* output) {
	Memfile linker = Memfile_New();
	Hash crcA = {};
	Hash crcB = {};
	
	if (sys_stat(gLinkerFile_User)) {
		Memfile_LoadStr(&linker, gLinkerFile_User);
		crcA = HashMem(linker.data, linker.size);
		Memfile_Null(&linker);
	}
	
	Elf64* elf = Elf64_Load("rom/lib_user/z_lib_user.elf");
	List list = List_New();
	
	nested(void, call, (void* arg, const char* name, ElfSymbol * sym)) {
		if (sym->visibility == ELF64_VISIBILITY_GLOBAL) {
			switch (sym->type) {
				case ELF64_TYPE_VAR:
				case ELF64_TYPE_FUNC:
					if (sym->value >= 0x80700000 && sym->value < 0x80800000) {
						List_Add(arg, x_fmt("%08X%-32s = ", sym->value, name));
					}
					break;
			}
		}
	};
	
	Elf64_ReadSyms(elf, &list, (void*)call);
	Elf64_Free(elf);
	List_Sort(&list);
	
	for (int i = 0; i < list.num; i++) {
		osLog("%d: %s", i, list.item[i]);
		Memfile_Fmt(&linker, "%s0x%.8s;\n", list.item[i] + 8, list.item[i]);
	}
	List_Free(&list);
	
	if (crcA.hashed) {
		crcB = HashMem(linker.data, linker.size);
		
		if (HashCmp(&crcA, &crcB)) {
			LinkerInfo_RegisterDiff(linker.data, gLinkerFile_User);
			Memfile_SaveBin(&linker, gLinkerFile_User);
			
			sMakeUserLibrary = true;
		}
		
	} else {
		Memfile_SaveBin(&linker, gLinkerFile_User);
		sMakeUserLibrary = true;
	}
	
	Memfile_Free(&linker);
}

static void Make_Thread_UserLibrary(MakeArg* arg) {
	List list = List_New();
	const char* elf = "rom/lib_user/z_lib_user.elf";
	const char* bin = "rom/lib_user/z_lib_user.bin";
	const char* ld = gLinkerFile_User;
	char* inputList = NULL;
	char* command = NULL;
	u32 inputStrLen = 0;
	u32 breaker = true;
	const char* pp = x_rep(arg->path, "rom/", "src/");
	
	List_SetFilters(&list, CONTAIN_END, ".c");
	List_Walk(&list, pp, -1, LIST_FILES | LIST_NO_DOT);
	
	for (var_t i = 0; i < list.num; i++) {
		strrep(list.item[i], "src/", "rom/");
		strrep(list.item[i], ".c", ".o");
	}
	
	if (!sys_stat(bin) || !sys_stat(ld))
		breaker = false;
	
	if (List_StatMax(&list) > sys_stat(bin))
		breaker = false;
	
	if (breaker) {
		for (int i = 0; i < list.num; i++) {
			Elf64* elf = Elf64_Load(list.item[i]);
			
			for (int j = 0; j < gLinkerNum; j++) {
				if (Elf64_FindSym(elf, gLinkerInfo[j].symbol) == 0) {
					Elf64_Free(elf);
					
					breaker = false;
					goto bailout;
				}
			}
			
			Elf64_Free(elf);
		}
		
		bailout:
		(void)0;
	}
	
	if (breaker) {
		List_Free(&list);
		
		return;
	}
	
	for (int i = 0; i < list.num; i++)
		inputStrLen += strlen(list.item[i]) + 4;
	
	command = calloc(2048);
	inputList = calloc(inputStrLen);
	
	for (int i = 0; i < list.num; i++) {
		strcat(inputList, list.item[i]);
		strcat(inputList, " ");
	}
	
	if (true != false /* ELF */) {
		FILE* f = FOPEN("rom/lib_user/entry.ld", "w");
		
		fprintf(f, "ENTRY_POINT = 0x80700000;\n");
		fclose(f);
		
		Proc* p = Proc_New("%s -o %s %s -Lrom/lib_user/ "
#ifdef _WIN32
					"-Ltools/mips64-binutils/lib/gcc/mips64/9.1.0"
#else
					"-Ltools/mips64-binutils/lib/gcc/mips64/10.2.0"
#endif
				" %s %s",
				Tools_Get(mips64_ld), elf, inputList, g64.linkerFlags.base, arg->flag);
		Proc_SetEnv(p, Make_EnvironmentPath());
		Proc_SetEnv(p, Make_TempPath());
		Proc_SetState(p, PROC_THROW_ERROR);
		sys_mkdir(elf);
		Proc_Exec(p);
		Proc_Join(p);
		
		Make_Info("ld", NULL, elf);
	}
	if (true == true /* BIN */) {
		Tools_Command(command, mips64_objcopy, "-R .MIPS.abiflags -O binary %s %s", elf, bin);
		osLog("ObjCopy");
		Make_Run(command);
	}
	if (false == false /* mips64_ld */) {
		Tools_Command(command, nm, "-nr %s", elf);
		sys_mkdir(ld);
		UserLibrary_ExtractSymbols(command, ld);
	}
	
	delete(inputList, command);
	List_Free(&list);
	
	Make_Info("NONE", NULL, "" PRNT_BLUE "include/z_code_lib.ld");
}

static void Make_Thread_Compiler(MakeArg* arg) {
	char* output = NULL;
	char* input = arg->itemList->item[arg->i];
	
	if (strstr(input, " "))
		errr(gLang.make.err_whitespace, input);
	
	if (strend(input, ".c")) {
		output = strdup(input);
		strrep(output, ".c", ".o");
		strrep(output, "src/", "rom/");
		
		osLog("GCC: %s", input);
		Binutil_Compiler(input, output, arg->flag, arg->callback);
	} else if (strend(input, ".s")) {
		output = strdup(input);
		strrep(output, ".s", ".o");
		strrep(output, "src/", "rom/");
		
		osLog("GCC: %s", input);
		Binutil_AsmCompiler(input, output, arg->callback);
	}
	
	delete(output);
}

static void Make_Thread_Linker(MakeArg* arg) {
	List list = List_New();
	char* input = arg->itemList->item[arg->i];
	char* ninput = NULL;
	char* output = NULL;
	
	if (sys_isdir(input)) {
		if (strend(input, x_fmt("%s/", g64.vanilla)))
			return;
		
		List_SetFilters(&list, CONTAIN_END, ".o");
		List_Walk(&list, input, -1, LIST_FILES | LIST_NO_DOT);
		ninput = List_Concat(&list, " ");
		
		if (!ninput)
			goto free;
		
		input = ninput;
		
		output = fmt("%sfile.elf", arg->itemList->item[arg->i]);
		strrep(output, "src/", "rom/");
	} else {
		if (!strend(input, ".o"))
			goto free;
		
		output = calloc(strlen(input) + 10);
		strcpy(output, input);
		strrep(output, ".o", ".elf");
		strrep(output, "src/", "rom/");
	}
	
	osLog("LD: %s", input);
	Binutil_Linker(input, output, arg->flag, arg->callback);
	
	free:
	List_Free(&list);
	delete(output, ninput);
}

// # # # # # # # # # # # # # # # # # # # #
// # Initialize Thread                   #
// # # # # # # # # # # # # # # # # # # # #

static ThreadFunc Make_ThreadInit_File(MakeArg* arg) {
	MakeArg* passArg;
	List* itemList = new(List);
	
	if (!sys_stat(arg->path))
		return;
	
	List_SetFilters(itemList, CONTAIN_END, ".c", CONTAIN_END, ".s", CONTAIN_END, ".h", CONTAIN_END, ".o", CONTAIN_END, ".elf");
	List_Walk(itemList, arg->path, -1, LIST_FILES | LIST_NO_DOT);
	
	passArg = calloc(sizeof(*passArg) * itemList->num);
	FreeList_Que(passArg);
	FreeList_QueCall(List_Free, itemList);
	FreeList_Que(itemList);
	
	for (int i = 0; i < itemList->num; i++) {
		memcpy(&passArg[i], arg, sizeof(MakeArg));
		passArg[i].itemList = itemList;
		passArg[i].i = i;
		
		void* par = Parallel_Add(arg->func, &passArg[i]);
		Parallel_SetID(par, arg->i);
	}
}

static ThreadFunc Make_ThreadInit_Folder(MakeArg* arg) {
	MakeArg* passArg;
	List* itemList = new(List);
	
	if (!sys_stat(arg->path))
		return;
	
	List_Walk(itemList, arg->path, 0, LIST_FOLDERS | LIST_NO_DOT);
	
	passArg = calloc(sizeof(*passArg) * itemList->num);
	FreeList_Que(passArg);
	FreeList_QueCall(List_Free, itemList);
	FreeList_Que(itemList);
	
	for (int i = 0; i < itemList->num; i++) {
		memcpy(&passArg[i], arg, sizeof(MakeArg));
		passArg[i].itemList = itemList;
		passArg[i].i = i;
		
		void* par = Parallel_Add(arg->func, &passArg[i]);
		Parallel_SetID(par, arg->i);
	}
}

// # # # # # # # # # # # # # # # # # # # #
// # Make                                #
// # # # # # # # # # # # # # # # # # # # #

typedef struct FuncNode {
	struct FuncNode* next;
	char* func;
	char* src;
} FuncNode;

static FuncNode* sFnHead = NULL;

static void UserLib_Validate(void) {
	Memfile mem = Memfile_New();
	List list = List_New();
	bool error = false;
	bool same = false;
	
	List_Walk(&list, "rom/lib_user/", -1, LIST_FILES);
	
	for (int i = 0; i < list.num; i++) {
		if (!strend(list.item[i], ".o"))
			continue;
		
		nested(void, call, (void* arg, const char* name, ElfSymbol * sym)) {
			if (sym->visibility != ELF64_VISIBILITY_GLOBAL)
				return;
			if (sym->type != ELF64_TYPE_FUNC)
				return;
			if (strstart(name, "__vanilla_hook_"))
				return;
			FuncNode* node = new(FuncNode);
			node->func = strdup(name);
			node->src = strdup(arg);
			
			Node_Add(sFnHead, node);
		};
		
		Elf64* elf = Elf64_Load(list.item[i]);
		
		Elf64_ReadSyms(elf, list.item[i], (void*)call);
	}
	
	Memfile_LoadStr(&mem, gLinkerFile_User);
	
	for (FuncNode* node = sFnHead; node; node = node->next) {
		if (!strwstr(mem.str, node->func) && !strwstr(mem.str, x_fmt("__vanilla_hook_%s", node->func))) {
			if (!error)
				warn(gLang.make.err_hidden_symbol);
			
			error = true;
			
			if (!same) {
				info_nl();
				warn("" PRNT_REDD "%s", x_rep(x_rep(node->src, "rom/", "src/"), ".o", ".c"));
			}
			
			same = false;
			if (node->next)
				if (!strcmp(node->src, node->next->src))
					same = true;
			printf("" PRNT_RSET "Asm_VanillaHook(%s);\n", node->func);
		}
	}
	
	while (sFnHead) {
		delete(sFnHead->func, sFnHead->src);
		Node_Kill(sFnHead, sFnHead);
	}
	
	List_Free(&list);
	
	if (error)
		info_nl(),
		errr(gLang.press_enter);
	
	Memfile_Free(&mem);
}

void Make_Enum(void) {
	Memfile mem = Memfile_New();
	Hash hashOld = {};
	Hash hashNew = {};
	
	if (sys_stat("include/project_enum.h")) {
		Memfile_LoadBin(&mem, "include/project_enum.h");
		hashOld = HashMem(mem.data, mem.size);
		Memfile_Clear(&mem);
	}
	
	List actors = List_New();
	List effects = List_New();
	List objects = List_New();
	
	List_SetFilters(&actors, CONTAIN_START, "0x");
	List_SetFilters(&effects, CONTAIN_START, "0x");
	List_SetFilters(&objects, CONTAIN_START, "0x");
	
	List_Walk(&actors, "src/actor/", 0, LIST_FOLDERS | LIST_RELATIVE | LIST_NO_DOT);
	List_Walk(&effects, "src/effect/", 0, LIST_FOLDERS | LIST_RELATIVE | LIST_NO_DOT);
	List_Walk(&objects, "src/object/", 0, LIST_FOLDERS | LIST_RELATIVE | LIST_NO_DOT);
	
	List_Sort(&actors);
	List_Sort(&effects);
	List_Sort(&objects);
	
	Memfile_Fmt(&mem,
		"#ifndef PROJECT_ENUM_H\n"
		"#define PROJECT_ENUM_H\n\n"
		"enum {\n");
	
	for (int i = 0; i < actors.num; i++) {
		const char* name = actors.item[i] + strcspn(actors.item[i], "-") + 1;
		name = x_enumify(name);
		
		Memfile_Fmt(&mem, "\tZACTOR_%-16s = 0x%04X,\n", name, shex(actors.item[i]));
	}
	
	Memfile_Fmt(&mem,
		"\tZACTOR_MAX,\n"
		"};\n\n"
		"enum {\n");
	
	for (int i = 0; i < effects.num; i++) {
		const char* name = effects.item[i] + strcspn(effects.item[i], "-") + 1;
		name = x_enumify(name);
		
		Memfile_Fmt(&mem, "\tZEFFECT_%-16s = 0x%02X,\n", name, shex(effects.item[i]));
	}
	
	Memfile_Fmt(&mem,
		"\tZEFFECT_MAX,\n"
		"};\n\n"
		"enum {\n");
	
	for (int i = 0; i < objects.num; i++) {
		const char* name = objects.item[i] + strcspn(objects.item[i], "-") + 1;
		name = x_enumify(name);
		
		Memfile_Fmt(&mem, "\tZOBJ_%-16s = 0x%04X,\n", name, shex(objects.item[i]));
	}
	
	Memfile_Fmt(&mem,
		"\tZOBJ_MAX,\n"
		"};\n\n"
		"#endif\n");
	
	hashNew = HashMem(mem.data, mem.size);
	if (!hashOld.hashed || HashCmp(&hashNew, &hashOld))
		Memfile_SaveBin(&mem, "include/project_enum.h");
	
	Memfile_Free(&mem);
	List_Free(&actors);
	List_Free(&effects);
	List_Free(&objects);
}

void Make_ParticleHeader(void) {
	List list = List_New();
	Memfile mem = Memfile_New();
	Hash hashOld = {};
	
	if (!sys_stat("src/effect/"))
		return;
	
	if (sys_stat("include/project_vfx.h"))
		hashOld = HashFile("include/project_vfx.h");
	
	List_SetFilters(&list, CONTAIN_END, ".h");
	List_Walk(&list, "src/effect/", -1, LIST_FILES | LIST_RELATIVE | LIST_NO_DOT);
	
	Memfile_Fmt(&mem,
		"#ifndef PROJECT_EFFECT_H\n"
		"#define PROJECT_EFFECT_H\n"
		"#define EFFECT_INIT_ONLY\n"
		"\n"
		"/**\n"
		" * V i s u a l   E f f e c t s\n"
		" * \n"
		" * Only includes headers of vfx that has magic word:\n"
		" * \"z64vfx-magic-word\"\n"
		" * \n"
		" * Anything else will be considered vanilla replacement which\n"
		" * already have their own initializer struct and index defined.\n"
		" * \n"
		" * Include this header whenever you are planning to spawn your\n"
		" * custom vfx.\n"
		" * \n"
		" * void main() {\n"
		" *     VfxMyNewCloudInitParams params = {\n"
		" *         .vel = { 1, 0, 0},\n"
		" *         .accel = { 0, 1, 0},\n"
		" *     };\n"
		" *     \n"
		" *     Vfx_Spawn(play, VFX_MY_NEW_CLOUD, 128, &params);\n"
		" * }\n"
		"**/\n"
		"\n"
		"#include <global.h>\n"
		"#include <asm_macros.h>\n"
		"\n"
		"#define VFX_PRIORITY_DEFAULT 128\n"
		"\n"
		"void Vfx_Spawn(PlayState*, s32 index, s32 priority, void* initParams);\n"
		"Asm_SymbolAlias(\"Vfx_Spawn\", EffectSs_Spawn);\n"
		"\n");
	
	const char* enm = "";
	const char* inc = "";
	
	for (int i = 0; i < list.num; i++) {
		if (memcmp(list.item[i], "0x", 2))
			continue;
		
		Memfile header = Memfile_New();
		char* find;
		
		Memfile_LoadBin(&header, x_fmt("src/effect/%s", list.item[i]));
		find = strstr(header.str, "z64vfx-magic-word");
		Memfile_Free(&header);
		
		if (!find)
			continue;
		
		const char* enum_name = x_enumify(x_basename(list.item[i]));
		
		if (!memcmp(enum_name, "VFX_", 4))
			enum_name += 4;
		
		enm = x_fmt("%s\tVFX_%-8s = %s,\n", enm, enum_name, x_strndup(list.item[i], strcspn(list.item[i], " -/\\.,")));
		inc = x_fmt("%s#include \"src/effect/%s\"\n", inc, list.item[i]);
	}
	
	Memfile_Fmt(&mem,
		"enum {\n"
		"%s"
		"\tVFX_MAX,\n};\n\n"
		"%s"
		"\n"
		"#undef EFFECT_INIT_ONLY\n"
		"#endif\n",
		enm, inc
	);
	
	Hash hashNew = HashMem(mem.data, mem.size);
	if (!hashOld.hashed || HashCmp(&hashNew, &hashOld))
		Memfile_SaveBin(&mem, "include/project_vfx.h");
	
	Memfile_Free(&mem);
	List_Free(&list);
}

void Make_SfxEnum(void) {
	Memfile mem = Memfile_New();
	List list = List_New();
	Hash hashOld = {};
	Hash hashNew = {};
	
	if (sys_stat("src/lib_user/sfx_enum.h")) {
		gForceCodeMake = true;
		sys_rm("src/lib_user/sfx_enum.h");
	}
	
	if (sys_stat("include/sfx_enum.h")) {
		Memfile_LoadBin(&mem, "include/sfx_enum.h");
		hashOld = HashMem(mem.data, mem.size);
		Memfile_Clear(&mem);
	}
	
	Memfile_Fmt(&mem, "#ifndef Z64ROM_SFX_ENUM_H\n");
	Memfile_Fmt(&mem, "#define Z64ROM_SFX_ENUM_H\n\n");
	Memfile_Fmt(&mem, "typedef enum {\n");
	
	List_Walk(&list, "rom/sound/sfx/", 0, LIST_RELATIVE | LIST_FOLDERS);
	
	for (int i = 0; i < list.num; i++) {
		if (!sys_emptydir(x_fmt("rom/sound/sfx/%s", list.item[i])))
			Memfile_Fmt(&mem, "    SOUND_%s,\n", x_enumify(x_basename(x_strndup(list.item[i], strlen(list.item[i]) - 1))));
	}
	
	Memfile_Fmt(&mem, "    SOUND_MAX,\n");
	Memfile_Fmt(&mem, "} SoundFile;\n\n");
	Memfile_Fmt(&mem, "#endif\n");
	
	hashNew = HashMem(mem.data, mem.size);
	if (!hashOld.hashed || HashCmp(&hashNew, &hashOld))
		Memfile_SaveBin(&mem, "include/sfx_enum.h");
	
	Memfile_Free(&mem);
	List_Free(&list);
}

void Make_Code(void) {
	const struct {
		const char*     src;
		const char*     rom;
		const char*     gccFlag;
		const char*     ldFlag;
		BinutilCallback callback;
		s32 multiFileProcess;
	} param[] = {
		{
			.src = "src/lib_user/",
			.rom = "rom/lib_user/",
			.gccFlag = g64.gccFlags.code,
			.ldFlag = g64.linkerFlags.ulib,
			.callback = NULL,
			.multiFileProcess = false,
		},{
			.src = "src/lib_code/",
			.rom = "rom/lib_code/",
			.gccFlag = g64.gccFlags.code,
			.ldFlag = g64.linkerFlags.code,
			.callback = Callback_Code,
			.multiFileProcess = false,
		},{
			.src = "src/actor/",
			.rom = "rom/actor/",
			.gccFlag = g64.gccFlags.actor,
			.ldFlag = g64.linkerFlags.code,
			.callback = Callback_Overlay,
			.multiFileProcess = true,
		},{
			.src = "src/effect/",
			.rom = "rom/effect/",
			.gccFlag = g64.gccFlags.actor,
			.ldFlag = g64.linkerFlags.code,
			.callback = Callback_Overlay,
			.multiFileProcess = true,
		},{
			.src = "src/system/state/",
			.rom = "rom/system/state/",
			.gccFlag = g64.gccFlags.state,
			.ldFlag = g64.linkerFlags.code,
			.callback = Callback_System,
			.multiFileProcess = true,
		},{
			.src = "src/system/kaleido/",
			.rom = "rom/system/kaleido/",
			.gccFlag = g64.gccFlags.kaleido,
			.ldFlag = g64.linkerFlags.code,
			.callback = Callback_Kaleido,
			.multiFileProcess = true,
		}
	};
	MakeArg argsA[ArrCount(param)] = {};
	MakeArg argsB[ArrCount(param)] = {};
	thread_t thdA[ArrCount(param)] = {};
	s32 UID = ArrCount(param);
	
	Make_SpawnCutsceneTable();
	
	for (int set = 0; set < ArrCount(param); set++) {
		if (set == 0) {
			argsB[set].path = param[0].rom;
			argsB[set].flag = param[0].ldFlag;
			void* par = Parallel_Add(Make_Thread_UserLibrary, &argsB[set]);
			Parallel_SetID(par, UID + 1);
			Parallel_SetDepID(par, 0);
		}
		
		argsA[set].i = set;
		argsA[set].path = param[set].src;
		argsA[set].flag = param[set].gccFlag;
		argsA[set].func = Make_Thread_Compiler;
		argsA[set].callback = param[set].callback;
		
		thd_create(&thdA[set], Make_ThreadInit_File, &argsA[set]);
	}
	
	for (int set = 0; set < ArrCount(param); set++)
		thd_join(&thdA[set]);
	
	Parallel_Exec(g64.threadNum);
	FreeList_Free();
	
	memset(thdA, 0, sizeof(thdA));
	for (int set = 0; set < ArrCount(param); set++) {
		if (set != 0) {
			argsB[set].i = set;
			argsB[set].path = param[set].rom;
			argsB[set].func = Make_Thread_Linker;
			argsB[set].callback = param[set].callback;
			argsB[set].flag = param[set].ldFlag;
			
			if (param[set].multiFileProcess == true)
				thd_create(&thdA[set], Make_ThreadInit_Folder, &argsB[set]);
			
			else
				thd_create(&thdA[set], Make_ThreadInit_File, &argsB[set]);
		}
	}
	
	for (int set = 0; set < ArrCount(param); set++)
		thd_join(&thdA[set]);
	
	Parallel_Exec(g64.threadNum);
	
	FreeList_Free();
	
	if (g64.info) info(
			"Kaleido: %.4f\n"
			"System:  %.4f\n"
			"Overlay: %.4f\n"
			"Code:    %.4f",
			sTimeCallbackKaleido,
			sTimeCallbackSystem,
			sTimeCallbackOverlay,
			sTimeCallbackCode);
	
	if (sMakeUserLibrary)
		UserLib_Validate();
}

void Make_RegDefine(const char* str) {
	List list = List_New();
	
	if (str[0]) {
		List_Tokenize(&list, str, ' ');
		
		char* t = List_Concat(&list, " -D");
		g64.ccdefine = fmt("-D%s", t);
		delete(t);
	}
}

void Make(Rom* rom, s32 message) {
	if (sys_stat("tools/.make")) {
		sys_cp(gLinkerFile_TempObject, gLinkerFile_Object);
		sys_cp(gLinkerFile_TempUser, gLinkerFile_User);
	}
	sys_touch("tools/.make");
	
	if (g64.buildID == ROM_DEV) {
		const char* flag = g64.gccFlags.main;
		g64.gccFlags.main = fmt("%s -DDEV_BUILD", flag);
		delete(flag);
	}
	
	const char* flag = g64.gccFlags.main;
	g64.gccFlags.main = fmt("%s -I.", flag);
	delete(flag);
	
	if (g64.instant.use) {
		flag = g64.gccFlags.main;
		g64.gccFlags.main = fmt(
			"%s "
			"-DDEV_SCENE_INDEX=0x%X"
			" -DDEV_SPAWN_INDEX=0x%X"
			" -DDEV_HEADER_INDEX=0x%X"
			" -DDEV_SPAWN_AGE=0x%X",
			flag,
			g64.instant.scene,
			g64.instant.spawn,
			g64.instant.header,
			g64.instant.age);
		delete(flag);
	}
	
	flag = g64.gccFlags.main;
	g64.gccFlags.main = fmt("%s -D__z64rom__=1", flag);
	delete(flag);
	
	Make_Enum();
	Make_ParticleHeader();
	
	if (g64.makeTarget) {
		if (stristr(g64.makeTarget, "object")) {
			Make_Object();
		}
		if (stristr(g64.makeTarget, "audio")) {
			Make_Sound();
			Make_SfxEnum();
			Make_Sequence();
		}
		
		if (stristr(g64.makeTarget, "scene"))
			Make_Fast64();
		
		if (stristr(g64.makeTarget, "code"))
			Make_Code();
	} else {
		osLog("Make_Object();");
		Make_Object();
		
		osLog("Make_Sequence();");
		Make_Sequence();
		
		osLog("Make_Sound();");
		Make_Sound();
		Make_SfxEnum();
		
		osLog("Make_Fast64();");
		Make_Fast64();
		
		osLog("Make_Code();");
		Make_Code();
	}
	
	sys_rm("tools/.make");
	sys_cp(gLinkerFile_Object, gLinkerFile_TempObject);
	sys_cp(gLinkerFile_User, gLinkerFile_TempUser);
	
	LinkerInfo_Free();
	IO_FixWin32();
	
	if (sMake && message)
		info(gLang.make.info_make_ok,
			BinToKb(sys_statsize("rom/lib_user/z_lib_user.bin")),
			BinToKb(0xB5000));
}
