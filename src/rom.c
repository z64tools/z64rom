#include "z64rom.h"

static const char* sRestrEntryTbl[] = {
	"bottles",
	"a_button",
	"b_button",
	"unused",
	
	"warp_song",
	"ocarina",
	"hookshot",
	"trade_item",
	
	"other",
	"din_nayru",
	"farores",
	"sun_song",
};

typedef struct StructBE {
	/* 0x00 */ u8  code;
	/* 0x01 */ u8  data1;
	/* 0x04 */ u32 data2;
} SceneCommand;

typedef struct StructBE {
	u32 vrom_start;
	u32 vrom_end;
} SceneRoomList;

int Scene_GetHeaderNum(void* segment) {
	SceneCommand* cmd = segment;
	u32 offset_header = 0;
	u32* offset_list = NULL;
	int offset_num = 0;
	int hdr_num = 1;
	
	for (;; cmd++) {
		if (cmd->code == 0x14)
			break;
		
		if (cmd->code == 0x18) {
			offset_header = cmd->data2;
			
			continue;
		}
		
		switch (cmd->code) {
			case 0x00 ... 0x04:
			case 0x06:
			case 0x0A ... 0x0F:
			case 0x13:
			case 0x17 ... 0x18:
				renew(offset_list, u32[offset_num + 1]);
				offset_list[offset_num] = cmd->data2;
				offset_num++;
				break;
				
			default:
				break;
		}
	}
	
	if (offset_header == 0)
		goto clean_up;
	
	osAssert(offset_list != NULL);
	qsort(offset_list, offset_num, sizeof(u32), qsort_u32);
	
	for (int i = 0; i < offset_num; i++) {
		if (offset_list[i] > offset_header) {
			hdr_num = (offset_list[i] - offset_header) / sizeof(u32) + 1;
			
			break;
		}
	}
	
	clean_up:
	delete(offset_list);
	
	return hdr_num;
}

static int Scene_GetRoomNum(void* segment) {
	SceneCommand* cmd = segment;
	
	for (; cmd->code != 0x14; cmd++)
		if (cmd->code == 0x4)
			return cmd->data1;
	
	return 0;
}

static SceneRoomList* Scene_GetRoomList(void* segment, int index) {
	SceneCommand* cmd = segment;
	SceneCommand* hdr = segment;
	SceneRoomList* room_list = NULL;
	
	SegmentSet(1, segment);
	
	if (index > 0) {
		u32* hdr_list = NULL;
		
		for (; cmd->code != 0x14; cmd++)
			if (cmd->code == 0x18)
				hdr_list = SegmentToVirtual(1, cmd->data2 & 0xFFFFFF);
		
		osAssert(hdr_list != NULL);
		
		hdr = SegmentToVirtual(1, ReadBE(hdr_list[index - 1]) & 0xFFFFFF);
	}
	
	for (; hdr->code != 0x14; hdr++) {
		if (hdr->code == 0x04) {
			room_list = SegmentToVirtual(1, hdr->data2 & 0xFFFFFF);
			break;
		}
	}
	
	return room_list;
}

static u32 Overlay_GetBssSize(Memfile* dataFile) {
	u32* bssSize;
	
	mutex_lock(&gSegmentMutex);
	SegmentSet(0x1, dataFile->data);
	bssSize = SegmentToVirtual(0x1, dataFile->size - 4);
	bssSize = SegmentToVirtual(0x1, dataFile->size - ReadBE(bssSize[0]));
	mutex_unlock(&gSegmentMutex);
	
	return ReadBE(bssSize[3]);
}

static RestrictEntry* Restriction_FindTable(Rom* rom, u32 sceneIndex) {
	RestrictEntry* flagList = rom->table.restrictionFlags;
	
	while (flagList->sceneIndex != 0xFF) {
		if (flagList->sceneIndex == sceneIndex)
			return flagList;
		
		flagList++;
	}
	
	return NULL;
}

static RestrictEntry* Restriction_NewTable(Rom* rom) {
	static s32 sFirstEntry;
	RestrictEntry* rf = rom->table.restrictionFlags;
	
	if (sFirstEntry) {
		while (rf->sceneIndex != 0xFF)
			rf++;
	} else
		memset(rf, 0, sizeof(RestrictEntry) * (256 + 1));
	
	sFirstEntry = true;
	
	return rf;
}

static void Restriction_DumpFlags(Rom* rom, SceneEntry* entry, u32 id) {
	RestrictEntry* restr = Restriction_FindTable(rom, id);
	Toml toml = Toml_New();
	
	Toml_SetVar(&toml, "draw_func_index", "%d", ReadBE(entry->config));
	for (var_t i = 0; i < ArrCount(sRestrEntryTbl); i++)
		Toml_SetVar(&toml, x_fmt("enables.%s", sRestrEntryTbl[i]), "true");
	if (restr)
		for (var_t i = 0; i < ArrCount(sRestrEntryTbl); i++)
			if (bitfield_get(restr, 8 + i * 2, 2))
				Toml_SetVar(&toml, x_fmt("enables.%s", sRestrEntryTbl[i]), "false");
	Toml_Save(&toml, fs_item("config.toml"));
	Toml_Free(&toml);
}

static void Restriction_WriteFlags(Rom* rom, Toml* toml, u32 id) {
	RestrictEntry* rf = Restriction_NewTable(rom);
	
	*rf = (RestrictEntry) { .sceneIndex = id };
	
	for (var_t i = 0; i < ArrCount(sRestrEntryTbl); i++) {
		int v = Toml_GetBool(toml, "enables.%s", sRestrEntryTbl[i]);
		
		v ^= true;
		
		bitfield_set(rf, (0xFF * v), (8 + i * 2), 2);
	}
	
	// Next is NULL entry
	rf[1].sceneIndex = 0xFF;
}

static void ExtensionTable_Init(Rom* rom) {
	Memfile ulib = Memfile_New();
	char* fname = "src/lib_user/uLib.h";
	char* word;
	
	if (Memfile_LoadStr(&ulib, fname))
		return;
	
	struct {
		const char* name;
		u32* ext;
	} item[] = {
		{ "EXT_DMA_MAX",    &rom->ext.dmaNum    },
		{ "EXT_ACTOR_MAX",  &rom->ext.actorNum  },
		{ "EXT_OBJECT_MAX", &rom->ext.objectNum },
		{ "EXT_SCENE_MAX",  &rom->ext.sceneNum  },
		{ "EXT_EFFECT_MAX", &rom->ext.effectNum },
	};
	
	for (int i = 0; i < ArrCount(item); i++) {
		if (!(word = strstr(ulib.str, item[i].name)))
			errr(gLang.rom.err_ext_tbl, fname, item[i].name);
		*item[i].ext = sint(strword(word, 1));
	}
	
	osLog("DmaNum: %d", rom->ext.dmaNum);
	osLog("ActorNum: %d", rom->ext.actorNum);
	osLog("ObjectNum: %d", rom->ext.objectNum);
	osLog("SceneNum: %d", rom->ext.sceneNum);
	osLog("EffectNum: %d", rom->ext.effectNum);
	
	Memfile_Free(&ulib);
	
	Dma_RegEntry(rom, DMA_ID_DMADATA, 0x10); Dma_WriteFlag(DMA_ID_DMADATA, false);
}

static void ExtensionTable_Alloc(Rom* rom) {
	#define Rom_MoveTable(OFFSET, O_TABLE, NUM, NEW_NUM) \
		OFFSET = rom->offset.table.dmaTable + size; \
		table = SegmentToVirtual(0, OFFSET); \
		memcpy(table, O_TABLE, sizeof(*O_TABLE) * NUM); \
		O_TABLE = table; \
		NUM = NEW_NUM; \
		size += sizeof(*O_TABLE) * NEW_NUM; \
		size = alignvar(size, 16);
	
	u32 size = 0;
	void* table;
	
	if (rom->ext.dmaNum == 0)
		return;
	
	size += sizeof(struct DmaEntry) * rom->ext.dmaNum;
	size = alignvar(size, 16);
	
	gDma.highest = rom->ext.dmaNum;
	
	Rom_MoveTable(rom->offset.table.actorTable, rom->table.actor, rom->table.num.actor, rom->ext.actorNum);
	Rom_MoveTable(rom->offset.table.objTable, rom->table.object, rom->table.num.obj, rom->ext.objectNum);
	Rom_MoveTable(rom->offset.table.sceneTable, rom->table.scene, rom->table.num.scene, rom->ext.sceneNum);
	Rom_MoveTable(rom->offset.table.effectTable, rom->table.effect, rom->table.num.effect, rom->ext.effectNum);
	
	if (rom->offset.table.dmaTable != Dma_AllocEntry(rom, 2, size))
		errr(gLang.rom.err_ext_alloc,
			rom->ext.actorNum, rom->ext.effectNum,
			rom->ext.sceneNum, rom->ext.objectNum,
			rom->ext.dmaNum);
	
	for (int i = rom->table.num.dma; i < rom->ext.dmaNum; i++) {
		if (i == rom->ext.dmaNum - 1)
			rom->table.dma[i] = (DmaEntry) { 0 };
		
		else
			rom->table.dma[i] = (DmaEntry) { -1, -1, -1, -1 };
	}
}

// # # # # # # # # # # # # # # # # # # # #
// # CONFIG                              #
// # # # # # # # # # # # # # # # # # # # #

static void Config_WriteActor(Memfile* config, ActorEntry* actorOvl, const char* name, char* out) {
	Memfile_Null(config);
	Ini_WriteComment(config, name);
	Ini_WriteHex(config, "vram_addr", ReadBE(actorOvl->vramStart), NO_COMMENT);
	Ini_WriteHex(config, "init_vars", ReadBE(actorOvl->initInfo), NO_COMMENT);
	Ini_WriteInt(config, "alloc_type", ReadBE(actorOvl->allocType), NO_COMMENT);
	Memfile_SaveStr(config, out);
}

static void Config_WriteEffect(Memfile* config, EffectEntry* actorOvl, const char* name, char* out) {
	Memfile_Null(config);
	Ini_WriteComment(config, name);
	Ini_WriteHex(config, "vram_addr", ReadBE(actorOvl->vramStart), NO_COMMENT);
	Ini_WriteHex(config, "init_vars", ReadBE(actorOvl->initInfo), NO_COMMENT);
	Memfile_SaveStr(config, out);
}

static void Config_WriteState(Memfile* config, GameStateEntry* stateOvl, const char* name, char* out) {
	Memfile_Null(config);
	Ini_WriteComment(config, name);
	Ini_WriteHex(config, "vram_addr", ReadBE(stateOvl->vramStart), NO_COMMENT);
	Ini_WriteHex(config, "init_func", ReadBE(stateOvl->init), NO_COMMENT);
	Ini_WriteHex(config, "dest_func", ReadBE(stateOvl->destroy), NO_COMMENT);
	Memfile_SaveStr(config, out);
}

static void Config_WriteKaleido(Rom* rom, Memfile* config, u32 id, const char* name, char* out) {
	KaleidoEntry* entry = &rom->table.kaleido[id];
	u16* dataHi;
	u16* dataLo;
	u32 init;
	u32 dest;
	u32 updt;
	u32 draw;
	
	Memfile_Null(config);
	Ini_WriteComment(config, name);
	Ini_WriteHex(config, "vram_addr", ReadBE(entry->vramStart), NO_COMMENT);
	
	if (id == 1) { // PLAYER
		dataLo = SegmentToVirtual(0x0, rom->offset.table.player.init.lo);
		dataHi = SegmentToVirtual(0x0, rom->offset.table.player.init.hi);
		init = (ReadBE(dataHi[1]) - ((s16)ReadBE(dataLo[1]) < 0)) << 16 | ReadBE(dataLo[1]);
		
		dataLo = SegmentToVirtual(0x0, rom->offset.table.player.dest.lo);
		dataHi = SegmentToVirtual(0x0, rom->offset.table.player.dest.hi);
		dest = (ReadBE(dataHi[1]) - ((s16)ReadBE(dataLo[1]) < 0)) << 16 | ReadBE(dataLo[1]);
		
		dataLo = SegmentToVirtual(0x0, rom->offset.table.player.updt.lo);
		dataHi = SegmentToVirtual(0x0, rom->offset.table.player.updt.hi);
		updt = (ReadBE(dataHi[1]) - ((s16)ReadBE(dataLo[1]) < 0)) << 16 | ReadBE(dataLo[1]);
		
		dataLo = SegmentToVirtual(0x0, rom->offset.table.player.draw.lo);
		dataHi = SegmentToVirtual(0x0, rom->offset.table.player.draw.hi);
		draw = (ReadBE(dataHi[1]) - ((s16)ReadBE(dataLo[1]) < 0)) << 16 | ReadBE(dataLo[1]);
		
		Ini_WriteHex(config, "init", init, NO_COMMENT);
		Ini_WriteHex(config, "dest", dest, NO_COMMENT);
		Ini_WriteHex(config, "updt", updt, NO_COMMENT);
		Ini_WriteHex(config, "draw", draw, NO_COMMENT);
	} else { // PAUSE_MENU
		dataHi = SegmentToVirtual(0x0, rom->offset.table.pauseMenu.updt.hi);
		dataLo = SegmentToVirtual(0x0, rom->offset.table.pauseMenu.updt.lo);
		init = ReadBE(dataHi[1]) << 16 | ReadBE(dataLo[1]);
		
		dataHi = SegmentToVirtual(0x0, rom->offset.table.pauseMenu.draw.hi);
		dataLo = SegmentToVirtual(0x0, rom->offset.table.pauseMenu.draw.lo);
		updt = ReadBE(dataHi[1]) << 16 | ReadBE(dataLo[1]);
		
		Ini_WriteHex(config, "init", init, NO_COMMENT);
		Ini_WriteHex(config, "updt", updt, NO_COMMENT);
	}
	
	Memfile_SaveStr(config, out);
}

