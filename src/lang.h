#ifndef LANG_H
#define LANG_H

#define ARG_TABLE(en, num, arg) __OBFS_PRIVATE_ ## en
enum {
#include "tbl_args.h"
	LANG_HELP_MAX,
};

enum {
	LANG_ACTOR,
	LANG_EFFECT,
	LANG_OBJECT,
	LANG_SCENE,
	LANG_STATE,
	LANG_KALEIDO,
	LANG_SKYBOX,
	LANG_STATIC,
	LANG_DMA,
	
	LANG_FONT,
	LANG_SEQ,
	LANG_SMPL,
	LANG_SFX,
	
	LANG_FIX_FONT,
	LANG_FIX_SMPL,
	
	LANG_COMPRESS,
	LANG_INJECT,
	
	LANG_TARGET_MAX,
};

typedef struct {
	char* success;
	char* rm;
	char* err_fail;
	char* err_create;
	char* err_load;
	char* err_mv;
	char* err_cp;
	char* err_rm;
	char* err_missing;
	char* press_enter;
	char* help[LANG_HELP_MAX];
	
	char* err_lang_set;
	char* info_lang_set;
	
	struct {
		char* reconfig_force;
		char* reconfig_ok;
		
		char* rcl_title;
		
		char* search_found;
		char* search_index;
		
		char* manifest_fix;
		
		char* unk_arg;
		char* unk_fmt;
		
		char* err_too_many_roms;
		char* err_custom_vanilla;
		
		char* warn_update_retry;
	} main;
	
	struct {
		char* baserom;
		char* buildrom;
		char* err_target_full;
		char* target[LANG_TARGET_MAX];
		
		char* info_patch_file;
		char* info_dma_free;
		char* info_dma_left;
		char* info_dma_entries;
		char* info_dma_slot_kb;
		char* info_dma_slot_mb;
		
		char* info_compress_rate;
		
		char* warn_orphan_folders;
		char* warn_not_oot_mq_debug;
		
		char* err_hook_unk_symbol;
		char* err_hook_fail_offset;
		char* err_hook_not_code_offset;
		
		char* err_custom_dma_table_index;
		char* err_custom_dma_reserved;
		
		char* err_ext_tbl;
		char* err_ext_alloc;
		
		char* err_trans_id;
		char* err_trans_nm;
		
		char* err_ulib_size;
		
		char* err_room_type_mismatch;
		
	} rom;
	
	struct {
		char* err_tbl_write_oos;
		char* err_env_mismatch;
		char* err_empty_sample;
		
		char* err_missing_name;
		char* err_missing_samprt;
		
		char* err_bad_sample_reference;
		
		char* err_unk_medium;
		char* err_unk_seqplr;
		
		char* warn_missing_loopbook;
		char* warn_max_16_env;
		char* warn_inst_bad_splits;
		char* warn_inst_fix_splits;
	} audio;
	
	struct {
		char* err_bin_space_limit;
		char* err_patch_space_limit;
		char* err_missing_bin_file;
		char* err_ci_not_supported;
		char* err_unk_texture_frmt;
		char* err_arg_fail;
		
		char* warn_texture_res_mismatch;
		char* warn_patch_mod_file;
		char* warn_bin_overwrite;
	} patch;
	
	struct {
		char* info_make_ok;
		char* err_sct_missing_var;
		char* err_sct_missing_sh;
		char* err_sct_variable;
		char* err_missing_item;
		char* err_whitespace;
		char* err_hidden_symbol;
	} make;
	
	struct {
		char* closedialog_download_mode[2];
		char* closedialog_switch;
		char* closedialog_try_again[3];
		
		char* filedialog_invalid;
		char* filedialog_copy_fail;
		char* filedialog_copy;
		
		char* info_chill;
		char* info_prompt_auto;
		char* info_get_file;
		char* info_all_ok;
		
		char* warn_hash_mismatch;
		char* warn_download_failed;
		
		char* err_missing_components;
		char* err_zip;
		
		char* validating;
		char* extracting;
		char* moving;
		char* downloading;
		
		char* update_available;
		char* update_prompt;
		char* update_api_limit;
	} setup;
	
	struct {
		char* err_new_obj_characters;
		
		char* recommended_id;
		char* prompt_object;
		char* prompt_actor_name;
		char* prompt_actor_index;
		char* prompt_effect_name;
		char* prompt_effect_index;
		char* prompt_seq;
		char* prompt_font;
		char* prompt_sample_name;
		char* prompt_sample_sfx;
		char* prompt_sample_nrm;
		char* prompt_sample_inh;
		char* prompt_sample_prc;
		
		char* selected_id;
		char* new_seq;
		char* new_actor;
		char* new_effect;
	} package;
} z64lang;

extern z64lang gLang;

void Lang_Init(const char* langFile);

#endif