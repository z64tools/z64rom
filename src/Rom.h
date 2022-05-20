#ifndef __Z64_ROM_H__
#define __Z64_ROM_H__

#include <ExtLib.h>

struct Rom;
struct RomFile;

void Rom_New(struct Rom* rom, char* romName);
void Rom_Free(struct Rom* rom);
void Rom_Dump(struct Rom* rom);
void Rom_Build(struct Rom* rom);

void Rom_Debug_ActorEntry(struct Rom* rom, u32 id);
void Rom_Debug_DmaEntry(struct Rom* rom, u32 id);
void Rom_Debug_SceneEntry(struct Rom* rom, u32 id);

void Rom_ItemList(ItemList* list, bool isNum, bool isDir);
void Rom_ItemList_NDIR(ItemList* list, const char* path, bool isNum, ListFlags flags);
s32 Rom_Extract(MemFile* mem, struct RomFile rom, char* name);

#endif