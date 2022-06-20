#include "z64rom.h"
#include <zip.h>

MemFile __actor_object_list;
MemFile* sDepList = &__actor_object_list;

void Package_Sound(struct zip_t* pkg, char* cfg) {
	char* name = Config_GetVariable(cfg, "name");
	char* file = Config_GetVariable(cfg, "file");
	ItemList list = ItemList_Initialize();
	MemFile mem = MemFile_Initialize();
	void* f;
	Size size;
	
	if (name == NULL || file == NULL) {
		Log("Package:\n%s", cfg);
		if (name == NULL) Log("No [name] provided");
		if (file == NULL) Log("No [file] provided");
		
		printf_error("Package Error!");
	}
	
	char* fldr = HeapPrint("rom/sound/sample/%s/%s/", gVanilla, name);
	char* mkfldr = HeapPrint("rom/sound/sample/%s/", name);
	
	Sys_MakeDir(mkfldr);
	
	if (Sys_Stat(fldr)) {
		char* file;
		FileSys_Path(fldr);
		
		file = FileSys_FindFile("config.cfg");
		
		if (file)
			Sys_Copy(file, HeapPrint("%sconfig.cfg", mkfldr));
	}
	
	zip_entry_open(pkg, file);
	if (zip_entry_read(pkg, &f, &size) < 0)
		printf_error("Could not extract [%s]", file);
	zip_entry_close(pkg);
	
	MemFile_Malloc(&mem, size);
	MemFile_Write(&mem, f, size);
	MemFile_SaveFile(&mem, HeapPrint("rom/sound/sample/%s/%s", name, Filename(file)));
	
	ItemList_Free(&list);
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
	MemFile mtom = MemFile_Initialize();
	
	MemFile_LoadMem(&mtom, cfg, strlen(cfg) + 1);
	
	MemFile_Malloc(&mout, MbToBin(32));
	Config_GetArray(&mtom, &list, "files");
	object = ItemList_GetWildItem(&list, ".zobj");
	
	printf_info("Import [%s] to ActorID: " PRNT_DGRY "provide values as hex or [free] to get first free index", name);
	
	for (s32 i = 0; !Value_ValidateHex(tmp); i++) {
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
		Rom_ItemList(&actorList, "rom/actor/", SORT_NUMERICAL, LIST_FOLDERS);
		Rom_ItemList(&objectList, "rom/object/", SORT_NUMERICAL, LIST_FOLDERS);
		
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
		actorID = Value_Hex(tmp);
	
	if (actorID == -1 && getFree)
		printf_error("Could not find free ActorID");
	if (object && objectID == -1 && getFree)
		printf_error("Could not find free ObjectID");
	
	if (object && objectID == -1) {
		char fmt[512] = "Import [%s] to ObjectID: " PRNT_DGRY "";
		ItemList recList = ItemList_Initialize();
		
		sprintf(buf, "0x%04X", Value_Hex(tmp));
		Config_GetArray(sDepList, &recList, buf);
		
		for (s32 i = 0; i < recList.num; i++) {
			if (i > 0)
				strcat(fmt, "  ");
			strcat(fmt, recList.item[i]);
			strcat(fmt, " - ");
			strcat(fmt, gObjectName_OoT[Value_Hex(recList.item[i])]);
		}
		
		tmp = "NotHex";
		printf_info(fmt, object);
		
		for (s32 i = 0; !Value_ValidateHex(tmp); i++) {
			if (i > 0)
				Terminal_ClearLines(2);
			tmp = Terminal_GetStr();
		}
		
		objectID = Value_Hex(tmp);
		ItemList_Free(&recList);
	}
	
	Terminal_ClearLines(3);
	
	for (s32 i = 0; i < list.num; i++) {
		char* file;
		Size size;
		void* f;
		s32 write = false;
		s32 string = false;
		
		if (StrEndCase(list.item[i], ".c") || StrEndCase(list.item[i], ".h")) {
			file = HeapPrint("src/actor/0x%04X-%s/%s", actorID, name, list.item[i]);
			Sys_MakeDir("src/actor/0x%04X-%s/", actorID, name);
			Sys_MakeDir("rom/actor/0x%04X-%s/", actorID, name);
			string = true;
			write = true;
		}
		
		if (StrEndCase(list.item[i], ".zovl") || StrEndCase(list.item[i], ".cfg")) {
			file = HeapPrint("rom/actor/0x%04X-%s/%s", actorID, name, list.item[i]);
			Sys_MakeDir("rom/actor/0x%04X-%s/", actorID, name);
			write = true;
			if (StrEndCase(list.item[i], ".cfg"))
				string = true;
		}
		
		if (objectID > 0 && StrEndCase(list.item[i], ".zobj")) {
			file = HeapPrint("rom/object/0x%04X-%s/%s", objectID, name, list.item[i]);
			Sys_MakeDir("rom/object/0x%04X-%s/", objectID, name);
			write = true;
		}
		
		if (write == false)
			continue;
		
		printf_info("Import: [%s]", file);
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
	Size cfgSize;
	char* sct;
	
	if (!Sys_Stat(item)) return;
	
	pkg = zip_open(item, 0, 'r');
	
	zip_entry_open(pkg, "package.cfg");
	if (zip_entry_read(pkg, (void*)&cfg, &cfgSize) < 0) {
		zip_close(pkg);
		
		return;
	}
	zip_entry_close(pkg);
	
	MemFile_LoadFile_String(sDepList, "tools/actor-object-deb.cfg");
	
	s32 lnum = LineNum(cfg);
	char* line = cfg;
	
	for (s32 i = 0; i< lnum; i++, line = Line(line, 1)) {
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
		
		if (*line == ' ' || *line == '\t') line++;
		
		if (*line != '[')
			continue;
		
		for (;; j++) {
			if (j >= ArrayCount(str))
				j = -1;
			if (!memcmp(cfg, str[j], strlen(str[j])))
				break;
		}
		
		if (j < 0)
			continue;
		
		line = Line(line, 1);
		
		Log("Func %s", str[j]);
		
		if (func[j])
			func[j](pkg, line);
		
		Free(sct);
	}
	
	MemFile_Free(sDepList);
	Free(cfg);
	zip_close(pkg);
}