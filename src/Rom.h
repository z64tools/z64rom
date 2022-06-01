#ifndef __Z64_ROM_H__
#define __Z64_ROM_H__

#include <ExtLib.h>

struct Rom;
struct RomFile;

typedef struct PatchNode {
	struct PatchNode* prev;
	struct PatchNode* next;
	u32  start;
	u32  end;
	char source[64];
} PatchNode;

void FileSys_Path(const char* fmt, ...);
char* FileSys_File(const char* str);
char* FileSys_FindFile(const char* str);

void Rom_New(struct Rom* rom, char* romName);
void Rom_Free(struct Rom* rom);
void Rom_Dump(struct Rom* rom);
void Rom_Build(struct Rom* rom);

void Rom_DeleteUnusedContent(s32 romType);

void Rom_Debug_ActorEntry(struct Rom* rom, u32 id);
void Rom_Debug_DmaEntry(struct Rom* rom, u32 id);
void Rom_Debug_SceneEntry(struct Rom* rom, u32 id);

void Rom_ItemListDir(ItemList* list, bool isNum, bool isDir);
void Rom_ItemList(ItemList* list, const char* path, bool isNum, ListFlag flags);
s32 Rom_Extract(MemFile* mem, struct RomFile rom, char* name);

void AudioOnly_Dump(struct Rom* rom);
void AudioOnly_Build(struct Rom* rom);

#endif