#include "z64rom.h"
#include <ExtZip.h>

MemFile __actor_object_list;
MemFile* sDepList = &__actor_object_list;

static s32 sSkip;
static u16 sActorID;
static u16 sObjectID;
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

typedef struct ListNode {
	struct ListNode* prev;
	struct ListNode* next;
	const char* name;
	ItemList    list;
} ListNode;

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
	ItemList sectionList = ItemList_Initialize();
	
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
	
	Config_ListSections(cfg, &sectionList);
	
	forlist(i, sectionList) {
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
		
		Config_GotoSection(sectionList.item[i]);
		
		if (Config_Variable(cfg->str, "actor_id"))
			sActorID = Config_GetInt(cfg, "actor_id");
		if (Config_Variable(cfg->str, "object_id"))
			sObjectID = Config_GetInt(cfg, "object_id");
		
		foreach(j, f) {
			if (Config_Variable(cfg->str, f[j].variable)) {
				Config_GetArray(cfg, f[j].variable, &var);
				
				Config_GotoSection(NULL);
				f[j].func(&zip, &var, sectionList.item[i]);
				Config_GotoSection(sectionList.item[i]);
				
				ItemList_Free(&var);
				printf_nl();
				
				if (sSkip)
					break;
			}
		}
		
		ItemList_Free(&var);
	}
	
	MemFile_Free(&sDepLookUp);
	ItemList_Free(&sActorList);
	ItemList_Free(&sObjectList);
	MemFile_Free(&config);
	ItemList_Free(&sectionList);
	ZipFile_Free(&zip);
	
	if (sSkip == false)
		printf_info("Package has been installed successfully!");
	else
		printf_info("Package installation has been terminated!");
#ifdef _WIN32
	printf_getchar("Press enter to exit.");
#endif
	exit(0);
}

void Package_Pack() {
	u32 writtenItems = 0;
	ListNode* listHead = NULL;
	ListNode* node;
	ZipFile zip;
	
	if (Sys_Stat("package.zip"))
		Sys_Delete("package.zip");
	ZipFile_Load(&zip, "package.zip", ZIP_WRITE);
	
	while (true) {
		printf_info("Item Name: " PRNT_GRAY "or \"done\" to save the package");
		const char* str = Terminal_GetStr();
		
		printf_nl();
		
		if (!stricmp(str, "done") || !stricmp(str, "\"done\""))
			break;
		
		Calloc(node, sizeof(*node));
		node->name = StrDup(str);
		Node_Add(listHead, node);
		ItemList_Alloc(&node->list, 32, 64 * 32);
		
		printf_info("Provide file or path for files to be packed:");
		str = Terminal_GetStr();
		
		Terminal_ClearLines(2);
		printf_nl();
		
		if (Sys_Stat(str)) {
			MemFile mem = MemFile_Initialize();
			ItemList list = ItemList_Initialize();
			char* entryName;
			
			if (Sys_IsDir(str)) {
				FileSys_Path(str);
				ItemList_List(&list, str, -1, LIST_FILES | LIST_RELATIVE);
				
				forlist(i, list) {
					entryName = xFmt("%s/%s", node->name, list.item[i]);
					
					MemFile_LoadFile(&mem, FileSys_File(list.item[i]));
					ZipFile_WriteEntry(&zip, &mem, entryName);
					
					ItemList_AddItem(&node->list, entryName);
					MemFile_Free(&mem);
					writtenItems++;
				}
			} else {
				entryName = xFmt("%s/%s", node->name, str);
				
				MemFile_LoadFile(&mem, str);
				ZipFile_WriteEntry(&zip, &mem, entryName);
				
				ItemList_AddItem(&node->list, entryName);
				MemFile_Free(&mem);
			}
		} else {
			Terminal_ClearLines(3);
			printf_warning("No file or path... Try again!\a");
			Sys_Sleep(2.0);
			Terminal_ClearLines(2);
		}
	}
	
	ZipFile_Free(&zip);
	
	if (writtenItems == 0)
		Sys_Delete("package.zip");
}