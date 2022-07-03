#include "z64rom.h"
#include <ExtZip.h>

MemFile __actor_object_list;
MemFile* sDepList = &__actor_object_list;

static void Package_ListSections(MemFile* cfg, ItemList* list) {
	char* p = cfg->str;
	s32 ln = LineNum(p);
	s32 sctCount = 0;
	s32 sctSize = 0;
	
	for (s32 i = 0; i < ln; i++, p = Line(p, 1)) {
		s32 sz = 0;
		
		while (!isgraph(*p)) p++;
		if (*p != '[')
			continue;
		sctCount++;
		
		while (p[sz] != ']') {
			sz++;
			sctSize++;
			
			if (p[sz] == '\0')
				printf_error("Missing ']' from section name at line %d for [%s]", i, cfg->info.name);
		}
	}
	
	ItemList_Alloc(list, sctCount, sctSize + sctCount);
	
	p = cfg->str;
	for (s32 i = 0; i < ln; i++, p = Line(p, 1)) {
		char* word;
		s32 sz = 0;
		
		while (!isgraph(*p)) p++;
		if (*p != '[')
			continue;
		sctCount++;
		
		word = StrDup(p + 1);
		
		for (s32 j = 0; ; j++) {
			if (word[j] == ']') {
				word[j] = '\0';
				break;
			}
		}
		
		ItemList_AddItem(list, word);
		Free(word);
	}
}

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

static s32 sSkip;
static u16 sActorID = 0xFFFF;
static u16 sObjectID = 0xFFFF;
static MemFile sDepLookUp;
static ItemList sActorList;
static ItemList sObjectList;

static s32 Package_ZipCallback(const char* name, MemFile* mem) {
	return 0;
}

static void Package_GetActorID(const char* name) {
	const char* input;
	
	if (sActorID != 0xFFFF)
		return;
	
	printf_info("" PRNT_BLUE "%s" PRNT_RSET " Actor ID: " PRNT_GRAY "[ hex / \"free\" / \"skip\" ]", name);
	
	while (true) {
		input = Terminal_GetStr();
		
		if (Value_ValidateHex(input)) {
			sActorID = Value_Hex(input);
			
			return;
		}
		
		if (!stricmp(input, "free") || !stricmp(input, "\"free\"")) {
			for (s32 i = 0;; i++) {
				if (i < sActorList.num) {
					if (sActorList.item[i] == NULL && i != 0) {
						sActorList.item[i] = (void*)0xDEAD;
						sActorID = i;
						break;
					}
				} else {
					sActorID = i;
					break;
				}
			}
			Terminal_ClearLines(2);
			
			printf_info("Using ID %04X", sActorID);
			
			return;
		}
		
		if (!stricmp(input, "skip") || !stricmp(input, "\"skip\"")) {
			sSkip = true;
			
			return;
		}
		
		Terminal_ClearLines(2);
	}
}

static void Package_ObjectID(const char* name) {
	const char* input;
	
	if (sObjectID != 0xFFFF)
		return;
	
	if (sActorID != 0xFFFF) {
		if (Config_Variable(sDepLookUp.str, xFmt("0x%04X", sActorID))) {
			ItemList list = ItemList_Initialize();
			
			Config_GetArray(&sDepLookUp, xFmt("0x%04X", sActorID), &list);
			
			if (list.num == 1 && strcmp(list.item[0], "None"))
				printf_info("" PRNT_GRAY "Recommended: %s", list.item[0]);
			
			else
				printf_info("" PRNT_GRAY "Recommended: free", list.item[0]);
			
			ItemList_Free(&list);
		}
	}
	
	printf_info("" PRNT_BLUE "%s" PRNT_RSET " Object ID: " PRNT_GRAY "[ hex / \"free\" / \"skip\" ]", name);
	
	while (true) {
		input = Terminal_GetStr();
		
		if (Value_ValidateHex(input)) {
			sObjectID = Value_Hex(input);
			
			return;
		}
		
		if (!stricmp(input, "free") || !stricmp(input, "\"free\"")) {
			for (s32 i = 0;; i++) {
				if (i < sObjectList.num) {
					if (sObjectList.item[i] == NULL && i != 0) {
						sObjectList.item[i] = (void*)0xDEAD;
						sObjectID = i;
						break;
					}
				} else {
					sObjectID = i;
					break;
				}
			}
			Terminal_ClearLines(2);
			
			printf_info("Using ID %04X", sObjectID);
			
			return;
		}
		
		if (!stricmp(input, "skip") || !stricmp(input, "\"skip\"")) {
			sSkip = true;
			
			return;
		}
		
		Terminal_ClearLines(2);
	}
}

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

