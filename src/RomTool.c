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
		FileSys_Path("rom/actor/0x%04X-%s/", i, gActorName_OoT[i]);
		new = FileSys_File("actor.zovl");
		cfg = FileSys_File("config.cfg");
		Sys_MakeDir(Path(new), gActorName_OoT[i]);
		
		if (!Sys_Stat(ovl)) {
			continue;
		}
		
		if (!Sys_Stat(txt))
			printf_error("Where is the conf.txt? [%s]", ovl);
		
		MemFile_LoadFile(&mem, ovl);
		
		deadbeef = mem.str;
		while (deadbeef && deadbeef < mem.str + mem.dataSize) {
			if (!memcmp(deadbeef + 10, "\xBE\xEF", 2))
				break;
			deadbeef++;
			deadbeef = MemMem(deadbeef, mem.dataSize - (u32)(deadbeef - mem.str), "\xDE\xAD", 2);
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
		FileSys_Path("rom/effect/0x%04X-%s/", i, gEffectName_OoT[i]);
		new = FileSys_File("effect.zovl");
		cfg = FileSys_File("config.cfg");
		Sys_MakeDir(Path(new), gEffectName_OoT[i]);
		
		if (!Sys_Stat(ovl))
			continue;
		
		MemFile_LoadFile(&mem, ovl);
		u16* id = (void*)(tuna = MemMem(mem.data, mem.dataSize, "tuna", 4));
		
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
		MemFile mem = MemFile_Initialize();
		char* zobj;
		char* new;
		
		printf_progress("Object", i + 1, list->num);
		
		FileSys_Path("%s%s/%s", path, folName, list->item[i]);
		zobj = FileSys_File("zobj.zobj");
		FileSys_Path("rom/object/0x%04X-%s/", i, gObjectName_OoT[i]);
		new = FileSys_File("object.zobj");
		Sys_MakeDir(Path(new), gObjectName_OoT[i]);
		
		if (!Sys_Stat(zobj))
			continue;
		
		Sys_Copy(zobj, new);
	}
	ItemList_Free(list);
}
static void Migrate_Scene(ItemList* list, const char* folName, const char* path) {
	ItemList_List(list, xFmt("%s%s/", path, folName), 0, LIST_FOLDERS | LIST_RELATIVE);
	ItemList_NumericalSort(list);
	
	forlist(i, *list) {
		ItemList files = ItemList_Initialize();
		char* zobj;
		char* new;
		s32 j = 0;
		
		printf_progress("Scene", i + 1, list->num);
		
		FileSys_Path("%s%s/%s", path, folName, list->item[i]);
		ItemList_List(&files, FileSys_File(""), 0, LIST_FILES | LIST_RELATIVE);
		
		for (; j < files.num; j++) {
			s32 copy = false;
			
			if (StrEndCase(files.item[j], ".zmap")) {
				copy = true;
			} else if (StrEndCase(files.item[j], ".zscene")) {
				copy = true;
			} else if (StrEndCase(files.item[j], ".png")) {
				copy = true;
			} else if (StrEndCase(files.item[j], ".txt")) {
				MemFile txt = MemFile_Initialize();
				MemFile cfg = MemFile_Initialize();
				
				MemFile_Alloc(&cfg, 0x1000);
				MemFile_LoadFile_String(&txt, FileSys_File(files.item[j]));
				
				Config_WriteInt(&cfg, "scene_func_id", Value_Int(Word(StrStr(txt.str, "shader:"), 1)), NO_COMMENT);
				
				FileSys_Path("rom/scene/0x%02X-%s/", i, gSceneName_OoT[i]);
				MemFile_SaveFile_String(&cfg, FileSys_File("config.cfg"));
				
				FileSys_Path("%s%s/%s", path, folName, list->item[i]);
				MemFile_Free(&txt);
				MemFile_Free(&cfg);
				
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
		
		ItemList_Free(&files);
	}
	ItemList_Free(list);
}
static void Migrate_EntranceTable(ItemList* list, const char* file, const char* path) {
}

static void RomTool_Migrate_RomToolLite(const char* path) {
}

static void RomTool_Migrate_RomTool(const char* path) {
	ItemList list = ItemList_Initialize();
	
	Migrate_Actor(&list, "actor", path);
	Migrate_Effect(&list, "particle", path);
	Migrate_Object(&list, "object", path);
	Migrate_Scene(&list, "scene", path);
}

void RomTool_Migrate(const char* type, const char* path) {
	if (PathIsRel(path))
		path = PathAbs_From(gWorkDir, path);
	else
		path = StrUnq(path);
	
	printf_info("[%s]", path);
	
	if (!strcmp(type, "zzromtool"))
		RomTool_Migrate_RomTool(path);
	else if (!strcmp(type, "zzrtl"))
		RomTool_Migrate_RomToolLite(path);
}