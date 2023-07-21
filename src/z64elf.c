#include <ext_lib.h>
#define MIPS_ELF_PRIVATE
#include "z64elf.h"

typedef struct Elf64 {
	u8*         data;
	ElfHeader   hdr;
	ElfSection* sect;
	u32         num_sect;
} Elf64;

typedef enum {
	SYM_SYMBOL_TBL    = 2,
	SYM_STRING_TBL    = 3,
	SYM_RELOC_ENTRIES = 9,
	SYM_DYNA_SYM_TBL  = 11,
} SectionType;

static void Elf64_GetSections(Elf64* this) {
	this->num_sect = this->hdr.num_section;
	this->sect = new(ElfSection[this->num_sect]);
	
	memcpy(this->sect,
		&this->data[this->hdr.offset_section],
		sizeof(ElfSection[this->num_sect]));
}

Elf64* Elf64_New(u8* data) {
	Elf64* this = new(Elf64);
	const u8 ident[16] = {
		'\177', 'E', 'L', 'F', 0x01, 0x02, 0x01
	};
	
	memcpy(&this->hdr, data, sizeof(this->hdr));
	if (memcmp(this->hdr.ident, ident, 16))
		goto bailout;
	
	this->data = data;
	Elf64_GetSections(this);
	
	return this;
	bailout:
	delete(this);
	return NULL;
}

Elf64* Elf64_Load(const char* file) {
	Memfile mem = Memfile_New();
	
	Memfile_LoadBin(&mem, file);
	
	Elf64* this = Elf64_New(mem.data);
	
	mem.data = 0;
	Memfile_Free(&mem);
	
	return this;
}

static void* Elf64_GetSection(Elf64* this, int* i, SectionType type) {
	for (; *i < this->num_sect; *i = *i + 1) {
		ElfSection* sect = &this->sect[*i];
		
		if (sect->type == type)
			return sect;
	}
	
	return NULL;
}

static void* Elf64_Offset(Elf64* this, off_t offset) {
	return &this->data[offset];
}

static u32 Elf64_FindSymImpl(Elf64* this, const char* name, SectionType type) {
	int i = 0;
	
	ElfSection* strsect = Elf64_GetSection(this, &i, SYM_STRING_TBL);
	
	if (!strsect) return NULL_SYM;
	char* str = Elf64_Offset(this, strsect->offset);
	char* head = str;
	char* end = str + strsect->size;
	int name_id = 0;
	
	while (str < end && !streq(str, name)) str += strlen(str) + 1;
	if (str >= end) return NULL_SYM;
	else name_id = str - head;
	
	i = 0;
	while (true) {
		ElfSection* section = Elf64_GetSection(this, &i, type);
		if (!section) break;
		ElfSymbol* sym = Elf64_Offset(this, section->offset);
		int num = section->size / sizeof(ElfSymbol);
		
		for (; num; num--, sym++)
			if (sym->name_id == name_id)
				return sym->value;
		
		i++;
	}
	
	return NULL_SYM;
}

u32 Elf64_FindSym(Elf64* this, const char* name) {
	return Elf64_FindSymImpl(this, name, SYM_SYMBOL_TBL);
}

void Elf64_ReadSyms(Elf64* this, void* arg, void (*call)(void*, const char*, ElfSymbol*)) {
	int i = 0;
	ElfSection* strsect = Elf64_GetSection(this, &i, SYM_STRING_TBL);
	char* str = Elf64_Offset(this, strsect->offset);
	char* head = str;
	
	if (!strsect) return;
	
	i = 0;
	while (true) {
		ElfSection* section = Elf64_GetSection(this, &i, SYM_SYMBOL_TBL);
		if (!section) break;
		ElfSymbol* sym = Elf64_Offset(this, section->offset);
		int num = section->size / sizeof(ElfSymbol);
		
		for (; num; num--, sym++)
			call(arg, &head[sym->name_id], sym);
		i++;
	}
}

void Elf64_Free(Elf64* this) {
	if (this) delete(this->sect, this);
}