// # # # # # # # # # # # # # # # # # # # #
// # PATCH                               #
// # # # # # # # # # # # # # # # # # # # #

Patch gPatch;

static void Patch_Init() {
	List list = List_New();
	
	List_SetFilters(&list, CONTAIN_END, ".cfg");
	List_Walk(&list, "patch/", -1, LIST_FILES | LIST_NO_DOT);
	gPatch.config.file = calloc(sizeof(struct Memfile) * list.num);
	
	for (int i = 0; i < list.num; i++) {
		Memfile* mem = &gPatch.config.file[gPatch.config.num++];
		char* line;
		
		Memfile_LoadStr(mem, list.item[i]);
		Ini_ParseIncludes(mem);
		Memfile_Realloc(mem, mem->memSize * 8);
		osLog("Processed");
		
		// @macro
		line = mem->str;
		
		do {
			if (line == NULL)
				break;
			if (line[0] != '@')
				continue;
			
			char* cmd = x_cpyword(line, 0);
			
			strwrep(mem->str, cmd + 1, Ini_GetVar(line, cmd));
			line[0] = '#';
		} while ( (line = strline(line, 1)) );
		mem->size = strlen(mem->str);
	}
	
	List_Free(&list);
	List_Walk(&list, "rom/lib_code/", -1, LIST_FILES | LIST_NO_DOT);
	gPatch.bin.file = calloc(sizeof(struct Memfile) * list.num);
	gPatch.bin.offset = calloc(sizeof(u32) * list.num);
	
	for (int i = 0; i < list.num; i++) {
		if (striend(list.item[i], ".bin")) {
			u32* offset = &gPatch.bin.offset[gPatch.bin.num];
			Memfile* mem = &gPatch.bin.file[gPatch.bin.num++];
			Memfile cfg = Memfile_New();
			char* tname = x_rep(list.item[i], ".bin", ".toml");
			
			Memfile_LoadBin(mem, list.item[i]);
			Memfile_LoadStr(&cfg, tname);
			*offset = Ini_GetInt(&cfg, "rom");
			
			if (Ini_Var(cfg.str, "next") && Ini_GetInt(&cfg, "next") - Ini_GetInt(&cfg, "ram") < mem->size) {
				errr(gLang.patch.err_bin_space_limit,
					list.item[i],
					BinToKb(mem->size),  BinToKb(Ini_GetInt(&cfg, "next") - Ini_GetInt(&cfg, "ram")));
			}
			
			Memfile_Free(&cfg);
		}
	}
	
	List_Free(&list);
}

static void Patch_Free() {
	for (int i = 0; i < gPatch.config.num; i++)
		Memfile_Free(&gPatch.config.file[i]);
	
	for (int i = 0; i < gPatch.bin.num; i++)
		Memfile_Free(&gPatch.bin.file[i]);
	
	delete(gPatch.bin.file, gPatch.bin.offset, gPatch.config.file);
}

static void PatchFunc_Hex(PatchNode* node, const char* patchFile, Memfile* mem, const char* variable, u32 reloc) {
	u32 wp = 0;
	
	do {
		u8 val;
		u8* dst = &mem->cast.u8[mem->seekPoint + (s32)floorf((f32)wp / 2)];
		
		osLog("Var: %s", variable);
		if (!memcmp(variable, "0x", 2))
			continue;
		
		if (*variable == 'x' || *variable == ' ' || *variable == '\t' || *variable == ',')
			continue;
		
		if (*variable == '.') {
			wp++;
			continue;
		}
		
		val = shex(x_fmt("%c", *variable));
		
		if (wp % 2 == 0) {
			*dst &= ~0xF0;
			*dst |= val << 4;
		} else {
			*dst &= ~0x0F;
			*dst |= val & 0xF;
		}
		wp++;
		
	} while (*(++variable) != '\0');
	
	node->end = mem->seekPoint + wp + reloc;
}

static void PatchFunc_File(PatchNode* node, const char* patchFile, Memfile* mem, const char* variable, u32 reloc) {
	Memfile bmem;
	char* bin = strdup(variable);
	u32 addr = mem->seekPoint;
	
	strrep(bin, "FILE(\"", "");
	strrep(bin, "\")", "");
	
	fs_set(x_path(patchFile));
	
	if (!sys_stat(fs_item(bin)))
		errr(gLang.patch.err_missing_bin_file, patchFile, fs_item(bin));
	
	Memfile_LoadBin(&bmem, fs_item(bin));
	node->end = addr + bmem.size + reloc;
	Memfile_Append(mem, &bmem);
	Memfile_Free(&bmem);
}

static void PatchFunc_Texture(PatchNode* node, const char* patchFile, Memfile* mem, const char* variable, u32 reloc) {
	const struct {
		const char*  en;
		TexelFormat  fmt;
		TexelBitSize bs;
	} fmtMap[] = {
		{
			"I4",
			TEX_FMT_I,
			TEX_BSIZE_4,
		},{
			"I8",
			TEX_FMT_I,
			TEX_BSIZE_8,
		},{
			"I16",
			TEX_FMT_I,
			TEX_BSIZE_16
		},{
			"IA4",
			TEX_FMT_IA,
			TEX_BSIZE_4,
		},{
			"IA8",
			TEX_FMT_IA,
			TEX_BSIZE_8
		},{
			"IA16",
			TEX_FMT_IA,
			TEX_BSIZE_16,
		},{
			"CI4",
			TEX_FMT_CI,
			TEX_BSIZE_4,
		},{
			"CI8",
			TEX_FMT_CI,
			TEX_BSIZE_8
		},{
			"RGBA16",
			TEX_FMT_RGBA,
			TEX_BSIZE_16,
		},{
			"RGBA32",
			TEX_FMT_RGBA,
			TEX_BSIZE_32
		}
	};
	u32 fmtID;
	u32 addr = mem->seekPoint;
	char* strarg;
	Image* tex = NULL;
	List args = List_New();
	char* file;
	
	variable += strlen("TEXTURE(");
	strarg = strndup(variable, strcspn(variable, ")"));
	List_Tokenize2(&args, strarg, ',');
	
	if (args.num == 1)
		errr(gLang.patch.err_arg_fail, patchFile, "TEXTURE()", "TEXTURE('file', RGBA32)");
	
	fs_set(x_path(patchFile));
	file = fs_item(x_strunq(args.item[0]));
	
	for (fmtID = 0;; fmtID++) {
		if (fmtID == ArrCount(fmtMap))
			errr(gLang.patch.err_unk_texture_frmt, patchFile, args.item[1]);
		
		if (!strcmp(args.item[1], fmtMap[fmtID].en))
			break;
	}
	
	if (!sys_stat(file))
		errr(gLang.patch.err_missing_bin_file, patchFile, file);
	if (fmtMap[fmtID].fmt == TEX_FMT_CI)
		errr(gLang.patch.err_ci_not_supported, patchFile);
	
	tex = Texture_Load(file, fmtMap[fmtID].fmt, fmtMap[fmtID].bs, 0, 0);
	node->end = addr + tex->size + reloc;
	Memfile_Write(mem, tex->data, tex->size);
	
	List_Free(&args);
	Image_Free(tex);
	delete(strarg);
	delete(tex);
}

static void PatchFunc_String(PatchNode* node, const char* patchFile, Memfile* mem, const char* variable, u32 reloc) {
	u32 addr = mem->seekPoint;
	
	osLog("IsString");
	Memfile_Write(mem, variable, strlen(variable));
	node->end = addr + strlen(variable) + reloc;
}

static void PatchFunc_High32(PatchNode* node, const char* patchFile, Memfile* mem, const char* variable, u32 reloc) {
	u32 addr = mem->seekPoint;
	List args = List_New();
	char* strarg;
	u32 value;
	PointerCast cast = { &value };
	
	variable += strlen("HIGH32(");
	strarg = strndup(variable, strcspn(variable, ")"));
	List_Tokenize2(&args, strarg, ',');
	
	if (strend(args.item[0], "f") || strstr(args.item[0], "."))
		cast.f32[0] = sfloat(args.item[0]);
	else if (strstart(args.item[0], "0x"))
		cast.u32[0] = shex(args.item[0]);
	else if (vldt_int(args.item[0]))
		cast.u32[0] = sint(args.item[0]);
	else
		errr(gLang.patch.err_arg_fail, patchFile, x_fmt("HIGH32(%s)", args.item[0]), "HIGH32(value)");
	
	SwapBE(value);
	
	Memfile_Write(mem, cast.u8, 2);
	node->end = addr + strlen(variable) + reloc;
	
	List_Free(&args);
	delete(strarg);
}

static void PatchFunc_Low32(PatchNode* node, const char* patchFile, Memfile* mem, const char* variable, u32 reloc) {
	u32 addr = mem->seekPoint;
	List args;
	char* strarg;
	u32 value;
	PointerCast cast = { &value };
	
	variable += strlen("LOW32(");
	strarg = strndup(variable, strcspn(variable, ")"));
	List_Tokenize2(&args, strarg, ',');
	
	if (strend(args.item[0], "f") || strstr(args.item[0], "."))
		cast.f32[0] = sfloat(args.item[0]);
	else if (strstart(args.item[0], "0x"))
		cast.u32[0] = shex(args.item[0]);
	else if (vldt_int(args.item[0]))
		cast.u32[0] = sint(args.item[0]);
	else
		errr(gLang.patch.err_arg_fail, patchFile, x_fmt("LOW32(%s)", args.item[0]), "LOW32(value)");
	
	SwapBE(value);
	
	Memfile_Write(mem, cast.u8 + 2, 2);
	node->end = addr + strlen(variable) + reloc;
	
	List_Free(&args);
	delete(strarg);
}

static void PatchFunc_MemSet(PatchNode* node, const char* patchFile, Memfile* mem, const char* variable, u32 reloc) {
	u32 addr = mem->seekPoint;
	char* token;
	List args = List_New();
	u8* m;
	u32 size;
	
	variable += strlen("MEMSET(");
	token = strndup(variable, strcspn(variable, ")"));
	List_Tokenize2(&args, token, ',');
	
	size = sint(args.item[1]);
	m = alloc(size);
	memset(m, sint(args.item[0]), size);
	
	Memfile_Write(mem, m, size);
	node->end = addr + strlen(variable) + reloc;
	
	List_Free(&args);
	delete(token);
	delete(m);
}

