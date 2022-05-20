#include "z64rom.h"
#include <zip.h>

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

static s32 Window_Index(Terminal* terminal, void* a1, void* a2, s32 key) {
	if (key == '\n')
		return 1;
	
	return 0;
}

void Package_Actor(struct zip_t* pkg, char* cfg) {
	ItemList list = ItemList_Initialize();
	char* file = Config_GetVariable(cfg, "file");
	char* name = Config_GetVariable(cfg, "name");
	s32 actorID = 0;
	s32 objectID = 0;
	char buffer[512] = { 0 };
	
	if (file == NULL) {
		Config_GetArray(&list, cfg, "files");
		
		for (s32 i = 0; i < list.num; i++)
			if (StrEndCase(list.item[i], ".zobj"))
				objectID = true;
	}
	
	Terminal_Window(Window_Index, 0, 0);
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
	
	Free(cfg);
	zip_close(pkg);
}