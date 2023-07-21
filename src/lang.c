#include <ext_lib.h>
#include "lang.h"

z64lang gLang;

#define Lang_Register(item) do { \
			if (Toml_Var(&lang_toml, #item)) \
			{ gLang.item = Toml_GetStr(&lang_toml, #item); } \
			else \
			{ gLang.item = Toml_GetStr(&base_lang_toml, #item); } \
} while (false)

#define Lang_RegisterArr(item, id) do { \
			if (Toml_Var(&lang_toml, #item "[%d]", id)) \
			{ gLang.item[id] = Toml_GetStr(&lang_toml, #item "[%d]", id); } \
			else \
			{ gLang.item[id] = Toml_GetStr(&base_lang_toml, #item "[%d]", id); } \
} while (false)

void Lang_Init(const char* langFile) {
	Toml lang_toml = Toml_New();
	Toml base_lang_toml = Toml_New();
	
	if (sys_stat(x_fmt("tools/%s.toml", langFile)))
		Toml_Load(&lang_toml, x_fmt("tools/%s.toml", langFile));
	Toml_Load(&base_lang_toml, "tools/lang_en.toml");
	
	Lang_Register(success);
	Lang_Register(rm);
	Lang_Register(err_fail);
	Lang_Register(err_create);
	Lang_Register(err_load);
	Lang_Register(err_mv);
	Lang_Register(err_cp);
	Lang_Register(err_rm);
	Lang_Register(err_missing);
	Lang_Register(press_enter);
	for (var_t i = 0; i < LANG_HELP_MAX; i++)
		Lang_RegisterArr(help, i);
	Lang_Register(err_lang_set);
	Lang_Register(info_lang_set);
	
	Lang_Register(main.reconfig_force);
	Lang_Register(main.reconfig_ok);
	Lang_Register(main.rcl_title);
	Lang_Register(main.search_found);
	Lang_Register(main.search_index);
	Lang_Register(main.manifest_fix);
	Lang_Register(main.unk_arg);
	Lang_Register(main.unk_fmt);
	Lang_Register(main.err_too_many_roms);
	Lang_Register(main.err_custom_vanilla);
	Lang_Register(main.warn_update_retry);
	
	Lang_Register(rom.baserom);
	Lang_Register(rom.buildrom);
	Lang_Register(rom.err_target_full);
	for (var_t i = 0; i < LANG_TARGET_MAX; i++)
		Lang_RegisterArr(rom.target, i);
	
	Lang_Register(rom.info_patch_file);
	Lang_Register(rom.info_dma_free);
	Lang_Register(rom.info_dma_left);
	Lang_Register(rom.info_dma_entries);
	Lang_Register(rom.info_dma_slot_kb);
	Lang_Register(rom.info_dma_slot_mb);
	Lang_Register(rom.info_compress_rate);
	Lang_Register(rom.warn_orphan_folders);
	Lang_Register(rom.warn_not_oot_mq_debug);
	Lang_Register(rom.err_hook_unk_symbol);
	Lang_Register(rom.err_hook_fail_offset);
	Lang_Register(rom.err_hook_not_code_offset);
	Lang_Register(rom.err_custom_dma_table_index);
	Lang_Register(rom.err_custom_dma_reserved);
	Lang_Register(rom.err_ext_tbl);
	Lang_Register(rom.err_ext_alloc);
	Lang_Register(rom.err_trans_id);
	Lang_Register(rom.err_trans_nm);
	Lang_Register(rom.err_ulib_size);
	Lang_Register(rom.err_room_type_mismatch);
	
	Lang_Register(audio.err_tbl_write_oos);
	Lang_Register(audio.err_env_mismatch);
	Lang_Register(audio.err_empty_sample);
	Lang_Register(audio.err_missing_name);
	Lang_Register(audio.err_missing_samprt);
	Lang_Register(audio.err_bad_sample_reference);
	Lang_Register(audio.err_unk_medium);
	Lang_Register(audio.err_unk_seqplr);
	Lang_Register(audio.warn_missing_loopbook);
	Lang_Register(audio.warn_max_16_env);
	Lang_Register(audio.warn_inst_bad_splits);
	Lang_Register(audio.warn_inst_fix_splits);
	
	Lang_Register(patch.err_bin_space_limit);
	Lang_Register(patch.err_patch_space_limit);
	Lang_Register(patch.err_missing_bin_file);
	Lang_Register(patch.err_ci_not_supported);
	Lang_Register(patch.err_unk_texture_frmt);
	Lang_Register(patch.err_arg_fail);
	Lang_Register(patch.warn_texture_res_mismatch);
	Lang_Register(patch.warn_patch_mod_file);
	Lang_Register(patch.warn_bin_overwrite);
	
	Lang_Register(make.info_make_ok);
	Lang_Register(make.err_sct_missing_var);
	Lang_Register(make.err_sct_missing_sh);
	Lang_Register(make.err_sct_variable);
	Lang_Register(make.err_missing_item);
	Lang_Register(make.err_whitespace);
	Lang_Register(make.err_hidden_symbol);
	
	for (var_t i = 0; i < 2; i++)
		Lang_RegisterArr(setup.closedialog_download_mode, i);
	Lang_Register(setup.closedialog_switch);
	for (var_t i = 0; i < 3; i++)
		Lang_RegisterArr(setup.closedialog_try_again, i);
	Lang_Register(setup.filedialog_invalid);
	Lang_Register(setup.filedialog_copy_fail);
	Lang_Register(setup.filedialog_copy);
	Lang_Register(setup.info_chill);
	Lang_Register(setup.info_prompt_auto);
	Lang_Register(setup.info_get_file);
	Lang_Register(setup.info_all_ok);
	Lang_Register(setup.warn_hash_mismatch);
	Lang_Register(setup.warn_download_failed);
	Lang_Register(setup.err_missing_components);
	Lang_Register(setup.err_zip);
	Lang_Register(setup.validating);
	Lang_Register(setup.extracting);
	Lang_Register(setup.moving);
	Lang_Register(setup.downloading);
	Lang_Register(setup.update_available);
	Lang_Register(setup.update_prompt);
	Lang_Register(setup.update_api_limit);
	
	Lang_Register(package.err_new_obj_characters);
	Lang_Register(package.recommended_id);
	Lang_Register(package.prompt_object);
	Lang_Register(package.prompt_actor_name);
	Lang_Register(package.prompt_actor_index);
	Lang_Register(package.prompt_effect_name);
	Lang_Register(package.prompt_effect_index);
	Lang_Register(package.prompt_seq);
	Lang_Register(package.prompt_font);
	Lang_Register(package.prompt_sample_name);
	Lang_Register(package.prompt_sample_sfx);
	Lang_Register(package.prompt_sample_nrm);
	Lang_Register(package.prompt_sample_inh);
	Lang_Register(package.prompt_sample_prc);
	Lang_Register(package.selected_id);
	Lang_Register(package.new_seq);
	Lang_Register(package.new_actor);
	Lang_Register(package.new_effect);
	
	Toml_Free(&lang_toml);
	Toml_Free(&base_lang_toml);
	
	for (var_t i = 0; i < sizeof(gLang) / sizeof(char*); i++) {
		if (((char**)&gLang)[i] == NULL) {
			warn("Lang Error: %d == NULL", i * sizeof(char*));
			errr("PrevString: %s", ((char**)&gLang)[i - 1]);
		}
	}
}

onexit_func_t Lang_Dest() {
	for (var_t i = 0; i < sizeof(gLang) / sizeof(char*); i++)
		delete(((char**)&gLang)[i]);
}