static s32 Patch_File(Memfile* memDest, const char* dstFile) {
	PatchNode* nodeHead = NULL;
	u32 isPatched = false;
	char* section = NULL;
	bool willPatch = false;
	
	for (s32 p = 0; p < gPatch.config.num; p++) {
		List vlist = List_New();
		Memfile* cfg = &gPatch.config.file[p];
		u32 reloc = 0;
		
		osLog("%s: %d", __FUNCTION__, p);
		
		if (dstFile) {
			List sectionList = List_New();
			const char* checkType[] = {
				"rom/actor/",
				"rom/object/",
			};
			
			if (!section) {
				section = strdup(dstFile);
				strrep(section, x_fmt("%s/", g64.vanilla), "");
			}
			
			osLog("Section: %s", section);
			osLog("File: %s", cfg->info.name);
			
			if (!strstr(cfg->str, section))
				continue;
			
			osLog("Listing Sections", section);
			Ini_ListTabs(cfg, &sectionList);
			
			for (s32 type = 0; type < ArrCount(checkType); type++) {
				if (strstart(section, checkType[type])) {
					for (int i = 0; i < sectionList.num; i++) {
						if (!strncmp(section, sectionList.item[i], strlen(checkType[type]) + strlen("0x0000"))) {
							strcpy((char*)section, sectionList.item[i]);
							
							type = ArrCount(checkType);
							break;
						}
					}
				}
			}
			
			osLog("List free");
			List_Free(&sectionList);
			osLog("Done");
			
			if (!willPatch && g64.info)
				info_align(gLang.rom.info_patch_file, dstFile ? section : "ROM");
			
			willPatch = true;
			
			if (willPatch && !strstr(dstFile, g64.vanilla)) {
				const char* srcFile = x_rep(dstFile, "rom/", "src/");
				
				if (sys_stat(x_path(srcFile)))
					warn(gLang.patch.warn_patch_mod_file, dstFile);
			}
			
			osLog("Wow");
		}
		
		osLog("Section: %s", section);
		Ini_GotoTab(section);
		Ini_ListVars(cfg, &vlist, section);
		
		for (int i = 0; i < vlist.num; i++) {
			char* variable;
			char* temp;
			u64 addr;
			u32 isHex = true;
			
			if (!strcmp(vlist.item[i], "reloc_from")) {
				reloc = Ini_GetInt(cfg, vlist.item[i]);
				
				// Comment out already processed variables
				linehead(
					Ini_Var(cfg->str, vlist.item[i]),
					cfg->str
				)[0] = '#';
				
				continue;
			}
			
			if (!vldt_hex(vlist.item[i]))
				continue;
			
			addr = shex(vlist.item[i]) - reloc;
			variable = Ini_GetVar(cfg->str, vlist.item[i]);
			
			isPatched = 1;
			temp = Ini_Var(cfg->str, vlist.item[i]);
			
			if (
				temp[0] == '\"' ||
				strstart(temp, "FILE(") ||
				strstart(temp, "TEXTURE(") ||
				strstart(temp, "HIGH32(") ||
				strstart(temp, "LOW32(") ||
				strstart(temp, "MEMSET(")
			)
				isHex = false;
			
			// Comment out already processed variables
			osLog("Comment out");
			linehead(temp, cfg->str)[0] = '#';
			
			if (addr + strlen(variable) >= memDest->size || addr < 0)
				errr(gLang.patch.err_patch_space_limit, cfg->info.name, addr, section);
			
			PatchNode* node = new(struct PatchNode);
			
			Node_Add(nodeHead, node);
			node->start = addr + reloc;
			strncpy(node->source, cfg->info.name, 128);
			if (section)
				strncpy(node->section, section, 128);
			
			Memfile_Seek(memDest, addr);
			
			// if (g64.info)
			//     info("    %08X = %s", (u32)addr, variable);
			
			if (isHex)
				PatchFunc_Hex(node, cfg->info.name, memDest, variable, reloc);
			
			else {
				if (strstart(variable, "FILE("))
					PatchFunc_File(node, cfg->info.name, memDest, variable, reloc);
				
				else if (strstart(variable, "TEXTURE("))
					PatchFunc_Texture(node, cfg->info.name, memDest, variable, reloc);
				
				else if (strstart(variable, "HIGH32("))
					PatchFunc_High32(node, cfg->info.name, memDest, variable, reloc);
				
				else if (strstart(variable, "LOW32("))
					PatchFunc_Low32(node, cfg->info.name, memDest, variable, reloc);
				
				else if (strstart(variable, "MEMSET("))
					PatchFunc_MemSet(node, cfg->info.name, memDest, variable, reloc);
				
				else
					PatchFunc_String(node, cfg->info.name, memDest, variable, reloc);
			}
		}
		
		List_Free(&vlist);
	}
	
	osLog("Patch Done [%s]", dstFile);
	
	Ini_GotoTab(NULL);
	
	if (!section) goto exit;
	if (!strstr(section, "z_code.bin") && !strstr(section, "z_boot.bin"))
		goto exit;
	
	u32 reloc;
	u32 end;
	
	if (strstr(section, "z_boot.bin")) {
		reloc = RELOC_BOOT;
		end = 0x00012F70;
	} else {
		reloc = RELOC_CODE;
		end = 0x00BCEF30;
	}
	
	for (s32 p = 0; p < gPatch.bin.num; p++) {
		u32 offset = gPatch.bin.offset[p];
		
		if (offset >= reloc && offset <= end) {
			PatchNode* node = nodeHead;
			Memfile_Seek(memDest, offset - reloc);
			Memfile_Append(memDest, &gPatch.bin.file[p]);
			isPatched = 1;
			
			while (node) {
				if (Intersect(node->start, node->end, offset, offset + gPatch.bin.file[p].size))
					errr(gLang.patch.warn_bin_overwrite,
						node->source,
						strlen(node->section) ? x_fmt("%s.", node->section) : "",
						node->start, gPatch.bin.file[p].info.name
					);
				
				node = node->next;
			}
		}
	}
	
	exit:
	while (nodeHead)
		Node_Kill(nodeHead, nodeHead);
	delete(section);
	
	return isPatched;
}

// # # # # # # # # # # # # # # # # # # # #
// # Dump                                #
// # # # # # # # # # # # # # # # # # # # #

static size_t sBaseromSize;

typedef enum {
	/*  0 */ TRANS_TYPE_WIPE,
	/*  1 */ TRANS_TYPE_TRIFORCE,
	/*  2 */ TRANS_TYPE_FADE_BLACK,
	/*  3 */ TRANS_TYPE_FADE_WHITE,
	/*  4 */ TRANS_TYPE_FADE_BLACK_FAST,
	/*  5 */ TRANS_TYPE_FADE_WHITE_FAST,
	/*  6 */ TRANS_TYPE_FADE_BLACK_SLOW,
	/*  7 */ TRANS_TYPE_FADE_WHITE_SLOW,
	/*  8 */ TRANS_TYPE_WIPE_FAST,
	/*  9 */ TRANS_TYPE_FILL_WHITE2,
	/* 10 */ TRANS_TYPE_FILL_WHITE,
	/* 11 */ TRANS_TYPE_INSTANT,
	/* 12 */ TRANS_TYPE_FILL_BROWN,
	/* 13 */ TRANS_TYPE_FADE_WHITE_CS_DELAYED,
	/* 14 */ TRANS_TYPE_SANDSTORM_PERSIST,
	/* 15 */ TRANS_TYPE_SANDSTORM_END,
	/* 16 */ TRANS_TYPE_CS_BLACK_FILL,
	/* 17 */ TRANS_TYPE_FADE_WHITE_INSTANT,
	/* 18 */ TRANS_TYPE_FADE_GREEN,
	/* 19 */ TRANS_TYPE_FADE_BLUE,
	
	TRANS_TYPE_CIRCLE = 0x20,
	TRANS_TYPE_WARP   = 0x2C,
	
	TRANS_TYPE_MAX    = 56,
} TransitionType;

const char* sTransName[] = {
	/*  0 */ "Wipe",
	/*  1 */ "Triforce",
	/*  2 */ "FadeBlack",
	/*  3 */ "FadeWhite",
	/*  4 */ "FadeBlackFast",
	/*  5 */ "FadeWhiteFast",
	/*  6 */ "FadeBlackSlow",
	/*  7 */ "FadeWhiteSlow",
	/*  8 */ "WipeFast",
	/*  9 */ "FillWhite2",
	/* 10 */ "FillWhite",
	/* 11 */ "Instant",
	/* 12 */ "FillBrown",
	/* 13 */ "FadeWhiteCsDelayed",
	/* 14 */ "Sandstorm",
	/* 15 */ "SandstormEnd",
	/* 16 */ "FillBlack",
	/* 17 */ "FadeWhiteInstant",
	/* 18 */ "FadeGreen",
	/* 19 */ "FadeBlue",
	/* 20 */ "",
	/* 21 */ "",
	/* 22 */ "",
	/* 23 */ "",
	/* 24 */ "",
	/* 25 */ "",
	/* 26 */ "",
	/* 27 */ "",
	/* 28 */ "",
	/* 29 */ "",
	/* 30 */ "",
	/* 31 */ "",
	/* 32 */ "Circle",
	/* 33 */ "",
	/* 34 */ "",
	/* 35 */ "",
	/* 36 */ "",
	/* 37 */ "",
	/* 38 */ "",
	/* 39 */ "",
	/* 40 */ "",
	/* 41 */ "",
	/* 42 */ "",
	/* 43 */ "",
	/* 44 */ "Warp",
};

const char* Transition_GetName(s32 type) {
	if (type > TRANS_TYPE_CIRCLE)
		type = TRANS_TYPE_WARP;
	
	if (type > 19)
		type = TRANS_TYPE_CIRCLE;
	
	if (strlen(sTransName[type]) == 0)
		errr(gLang.rom.err_trans_id, type);
	
	return sTransName[type];
}

s32 Transition_GetType(const char* str) {
	TransitionType i = 0;
	
	for (;; i++) {
		if (!strcmp(str, sTransName[i]))
			return i;
		
		if (i >= TRANS_TYPE_MAX)
			errr(gLang.rom.err_trans_nm, str);
	}
	
	return 0;
}

static void Dump_Actor(Rom* rom, Memfile* data, Memfile* config) {
	RomFile rf;
	
	for (int i = 0; i < rom->table.num.actor; i++) {
		rf = Dma_RomFile_Actor(rom, i);
		
		if (rf.size == 0)
			continue;
		
		info_prog(gLang.rom.target[LANG_ACTOR], i + 1, rom->table.num.actor);
		fs_set("rom/actor/%s/0x%04X-%s/", g64.vanilla, i, gOoT_Actor[i]);
		
		if (Rom_Extract(data, rf, fs_item("overlay.zovl")))
			Config_WriteActor(config, &rom->table.actor[i], gOoT_Actor[i], fs_item("config.toml"));
	}
}

static void Dump_Effect(Rom* rom, Memfile* data, Memfile* config) {
	RomFile rf;
	
	for (int i = 0; i < rom->table.num.effect; i++) {
		rf = Dma_RomFile_Effect(rom, i);
		
		if (rf.size == 0)
			continue;
		
		info_prog(gLang.rom.target[LANG_EFFECT], i + 1, rom->table.num.effect);
		fs_set("rom/effect/%s/0x%04X-%s/", g64.vanilla, i, gOoT_Effect[i]);
		
		if (Rom_Extract(data, rf, fs_item("overlay.zovl")))
			Config_WriteEffect(config, &rom->table.effect[i], gOoT_Effect[i], fs_item("config.toml"));
	}
}

static void Dump_Object(Rom* rom, Memfile* data, Memfile* config) {
	RomFile rf;
	
	for (int i = 0; i < rom->table.num.obj; i++) {
		rf = Dma_RomFile_Object(rom, i);
		
		if (rf.size == 0)
			continue;
		
		info_prog(gLang.rom.target[LANG_OBJECT], i + 1, rom->table.num.obj);
		fs_set("rom/object/%s/0x%04X-%s/", g64.vanilla, i, gOoT_Object[i]);
		
		Rom_Extract(data, rf, fs_item("object.zobj"));
		
		if (i != 0x1)
			continue;
		
		SegmentSet(1, rf.data);
		PlayerAnimEntry* entry = (PlayerAnimEntry*)SegmentToVirtual(1, 0x2310);
		char* anim = SegmentToVirtual(0, 0x4E5C00);
		
		sys_mkdir("rom/system/animation/%s/", g64.vanilla);
		
		for (int j = 0; entry[j].__pad == 0; j++) {
			u32 segment = entry[j].segment & 0xFFFFFF;
			const char* file = gOoT_PlayerAnim[j];
			
			if (segment == 0)
				continue;
			
			rf.data = anim + segment;
			rf.size = sizeof(PlayerAnimFrame) * entry[j].frameCount;
			
			if (!file)
				file = "Anim";
			
			Rom_Extract(data, rf, x_fmt("rom/system/animation/%s/%d-%s.bin", g64.vanilla, j, file));
		}
	}
}

static void Dump_Scene(Rom* rom, Memfile* data, Memfile* config) {
	for (var_t i = 0; i < rom->table.num.scene; i++) {
		SceneEntry* entry = &rom->table.scene[i];
		RomFile rf = Dma_RomFile_Scene(rom, i);
		
		if (rf.size == 0) continue;
		
		info_prog(gLang.rom.target[LANG_SCENE], i + 1, rom->table.num.scene);
		fs_set("rom/scene/%s/0x%02X-%s/", g64.vanilla, i, gOoT_Scene[i]);
		
		if (Rom_Extract(data, rf, fs_item("scene.zscene"))) {
			u32 num = Scene_GetRoomNum(rf.data);
			SceneRoomList* room_list = Scene_GetRoomList(rf.data, 0);
			
			for (var_t i = 0; i < num; i++)
				Rom_Extract(data,
					Rom_GetRomFile(rom,
					ReadBE(room_list[i].vrom_start), ReadBE(room_list[i].vrom_end)),
					fs_item("room_%d.zroom", i));
			
			Restriction_DumpFlags(rom, entry, i);
			RomFile img = Rom_GetRomFile(rom, entry->titleVromStart, entry->titleVromEnd);
			
			if (img.romStart && img.size)
				Texture_Dump(&img, fs_item("title.png"), TEX_FMT_IA, TEX_BSIZE_8, 144, 24);
		}
		
	}
}

static void Dump_State(Rom* rom, Memfile* data, Memfile* config) {
	RomFile rf;
	
	for (int i = 0; i < rom->table.num.state; i++) {
		rf = Dma_RomFile_GameState(rom, i);
		
		if (rf.size == 0)
			continue;
		
		info_prog(gLang.rom.target[LANG_STATE], i + 1, rom->table.num.state);
		fs_set("rom/system/state/%s/0x%02X-%s/", g64.vanilla, i, gOoT_State[i]);
		
		if (Rom_Extract(data, rf, fs_item("overlay.zovl")))
			Config_WriteState(config, &rom->table.state[i], gOoT_State[i], fs_item("config.toml"));
	}
}

