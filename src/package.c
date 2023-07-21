#include "z64rom.h"
#include <ext_zip.h>

Memfile __actor_object_list;
Memfile* sDepList = &__actor_object_list;

static s32 sSkip;
static u16 sActorID;
static u16 sEffectID;
static u16 sObjectID;
static Memfile sDepLookUp;
static List sActorList;
static List sEffectList;
static List sObjectList;

static void Package_InitCtx() {
	List srcActor = List_New();
	List srcEffect = List_New();
	List srcObject = List_New();
	
	sActorID = 0xFFFF;
	sEffectID = 0xFFFF;
	sObjectID = 0xFFFF;
	
	Memfile_LoadStr(&sDepLookUp, "tools/actor-object-deb.toml");
	
	osLog("Listing ROM");
	Rom_ItemList(&sActorList, "rom/actor/", SORT_NUMERICAL, LIST_FOLDERS);
	Rom_ItemList(&sEffectList, "rom/effect/", SORT_NUMERICAL, LIST_FOLDERS);
	Rom_ItemList(&sObjectList, "rom/object/", SORT_NUMERICAL, LIST_FOLDERS);
	
	osLog("Listing SRC");
	List_Walk(&srcActor, "src/actor/", 0, LIST_FOLDERS | LIST_RELATIVE);
	List_Walk(&srcEffect, "src/effect/", 0, LIST_FOLDERS | LIST_RELATIVE);
	List_Walk(&srcObject, "src/object/", 0, LIST_FOLDERS | LIST_RELATIVE);
	List_SortSlot(&srcActor, true);
	List_SortSlot(&srcEffect, true);
	List_SortSlot(&srcObject, true);
	
	for (int i = 0; i < srcActor.num; i++) {
		if (i < sActorList.num) {
			if (sActorList.item[i] == NULL && srcActor.item[i])
				sActorList.item[i] = fmt("src/actor/%s", srcActor.item[i]);
		}
	}
	
	for (int i = 0; i < srcEffect.num; i++) {
		if (i < sEffectList.num) {
			if (sEffectList.item[i] == NULL && srcEffect.item[i])
				sEffectList.item[i] = fmt("src/effect/%s", srcEffect.item[i]);
		}
	}
	
	for (int i = 0; i < srcObject.num; i++) {
		if (i < sObjectList.num) {
			if (sObjectList.item[i] == NULL && srcObject.item[i])
				sObjectList.item[i] = fmt("src/object/%s", srcObject.item[i]);
		}
	}
	
	List_Free(&srcActor);
	List_Free(&srcEffect);
	List_Free(&srcObject);
	
	osLog("Wow");
}

static void Package_FreeCtx() {
	Memfile_Free(&sDepLookUp);
	List_Free(&sActorList);
	List_Free(&sEffectList);
	List_Free(&sObjectList);
}

static void Package_GetActorID(const char* name, bool getFree) {
	const char* input;
	
	if (sActorID != 0xFFFF)
		return;
	
	if (getFree)
		goto assign_free;
	
	info(gLang.package.prompt_actor_index, name);
	
	while (true) {
		input = cli_gets();
		
		if (vldt_hex(input)) {
			sActorID = shex(input);
			
			return;
		}
		
		if (!stricmp(input, "free") || !stricmp(input, "\"free\"")) {
			assign_free:
			for (int i = 1;; i++) {
				if (i < sActorList.num) {
					if (i == 0x15)
						continue;
					
					if (sActorList.item[i] == NULL) {
						sActorList.item[i] = strdup("wow");
						sActorID = i;
						break;
					}
				} else {
					sActorID = i;
					break;
				}
			}
			if (getFree)
				return;
			
			cli_clearln(2);
			
			info(gLang.package.selected_id, sActorID);
			
			return;
		}
		
		if (!stricmp(input, "skip") || !stricmp(input, "\"skip\"")) {
			sSkip = true;
			
			return;
		}
		
		cli_clearln(2);
	}
}

