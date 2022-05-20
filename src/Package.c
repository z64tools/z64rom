#include "z64rom.h"
#include <zip.h>

MemFile __actor_object_list;
MemFile* sDepList = &__actor_object_list;

static inline char* __ForSection(char* cfg) {
	u32 lineCount = String_LineNum(cfg);
	char* scfg = cfg;
	
	for (s32 k = 0; k < lineCount; k++) {
		scfg = String_Line(cfg, k);
		
		while (scfg[0] == ' ' || scfg[0] == '\t')
			scfg++;
		
		if (scfg[0] == '[')
			break;
		
		for (s32 i = 0;; i++) {
			if (scfg[i] == '\n')
				break;
			if (scfg[i] == '\0')
				return NULL;
		}
	}
	
	return scfg;
}

static char* Package_GetSection(char* cfg, s32 i) {
	char* ret;
	char* scfg;
	u32 size;
	
	for (s32 j = 0; j <= i; j++) {
		cfg = __ForSection(cfg);
		
		if (cfg == NULL)
			return NULL;
		
		// Readvance
		if (j < i)
			cfg = String_Line(cfg, 1);
	}
	
	scfg = cfg + 1;
	scfg = __ForSection(scfg);
	scfg = &cfg[strlen(cfg)];
	
	size = (uPtr)scfg - (uPtr)cfg;
	
	Log("Copying [%d]", size);
	
	ret = Calloc(ret, size + 2);
	memcpy(ret, cfg, size);
	
	return ret;
}

void Package_Sound(struct zip_t* pkg, char* cfg) {
	char* name = Config_GetVariable(cfg, "name");
	char* file = Config_GetVariable(cfg, "file");
	ItemList list = ItemList_Initialize();
	MemFile mem = MemFile_Initialize();
	void* f;
	size_t size;
	
	if (name == NULL || file == NULL) {
		Log("Package:\n%s", cfg);
		if (name == NULL) Log("No [name] provided");
		if (file == NULL) Log("No [file] provided");
		
		printf_error("Package Error!");
	}
	
	char* fldr = Tmp_Printf("rom/sound/sample/.vanilla/%s/", name);
	char* mkfldr = Tmp_Printf("rom/sound/sample/%s/", name);
	
	Sys_MakeDir(mkfldr);
	
	if (Sys_Stat(fldr)) {
		ItemList_List(&list, fldr, -1, LIST_FILES);
		
		for (s32 i = 0; i < list.num; i++) {
			if (!StrEndCase(list.item[i], ".cfg") && !StrEndCase(list.item[i], ".book.bin"))
				break;
			
			char* target = Tmp_String(list.item[i]);
			String_Replace(target, ".vanilla/", "");
			
			Sys_Copy(list.item[i], target, StrEndCase(list.item[i], ".cfg") ? true : false);
		}
		
		ItemList_Free(&list);
	}
	
	zip_entry_open(pkg, file);
	if (zip_entry_read(pkg, &f, &size) < 0)
		printf_error("Could not extract [%s]", file);
	zip_entry_close(pkg);
	
	MemFile_Malloc(&mem, size);
	MemFile_Write(&mem, f, size);
	MemFile_SaveFile(&mem, Tmp_Printf("rom/sound/sample/%s/%s", name, String_GetFilename(file)));
	
	MemFile_Free(&mem);
	Free(f);
}