static void Dump_Kaleido(Rom* rom, Memfile* data, Memfile* config) {
	
	for (int i = 0; i < rom->table.num.kaleido; i++) {
		RomFile rf;
		fs_set("rom/system/kaleido/%s/0x%02X-%s/", g64.vanilla, i, gOoT_Kaleido[i]);
		
		info_prog(gLang.rom.target[LANG_KALEIDO], i + 1, rom->table.num.kaleido);
		
		rf = Rom_GetRomFile(rom, rom->table.kaleido[i].vromStart, rom->table.kaleido[i].vromEnd);
		Rom_Extract(data, rf, fs_item("overlay.zovl"));
		Config_WriteKaleido(rom, config, i, gOoT_Kaleido[i], fs_item("config.toml"));
	}
}

static void Dump_Skybox(Rom* rom, Memfile* data, Memfile* config) {
	RomFile rf;
	
	for (int i = 0; i < rom->table.num.skybox; i++) {
		info_prog(gLang.rom.target[LANG_SKYBOX], i + 1, rom->table.num.skybox);
		
		fs_set("rom/system/skybox/%s/0x%02X-%s/", g64.vanilla, i, gOoT_Skybox[i]);
		
		rf.romStart = ReadBE(rom->table.dma[941 + i * 2].vromStart);
		rf.romEnd = ReadBE(rom->table.dma[941 + i * 2].vromEnd);
		rf.size = rf.romEnd - rf.romStart;
		rf.data = SegmentToVirtual(0x0, rf.romStart);
		Rom_Extract(data, rf, fs_item("skybox.tex"));
		
		rf.romStart = ReadBE(rom->table.dma[942 + i * 2].vromStart);
		rf.romEnd = ReadBE(rom->table.dma[942 + i * 2].vromEnd);
		rf.size = rf.romEnd - rf.romStart;
		rf.data = SegmentToVirtual(0x0, rf.romStart);
		Rom_Extract(data, rf, fs_item("skybox.pal"));
	}
}

static void Dump_Static(Rom* rom, Memfile* data, Memfile* config) {
	RomFile rf;
	
	foreach(i, gOoT_Static) {
		u32 id = gOoT_Static[i].id;
		MessageTableEntry* tbl = NULL;
		
		info_prog(gLang.rom.target[LANG_STATIC], i + 1, ArrCount(gOoT_Static));
		fs_set("rom/system/static/%s/", g64.vanilla);
		
		rf.romStart = ReadBE(rom->table.dma[id].vromStart);
		rf.romEnd = ReadBE(rom->table.dma[id].vromEnd);
		rf.size = rf.romEnd - rf.romStart;
		rf.data = SegmentToVirtual(0x0, rf.romStart);
		
		switch (id) {
			case DMA_ID_LINK_ANIMATION:
				continue;
			case DMA_ID_DO_ACTION_STATIC:
				rf.size = 0x2B80;
				break;
			case DMA_ID_ITEM_NAME_STATIC:
				rf.size = 0x1EC00;
				break;
			case DMA_ID_MESSAGE_DATA_STATIC_NES:
				tbl = rom->table.nesMsg;
				break;
			case DMA_ID_MESSAGE_DATA_STATIC_STAFF:
				tbl = rom->table.staffMsg;
				break;
		}
		
		if (tbl) {
			RomFile tblRf = { 0 };
			
			tblRf.data = tbl;
			
			for (int i = 0;; i++) {
				tblRf.size += sizeof(MessageTableEntry);
				if (tbl[i].textId == 0xFFFF)
					break;
			}
			
			Rom_Extract(data, tblRf, fs_item(x_fmt("%s.tbl", gOoT_Static[i].name)));
		}
		
		Rom_Extract(data, rf, fs_item(x_fmt("%s.bin", gOoT_Static[i].name)));
	}
}

static void Dump_EntranceTable(Rom* rom, Memfile* memData, Memfile* memCfg) {
	EntranceInfo* ent = rom->table.entrance;
	
	Memfile_Null(memData);
	
	Memfile_Fmt(memData, "# Transition Types:\n");
	foreach(i, sTransName) {
		if (strlen(sTransName[i]))
			Memfile_Fmt(memData, "\t# %s\n", sTransName[i]);
	}
	Memfile_Fmt(memData, "\n");
	
	Memfile_Fmt(memData, "# Array Items: [ scene_id, spawn_id, continue_bgm, title_card, fade_in, fade_out ]\n\n");
	
	for (int i = 0; i < rom->table.num.entrance; i++, ent++) {
		List list = List_New();
		
		if (i == 0 || (ent->scene != ent[-1].scene || ent->spawn != ent[-1].spawn))
			Memfile_Fmt(memData, "# %s \n", gOoT_Scene[ent->scene]);
		
		List_Alloc(&list, 6);
		List_Add(&list, x_fmt("0x%02X", ent->scene));
		List_Add(&list, x_fmt("0x%02X", ent->spawn));
		List_Add(&list, x_fmt("%s", ent->continueBgm ? "true" : "false"));
		List_Add(&list, x_fmt("%s", ent->titleCard ? "true" : "false"));
		List_Add(&list, x_fmt("\"%s\"", Transition_GetName(ent->fadeIn)));
		List_Add(&list, x_fmt("\"%s\"", Transition_GetName(ent->fadeOut)));
		
		Ini_WriteArr(memData, x_fmt("0x%04X", i), &list, NO_QUOTES, NO_COMMENT);
		
		List_Free(&list);
	}
	
	if (!sys_stat("src/entrance_table.toml"))
		Memfile_SaveStr(memData, "src/entrance_table.toml");
	Memfile_SaveStr(memData, "src/vanilla.entrance_table.toml");
}

// # # # # # # # # # # # # # # # # # # # #
// # Build                               #
// # # # # # # # # # # # # # # # # # # # #

static bool sStatWarning;

static inline bool Build_StatFile(const char* file) {
	if (!sys_stat(file)) {
		warn_align(gLang.err_missing, "%s" PRNT_GRAY "%s", x_path(file), x_filename(file));
		sStatWarning = true;
		
		return true;
	}
	
	return false;
}

