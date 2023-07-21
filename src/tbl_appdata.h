#ifndef APPDATA_TABLE
#define PLEASE_THE_IDE
#define APPDATA_TABLE(enum, path, fmt, default) 0
int wow[] = {
#endif

APPDATA_TABLE(APPDATA_BUILD_TYPE, "%s.build_type", "%s", "-1"),
APPDATA_TABLE(APPDATA_YAZ_HEADER, "%s.yaz_header", "%s", "-1"),
APPDATA_TABLE(APPDATA_CCDEFINE, "%s.ccdefine", "\"\"", ""),
APPDATA_TABLE(APPDATA_LANG, "lang", "\"%s\"", "lang_en"),

#ifdef PLEASE_THE_IDE
};
#endif

#undef APPDATA_TABLE