void Package_Actor(struct zip_t* pkg, char* cfg) {
	char* name = Config_GetVariable(cfg, "name");
	ItemList list = ItemList_Initialize();
	ItemList actorList = ItemList_Initialize();
	ItemList objectList = ItemList_Initialize();
	const char* tmp = "NotHex";
	char buf[512];
	s32 actorID = -1;
	s32 objectID = -1;
	char* object;
	s32 getFree = false;
	MemFile mout = MemFile_Initialize();
	
	MemFile_Malloc(&mout, MbToBin(32));
	Config_GetArray(&list, cfg, "files");
	object = ItemList_GetWildItem(&list, ".zobj");
	
	printf_info("Import [%s] to ActorID: " PRNT_DGRY "provide values as hex or [free] to get first free index", name);
	
	for (s32 i = 0; !String_Validate_Hex(tmp); i++) {
		if (i > 0)
			Terminal_ClearLines(2);
		tmp = Terminal_GetStr();
		
		if (StrMtch(tmp, "free")) {
			getFree = true;
			break;
		}
	}
	
	Terminal_ClearLines(3);
	SleepF(0.1);
	
	if (getFree) {
		Rom_ItemList_NDIR(&actorList, "rom/actor/", SORT_NUMERICAL, LIST_FOLDERS);
		Rom_ItemList_NDIR(&objectList, "rom/object/", SORT_NUMERICAL, LIST_FOLDERS);
		
		for (s32 i = 1; i < actorList.num; i++) {
			if (actorList.item[i] == NULL) {
				actorID = i;
				break;
			}
		}
		if (object) {
			for (s32 i = 1; i < objectList.num; i++) {
				if (objectList.item[i] == NULL) {
					objectID = i;
					break;
				}
			}
		}
	} else
		actorID = String_GetHexInt(tmp);
	
	if (actorID == -1 && getFree)
		printf_error("Could not find free ActorID");
	if (object && objectID == -1 && getFree)
		printf_error("Could not find free ObjectID");
	
	if (object && objectID == -1) {
		char fmt[512] = "Import [%s] to ObjectID: " PRNT_DGRY "";
		ItemList recList = ItemList_Initialize();
		
		sprintf(buf, "0x%04X", String_GetHexInt(tmp));
		Config_GetArray(&recList, sDepList->str, buf);
		
		for (s32 i = 0; i < recList.num; i++) {
			if (i > 0)
				strcat(fmt, "  ");
			strcat(fmt, recList.item[i]);
			strcat(fmt, " - ");
			strcat(fmt, gObjectName_OoT[String_GetHexInt(recList.item[i])]);
		}
		
		tmp = "NotHex";
		printf_info(fmt, object);
		
		for (s32 i = 0; !String_Validate_Hex(tmp); i++) {
			if (i > 0)
				Terminal_ClearLines(2);
			tmp = Terminal_GetStr();
		}
		
		objectID = String_GetHexInt(tmp);
		ItemList_Free(&recList);
	}
	
	Terminal_ClearLines(3);
	printf_info("%s, ActorID 0x%04X ObjectID 0x%04X\n", name, actorID, objectID);
	
	for (s32 i = 0; i < list.num; i++) {
		char* file;
		u32 size;
		void* f;
		s32 write = false;
		s32 string = false;
		
		if (StrEndCase(list.item[i], ".c") || StrEndCase(list.item[i], ".h")) {
			file = Tmp_Printf("src/actor/0x%04X-%s/%s", actorID, name, list.item[i]);
			Sys_MakeDir("src/actor/0x%04X-%s/", actorID, name);
			string = true;
			write = true;
		}
		
		if (StrEndCase(list.item[i], ".zovl") || StrEndCase(list.item[i], ".cfg")) {
			file = Tmp_Printf("rom/actor/0x%04X-%s/%s", actorID, name, list.item[i]);
			Sys_MakeDir("rom/actor/0x%04X-%s/", actorID, name);
			write = true;
			if (StrEndCase(list.item[i], ".cfg"))
				string = true;
		}
		
		if (objectID > 0 && StrEndCase(list.item[i], ".zobj")) {
			file = Tmp_Printf("rom/object/0x%04X-%s/%s", objectID, name, list.item[i]);
			Sys_MakeDir("rom/object/0x%04X-%s/", objectID, name);
			write = true;
		}
		
		if (write == false)
			continue;
		
		zip_entry_open(pkg, list.item[i]);
		if (zip_entry_read(pkg, &f, &size) < 0)
			printf_error("Could not extract [%s]", list.item[i]);
		zip_entry_close(pkg);
		
		MemFile_Reset(&mout);
		MemFile_Write(&mout, f, size);
		if (string)
			MemFile_SaveFile_String(&mout, file);
		else
			MemFile_SaveFile(&mout, file);
	}
	
	MemFile_Free(&mout);
	ItemList_Free(&list);
	ItemList_Free(&actorList);
	ItemList_Free(&objectList);
}

void Package_Load(const char* item) {
	struct zip_t* pkg;
	char* cfg;
	size_t cfgSize;
	char* sct;
	
	if (!Sys_Stat(item)) return;
	
	pkg = zip_open(item, 0, 'r');
	
	zip_entry_open(pkg, "package.toml");
	if (zip_entry_read(pkg, (void*)&cfg, &cfgSize) < 0) {
		zip_close(pkg);
		
		return;
	}
	zip_entry_close(pkg);
	
	MemFile_LoadFile_String(sDepList, "tools/actor-object-deb.toml");
	
	for (s32 i = 0; (sct = Package_GetSection(cfg, i)) != NULL; i++) {
		s32 j = 0;
		const char* str[] = {
			"[sound]",
			"[actor]",
			"[object]",
		};
		void (*func[])(struct zip_t*, char*) = {
			Package_Sound,
			Package_Actor,
			NULL,
		};
		
		for (; j < ArrayCount(str); j++)
			if (StrMtch(cfg, str[j]))
				break;
		
		Log("Func %s", str[j]);
		
		if (func[j])
			func[j](pkg, sct);
		
		Free(sct);
	}
	
	MemFile_Free(sDepList);
	Free(cfg);
	zip_close(pkg);
}