static inline void Rom_ProgressInfo(Rom* rom, const int lang, const int i, List* list) {
	int max = 0;
	
	switch (lang) {
		case LANG_ACTOR:
			max = rom->table.num.actor;
			break;
		case LANG_EFFECT:
			max = rom->table.num.effect;
			break;
		case LANG_OBJECT:
			max = rom->table.num.obj;
			break;
		case LANG_SCENE:
			max = rom->table.num.scene;
			break;
		case LANG_STATE:
			max = rom->table.num.state;
			break;
		case LANG_KALEIDO:
			max = rom->table.num.kaleido;
			break;
		case LANG_SKYBOX:
			max = rom->table.num.skybox;
			break;
		case LANG_STATIC:
			max = ArrCount(gOoT_Static);
			list = NULL;
			break;
		case LANG_DMA:
			max = 0x20 + 40;
			break;
	}
	
	osLog("%d", lang);
	osAssert(max != 0);
	
	if (i >= max)
		errr(gLang.rom.err_target_full, gLang.rom.target[lang], dighex(max), i, dighex(max), max);
	else
		info_prog(gLang.rom.target[lang], i + 1, list ? list->num : max);
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

typedef struct {
	Rom* rom;
	const char* item;
	ActorEntry* entry;
} ActorBuildInstance;

static mutex_t sDmaMutex;

static void Build_ActorThread(ActorBuildInstance* this) {
	if (this->item == NULL) {
		if (this->entry->vromStart && this->entry->vromEnd)
			*this->entry = (ActorEntry) { 0 };
		return;
	}
	
	fs_set(this->item);
	const char* file = fs_item("overlay.zovl");
	
	if (Build_StatFile(file)) {
		*this->entry = (ActorEntry) { 0 };
		return;
	}
	
	Memfile config = Memfile_New();
	Memfile data = Memfile_New();
	
	Memfile_LoadBin(&data, file);
	Memfile_LoadStr(&config, fs_item("config.toml"));
	
	this->entry->allocType = Ini_GetInt(&config, "alloc_type");
	this->entry->initInfo = Ini_GetInt(&config, "init_vars");
	this->entry->vramStart = Ini_GetInt(&config, "vram_addr");
	if (Ini_GetError()) errr_align(gLang.err_fail, "config");
	
	this->entry->vramEnd = this->entry->vramStart + data.size + Overlay_GetBssSize(&data);
	
	s32 p = Patch_File(&data, file);
	mutex_scope(sDmaMutex,
		this->entry->vromStart = Dma_WriteMemfile(this->rom, DMA_FIND_FREE, &data, p ? NOCACHE_COMPRESS : COMPRESS);
		this->entry->vromEnd = Dma_GetVRomEnd();
	);
	
	SwapBE(this->entry->vromStart);
	SwapBE(this->entry->vromEnd);
	SwapBE(this->entry->vramStart);
	SwapBE(this->entry->vramEnd);
	SwapBE(this->entry->allocType);
	SwapBE(this->entry->initInfo);
	this->entry->loadedRamAddr = 0;
	
	Memfile_Free(&config);
	Memfile_Free(&data);
}

static void Build_Actor(Rom* rom, Memfile* memData, Memfile* memCfg) {
	List list = List_New();
	ActorEntry* entry = rom->table.actor;
	
	Rom_ItemList(&list, "rom/actor/", SORT_NUMERICAL, LIST_FOLDERS);
	ActorBuildInstance* inst = new(ActorBuildInstance[list.num]);
	
	for (int i = 0; i < list.num; i++) {
		inst[i] = (ActorBuildInstance) {
			.rom = rom,
			.item = list.item[i],
			.entry = &entry[i],
		};
		
		Parallel_Add(Build_ActorThread, &inst[i]);
	}
	
	gParallel_ProgMsg = gLang.rom.target[LANG_ACTOR];
	Parallel_Exec(g64.threadNum);
	delete(inst);
	
	List_Free(&list);
}

static void Build_Effect(Rom* rom, Memfile* memData, Memfile* memCfg) {
	List list = List_New();
	EffectEntry* entry = rom->table.effect;
	const char* file;
	
	Rom_ItemList(&list, "rom/effect/", SORT_NUMERICAL, LIST_FOLDERS);
	
	for (int i = 0; i < list.num; i++) {
		
		if (list.item[i] == NULL) {
			entry[i] = (EffectEntry) { 0 };
			continue;
		}
		
		Rom_ProgressInfo(rom, LANG_EFFECT, i, &list);
		
		fs_set(list.item[i]);
		file = fs_item("overlay.zovl");
		
		if (Build_StatFile(file)) {
			entry[i] = (EffectEntry) { 0 };
			continue;
		}
		
		Memfile_LoadBin(memData, fs_item("overlay.zovl"));
		Memfile_LoadStr(memCfg, fs_item("config.toml"));
		
		entry[i].initInfo = Ini_GetInt(memCfg, "init_vars");
		entry[i].vramStart = Ini_GetInt(memCfg, "vram_addr");
		if (Ini_GetError()) errr(gLang.err_fail, "config");
		
		entry[i].vramEnd = entry[i].vramStart + memData->size + Overlay_GetBssSize(memData);
		
		s32 p = Patch_File(memData, memData->info.name);
		entry[i].vromStart = Dma_WriteMemfile(rom, DMA_FIND_FREE, memData, p ? NOCACHE_COMPRESS : COMPRESS);
		entry[i].vromEnd = Dma_GetVRomEnd();
		
		SwapBE(entry[i].initInfo);
		SwapBE(entry[i].vramStart);
		SwapBE(entry[i].vramEnd);
		SwapBE(entry[i].vromStart);
		SwapBE(entry[i].vromEnd);
		entry[i].loadedRamAddr = 0;
	}
	
	List_Free(&list);
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

typedef struct {
	Rom* rom;
	const char*  item;
	ObjectEntry* entry;
	bool is_keep;
} ObjectBuildInstance;

static void Build_ObjectThread(ObjectBuildInstance* this) {
	ObjectEntry* entry = this->entry;
	Rom* rom = this->rom;
	
	s32 compress = COMPRESS;
	
	entry->vromStart = 0;
	entry->vromEnd = 0;
	
	if (this->item == NULL)
		return;
	
	fs_set(this->item);
	const char* file = fs_item("object.zobj");
	
	if (Build_StatFile(file))
		return;
	
	Memfile data = Memfile_New();
	
	Memfile_LoadBin(&data, fs_item("object.zobj"));
	
	if (this->is_keep) {
		List animList = List_New();
		Memfile mem = Memfile_New();
		PlayerAnimEntry* animEntry;
		
		Rom_ItemList(&animList, "rom/system/animation/", SORT_NUMERICAL, LIST_FILES);
		SegmentSet(1, data.data);
		animEntry = (PlayerAnimEntry*)SegmentToVirtual(1, 0x2310);
		
		for (int j = 0; j < animList.num; j++) {
			
			if (animList.item[j] == NULL)
				continue;
			
			Memfile_LoadBin(&mem, animList.item[j]);
			
			animEntry[j].segment = rom->playerAnim.seekPoint | 0x07000000;
			animEntry[j].frameCount = mem.size / sizeof(PlayerAnimFrame);
			
			Memfile_Append(&rom->playerAnim, &mem);
			Memfile_Align(&rom->playerAnim, 0x2);
		}
		
		Memfile_Free(&mem);
		List_Free(&animList);
		compress = NOCACHE_COMPRESS;
	}
	
	if (Patch_File(&data, data.info.name))
		compress = NOCACHE_COMPRESS;
	
	mutex_scope(sDmaMutex,
		entry->vromStart = Dma_WriteMemfile(rom, DMA_FIND_FREE, &data, compress);
		entry->vromEnd = Dma_GetVRomEnd();
	);
	
	SwapBE(entry->vromStart);
	SwapBE(entry->vromEnd);
	
	Memfile_Free(&data);
}

static void Build_Object(Rom* rom, Memfile* memData, Memfile* memCfg) {
	List list = List_New();
	ObjectEntry* entry = rom->table.object;
	
	memset(entry, 0, sizeof(ObjectEntry[rom->table.num.obj]));
	
	Rom_ItemList(&list, "rom/object/", SORT_NUMERICAL, LIST_FOLDERS);
	ObjectBuildInstance* inst = new(ObjectBuildInstance[list.num]);
	
	if (list.num >= rom->ext.objectNum)
		errr(gLang.rom.err_target_full, gLang.rom.target[LANG_OBJECT], dighex(list.num), list.num, dighex(list.num), rom->ext.objectNum);
	
	for (int i = 0; i < list.num; i++) {
		inst[i] = (ObjectBuildInstance) {
			.rom = rom,
			.item = list.item[i],
			.entry = &entry[i],
			.is_keep = i == 1
		};
		
		Parallel_Add(Build_ObjectThread, &inst[i]);
	}
	
	gParallel_ProgMsg = gLang.rom.target[LANG_OBJECT];
	Parallel_Exec(g64.threadNum);
	delete(inst);
	
	List_Free(&list);
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

static void Build_LevelSelectTable(Rom* rom, Memfile* mem_data, List* list) {
	Memfile_Null(mem_data);
	
	for (int i = 0; i < list->num; i++) {
		if (list->item[i] == NULL)
			continue;
		
		char* str = x_rep(strstr(list->item[i] + strlen("rom/scene/"), "-") + 1, "/", "");
		u8 id = i;
		
		Memfile_Write(mem_data, &id, 1);
		Memfile_Write(mem_data, x_rep(str, "_", " "), strlen(str) + 1);
	}
	
	Memfile_Write(mem_data, "\xFF", 1);
	Memfile_Align(mem_data, 16);
	
	Dma_WriteMemfile(rom, DMA_ID_UNUSED_4, mem_data, NOCACHE_COMPRESS);
}

static void Build_TitleCard(Rom* rom, SceneEntry* entry_table, List* title_list, int* title_card_scene_index) {
	Hash* hash_list = new(Hash[title_list->num]);
	
	for (int i = 0; i < title_list->num; i++) {
		Image* texel = Texture_Load(title_list->item[i], TEX_FMT_IA, TEX_BSIZE_8, 144, 24);
		
		hash_list[i] = HashMem(texel->data, texel->size);
		
		for (int k = 0; k < i; k++) {
			if (!HashCmp(&hash_list[i], &hash_list[k])) {
				entry_table[title_card_scene_index[i]].titleVromStart = entry_table[title_card_scene_index[k]].titleVromStart;
				entry_table[title_card_scene_index[i]].titleVromEnd = entry_table[title_card_scene_index[k]].titleVromEnd;
				goto use_same;
			}
		}
		
		entry_table[title_card_scene_index[i]].titleVromStart = Dma_Write(rom, DMA_FIND_FREE, texel->data, texel->size, NULL, NOCACHE_COMPRESS);
		entry_table[title_card_scene_index[i]].titleVromEnd = Dma_GetVRomEnd();
		SwapBE(entry_table[title_card_scene_index[i]].titleVromStart);
		SwapBE(entry_table[title_card_scene_index[i]].titleVromEnd);
		
		use_same:
		Image_Free(texel);
		delete(texel);
	}
	
	delete(title_card_scene_index, hash_list);
}

typedef struct {
	Rom* rom;
	const char* item;
	SceneEntry* entry;
	int   scene_id;
	List* title_list;
	int*  title_card_scene_index;
} SceneBuildInstance;

static mutex_t sTitleMutex;
static mutex_t sRestrictionMutex;

static void Build_SceneThread(SceneBuildInstance* this) {
	Rom* rom = this->rom;
	SceneEntry* entry = this->entry;
	List* title_list = this->title_list;
	int* title_card_scene_index = this->title_card_scene_index;
	
	if (!this->item) {
		*entry = (SceneEntry) {};
		
		return;
	}
	
	fs_set(this->item);
	
	Toml toml = Toml_New();
	List room_list = List_New();
	const char* fscene = fs_item("scene.zscene");
	const char* fconfig = fs_item("config.toml");
	RomFile* room_dma;
	
	if (!sys_stat(fscene))
		fscene = fs_find("*.zscene");
	
	if (!fscene) {
		*entry = (SceneEntry) {};
		
		return;
	}
	
	if (sys_stat(fconfig)) {
		Toml_Load(&toml, fconfig);
		entry->config = Toml_GetInt(&toml, "draw_func_index");
		
		mutex_scope(sRestrictionMutex,
			Restriction_WriteFlags(rom, &toml, this->scene_id);
		);
		
		Toml_Free(&toml);
	} else
		entry->config = 4;
	
	const char* ftitle = fs_item("title.png");
	if (sys_stat(ftitle)) {
		mutex_scope(sTitleMutex,
			title_card_scene_index[title_list->num] = this->scene_id;
			List_Add(title_list, ftitle);
		);
	}
	
	List_SetFilters(&room_list, CONTAIN_END, ".zroom", CONTAIN_END, ".zmap");
	List_Walk(&room_list, this->item, 0, LIST_FILES | LIST_RELATIVE | LIST_NO_DOT);
	List_Sort(&room_list);
	room_dma = new(RomFile[room_list.num]);
	
	u32 num_room = 0;
	u32 cnum_room = 0;
	Memfile data = Memfile_New();
	const char* type = NULL;
	
	for (int j = 0; j < room_list.num; j++) {
		if (!strstart(room_list.item[j], "room_")) continue;
		
		if (type && strcmp(type, strfext(room_list.item[j]))) {
			warn_align(gLang.err_fail, x_path(room_list.item[j]));
			errr(gLang.rom.err_room_type_mismatch);
		}
		type = strfext(room_list.item[j]);
		
		Memfile_LoadBin(&data, fs_item(room_list.item[j]));
		
		mutex_scope(sDmaMutex,
			room_dma[num_room].romStart = Dma_WriteMemfile(rom, DMA_FIND_FREE, &data, COMPRESS);
			room_dma[num_room].romEnd = Dma_GetVRomEnd();
		);
		
		osLog("%s == %d", room_list.item[j], num_room);
		num_room++;
	}
	
	u32 num_header = 0;
	
	Memfile_LoadBin(&data, fscene);
	num_header = Scene_GetHeaderNum(data.data);
	cnum_room = Scene_GetRoomNum(data.data);
	
	osLog("scene:      %s", fscene);
	osLog("num_header: %d", num_header);
	osLog("cnum_room:  %d", cnum_room);
	osLog("num_room:   %d", num_room);
	
	if (cnum_room != num_room)
		osLog("mismatch");
	
	for (int j = 0; j < num_header; j++) {
		mutex_lock(&gSegmentMutex);
		SceneRoomList* scene_room_list = Scene_GetRoomList(data.data, j);
		
		osLog("scene_room_list: %08X", VirtualToSegment(1, scene_room_list));
		mutex_unlock(&gSegmentMutex);
		
		for (int k = 0; k < num_room; k++) {
			scene_room_list[k].vrom_start = room_dma[k].romStart;
			scene_room_list[k].vrom_end = room_dma[k].romEnd;
		}
	}
	
	mutex_scope(sDmaMutex,
		entry->vromStart = Dma_WriteMemfile(rom, DMA_FIND_FREE, &data, NOCACHE_COMPRESS);
		entry->vromEnd = Dma_GetVRomEnd();
	);
	
	SwapBE(entry->vromStart);
	SwapBE(entry->vromEnd);
	entry->titleVromStart = entry->titleVromEnd = 0;
	
	Memfile_Free(&data);
	List_Free(&room_list);
	delete(room_dma);
}

static void Build_Scene(Rom* rom, Memfile* mem_data, Memfile* mem_cfg) {
	List list = List_New();
	List title_list = List_New();
	SceneEntry* entry_table = rom->table.scene;
	int* title_card_scene_index;
	
	memset(entry_table, 0, sizeof(SceneEntry[rom->table.num.scene]));
	
	Rom_ItemList(&list, "rom/scene/", SORT_NUMERICAL, LIST_FOLDERS);
	
	List_Alloc(&title_list, list.num);
	title_card_scene_index = new(int[list.num]);
	for (int i = 0; i < list.num; i++) title_card_scene_index[i] = -1;
	
	SceneBuildInstance* inst = new(SceneBuildInstance[list.num]);
	
	for (int i = 0; i < list.num; i++) {
		inst[i] = (SceneBuildInstance) {
			.rom = rom,
			.item = list.item[i],
			.entry = &entry_table[i],
			.scene_id = i,
			.title_list = &title_list,
			.title_card_scene_index = title_card_scene_index
		};
		
		Parallel_Add(Build_SceneThread, &inst[i]);
	}
	
	mutex_init(&sTitleMutex);
	mutex_init(&sRestrictionMutex);
	
	gParallel_ProgMsg = gLang.rom.target[LANG_SCENE];
	Parallel_Exec(g64.threadNum);
	delete(inst);
	
	mutex_dest(&sTitleMutex);
	mutex_dest(&sRestrictionMutex);
	
	Build_TitleCard(rom, entry_table, &title_list, title_card_scene_index);
	Build_LevelSelectTable(rom, mem_data, &list);
	
	List_Free(&list);
	List_Free(&title_list);
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

static void Build_State(Rom* rom, Memfile* memData, Memfile* memCfg) {
	List list = List_New();
	GameStateEntry* entry = rom->table.state;
	
	Rom_ItemList(&list, "rom/system/state/", SORT_NUMERICAL, LIST_FOLDERS);
	
	for (int i = 0; i < list.num; i++) {
		char* file;
		s32 patched;
		Toml toml = Toml_New();
		
		Rom_ProgressInfo(rom, LANG_STATE, i, &list);
		
		if (list.item[i] == NULL)
			continue;
		
		fs_set(list.item[i]);
		file = fs_item("overlay.zovl");
		
		if (Build_StatFile(file))
			continue;
		
		Toml_Load(&toml, fs_item("config.toml"));
		Memfile_LoadBin(memData, file);
		patched = Patch_File(memData, file);
		
		entry[i].init = Toml_GetInt(&toml, "init_func");
		entry[i].destroy = Toml_GetInt(&toml, "dest_func");
		entry[i].vramStart = Toml_GetInt(&toml, "vram_addr");
		
		entry[i].vramEnd = entry[i].vramStart + memData->size + Overlay_GetBssSize(memData);
		entry[i].vromStart = Dma_WriteMemfile(rom, DMA_FIND_FREE, memData, patched ? NOCACHE_COMPRESS : COMPRESS);
		entry[i].vromEnd = Dma_GetVRomEnd();
		
		SwapBE(entry[i].init);
		SwapBE(entry[i].destroy);
		SwapBE(entry[i].vromStart);
		SwapBE(entry[i].vromEnd);
		SwapBE(entry[i].vramStart);
		SwapBE(entry[i].vramEnd);
		
		Toml_Free(&toml);
	}
	
	List_Free(&list);
}

static void Build_Kaleido(Rom* rom, Memfile* memData, Memfile* memCfg) {
	List list = List_New();
	KaleidoEntry* entry = rom->table.kaleido;
	RomOffset* romOff = &rom->offset;
	
	Rom_ItemList(&list, "rom/system/kaleido/", SORT_NUMERICAL, LIST_FOLDERS);
	
	for (int i = 0; i < list.num; i++) {
		char* file;
		
		Rom_ProgressInfo(rom, LANG_KALEIDO, i, &list);
		
		fs_set(list.item[i]);
		file = fs_item("overlay.zovl");
		
		if (Build_StatFile(file))
			continue;
		
		Memfile_LoadStr(memCfg, fs_item("config.toml"));
		Memfile_LoadBin(memData, file);
		Patch_File(memData, file);
		
		entry[i].vramStart = Ini_GetInt(memCfg, "vram_addr");
		entry[i].vramEnd = entry[i].vramStart + memData->size + Overlay_GetBssSize(memData);
		entry[i].vromStart = Dma_WriteMemfile(rom, DMA_FIND_FREE, memData, NOCACHE_COMPRESS);
		entry[i].vromEnd = Dma_GetVRomEnd();
		
		SwapBE(entry[i].vromStart);
		SwapBE(entry[i].vromEnd);
		SwapBE(entry[i].vramStart);
		SwapBE(entry[i].vramEnd);
		
		if (i == 1) {          // PLAYER
			Mips64_SplitLoad(
				SegmentToVirtual(SEG_CODE, romOff->table.player.init.hi - RELOC_CODE),
				SegmentToVirtual(SEG_CODE, romOff->table.player.init.lo - RELOC_CODE),
				MIPS_REG_A0,
				Ini_GetInt(memCfg, "init")
			);
			Mips64_SplitLoad(
				SegmentToVirtual(SEG_CODE, romOff->table.player.dest.hi - RELOC_CODE),
				SegmentToVirtual(SEG_CODE, romOff->table.player.dest.lo - RELOC_CODE),
				MIPS_REG_A0,
				Ini_GetInt(memCfg, "dest")
			);
			Mips64_SplitLoad(
				SegmentToVirtual(SEG_CODE, romOff->table.player.updt.hi - RELOC_CODE),
				SegmentToVirtual(SEG_CODE, romOff->table.player.updt.lo - RELOC_CODE),
				MIPS_REG_A0,
				Ini_GetInt(memCfg, "updt")
			);
			Mips64_SplitLoad(
				SegmentToVirtual(SEG_CODE, romOff->table.player.draw.hi - RELOC_CODE),
				SegmentToVirtual(SEG_CODE, romOff->table.player.draw.lo - RELOC_CODE),
				MIPS_REG_A0,
				Ini_GetInt(memCfg, "draw")
			);
		} else {               // PAUSE_MENU
			Mips64_SplitLoad(
				SegmentToVirtual(SEG_CODE, romOff->table.pauseMenu.updt.hi - RELOC_CODE),
				SegmentToVirtual(SEG_CODE, romOff->table.pauseMenu.updt.lo - RELOC_CODE),
				MIPS_REG_A0,
				Ini_GetInt(memCfg, "updt")
			);
			Mips64_SplitLoad(
				SegmentToVirtual(SEG_CODE, romOff->table.pauseMenu.draw.hi - RELOC_CODE),
				SegmentToVirtual(SEG_CODE, romOff->table.pauseMenu.draw.lo - RELOC_CODE),
				MIPS_REG_A0,
				Ini_GetInt(memCfg, "draw")
			);
		}
	}
	
	List_Free(&list);
}

static void Build_Skybox(Rom* rom, Memfile* memData, Memfile* memCfg) {
	List list = List_New();
	
	Rom_ItemList(&list, "rom/system/skybox/", SORT_NUMERICAL, LIST_FOLDERS);
	
	for (int i = 0; i < list.num; i++) {
		if (list.item[i] == NULL)
			continue;
		
		Rom_ProgressInfo(rom, LANG_SKYBOX, i, &list);
		
		u32 texId = 941 + i * 2;
		u32 palId = 942 + i * 2;
		
		fs_set(list.item[i]);
		const char* palette = fs_item("skybox.pal");
		const char* texel = fs_item("skybox.tex");
		
		if (Build_StatFile(texel) || Build_StatFile(palette))
			continue;
		
		Memfile_LoadBin(memData, texel);
		Dma_WriteMemfile(rom, texId, memData, true);
		
		Memfile_LoadBin(memData, palette);
		Dma_WriteMemfile(rom, palId, memData, false);
	}
	
	List_Free(&list);
}

static void Build_VanillaHook(Rom* rom) {
	char* s;
	char* func;
	char* ram;
	u32 offset;
	u32* data = NULL;
	u32 sym;
	Memfile* map = new(Memfile);
	Memfile* ld = new(Memfile);
	
	Memfile_LoadStr(map, "include/z64hdr/oot_mq_debug/sym_src.ld");
	Memfile_LoadStr(ld, "include/z_lib_user.ld");
	
	s = ld->data;
	
	do {
		if (!strstart(s, "__vanilla_hook_"))
			continue;
		s += strlen("__vanilla_hook_");
		func = x_strndup(s, strcspn(s, "= \n\t"));
		osLog("func: [%s]", func);
		ram = strwstr(map->str, func);
		if (!ram) errr(gLang.rom.err_hook_unk_symbol, func);
		ram = strstr(ram, "ROM:") + strlen("ROM:");
		if (!ram) errr(gLang.rom.err_hook_fail_offset, func);
		offset = shex(ram);
		sym = shex(strstr(s, "0x"));
		
		osLog("Offset: 0x%08X", offset);
		osLog("GetSymInfo:    0x%08X", sym);
		
		switch (offset) {
			case 0x00001060 ... 0x00012F70:
				data = SegmentToVirtual(SEG_BOOT, offset - RELOC_BOOT);
				break;
			case 0x00A94000 ... 0x00BCEF30:
				data = SegmentToVirtual(SEG_CODE, offset - RELOC_CODE);
				break;
			default:
				errr(gLang.rom.err_hook_not_code_offset, func);
		}
		
		data[0] = Mips64_J(sym);
		data[1] = Mips64_Nop();
		data[2] = Mips64_Jr(MIPS_REG_RA);
		data[3] = Mips64_Nop();
		
		for (int i = 0; i < 4; i++)
			SwapBE(data[i]);
		
	} while ((s = strline(s, 1)));
	
	Memfile_Free(map);
	Memfile_Free(ld);
	delete(map);
	delete(ld);
}

static void Build_Static(Rom* rom, Memfile* memData, Memfile* memCfg) {
	List list = List_New();
	
	Rom_ItemList(&list, "rom/system/static/", SORT_NO, LIST_FILES);
	
	foreach(i, gOoT_Static) {
		s32 id = -1;
		s32 k = 0;
		u32 start, end;
		u32* data;
		s32 compress;
		Memfile mem;
		char* table;
		
		Rom_ProgressInfo(rom, LANG_STATIC, i, &list);
		
		for (int j = 0; j < list.num; j++) {
			if (striend(list.item[j], x_fmt("%s.bin", gOoT_Static[i].name))) {
				id = gOoT_Static[i].id;
				k = j;
			}
		}
		
		if (id < 0) {
			if (gOoT_Static[i].id == DMA_ID_LINK_ANIMATION)
				id = DMA_ID_LINK_ANIMATION;
			
			else
				errr_align(gLang.err_missing, gOoT_Static[i].name);
		}
		
		switch (id) {
			case DMA_ID_BOOT:
				if (!g64.yazHeader) {
					u8* data = SegmentToVirtual(SEG_BOOT, 0x28B0 - RELOC_BOOT);
					struct {
						u32 offset;
						u8  data[4];
					} patch[] = {
						{ 0x1C, { 0x8C, 0x8E, 0x00, 0x00 } },
						{ 0x30, { 0x24, 0x91, 0x00, 0x04 } },
					};
					
					for (int pid = 0; pid < ArrCount(patch); pid++)
						for (int off = 0; off < 4; off++)
							data[patch[pid].offset + off] = patch[pid].data[off];
				}
				
				Patch_File(&rom->boot, list.item[k]);
				Memfile_Seek(&rom->file, RELOC_BOOT);
				Memfile_Append(&rom->file, &rom->boot);
				
				continue;
				
			case DMA_ID_CODE:
			
				Patch_File(&rom->code, list.item[k]);
				Build_VanillaHook(rom);
				
				Dma_WriteMemfile(rom, id, &rom->code, NOCACHE_COMPRESS);
				
				continue;
				
			case DMA_ID_LINK_ANIMATION:
				Dma_WriteMemfile(rom, id, &rom->playerAnim, false);
				continue;
				
			case DMA_ID_MESSAGE_DATA_STATIC_NES:
			case DMA_ID_MESSAGE_DATA_STATIC_STAFF:
				mem = Memfile_New();
				table = x_strdup(list.item[k]);
				strrep(table, ".bin", ".tbl");
				Memfile_LoadBin(&mem, table);
				
				if (strstr(x_basename(table), "NES"))
					memcpy(rom->table.nesMsg, mem.data, mem.size);
				
				else
					memcpy(rom->table.staffMsg, mem.data, mem.size);
				
				Memfile_Free(&mem);
				
				break;
		}
		
		Memfile_Null(memData);
		Memfile_LoadBin(memData, list.item[k]);
		
		switch (id) {
			case DMA_ID_Z_SELECT_STATIC:
			case DMA_ID_NINTENDO_ROGO_STATIC:
			case DMA_ID_TITLE_STATIC:
			case DMA_ID_ELF_MESSAGE_FIELD:
			case DMA_ID_ELF_MESSAGE_YDAN:
				compress = COMPRESS;
				break;
			default:
				compress = false;
				break;
		}
		
		if (Patch_File(memData, list.item[k]) && compress)
			compress = NOCACHE_COMPRESS;
		
		start = Dma_WriteMemfile(rom, id, memData, compress);
		end = Dma_GetVRomEnd();
		
		switch (id) {
			case DMA_ID_ELF_MESSAGE_FIELD:
				data = SegmentToVirtual(SEG_CODE, 0xB9E6A8 - RELOC_CODE);
				data[0] = ReadBE(start);
				data[1] = ReadBE(end);
				
				break;
			case DMA_ID_ELF_MESSAGE_YDAN:
				data = SegmentToVirtual(SEG_CODE, 0xB9E6A8 - RELOC_CODE);
				data[2] = ReadBE(start);
				data[3] = ReadBE(end);
				
				break;
		}
	}
	
	List_Free(&list);
}

static void Build_EntranceTable(Rom* rom, Memfile* memData, Memfile* memCfg) {
	EntranceInfo* ent = rom->table.entrance;
	Toml toml = Toml_New();
	
	Toml_Load(&toml, "src/entrance_table.toml");
	
	for (int i = 0; i < rom->table.num.entrance; i++, ent++) {
		if (!Toml_Var(&toml, "0x%04X[0]", i))
			break;
		
		ent->scene = Toml_GetInt(&toml, "0x%04X[0]", i);
		ent->spawn = Toml_GetInt(&toml, "0x%04X[1]", i);
		ent->continueBgm = Toml_GetBool(&toml, "0x%04X[2]", i);
		ent->titleCard = Toml_GetBool(&toml, "0x%04X[3]", i);
		ent->fadeIn = Transition_GetType(FreeList_Que(Toml_GetStr(&toml, "0x%04X[4]", i)));
		ent->fadeOut = Transition_GetType(FreeList_Que(Toml_GetStr(&toml, "0x%04X[5]", i)));
	}
	
	FreeList_Free();
	Toml_Free(&toml);
}

static void Build_CustomDma(Rom* rom, bool reg) {
	Memfile mem = Memfile_New();
	List tab = List_New();
	Toml toml = Toml_New();
	
	if (!sys_stat("dma.toml"))
		return;
	
	Toml_Load(&toml, "dma.toml");
	Toml_ListTabs(&toml, &tab, "");
	
	for (int i = 0; i < tab.num; i++) {
		const char* tab_name = tab.item[i];
		u32 id;
		
		Rom_ProgressInfo(rom, LANG_DMA, i, &tab);
		
		if (!vldt_hex(tab_name))
			errr(gLang.rom.err_custom_dma_table_index, tab_name);
		
		id = sint(tab_name);
		if (id < 0x20 || id > 0x20 + 40)
			errr(gLang.rom.err_custom_dma_reserved, id);
		
		if (!Toml_Var(&toml, "%s.file", tab_name))
			errr_align(gLang.err_missing, "dma.toml: %s.file", tab_name);
		if (!Toml_Var(&toml, "%s.compress", tab_name))
			errr_align(gLang.err_missing, "dma.toml: %s.compress", tab_name);
		
		const char* file = Toml_GetStr(&toml, "%s.file", tab_name);
		bool compress = Toml_GetBool(&toml, "%s.compress", tab_name);
		
		Memfile_LoadBin(&mem, file);
		
		if (reg)
			Dma_WriteFlag(id, false);
		else
			Dma_WriteMemfile(rom, id, &mem, compress);
		
		Ini_GotoTab(NULL);
		
		delete(file);
	}
	
	Memfile_Free(&mem);
	Toml_Free(&toml);
	List_Free(&tab);
}

// # # # # # # # # # # # # # # # # # # # #
// # Global                              #
// # # # # # # # # # # # # # # # # # # # #

void Rom_New(Rom* rom, const char* romName) {
	u16* addr;
	
	rom->file = Memfile_New();
	Memfile_Alloc(&rom->file, MbToBin(80));
	Memfile_LoadBin(&rom->file, romName);
	sBaseromSize = rom->file.size;
	SegmentSet(0x0, rom->file.data);
	
	if (strcmp(SegmentToVirtual(0, 0xBCF8F0), "NOT MARIO CLUB VERSION"))
		warn(gLang.rom.warn_not_oot_mq_debug);
	else rom->ootDebug = true;
	
	rom->offset.table.dmaTable = 0x012F70;
	rom->offset.table.objTable = 0xB9E6C8;
	rom->offset.table.actorTable = 0xB8D440;
	rom->offset.table.effectTable = 0xB8CB50;
	rom->offset.table.stateTable = 0xB969D0;
	rom->offset.table.sceneTable = 0xBA0BB0;
	rom->offset.table.kaleidoTable = 0xBA4340;
	
	rom->offset.table.seqFontTbl = 0xBCC4E0;
	rom->offset.table.seqTable = 0xBCC6A0;
	rom->offset.table.fontTable = 0xBCC270;
	rom->offset.table.sampleTable = 0xBCCD90;
	
	rom->offset.table.nesEntryTable = 0xBC24C0;
	rom->offset.table.staffEntryTable = 0xBCA908;
	
	rom->offset.table.entranceTable = 0xB9F360;
	rom->offset.table.restrictionFlags = 0x00B9CA10;
	
	addr = SegmentToVirtual(0x0, 0xB5A4AE);
	rom->offset.segment.seqRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
	rom->offset.segment.seqRom |= ReadBE(addr[2]);
	
	addr = SegmentToVirtual(0x0, 0xB5A4C2);
	rom->offset.segment.fontRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
	rom->offset.segment.fontRom |= ReadBE(addr[2]);
	
	addr = SegmentToVirtual(0x0, 0xB5A4D6);
	rom->offset.segment.smplRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
	rom->offset.segment.smplRom |= ReadBE(addr[2]);
	
	rom->table.num.dma = 1548;
	rom->table.num.obj = 402;
	rom->table.num.actor = 471;
	rom->table.num.effect = 37;
	rom->table.num.state = 6;
	rom->table.num.scene = 110;
	rom->table.num.kaleido = 2;
	rom->table.num.entrance = 1672;
	rom->table.num.skybox = 32;
	
	rom->offset.table.player.init = (HiLo) { 0x00B288F8, 0x00B28900 };
	rom->offset.table.player.dest = (HiLo) { 0x00B28908, 0x00B28914 };
	rom->offset.table.player.updt = (HiLo) { 0x00B2891C, 0x00B28928 };
	rom->offset.table.player.draw = (HiLo) { 0x00B28930, 0x00B2893C };
	
	rom->offset.table.pauseMenu.updt = (HiLo) { 0x00B33208, 0x00B3320C };
	rom->offset.table.pauseMenu.draw = (HiLo) { 0x00B33218, 0x00B33220 };
	
	rom->table.dma = SegmentToVirtual(0x0, rom->offset.table.dmaTable);
	rom->table.object = SegmentToVirtual(0x0, rom->offset.table.objTable);
	rom->table.actor = SegmentToVirtual(0x0, rom->offset.table.actorTable);
	rom->table.effect = SegmentToVirtual(0x0, rom->offset.table.effectTable);
	rom->table.state = SegmentToVirtual(0x0, rom->offset.table.stateTable);
	rom->table.scene = SegmentToVirtual(0x0, rom->offset.table.sceneTable);
	rom->table.kaleido = SegmentToVirtual(0x0, rom->offset.table.kaleidoTable);
	
	rom->table.nesMsg = SegmentToVirtual(0x0, rom->offset.table.nesEntryTable);
	rom->table.staffMsg = SegmentToVirtual(0x0, rom->offset.table.staffEntryTable);
	
	rom->table.entrance = SegmentToVirtual(0x0, rom->offset.table.entranceTable);
	rom->table.restrictionFlags = SegmentToVirtual(0x0, rom->offset.table.restrictionFlags);
	
	rom->mem.sampleTbl = Memfile_New();
	rom->mem.fontTbl = Memfile_New();
	rom->mem.seqTbl = Memfile_New();
	rom->mem.seqFontTbl = Memfile_New();
	Memfile_Alloc(&rom->mem.sampleTbl, MbToBin(0.1));
	Memfile_Alloc(&rom->mem.fontTbl, MbToBin(0.1));
	Memfile_Alloc(&rom->mem.seqTbl, MbToBin(0.1));
	Memfile_Alloc(&rom->mem.seqFontTbl, MbToBin(0.1));
	
	if (!g64.dump) {
		s32 targetId[] = {
			DMA_ID_CODE,
			DMA_ID_BOOT
		};
		Memfile* targetMem[] = {
			&rom->code,
			&rom->boot
		};
		char* targetFile[] = {
			"z_code.bin",
			"z_boot.bin"
		};
		u8 targetSegment[] = {
			SEG_CODE,
			SEG_BOOT
		};
		
		foreach(k, targetId) {
			s32 id = -1;
			char* file;
			
			foreach(i, gOoT_Static) {
				if (gOoT_Static[i].id == targetId[k])
					id = i;
			}
			
			if (id == -1)
				errr_align(gLang.err_fail, x_fmt("unsolvable index: %s", targetFile[k]));
			
			file = x_fmt("rom/system/static/%s.bin", gOoT_Static[id].name);
			if (!sys_stat(file)) file = x_fmt("rom/system/static/%s/%s.bin", g64.vanilla, gOoT_Static[id].name);
			if (!sys_stat(file)) errr_align(gLang.err_missing, gOoT_Static[id].name);
			
			Memfile_LoadBin(targetMem[k], file);
			SegmentSet(targetSegment[k], targetMem[k]->data);
		}
		
		// Relocate restriction table to vanilla actor table
		rom->offset.table.restrictionFlags = rom->offset.table.actorTable;
		
		rom->table.state = SegmentToVirtual(SEG_CODE, rom->offset.table.stateTable - RELOC_CODE);
		rom->table.kaleido = SegmentToVirtual(SEG_CODE, rom->offset.table.kaleidoTable - RELOC_CODE);
		rom->table.nesMsg = SegmentToVirtual(SEG_CODE, rom->offset.table.nesEntryTable - RELOC_CODE);
		rom->table.staffMsg = SegmentToVirtual(SEG_CODE, rom->offset.table.staffEntryTable - RELOC_CODE);
		rom->table.entrance = SegmentToVirtual(SEG_CODE, rom->offset.table.entranceTable - RELOC_CODE);
		rom->table.restrictionFlags = SegmentToVirtual(SEG_CODE, rom->offset.table.restrictionFlags - RELOC_CODE);
		
		Memfile_Alloc(&rom->playerAnim, MbToBin(16));
	}
}

void Rom_Free(Rom* rom) {
	Memfile_Free(&rom->file);
	Memfile_Free(&rom->mem.sampleTbl);
	Memfile_Free(&rom->mem.fontTbl);
	Memfile_Free(&rom->mem.seqTbl);
	Memfile_Free(&rom->mem.seqFontTbl);
	Memfile_Free(&rom->code);
	Memfile_Free(&rom->boot);
	Memfile_Free(&rom->playerAnim);
	Toml_Free(&rom->toml);
	memset(rom, 0, sizeof(struct Rom));
}

void Rom_Dump(Rom* rom) {
	Memfile dataFile = Memfile_New();
	Memfile config = Memfile_New();
	
	Memfile_Alloc(&dataFile, 0x460000); // Slightly larger than audiotable
	Memfile_Alloc(&config, 0x25000);
	
	info_align(gLang.rom.baserom, "%s", x_filename(rom->file.info.name));
	
	fs_mkflag(true);
	Dump_Actor(rom, &dataFile, &config);
	Dump_Effect(rom, &dataFile, &config);
	Dump_Object(rom, &dataFile, &config);
	Dump_Scene(rom, &dataFile, &config);
	Dump_State(rom, &dataFile, &config);
	Dump_Kaleido(rom, &dataFile, &config);
	Dump_Static(rom, &dataFile, &config);
	Dump_Skybox(rom, &dataFile, &config);
	Dump_EntranceTable(rom, &dataFile, &config);
	
	Audio_InitDump();
	Audio_DumpSoundFont(rom, &dataFile, &config);
	Audio_DumpSequence(rom, &dataFile, &config);
	Audio_DumpSampleTable(rom, &dataFile, &config);
	
	Audio_Free();
	
	Memfile_Free(&dataFile);
	Memfile_Free(&config);
	
	sys_mkdir("src/actor/");
	sys_mkdir("src/object/");
	sys_mkdir("src/scene/");
	sys_mkdir("src/effect/");
	sys_mkdir("src/sound/sample/");
	sys_mkdir("src/sound/sequence/");
	sys_mkdir("src/sound/soundfont/");
	sys_mkdir("src/sound/sfx/");
	sys_mkdir("rom/sound/sfx/");
	sys_mkdir("src/scene/");
}

void Rom_Build(Rom* rom) {
	Memfile dataFile = Memfile_New();
	Memfile config = Memfile_New();
	
	#define Dma_RegEntry(...) { \
			if (NARGS(__VA_ARGS__) == 3) \
			Dma_RegEntry(VA1(__VA_ARGS__), VA2(__VA_ARGS__), VA3(__VA_ARGS__)); \
			else \
			{ Dma_RegEntry(VA1(__VA_ARGS__), VA2(__VA_ARGS__), VA3(__VA_ARGS__)); Dma_WriteFlag(VA2(__VA_ARGS__), false); } } \

	
	Patch_Init();
	
	Memfile_Alloc(&dataFile, 0x460000);
	Memfile_Alloc(&config, 0x25000);
	
	Memfile_Set(&dataFile, MEM_REALLOC, true, MEM_END);
	Memfile_Set(&config, MEM_REALLOC, true, MEM_END);
	
	info_align(gLang.rom.baserom, "%s", x_filename(rom->file.info.name));
	info_align(gLang.rom.buildrom, "%s",  g64.build[g64.buildID]);
	
	ExtensionTable_Init(rom);
	
	Dma_RegEntry(rom, DMA_ID_UNUSED_3, 0x10, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_UNUSED_4, 0x10, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_UNUSED_5, 0x10, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_LINK_ANIMATION, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_ICON_ITEM_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_ICON_ITEM_24_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_ICON_ITEM_FIELD_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_ICON_ITEM_DUNGEON_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_ICON_ITEM_GAMEOVER_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_ICON_ITEM_NES_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_ICON_ITEM_GER_STATIC, 0x1000);
	Dma_RegEntry(rom, DMA_ID_ICON_ITEM_FRA_STATIC, 0x1000);
	Dma_RegEntry(rom, DMA_ID_ITEM_NAME_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_MAP_NAME_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_DO_ACTION_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_MESSAGE_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_MESSAGE_TEXTURE_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_NES_FONT_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_MESSAGE_DATA_STATIC_NES, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_MESSAGE_DATA_STATIC_GER, 0x1000);
	Dma_RegEntry(rom, DMA_ID_MESSAGE_DATA_STATIC_FRA, 0x1000);
	Dma_RegEntry(rom, DMA_ID_MESSAGE_DATA_STATIC_STAFF, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_MAP_GRAND_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_MAP_I_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_MAP_48X85_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_CODE, 0x10, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_Z_SELECT_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_NINTENDO_ROGO_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_TITLE_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_PARAMETER_STATIC, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_ELF_MESSAGE_FIELD, 0x1000, ReserveSlot);
	Dma_RegEntry(rom, DMA_ID_ELF_MESSAGE_YDAN, 0x1000, ReserveSlot);
	Dma_RegGroup(rom, DMA_ACTOR);
	Dma_RegGroup(rom, DMA_STATE);
	Dma_RegGroup(rom, DMA_KALEIDO);
	Dma_RegGroup(rom, DMA_EFFECT);
	Dma_RegGroup(rom, DMA_OBJECT);
	Dma_RegGroup(rom, DMA_SCENES);
	Dma_RegGroup(rom, DMA_PLACE_NAME);
	Dma_RegGroup(rom, DMA_SKYBOX_TEXEL);
	Dma_RegGroup(rom, DMA_UNUSED);
	
	Dma_WriteFlag(DMA_ID_BUILD_INFO, false);
	
#undef Dma_RegEntry
	Dma_CombineSlots();
	
	if (g64.info)
		Dma_SlotInfo(rom, gLang.rom.info_dma_free, gSlotHead);
	
	ExtensionTable_Alloc(rom);
	
	osLog("Build");
	Build_CustomDma(rom, true);
	
	Audio_InitBuild();
	Audio_BuildSampleTable(rom, &dataFile, &config);
	Audio_BuildSoundFont(rom, &dataFile, &config);
	Audio_BuildSequence(rom, &dataFile, &config);
	Audio_BuildSfxTable(rom, &dataFile, &config);
	Audio_UpdateSegments(rom);
	Audio_Free();
	
	const struct {
		void (*func)(Rom*, Memfile*, Memfile*);
		const char* name;
	} buildList[] = {
		{ Build_Actor,         gLang.rom.target[LANG_ACTOR]         },
		{ Build_Effect,        gLang.rom.target[LANG_EFFECT]        },
		{ Build_Object,        gLang.rom.target[LANG_OBJECT]        },
		{ Build_Scene,         gLang.rom.target[LANG_SCENE]         },
		{ Build_EntranceTable, NULL                                 },
		{ Build_State,         gLang.rom.target[LANG_STATE]         },
		{ Build_Kaleido,       gLang.rom.target[LANG_KALEIDO]       },
		{ Build_Skybox,        gLang.rom.target[LANG_SKYBOX]        },
	};
	
	mutex_init(&sDmaMutex);
	for (int i = 0; i < ArrCount(buildList); i++)
		buildList[i].func(rom, &dataFile, &config);
	mutex_dest(&sDmaMutex);
	
	if (g64.info) {
		{
			SceneEntry* table = rom->table.scene;
			SceneEntry* end = table + rom->table.num.scene;
			Toml __t = {}, * toml = &__t;
			int i = 0;
			
			*toml = Toml_New();
			
			for (; table < end; table++, i++) {
				if (!table->vromEnd || !table->vromStart)
					continue;
				
				Toml_SetVar(toml, x_fmt("scene[%d].index", i), "0x%02X", i);
				Toml_SetVar(toml, x_fmt("scene[%d].vrom[0]", i), "0x%08X", ReadBE(table->vromStart));
				Toml_SetVar(toml, x_fmt("scene[%d].vrom[1]", i), "0x%08X", ReadBE(table->vromEnd));
				Toml_SetVar(toml, x_fmt("scene[%d].title_vrom[0]", i), "0x%08X", ReadBE(table->titleVromStart));
				Toml_SetVar(toml, x_fmt("scene[%d].title_vrom[1]", i), "0x%08X", ReadBE(table->titleVromEnd));
			}
			
			Toml_Save(toml, "scene_info.toml");
			Toml_Free(toml);
		}
		{
			ObjectEntry* table = rom->table.object;
			ObjectEntry* end = table + rom->table.num.obj;
			Toml __t = {}, * toml = &__t;
			int i = 0;
			
			*toml = Toml_New();
			
			for (; table < end; table++, i++) {
				if (!table->vromEnd || !table->vromStart)
					continue;
				
				Toml_SetVar(toml, x_fmt("object[%d].index", i), "0x%02X", i);
				Toml_SetVar(toml, x_fmt("object[%d].vrom[0]", i), "0x%08X", ReadBE(table->vromStart));
				Toml_SetVar(toml, x_fmt("object[%d].vrom[1]", i), "0x%08X", ReadBE(table->vromEnd));
			}
			
			Toml_Save(toml, "object_info.toml");
			Toml_Free(toml);
		}
	}
	
	Patch_File(&rom->file, NULL);
	
	Build_Static(rom, &dataFile, &config);
	Build_CustomDma(rom, false);
	
	if (g64.buildID == ROM_DEV && sys_stat("build_info.txt")) {
		Memfile mem = Memfile_New();
		
		Memfile_LoadStr(&mem, "build_info.txt");
		Dma_WriteMemfile(rom, DMA_ID_BUILD_INFO, &mem, 0);
		
		Memfile_Free(&mem);
	}
	
	if (sys_stat("rom/lib_user/z_lib_user.bin")) {
		Memfile_Null(&dataFile);
		Memfile_LoadBin(&dataFile, "rom/lib_user/z_lib_user.bin");
		if (dataFile.size > 0xB5000)
			errr(gLang.rom.err_ulib_size, BinToKb(dataFile.size), BinToKb(0xB5000));
		Dma_WriteMemfile(rom, DMA_ID_UNUSED_3, &dataFile, true);
	}
	
	if (g64.info)
		Dma_SlotInfo(rom, gLang.rom.info_dma_left, g64.compress ? gSlotYazHead : gSlotHead);
	
	Dma_UpdateRomSize(rom);
	fix_crc(rom->file.data);
	Memfile_SaveBin(&rom->file, g64.build[g64.buildID]);
	Dma_Free();
	
	if (g64.compress)
		info_align(gLang.rom.info_compress_rate, "[ %.1f%c ]", ((f32)rom->file.size / sBaseromSize) * 100.0, '%');
	
	Memfile_Free(&dataFile);
	Memfile_Free(&config);
	Patch_Free();
	
	if (sStatWarning) {
		info_nl();
		warn(gLang.rom.warn_orphan_folders);
	}
}

void Rom_ItemList(List* list, const char* path, bool isNum, ListFlag flags) {
	List vanilla = List_New();
	List modified = List_New();
	
	List_Walk(&vanilla, x_fmt("%s%s/", path, g64.vanilla), 0, flags | LIST_RELATIVE);
	List_SetFilters(&modified, FILTER_WORD, g64.vanilla);
	List_Walk(&modified, path, 0, flags | LIST_NO_DOT | LIST_RELATIVE);
	osLog("Form Romlist");
	
	if (isNum) {
		forlist(item, modified) {
			if (strstart(item, "0")) {
				if (!strstart(item, "0x"))
					errr("Hex name must start with '0x':\n\"%s\"", item);
				
			} else if (strspn(item, "0123456789") != strcspn(item, " -"))
				errr("Numeric name must only contain numbers:\n\"%s\"", item);
		}
		
		osLog("Sort Van");
		List_SortSlot(&vanilla, false);
		
		osLog("Sort Mod");
		List_SortSlot(&modified, true);
	}
	
	List_Free(list);
	
	if (!modified.num) {
		if (!vanilla.num)
			goto free;
		
		List_Alloc(list, vanilla.num);
		
		for (var_t i = 0; i < vanilla.num; i++) {
			if (vanilla.item[i])
				List_Add(list, x_fmt("%s%s/%s", path, g64.vanilla, vanilla.item[i]));
			else
				List_Add(list, NULL);
		}
		
		goto free;
	}
	
	List_Alloc(list, modified.num + vanilla.num);
	
	if (isNum) {
		osLog("Num");
		u32 maxNum = 0;
		
		for (int i = 0; i < modified.num; i++) {
			if (modified.item[i] == NULL)
				continue;
			
			maxNum = Max(sint(modified.item[i]), maxNum);
		}
		
		for (int i = 0; i < vanilla.num; i++) {
			if (vanilla.item[i] == NULL)
				continue;
			
			maxNum = Max(sint(vanilla.item[i]), maxNum);
		}
		
		for (int i = 0; i < maxNum + 1; i++) {
			if (i < modified.num && modified.item[i] && sint(modified.item[i]) == i)
				list->item[list->num] = fmt("%s%s", path, modified.item[i]);
			else if (i < vanilla.num && vanilla.item[i] && sint(vanilla.item[i]) == i)
				list->item[list->num] = fmt("%s%s/%s", path, g64.vanilla, vanilla.item[i]);
			else
				list->item[list->num] = NULL;
			list->num++;
		}
	} else {
		osLog("Char");
		u32 i = 0;
		
		while (i < modified.num) {
			list->item[list->num] = fmt("%s%s", path, modified.item[i]);
			list->num++;
			i++;
		}
		
		i = 0;
		while (i < vanilla.num) {
			u32 cont = 0;
			for (int j = 0; j < list->num; j++) {
				if (!strcmp(vanilla.item[i], x_filename(list->item[j]))) {
					cont = 1;
					i++;
					break;
				}
			}
			
			if (cont) continue;
			
			list->item[list->num] = fmt("%s%s/%s", path, g64.vanilla, vanilla.item[i]);
			list->num++;
			i++;
		}
	}
	
	free:
	List_Free(&vanilla);
	List_Free(&modified);
}

void Rom_Compress(void) {
	struct {
		const char* path;
		const char* objname;
		int targetId;
	} param[] = {
		{ "rom/actor/",         "overlay.zovl",         LANG_ACTOR            },
		{ "rom/effect/",        "overlay.zovl",         LANG_EFFECT           },
		{ "rom/object/",        "object.zobj",          LANG_OBJECT           },
		{ "rom/scene/",         "scene.zscene",         LANG_SCENE            },
		{ "rom/system/skybox/", "skybox.tex",           LANG_SKYBOX           },
		{ "rom/system/state/",  "overlay.zovl",         LANG_STATE            },
	};
	
	for (int o = 0; o < ArrCount(param); o++) {
		List list = List_New();
		
		Rom_ItemList(&list, param[o].path, SORT_NUMERICAL, LIST_FOLDERS);
		
		for (int i = 0; i < list.num; i++) {
			char* file;
			
			if (!list.item[i])
				continue;
			
			fs_set(list.item[i]);
			
			if (sys_stat((file = fs_item(param[o].objname)))) {
				
				if (streq(param[o].objname, "scene.zscene")) {
					List sublist = List_New();
					
					List_Walk(&sublist, x_path(file), 0, LIST_FILES);
					
					for (int j = 0; j < sublist.num; j++) {
						if (striend(sublist.item[j], ".zroom") ||
							striend(sublist.item[j], ".zmap")) {
							const char* out = Yaz_Filename(sublist.item[j]);
							
							if (sys_stat(out) < sys_stat(file))
								Parallel_Add(Yaz_EncodeThread,
									FreeList_Que(strdup(sublist.item[j])));
							
							delete(out);
						}
					}
					
					List_Free(&sublist);
				} else {
					const char* out = Yaz_Filename(file);
					
					if (sys_stat(out) < sys_stat(file))
						Parallel_Add(Yaz_EncodeThread, FreeList_Que(strdup(file)));
					
					delete(out);
				}
			}
		}
		
		List_Free(&list);
		
		gParallel_ProgMsg = strndup(gLang.rom.target[LANG_COMPRESS], 0x80);
		strrep((void*)gParallel_ProgMsg, ":", " :");
		strrep((void*)gParallel_ProgMsg, ":", gLang.rom.target[param[o].targetId]);
		
		Parallel_Exec(g64.threadNum);
		FreeList_Free();
		
		delete(gParallel_ProgMsg);
	}
}

void Rom_DeleteUnusedContent(void) {
	List list = List_New();
	char* item;
	u32 id;
	
	List_Walk(&list, x_fmt("rom/actor/%s/", g64.vanilla), 0, LIST_FOLDERS | LIST_RELATIVE);
	List_Sort(&list);
	for (int i = 0; i < ArrCount(gOoT_BetaActor); i++) {
		id = gOoT_BetaActor[i];
		
		if (list.item[id] == NULL || id >= list.num)
			continue;
		
		item = x_fmt("rom/actor/%s/%s", g64.vanilla, list.item[id]);
		
		warn_align(gLang.rm, item);
		sys_rmdir(item);
	}
	List_Free(&list);
	
	List_Walk(&list, x_fmt("rom/object/%s/", g64.vanilla), 0, LIST_FOLDERS | LIST_RELATIVE);
	List_Sort(&list);
	for (int i = 0; i < ArrCount(gOoT_BetaObject); i++) {
		id = gOoT_BetaObject[i];
		
		if (list.item[id] == NULL || id >= list.num)
			continue;
		
		item = x_fmt("rom/object/%s/%s", g64.vanilla, list.item[id]);
		
		warn_align(gLang.rm, item);
		sys_rmdir(item);
	}
	List_Free(&list);
	
	List_Walk(&list, x_fmt("rom/scene/%s/", g64.vanilla), 0, LIST_FOLDERS | LIST_RELATIVE);
	List_Sort(&list);
	for (int i = 0; i < ArrCount(gOoT_BetaScene); i++) {
		id = gOoT_BetaScene[i];
		
		if (list.item[id] == NULL || id >= list.num)
			continue;
		
		item = x_fmt("rom/scene/%s/%s", g64.vanilla, list.item[id]);
		
		warn_align(gLang.rm, item);
		sys_rmdir(item);
	}
	List_Free(&list);
}

s32 Rom_Extract(Memfile* mem, RomFile rom, const char* name) {
	if (rom.size == 0)
		return 0;
	Memfile_Null(mem);
	mem->size = rom.size;
	Memfile_Realloc(mem, rom.size);
	Memfile_Write(mem, rom.data, rom.size);
	Memfile_SaveBin(mem, name);
	
	return 1;
}

void AudioOnly_Dump(Rom* rom) {
	Memfile dataFile = Memfile_New();
	Memfile config = Memfile_New();
	
	Memfile_Alloc(&dataFile, 0x460000); // Slightly larger than audiotable
	Memfile_Alloc(&config, 0x25000);
	
	info_align(gLang.rom.baserom, "%s", x_filename(rom->file.info.name));
	
	Audio_InitDump();
	Audio_DumpSoundFont(rom, &dataFile, &config);
	Audio_DumpSequence(rom, &dataFile, &config);
	Audio_DumpSampleTable(rom, &dataFile, &config);
	Audio_Free();
	
	Memfile_Free(&dataFile);
	Memfile_Free(&config);
}

void AudioOnly_Build(Rom* rom) {
	Memfile dataFile = Memfile_New();
	Memfile config = Memfile_New();
	
	Memfile_Alloc(&dataFile, 0x460000);
	Memfile_Alloc(&config, 0x25000);
	
	Memfile_Set(&dataFile, MEM_REALLOC, true, MEM_END);
	Memfile_Set(&config, MEM_REALLOC, true, MEM_END);
	
	Audio_InitBuild();
	Audio_BuildSampleTable(rom, &dataFile, &config);
	Audio_BuildSoundFont(rom, &dataFile, &config);
	Audio_BuildSequence(rom, &dataFile, &config);
	Audio_Free();
	
	Memfile_SaveBin(&rom->mem.sampleTbl, "table_sample.bin");
	Memfile_SaveBin(&rom->mem.fontTbl, "table_font.bin");
	Memfile_SaveBin(&rom->mem.seqTbl, "table_seq.bin");
	Memfile_SaveBin(&rom->mem.seqFontTbl, "table_seqfont.bin");
	
	Memfile_Free(&dataFile);
	Memfile_Free(&config);
}
