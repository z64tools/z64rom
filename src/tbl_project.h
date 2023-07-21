#ifndef PROJECT_TABLE
#define PLEASE_THE_IDE
#define PROJECT_TABLE(enum, item, old_item, fmt, default) 0
int wow[] = {
#endif

PROJECT_TABLE(BASEROM,       "project.baserom",      "z_baserom",       "\"%s\"",              ""),
PROJECT_TABLE(BUILDROM,      "project.buildrom",     "z_buildrom",      "\"%s\"",              "build"),
PROJECT_TABLE(VANILLA,       "project.vanilla",      "z_vanilla",       "\"%s\"",              ".vanilla"),
PROJECT_TABLE(CHECK_UPDATES, "project.check_update", "z_updates",       "%s",                  "true"),
PROJECT_TABLE(PROJECT64,     "project.project64",    "project64",       "\"%s\"",              ""),
PROJECT_TABLE(BASEWAD,       "project.basewad",      "vc_basewad",      "\"%s\"",              ""),
PROJECT_TABLE(DOLPHIN,       "project.dolphin",      "vc_dolphin",      "\"%s\"",              ""),
PROJECT_TABLE(SUFFIX_REL,    "project.suffix[0]",    NULL,              "\"%s\"",              "-release"),
PROJECT_TABLE(SUFFIX_DEV,    "project.suffix[1]",    NULL,              "\"%s\"",              "-dev"),
PROJECT_TABLE(YAZ_HEADER,    "project.yaz_header",   NULL,              "%s",                  "true"),

PROJECT_TABLE(GCC_GCC,       "gcc.gcc_flags",        NULL,              "\"%s\"",              ""),
PROJECT_TABLE(GCC_MAIN,      "gcc.main_flags",       NULL,              "\"\"\"\n%s\n\t\"\"\"",
	"\t\t-c -G0 -Os -march=vr4300 -mabi=32 -mips3 \\\n"
	"\t\t-mno-explicit-relocs -mno-check-zero-division -mno-memcpy \\\n"
	"\t\t-fno-common -fno-reorder-blocks -fno-optimize-sibling-calls \\\n"
	"\t\t-Wall -Wno-builtin-declaration-mismatch -Wno-strict-aliasing -Wno-missing-braces \\\n"
	"\t\t-Isrc/lib_user -Iinclude/z64hdr/include -Iinclude/z64hdr/oot_mq_debug -Iinclude/\\"),
PROJECT_TABLE(GCC_ACTOR,     "gcc.actor_flags",      NULL,              "\"%s\"",              ""),
PROJECT_TABLE(GCC_CODE,      "gcc.code_flags",       NULL,              "\"%s\"",              "-mno-gpopt -fomit-frame-pointer"),
PROJECT_TABLE(GCC_KALEIDO,   "gcc.kaleido_flags",    NULL,              "\"%s\"",              ""),
PROJECT_TABLE(GCC_STATE,     "gcc.state_flags",      NULL,              "\"%s\"",              ""),

PROJECT_TABLE(LD_BASE,       "ld.base_flags",        NULL,              "\"%s\"",              "-Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/"),
PROJECT_TABLE(LD_CODE,       "ld.code_flags",        NULL,              "\"%s\"",              "-T z64hdr.ld -T z_lib_user.ld -T z_object_user.ld --emit-relocs"),
PROJECT_TABLE(LD_SCENE,      "ld.scene_flags",       NULL,              "\"%s\"",              "-T z64hdr_actor.ld --emit-relocs"),
PROJECT_TABLE(LD_ULIB,       "ld.ulib_flags",        NULL,              "\"%s\"",              "-T ulib_linker.ld -T z_object_user.ld --emit-relocs"),

#ifdef PLEASE_THE_IDE
};
#endif

#undef PROJECT_TABLE