static void Package_GetEffectID(const char* name, bool getFree) {
	const char* input;
	
	if (sEffectID != 0xFFFF)
		return;
	
	if (getFree)
		goto assign_free;
	
	info(gLang.package.prompt_effect_index, name);
	
	while (true) {
		input = cli_gets();
		
		if (vldt_hex(input)) {
			sEffectID = shex(input);
			
			return;
		}
		
		if (!stricmp(input, "free") || !stricmp(input, "\"free\"")) {
			assign_free:
			for (int i = 1;; i++) {
				if (i < sEffectList.num) {
					if (i == 0x15)
						continue;
					
					if (sEffectList.item[i] == NULL) {
						sEffectList.item[i] = strdup("wow");
						sEffectID = i;
						break;
					}
				} else {
					sEffectID = i;
					break;
				}
			}
			if (getFree)
				return;
			
			cli_clearln(2);
			
			info(gLang.package.selected_id, sEffectID);
			
			return;
		}
		
		if (!stricmp(input, "skip") || !stricmp(input, "\"skip\"")) {
			sSkip = true;
			
			return;
		}
		
		cli_clearln(2);
	}
}

static void Package_GetObjectID(const char* name, bool getFree) {
	const char* input;
	
	if (sObjectID != 0xFFFF)
		return;
	
	if (getFree)
		goto assign_free;
	
	if (sActorID != 0xFFFF) {
		if (Ini_Var(sDepLookUp.str, x_fmt("0x%04X", sActorID))) {
			List list = List_New();
			
			Ini_GetArr(&sDepLookUp, x_fmt("0x%04X", sActorID), &list);
			
			if (list.num == 1 && strcmp(list.item[0], "None"))
				info(gLang.package.recommended_id, list.item[0]);
			
			else
				info(gLang.package.recommended_id, "free");
			
			List_Free(&list);
		}
	}
	
	info(gLang.package.prompt_object, name);
	
	while (true) {
		input = cli_gets();
		
		if (vldt_hex(input)) {
			sObjectID = shex(input);
			
			return;
		}
		
		if (!stricmp(input, "free") || !stricmp(input, "\"free\"")) {
			assign_free:
			for (int i = 1;; i++) {
				if (i < sObjectList.num) {
					if (sObjectList.item[i] == NULL) {
						sObjectList.item[i] = strdup("wow");
						sObjectID = i;
						break;
					}
				} else {
					sObjectID = i;
					break;
				}
			}
			if (getFree)
				return;
			
			cli_clearln(2);
			
			info(gLang.package.selected_id, sObjectID);
			
			return;
		}
		
		if (!stricmp(input, "skip") || !stricmp(input, "\"skip\"")) {
			sSkip = true;
			
			return;
		}
		
		cli_clearln(2);
	}
}

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

typedef struct ListNode {
	struct ListNode* prev;
	struct ListNode* next;
	const char*      name;
	List list;
} ListNode;

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

static s32 ValidateName(char* name) {
	u32 fail = false;
	
	if (strlen(name) < 3)
		return -1;
	
	for (int i = 0; i < strlen(name); i++) {
		if (!isgraph(name[i]) || (ispunct(name[i]) && name[i] != '_') || (i == 0 && !isalpha(name[i]))) {
			fail = true;
			
			if (!isgraph(name[i]))
				name[i] = '_';
			
			strinsat(&name[i + 1], PRNT_RSET);
			strinsat(&name[i], PRNT_REDD);
			
			i += strlen(PRNT_YELW);
			i += strlen(PRNT_RSET);
		}
	}
	
	return fail;
}

