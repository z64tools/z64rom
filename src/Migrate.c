#include "z64rom.h"

static void Migrate_Actor(ItemList* list, const char* folName, const char* path) {
	ItemList_List(list, xFmt("%s%s/", path, folName), 0, LIST_FOLDERS | LIST_RELATIVE);
	ItemList_NumericalSort(list);
	
	forlist(i, *list) {
		MemFile mem = MemFile_Initialize();
		char* ovl;
		char* txt;
		char* new;
		char* cfg;
		char* deadbeef;
		u32 allocType;
		u32 vramAddr;
		u32 offset;
		
		printf_progress("Actor", i + 1, list->num);
		
		if (i == 0)
			continue;
		
		FileSys_Path("%s%s/%s", path, folName, list->item[i]);
		
		ovl = FileSys_File("actor.zovl");
		txt = FileSys_File("conf.txt");
		
		if (!Sys_Stat(ovl) || !Sys_Stat(txt)) {
			continue;
		}
		
		FileSys_Path("rom/actor/0x%04X-%s/", i, gActorName_OoT[i]);
		new = FileSys_File("actor.zovl");
		cfg = FileSys_File("config.cfg");
		Sys_MakeDir(Path(new), gActorName_OoT[i]);
		
		if (!Sys_Stat(txt))
			printf_error("Where is the conf.txt? [%s]", ovl);
		
		MemFile_LoadFile(&mem, ovl);
		
		deadbeef = mem.str;
		while (deadbeef && deadbeef < mem.str + mem.size) {
			if (!memcmp(deadbeef + 10, "\xBE\xEF", 2))
				break;
			deadbeef++;
			deadbeef = MemMem(deadbeef, mem.size - (u32)(deadbeef - mem.str), "\xDE\xAD", 2);
		}
		
		if (deadbeef == NULL)
			printf_error("No DEADBEEF? [%s]", ovl);
		
		offset = deadbeef - mem.str;
		u16* actorID = (void*)deadbeef;
		u16* zero = (void*)deadbeef + 10;
		
		*actorID = i;
		*zero = 0;
		
		SwapBE(*actorID);
		MemFile_SaveFile(&mem, new);
		
		MemFile_LoadFile_String(&mem, txt);
		
		Log("Get 'allocType' from '%s'", txt);
		allocType = Value_Hex(Word(StrStr(mem.str, "Allocation"), 1));
		Log("Get 'VRAM' from '%s'", txt);
		vramAddr = Value_Hex(Word(StrStr(mem.str, "VRAM"), 1));
		offset += vramAddr;
		
		MemFile_Reset(&mem);
		Config_WriteHex(&mem, "vram_addr", vramAddr, NO_COMMENT);
		Config_WriteHex(&mem, "init_vars", offset, NO_COMMENT);
		Config_WriteInt(&mem, "alloc_type", allocType, NO_COMMENT);
		MemFile_SaveFile_String(&mem, cfg);
		
		MemFile_Free(&mem);
	}
	ItemList_Free(list);
}
static void Migrate_Effect(ItemList* list, const char* folName, const char* path) {
	ItemList_List(list, xFmt("%s%s/", path, folName), 0, LIST_FOLDERS | LIST_RELATIVE);
	ItemList_NumericalSort(list);
	
	forlist(i, *list) {
		MemFile mem = MemFile_Initialize();
		char* ovl;
		char* new;
		char* cfg;
		char* tuna;
		u32 vramAddr;
		u32 offset;
		
		printf_progress("Effect", i + 1, list->num);
		
		FileSys_Path("%s%s/%s", path, folName, list->item[i]);
		ovl = FileSys_File("particle.zovl");
		
		if (!Sys_Stat(ovl))
			continue;
		
		FileSys_Path("rom/effect/0x%04X-%s/", i, gEffectName_OoT[i]);
		new = FileSys_File("effect.zovl");
		cfg = FileSys_File("config.cfg");
		Sys_MakeDir(Path(new), gEffectName_OoT[i]);
		
		if (!Sys_Stat(ovl))
			continue;
		
		MemFile_LoadFile(&mem, ovl);
		u16* id = (void*)(tuna = MemMem(mem.data, mem.size, "tuna", 4));
		
		*id = i;
		SwapBE(*id);
		MemFile_SaveFile(&mem, new);
		
		vramAddr = Value_Hex(Word(StrStr(mem.str, "VRAM"), 1));
		offset = vramAddr + (tuna - mem.str);
		
		MemFile_Reset(&mem);
		Config_WriteHex(&mem, "vram_addr", vramAddr, NO_COMMENT);
		Config_WriteHex(&mem, "init_vars", offset, NO_COMMENT);
		MemFile_SaveFile_String(&mem, cfg);
		
		MemFile_Free(&mem);
	}
	ItemList_Free(list);
}
static void Migrate_Object(ItemList* list, const char* folName, const char* path) {
	ItemList_List(list, xFmt("%s%s/", path, folName), 0, LIST_FOLDERS | LIST_RELATIVE);
	ItemList_NumericalSort(list);
	
	forlist(i, *list) {
		char* zobj;
		char* new;
		
		printf_progress("Object", i + 1, list->num);
		
		FileSys_Path("%s%s/%s", path, folName, list->item[i]);
		zobj = FileSys_File("zobj.zobj");
		
		if (!Sys_Stat(zobj))
			continue;
		
		FileSys_Path("rom/object/0x%04X-%s/", i, gObjectName_OoT[i]);
		new = FileSys_File("object.zobj");
		Sys_MakeDir(Path(new), gObjectName_OoT[i]);
		
		Sys_Copy(zobj, new);
	}
	ItemList_Free(list);
}
static void Migrate_Scene(ItemList* list, const char* folName, const char* path) {
	ItemList_List(list, xFmt("%s%s/", path, folName), 0, LIST_FOLDERS | LIST_RELATIVE);
	ItemList_NumericalSort(list);
	
	forlist(i, *list) {
		ItemList files = ItemList_Initialize();
		s32 j = 0;
		MemFile cfg = MemFile_Initialize();
		u32 roomNum = 0;
		ItemList rooms = ItemList_Initialize();
		
		printf_progress("Scene", i + 1, list->num);
		
		ItemList_Alloc(&rooms, 64, 0x2000);
		MemFile_Alloc(&cfg, 0x1000);
		FileSys_Path("%s%s/%s", path, folName, list->item[i]);
		ItemList_List(&files, FileSys_File(""), 0, LIST_FILES | LIST_RELATIVE);
		
		for (; j < files.num; j++) {
			s32 copy = false;
			
			if (StrEndCase(files.item[j], ".zmap")) {
				copy = true;
				ItemList_AddItem(&rooms, xFmt("room_%d.zmap", roomNum++));
			} else if (StrEndCase(files.item[j], ".zscene")) {
				copy = true;
			} else if (StrEndCase(files.item[j], ".png")) {
				copy = true;
			} else if (StrEndCase(files.item[j], ".txt")) {
				MemFile txt = MemFile_Initialize();
				
				MemFile_LoadFile_String(&txt, FileSys_File(files.item[j]));
				
				Config_WriteInt(&cfg, "scene_func_id", Value_Int(Word(StrStr(txt.str, "shader:"), 1)), NO_COMMENT);
				
				FileSys_Path("%s%s/%s", path, folName, list->item[i]);
				MemFile_Free(&txt);
				
				continue;
			}
			
			if (copy) {
				char* input;
				char* output;
				
				input = FileSys_File(files.item[j]);
				FileSys_Path("rom/scene/0x%02X-%s/", i, gSceneName_OoT[i]);
				
				output = FileSys_File(files.item[j]);
				FileSys_Path("%s%s/%s", path, folName, list->item[i]);
				
				Sys_MakeDir(Path(output));
				Sys_Copy(input, output);
			}
		}
		
		FileSys_Path("rom/scene/0x%02X-%s/", i, gSceneName_OoT[i]);
		
		Config_WriteArray(&cfg, "rooms", &rooms, QUOTES, NO_COMMENT);
		MemFile_SaveFile_String(&cfg, FileSys_File("config.cfg"));
		
		ItemList_Free(&files);
		MemFile_Free(&cfg);
	}
	
	ItemList_Free(list);
}
static void Migrate_System(ItemList* list, const char* folName, const char* path) {
	const char* map[][2] = {
		{ "do_action_static",          map[0][0] },
		{ "elf_message_field",         map[1][0] },
		{ "elf_message_ydan",          map[2][0] },
		{ "icon_item_24_static",       map[3][0] },
		{ "icon_item_static",          map[4][0] },
		{ "item_name_static",          map[5][0] },
		{ "link_animetion",            map[6][0] },
		{ "map_48x85_static",          map[7][0] },
		{ "map_grand_static",          map[8][0] },
		{ "map_i_static",              map[9][0] },
		{ "map_name_static",           map[10][0] },
		{ "message_static",            map[11][0] },
		{ "message_texture_static",    map[12][0] },
		{ "nes_font_static",           map[13][0] },
		{ "nes_message_data_static",   "message_data_static_NES" },
		{ "staff_message_data_static", "message_data_static_staff" },
	};
	
	ItemList_List(list, xFmt("%s%s/", path, folName), 0, LIST_FILES | LIST_RELATIVE);
	
	ItemList_NumericalSort(list);
	
	forlist(i, *list) {
		char* mig;
		char* new = NULL;
		
		printf_progress("System", i + 1, list->num);
		FileSys_Path("%s%s/", path, folName);
		mig = FileSys_File(list->item[i]);
		
		FileSys_Path("rom/system/static/");
		for (s32 j = 0; j < 16; j++) {
			if (!strcmp(list->item[i], map[j][0])) {
				new = FileSys_File(map[j][1]);
			}
		}
		
		if (!new)
			continue;
		
		Sys_Copy(mig, new);
	}
	ItemList_Free(list);
}
static void Migrate_EntranceTable(const char* table, const char* path) {
	MemFile mem = MemFile_Initialize();
	MemFile mou = MemFile_Initialize();
	u32 lnum;
	char* line;
	
	FileSys_Path(path);
	if (!Sys_Stat(FileSys_File(table)))
		return;
	MemFile_LoadFile_String(&mem, FileSys_File(table));
	MemFile_Alloc(&mou, MbToBin(69));
	
	line = Line(mem.str, 1);
	lnum = LineNum(line);
	
	for (s32 i = 0; i < lnum; i++, line = Line(line, 1)) {
		while (isspace(*line)) line++;
		
		ItemList list = ItemList_Initialize();
		
		ItemList_Alloc(&list, 6, 0x80);
		ItemList_AddItem(&list, xFmt("0x%02X", Value_Int(Word(line, 1))));
		ItemList_AddItem(&list, xFmt("0x%02X", Value_Int(Word(line, 2))));
		ItemList_AddItem(&list, xFmt("%s", !stricmp(CopyWord(line, 3), "GO") ? "true" : "false"));
		ItemList_AddItem(&list, xFmt("%s", !stricmp(CopyWord(line, 4), "ON") ? "true" : "false"));
		ItemList_AddItem(&list, xFmt("\"%s\"", Transition_GetName(Value_Int(Word(line, 5)))));
		ItemList_AddItem(&list, xFmt("\"%s\"", Transition_GetName(Value_Int(Word(line, 6)))));
		
		Config_WriteArray(&mou, xFmt("0x%04X", i), &list, NO_QUOTES, NO_COMMENT);
		
		ItemList_Free(&list);
	}
	
	MemFile_SaveFile_String(&mou, "rom/system/entrance_table.cfg");
	
	MemFile_Free(&mem);
	MemFile_Free(&mou);
}

