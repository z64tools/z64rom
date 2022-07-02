#include "z64rom.h"
#include <ExtZip.h>

MemFile __actor_object_list;
MemFile* sDepList = &__actor_object_list;

static void Package_ListSections(MemFile* cfg, ItemList* list) {
	char* p = cfg->str;
	s32 ln = LineNum(p);
	
	for (s32 i = 0; i < ln; i++) {
		
	}
}

void Package_Load(const char* item) {
	ZipFile zip;
	MemFile config = MemFile_Initialize();
	MemFile* cfg = &config;
	ItemList list = ItemList_Initialize();
	
	if (!Sys_Stat(item)) return;
	
	ZipFile_Load(&zip, item, ZIP_READ);
	if (ZipFile_ReadEntry_Name(&zip, "package.cfg", cfg))
		printf_error("Failed to read entry \"package.cfg\" from \"%s\"", item);
	
	if (Config_Variable(cfg->str, "author"))
		printf_info_align("Author:", Config_GetStr(cfg, "author"));
	if (Config_Variable(cfg->str, "version"))
		printf_info_align("Version:", Config_GetStr(cfg, "version"));
	
	Package_ListSections(cfg, &list);
}