void Template_NewActor(const char* argName, const char* aId, const char* oId) {
	char* name = new(char[1024]);
	const char* examples[] = {
		"EnDiamond",
		"EnOctorock",
		"EnZelda",
		"EnGanondorf",
		"EnBridge",
	};
	u32 rnd = (u32)(randf() * 15.0f) % ArrCount(examples);
	
	Package_InitCtx();
	
	if (argName && aId && oId) {
		strcpy(name, argName);
		
		if (ValidateName(name))
			errr(gLang.package.err_new_obj_characters, name);
		
		if (!strcmp(aId, "free"))
			Package_GetActorID(name, true);
		else
			sActorID = shex(aId);
		
		if (!strcmp(oId, "free"))
			Package_GetObjectID(name, true);
		else
			sObjectID = shex(oId);
	} else {
		info(gLang.package.prompt_actor_name, examples[rnd]);
		while (true) {
			strcpy(name, x_strunq(x_strdup(cli_gets())));
			
			switch (ValidateName(name)) {
				case 1:
					cli_clearln(2);
					
					warn(gLang.package.err_new_obj_characters, name);
					
					cli_getc();
					cli_clearln(4);
					continue;
					break;
				case -1:
					cli_clearln(2);
					continue;
					break;
			}
			
			break;
		}
		
		cli_clearln(3);
		
		Package_GetActorID(name, false);
		info_nl();
		Package_GetObjectID(name, false);
	}
	
	Memfile c = Memfile_New();
	Memfile h = Memfile_New();
	List d = List_New();
	
	if (!sys_stat("tools/ActorTemplate.c")) errr_align(gLang.err_missing, "tools/ActorTemplate.c");
	if (!sys_stat("tools/ActorTemplate.h")) errr_align(gLang.err_missing, "tools/ActorTemplate.h");
	
	Memfile_LoadStr(&c, "tools/ActorTemplate.c");
	Memfile_LoadStr(&h, "tools/ActorTemplate.h");
	
	Memfile_Realloc(&c, c.size * 16);
	Memfile_Realloc(&h, h.size * 16);
	
	c.size += strrep(c.str, "EnActor", name);
	c.size += strrep(c.str, "ActorTemplate", name);
	c.size += strrep(c.str, "[[ACTOR_ID_PLACEHOLDER]]", x_fmt(".id = 0x%04X,", sActorID));
	c.size += strrep(c.str, "[[OBJECT_ID_PLACEHOLDER]]", x_fmt(".objectId = 0x%04X,", sObjectID));
	h.size += strrep(h.str, "EnActor", name);
	h.size += strrep(h.str, "EN_ACTOR", x_enumify(name));
	
	fs_mkflag(true);
	fs_set("src/actor/0x%04X-%s/", sActorID, name);
	Memfile_SaveStr(&c, fs_item("%s.c", name));
	Memfile_SaveStr(&h, fs_item("%s.h", name));
	
	info_nl();
	info(gLang.package.new_actor, sActorID, name);
	
	delete(name);
	List_Free(&d);
	Memfile_Free(&c);
	Memfile_Free(&h);
	
	Package_FreeCtx();
	
	info(gLang.success);
	info_getc(gLang.press_enter);
}

void Template_NewEffect(const char* argName, const char* index) {
	char* name = new(char[1024]);
	const char* examples[] = {
		"VfxBlast",
		"VfxSmoke",
		"VfxDust",
	};
	u32 rnd = (u32)(randf() * 15.0f) % ArrCount(examples);
	
	Package_InitCtx();
	
	if (argName && index) {
		strcpy(name, argName);
		
		if (ValidateName(name))
			errr(gLang.package.err_new_obj_characters, name);
		
		if (!strcmp(index, "free"))
			Package_GetActorID(name, true);
		else
			sActorID = shex(index);
	} else {
		info(gLang.package.prompt_effect_name, examples[rnd]);
		
		while (true) {
			strcpy(name, x_strunq(x_strdup(cli_gets())));
			
			switch (ValidateName(name)) {
				case 1:
					cli_clearln(2);
					
					warn(gLang.package.err_new_obj_characters, name);
					
					cli_getc();
					cli_clearln(4);
					continue;
					break;
				case -1:
					cli_clearln(2);
					continue;
					break;
			}
			
			break;
		}
		
		cli_clearln(3);
		
		Package_GetEffectID(name, false);
	}
	
	Memfile c = Memfile_New();
	Memfile h = Memfile_New();
	List d = List_New();
	
	if (!sys_stat("tools/VfxTemplate.c")) errr_align(gLang.err_missing, "tools/VfxTemplate.c");
	if (!sys_stat("tools/VfxTemplate.h")) errr_align(gLang.err_missing, "tools/VfxTemplate.h");
	
	Memfile_LoadStr(&c, "tools/VfxTemplate.c");
	Memfile_LoadStr(&h, "tools/VfxTemplate.h");
	
	Memfile_Realloc(&c, c.size * 16);
	Memfile_Realloc(&h, h.size * 16);
	
	c.size += strrep(c.str, "VfxTemplate", name);
	c.size += strrep(c.str, "[[VFX_ID_PLACEHOLDER]]", x_fmt("0x%02X", sEffectID));
	h.size += strrep(h.str, "VfxTemplate", name);
	h.size += strrep(h.str, "VFX_TEMPLATE", x_enumify(name));
	
	fs_mkflag(true);
	fs_set("src/effect/0x%02X-%s/", sEffectID, name);
	Memfile_SaveStr(&c, fs_item("%s.c", name));
	Memfile_SaveStr(&h, fs_item("%s.h", name));
	
	info_nl();
	info(gLang.package.new_effect, sEffectID, name);
	
	delete(name);
	List_Free(&d);
	Memfile_Free(&c);
	Memfile_Free(&h);
	
	Package_FreeCtx();
	
	info(gLang.success);
	info_getc(gLang.press_enter);
}

