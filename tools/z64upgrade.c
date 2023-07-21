#include <ext_lib.h>
#include <ext_zip.h>

const char* gToolName = PRNT_BLUE "z64upgrade " PRNT_GRAY "1.0.1";
const char* gVersion;

const char* gTool =
#ifdef _WIN32
	"tools\\wget.exe"
#else
	"wget"
#endif
;

static void SetWordDir(void) {
	char* newWorkDir = x_strdup(sys_appdir());
	
	strstr(newWorkDir, "tools/")[0] = '\0';
	sys_setworkdir(newWorkDir);
	osAssert(sys_stat("z64rom.exe"));
}

static s32 ZipExtractCallback(const char* name, f32 prcnt) {
	if (sys_stat(name))
		return 1;
	
	sys_mkdir(x_path(name));
	info_align("New", name);
	
	return 0;
}

static void CreateUpdatePackage(Zip* prev) {
	info("cp info.cfg");
	sys_cp("../project/tools/info.cfg", "tools/info.cfg");
	
	Zip* new = new(Zip);
	FILE* nfo = fopen("tools/info.cfg", "a");
	u32 entries;
	char* remFileList = "";
	
	if (!nfo)
		errr("Could not fopen [tools/info]");
	
	info("remove app_win32.zip");
	sys_rm("app_win32.zip");
	
	info("open new zip");
	if (!Zip_Load(new, "app_win32.zip", ZIP_WRITE))
		errr("Could not open [app_win32.zip]");
	info("open updt zip");
	if (!Zip_Load(prev, "update.zip", ZIP_READ))
		errr("Could not open [update.zip]");
	entries = Zip_GetEntryNum(prev);
	
	fprintf(nfo, "[%s]\n    new = [ \"z64rom.exe\", \"tools/lang_en.toml\"", gVersion);
	
	for (int i = 0; i < entries; i++) {
		Memfile mem = Memfile_New();
		Memfile nem = Memfile_New();
		
		info_prog("entry", i + 1, entries);
		
		osLog("Load Index %d", i);
		if (Zip_ReadByID(prev, i, &mem))
			errr("Zip Error!");
		
		if (!mem.data) {
			Memfile_Free(&mem);
			
			continue;
		}
		
		if (
			strend(mem.info.name, "z64rom.exe") ||
			strend(mem.info.name, ".wget-hsts") ||
			strend(mem.info.name, ".update-check")
		) {
			Memfile_Free(&mem);
			continue;
		}
		
		if (sys_stat(mem.info.name)) {
			bool isString = false;
			
			osLog("replace entry: [%s]", mem.info.name);
			
			if (striend(mem.info.name, ".c") ||
				striend(mem.info.name, ".h") ||
				striend(mem.info.name, ".cfg") ||
				striend(mem.info.name, ".toml") ||
				striend(mem.info.name, ".txt") ||
				striend(mem.info.name, ".gzi") ||
				striend(mem.info.name, ".xml"))
				isString = true;
			
			Memfile_LoadBin(&nem, mem.info.name);
			
			if (isString) {
				u8* point;
				
				for (var_t t = 0; t < 2; t++) {
					Memfile* mf[] = {
						&mem, &nem
					};
					const char* name[] = {
						"New", "Prev"
					};
					
					osLog("Normalize Text [%s]", name[t]);
					while ((point = memmem(mf[t]->str, mf[t]->size, "\x0D\x0A", 2))) {
						uaddr_t left = mf[t]->size - (point - mf[t]->cast.u8);
						
						memmove(point, point + 1, left - 1);
						mf[t]->size--;
					}
					osLog("OK");
				}
			}
			
			osLog("Calculate Checksums");
			Hash pfile = HashMem(mem.data, mem.size);
			Hash nfile = HashMem(nem.data, nem.size);
			
			osLog("Compare");
			if (HashCmp(&pfile, &nfile))
				fprintf(nfo, ", \"%s\"", mem.info.name),
				osLog("" PRNT_REDD "Different!" PRNT_RSET);
			
		} else {
			info("remove entry: [%s]", mem.info.name);
			if (strlen(remFileList) == 0)
				remFileList = x_fmt(" \"%s\"", mem.info.name);
			
			else
				remFileList = x_fmt("%s, \"%s\"", remFileList, mem.info.name);
		}
		
		osLog("Clean");
		Memfile_Free(&mem);
		Memfile_Free(&nem);
	}
	
	info("write rm list");
	fprintf(nfo, " ]\n");
	fprintf(nfo, "    rem = [%s ]\n", remFileList);
	fprintf(nfo, "\n");
	fclose(nfo);
	
	List list = List_New();
	
	info("list files");
	List_Walk(&list, "", -1, LIST_FILES);
	
	for (int i = 0; i < list.num; i++) {
		Memfile mem = Memfile_New();
		
		if (
			strend(list.item[i], ".z64") ||
			strend(list.item[i], ".zip") ||
			strstart(list.item[i], ".update_backup") ||
			strstart(list.item[i], ".backup") ||
			strend(list.item[i], ".wget-hsts") ||
			strend(list.item[i], ".update-check")
		) continue;
		
		info("write file: %s", list.item[i]);
		osAssert (!Memfile_LoadBin(&mem, list.item[i]));
		osAssert (!Zip_Write(new, &mem, list.item[i]));
		
		Memfile_Free(&mem);
	}
	
	info("done");
	Zip_Free(new);
	Zip_Free(prev);
	
	sys_cp("tools/info.cfg", "../project/tools/info.cfg");
}