static void Package_ActorSrc(ZipFile* zip, ItemList* list, const char* name) {
	Package_GetActorID(name);
	
	if (sSkip)
		return;
	
	forlist(i, *list) {
		if (StrEnd(list->item[i], "/")) {
			ZipFile_ReadEntries_Path(zip, list->item[i], Package_ZipCallback);
		}
	}
}

static void Package_ActorBin(ZipFile* zip, ItemList* list, const char* name) {
	Package_GetActorID(name);
	
	if (sSkip)
		return;
	
	forlist(i, *list) {
		if (StrEnd(list->item[i], "/")) {
			ZipFile_ReadEntries_Path(zip, list->item[i], Package_ZipCallback);
		}
	}
}

static void Package_ObjectSrc(ZipFile* zip, ItemList* list, const char* name) {
	Package_ObjectID(name);
	
	if (sSkip)
		return;
	
	forlist(i, *list) {
		if (StrEnd(list->item[i], "/")) {
			ZipFile_ReadEntries_Path(zip, list->item[i], Package_ZipCallback);
		}
	}
}

static void Package_ObjectBin(ZipFile* zip, ItemList* list, const char* name) {
	Package_ObjectID(name);
	
	if (sSkip)
		return;
	
	forlist(i, *list) {
		if (StrEnd(list->item[i], "/")) {
			ZipFile_ReadEntries_Path(zip, list->item[i], Package_ZipCallback);
		}
	}
}

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

void Package_Load(const char* item) {
	ZipFile zip;
	MemFile config = MemFile_Initialize();
	MemFile* cfg = &config;
	ItemList list = ItemList_Initialize();
	
	if (!Sys_Stat(item)) return;
	
	ZipFile_Load(&zip, item, ZIP_READ);
	if (ZipFile_ReadEntry_Name(&zip, "package.cfg", cfg))
		printf_error("Failed to read entry \"package.cfg\" from \"%s\"", item);
	
	if (Config_Variable(cfg->str, "author")) {
		printf_info_align("Author:", xFmt("" PRNT_GREN "%s", Config_GetStr(cfg, "author")));
		if (Config_Variable(cfg->str, "version"))
			printf_info_align("Version:", xFmt("" PRNT_YELW "%s", Config_GetStr(cfg, "version")));
		printf_nl();
	}
	
	MemFile_LoadFile_String(&sDepLookUp, "tools/actor-object-deb.cfg");
	Rom_ItemList(&sActorList, "rom/actor/", SORT_NUMERICAL, LIST_FOLDERS);
	Rom_ItemList(&sObjectList, "rom/object/", SORT_NUMERICAL, LIST_FOLDERS);
	
	Package_ListSections(cfg, &list);
	
	forlist(i, list) {
		ItemList var = ItemList_Initialize();
		
		struct {
			const char* variable;
			void (*func)(ZipFile*, ItemList*, const char*);
		} f[] = {
			{
				"actor_src",
				Package_ActorSrc
			},
			{
				"actor_bin",
				Package_ActorBin
			},
			{
				"object_src",
				Package_ObjectSrc
			},
			{
				"object_bin",
				Package_ObjectBin
			},
		};
		
		sActorID = 0xFFFF;
		sObjectID = 0xFFFF;
		
		Config_GotoSection(list.item[i]);
		
		if (Config_Variable(cfg->str, "actor_id"))
			sActorID = Config_GetInt(cfg, "actor_id");
		if (Config_Variable(cfg->str, "object_id"))
			sObjectID = Config_GetInt(cfg, "object_id");
		
		foreach(j, f) {
			if (Config_Variable(cfg->str, f[j].variable)) {
				Config_GetArray(cfg, f[j].variable, &var);
				
				Config_GotoSection(NULL);
				f[j].func(&zip, &var, list.item[i]);
				Config_GotoSection(list.item[i]);
				
				ItemList_Free(&var);
				printf_nl();
			}
		}
		
		ItemList_Free(&var);
	}
	
	MemFile_Free(&sDepLookUp);
	ItemList_Free(&sActorList);
	ItemList_Free(&sObjectList);
	MemFile_Free(&config);
	ItemList_Free(&list);
	ZipFile_Free(&zip);
	
	printf_info("All packages have been installed successfully!");
#ifdef _WIN32
	printf_getchar("Press enter to exit.");
#endif
	exit(0);
}