void Template_NewSequence(const char* file) {
	u32 id;
	Memfile mem = Memfile_New();
	List list = List_New();
	
	Memfile_Alloc(&mem, 4096);
	
	info(gLang.package.prompt_seq);
	
	while (true) {
		const char* str = cli_gets();
		
		if (!vldt_hex(str))
			cli_clearln(2);
		
		else {
			id = shex(str);
			break;
		}
	}
	
	info(gLang.package.prompt_font);
	while (true) {
		const char* str = cli_gets();
		
		if (!vldt_hex(str))
			cli_clearln(2);
		
		else {
			List_Add(&list, x_fmt("0x%02X", shex(str)));
			Ini_WriteArr(&mem, "bank_id", &list, NO_QUOTES, NO_COMMENT);
			break;
		}
	}
	
	List_Free(&list); List_Add(&list, "allow_enemy_bgm");
	
	Ini_WriteStr(&mem, "medium_type", "cart", QUOTES, NO_COMMENT);
	Ini_WriteStr(&mem, "sequence_player", "bgm", QUOTES, NO_COMMENT);
	Ini_WriteArr(&mem, "sequence_flags", &list, QUOTES, NO_COMMENT);
	
	Ini_WriteTab(&mem, "seq64", NO_COMMENT);
	Ini_Fmt(&mem, "\t"); Ini_WriteInt(&mem, "master_volume", 88, NO_COMMENT);
	Ini_Fmt(&mem, "\t"); Ini_WriteBool(&mem, "flstudio", false, NO_COMMENT);
	Ini_Fmt(&mem, "\t"); Ini_WriteBool(&mem, "loop", true, NO_COMMENT);
	
	sys_mkdir("src/sound/sequence/0x%02X-%s/", id, x_basename(file));
	sys_cp(file, x_fmt("src/sound/sequence/0x%02X-%s/%s", id, x_basename(file), x_filename(file)));
	Memfile_SaveStr(&mem, x_fmt("src/sound/sequence/0x%02X-%s/config.toml", id, x_basename(file)));
	
	List_Free(&list);
	Memfile_Free(&mem);
	
	info(gLang.package.new_seq, id, x_basename(file));
	info_getc(gLang.press_enter);
	
	exit(0);
}

static void Template_NewSampleImpl(const char* file, const char* name, Memfile* mem, bool normalize, bool inherit, bool halfPrecision, bool gets) {
	const char* dest = "src/sound/sample";
	Toml toml = Toml_New();
	
	if (gets) {
		info(gLang.package.prompt_sample_name);
		name = cli_gets();
		
		info(gLang.package.prompt_sample_sfx);
		if (cli_yesno())
			dest = "src/sound/sfx";
		
		info(gLang.package.prompt_sample_nrm);
		if (cli_yesno())
			normalize = true;
		
		if (!strcmp(dest, "src/sound/sample")) {
			info(gLang.package.prompt_sample_inh);
			if (cli_yesno())
				inherit = true;
		}
		
		info(gLang.package.prompt_sample_prc);
		if (cli_yesno())
			halfPrecision = true;
	}
	
	Toml_SetVar(&toml, "normalize", "%s", normalize ? "true" : "false");
	Toml_SetVar(&toml, "inherit_vanilla", "%s", inherit ? "true" : "false");
	Toml_SetVar(&toml, "half_precision", "%s", halfPrecision ? "true" : "false");
	
	char* path = x_fmt("%s/%s", dest, name);
	
	if (sys_stat(path))
		sys_rmdir(path);
	
	sys_mkdir("%s/%s/", dest, name);
	
	if (!mem) {
		sys_cp(file, x_fmt("%s/%s/%s", dest, name, x_filename(file)));
	} else {
		Memfile_SaveBin(mem, x_fmt("%s/%s/%s", dest, name, x_filename(file)));
		info("save: %s", x_fmt("%s/%s/%s", dest, name, x_filename(file)));
	}
	
	Toml_Save(&toml, x_fmt("%s/%s/config.toml", dest, name));
	Toml_Free(&toml);
	
	if (gets) {
		void Make_SfxOnly(void);
		void Make_SfxEnum(void);
		Make_SfxOnly();
		Make_SfxEnum();
		
		info(gLang.success);
		info_getc(gLang.press_enter);
		
		exit(0);
	}
}