static void BackupFile(const char* name) {
	char* dest = x_fmt(".backup/%s/%s", gVersion, name);
	
	sys_mkdir(x_path(dest));
	sys_cp(name, dest);
}

int main(int n, const char** arg) {
	Zip* z = new(Zip);
	Memfile* mem = new(Memfile);
	s32 narg;
	s32 version[3] = { };
	List files = List_New();
	
#ifndef _WIN32
	errr("Updater is not supported for Linux");
#endif
	
	List_Alloc(&files, 1024);
	info_title(gToolName, NULL);
	SetWordDir();
	
	sys_rm("tools/.wget-hsts");
	
	if (!(narg = strarg(arg, "version")))
		errr("z64upgrade can be only called by z64rom!");
	gVersion = x_strunq(arg[narg]);
	
	sscanf(gVersion, "%d.%d.%d", &version[0], &version[1], &version[2]);
	
	if (!sys_stat("update.zip")) {
		Memfile mem = Memfile_New();
		
		if (Memfile_Download(&mem, "https://github.com/z64tools/z64rom/releases/latest/download/app_win32.zip", "Downloading"))
			errr("Failed to retrieve update!");
		
		Memfile_SaveBin(&mem, "update.zip");
		Memfile_Free(&mem);
		
		cli_clearln(2);
	}
	
	if (strarg(arg, "pack")) {
		CreateUpdatePackage(z);
		delete(z);
		
		return 0;
	}
	
	sys_sleep(1.0f);
	
	osAssert(sys_stat("update.zip"));
	osAssert(Zip_Load(z, "update.zip", ZIP_READ));
	
	if (Zip_ReadByName(z, "tools/info.cfg", mem)) {
		Zip_Free(z);
		sys_rm("update.zip");
		
		errr("Update is missing [tools/info.cfg]!");
	}
	
	List sections = List_New();
	bool current = false;
	
	Ini_ListTabs(mem, &sections);
	
	for (int i = 0; i < sections.num; i++) {
		s32 nv[3] = {};
		List newList = List_New();
		List remList = List_New();
		
		if (!current) {
			u32 this;
			u32 that;
			
			sscanf(sections.item[i], "%d.%d.%d", &nv[0], &nv[1], &nv[2]);
			
			this = version[0] * 1000 + version[1] * 100 + version[2];
			that = nv[0] * 1000 + nv[1] * 100 + nv[2];
			
			if (that <= this)
				continue;
			
			current = true;
		}
		
		Ini_GotoTab(sections.item[i]);
		
		Ini_GetArr(mem, "new", &newList);
		Ini_GetArr(mem, "rem", &remList);
		
		if (Ini_GetError())
			errr("Update Error!");
		
		for (int k = 0; k < newList.num; k++) {
			Memfile mem = Memfile_New();
			bool escape = false;
			
			if (strend(newList.item[k], "z64upgrade.exe"))
				continue;
			
			osLog("REPLACE: %s", newList.item[k]);
			
			for (int j = 0; j < files.num; j++) {
				osLog("%s", files.item[j]);
				if (!strcmp(files.item[j], newList.item[k])) {
					osLog("Escape!");
					
					escape = true;
					break;
				}
			}
			
			if (escape) continue;
			
			osLog("YES!");
			warn_align("Replace", newList.item[k]);
			BackupFile(newList.item[k]);
			
			if (Zip_ReadByName(z, newList.item[k], &mem))
				errr("Could not load entry [%s] from [update.zip]", newList.item[k]);
			
			if (Memfile_SaveBin(&mem, newList.item[k]))
				errr("Could not save file [%s]", newList.item[k]);
			List_Add(&files, newList.item[k]);
			
			Memfile_Free(&mem);
		}
		
		for (int k = 0; k < remList.num; k++) {
			bool escape = false;
			
			if (strlen(remList.item[k]) == 0)
				continue;
			
			for (int j = 0; j < files.num; j++) {
				if (!strcmp(files.item[j], remList.item[k])) {
					escape = true;
					break;
				}
			}
			if (escape) continue;
			
			warn_align("Remove", remList.item[k]);
			BackupFile(newList.item[k]);
			sys_rm(newList.item[k]);
			
			List_Add(&files, newList.item[k]);
		}
		
		List_Free(&newList);
		List_Free(&remList);
	}
	
	if (Zip_Dump(z, "", ZipExtractCallback))
		errr("Zip Failure!");
	List_Free(&files);
	Zip_Free(z);
	delete(z);
	
	if (sys_exe("z64rom.exe --reconfig --no-wait"))
		warn("Reconfig Failed!");
	
	info_nl();
	info("Update paused, make sure to check the list in case if your file has been replaced!");
	info_getc("Press enter to continue update!");
	sys_exed("z64rom.exe --post-update");
	
	return 0;
}