static void Migrate_RomToolLite(const char* path) {
	ItemList list = ItemList_Initialize();
	
	Migrate_Actor(&list, "actor", path);
	Migrate_Effect(&list, "particle", path);
	Migrate_Object(&list, "object", path);
	Migrate_Scene(&list, "scene", path);
	Migrate_EntranceTable("scene/route.txt", path);
	
	ItemList_Free(&list);
}

static void Migrate_RomTool(const char* path) {
	ItemList list = ItemList_Initialize();
	
	Migrate_Actor(&list, "actor", path);
	Migrate_Effect(&list, "particle", path);
	Migrate_Object(&list, "object", path);
	Migrate_Scene(&list, "scene", path);
	Migrate_System(&list, "misc", path);
	Migrate_EntranceTable("route.txt", path);
	
	ItemList_Free(&list);
}

void Migrate(const char* type, const char* path) {
	if (PathIsRel(path))
		path = PathAbs_From(gWorkDir, path);
	else
		path = StrUnq(path);
	
	printf_toolinfo(gToolName, "Migrating Project");
	printf_info("%s", path);
	
	if (!strcmp(type, "zzromtool"))
		Migrate_RomTool(path);
	else if (!strcmp(type, "zzrtl"))
		Migrate_RomToolLite(path);
}