void Template_NewSample(const char* file) {
	Template_NewSampleImpl(file, NULL, NULL, true, true, false, true);
}

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

typedef enum {
	PKG_ACTOR_SRC,
	PKG_ACTOR_ROM,
	PKG_OBJECT_SRC,
	PKG_OBJECT_ROM,
	PKG_SAMPLE,
} PackageFileType;

void Package_Load(const char* fname) {
	Zip zip;
	Memfile file = Memfile_New();
	Toml package = Toml_New();
	
	if (!sys_stat(fname)) return;
	
	Package_InitCtx();
	
	Zip_Load(&zip, fname, ZIP_READ);
	
	if (Zip_ReadByName(&zip, "package.toml", &file))
		errr("Missing 'package.toml'!", fname);
	
	Toml_LoadMem(&package, file.str);
	
	const char* fmt = "Package:         %s";
	const char* argv[2] = {};
	int argc = 0;
	
	if (Toml_Var(&package, "author")) {
		fmt = x_fmt("%s\nAuthor:          %%s", fmt);
		argv[argc++] = x_strtrim(Toml_Var(&package, "author"), "\" \t");
	}
	
	if (Toml_Var(&package, "version")) {
		fmt = x_fmt("%s\nVersion:         %%s", fmt);
		argv[argc++] = x_strtrim(Toml_Var(&package, "version"), "\" \t");
	}
	
	info(fmt, fname, argv[0], argv[1]);
	
	nested(void, sample, (int i)) {
		Memfile mem = Memfile_New();
		const char* name = Toml_Var(&package, "sample[%d].name", i);
		const char* file = Toml_Var(&package, "sample[%d].file", i);
		
		if (!name) {
			warn(gLang.err_missing, x_fmt("sample[%d].name", i));
			return;
		}
		if (!file) {
			warn(gLang.err_missing, x_fmt("sample[%d].file", i));
			return;
		}
		
		name = x_strtrim(name, "\"");
		file = x_strtrim(file, "\"");
		
		if (Zip_ReadByName(&zip, file, &mem)) {
			warn(gLang.err_missing, file);
			return;
		}
		
		Template_NewSampleImpl(file, name, &mem, true, true, false, false);
		Memfile_Free(&mem);
	};
	
	int numSample = Toml_ArrCount(&package, "sample");
	for (int i = 0; i < numSample; i++)
		sample(i);
	
	Toml_Free(&package);
	Zip_Free(&zip);
	
	info(gLang.success);
	
	exit(0);
}

void Package_Pack() {
	u32 writtenItems = 0;
	
	ListNode* listHead = NULL;
	ListNode* node;
	Zip zip;
	
	if (sys_stat("package.zip"))
		sys_rm("package.zip");
	Zip_Load(&zip, "package.zip", ZIP_WRITE);
	
	while (true) {
		info("Item Name: " PRNT_GRAY "or \"done\" to save the package");
		const char* str = cli_gets();
		
		info_nl();
		
		if (!stricmp(str, "done") || !stricmp(str, "\"done\""))
			break;
		
		node = calloc(sizeof(*node));
		node->name = strdup(str);
		Node_Add(listHead, node);
		List_Alloc(&node->list, 32);
		
		info("Provide file or path for files to be packed:");
		str = cli_gets();
		
		cli_clearln(2);
		info_nl();
		
		if (sys_stat(str)) {
			Memfile mem = Memfile_New();
			List list = List_New();
			char* entryName;
			
			if (sys_isdir(str)) {
				fs_set(str);
				List_Walk(&list, str, -1, LIST_FILES | LIST_RELATIVE);
				
				for (int i = 0; i < list.num; i++) {
					entryName = x_fmt("%s/%s", node->name, list.item[i]);
					
					Memfile_LoadBin(&mem, fs_item(list.item[i]));
					Zip_Write(&zip, &mem, entryName);
					
					List_Add(&node->list, entryName);
					Memfile_Free(&mem);
					writtenItems++;
				}
			} else {
				entryName = x_fmt("%s/%s", node->name, str);
				
				Memfile_LoadBin(&mem, str);
				Zip_Write(&zip, &mem, entryName);
				
				List_Add(&node->list, entryName);
				Memfile_Free(&mem);
			}
		} else {
			cli_clearln(3);
			warn("No file or path... Try again!\a");
			sys_sleep(2.0);
			cli_clearln(2);
		}
	}
	
	Zip_Free(&zip);
	
	if (writtenItems == 0)
		sys_rm("package.zip");
}
