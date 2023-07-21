/* This file contains enough of the ELF format definitions for MIPS to enable Fairy and Fado to work. More may need to
 * be added later. The content is excerpted directly from elf.h from the GNU C Library, and therefore: */
/* Copyright (C) 1995-2020 Free Software Foundation, Inc. */
/* SPDX-License-Identifier: LGPL-2.1-or-later */

#ifndef MIPS_ELF_H
#define MIPS_ELF_H

#include <ext_type.h>

#define NULL_SYM -1U

typedef struct Elf64 Elf64;

enum {
	ELF64_TYPE_NONE        = 0,
	ELF64_TYPE_VAR,
	ELF64_TYPE_FUNC,
	
	ELF64_VISIBILITY_LOCAL = 0,
	ELF64_VISIBILITY_GLOBAL,
	ELF64_VISIBILITY_WEAK,
};

typedef struct StructBE {
	u32 name_id; /* Symbol name (string tbl index) */
	u32 value; /* Symbol value */
	u32 size; /* Symbol size */
	union {
		struct {
			u8 type       : 4;
			u8 visibility : 4;
		};
		u8 info;
	};
	u8  _; /* Symbol visibility */
	u16 section_id; /* Section index */
} ElfSymbol;

Elf64* Elf64_New(u8* data);
Elf64* Elf64_Load(const char* file);
u32 Elf64_FindSym(Elf64* this, const char* name);
void Elf64_Free(Elf64* this);
void Elf64_ReadSyms(Elf64* this, void* arg, void (*call)(void*, const char*, ElfSymbol*));

#ifdef MIPS_ELF_PRIVATE
typedef struct StructBE {
	unsigned char ident[16]; /* Magic number and other info */
	u16 obj_type;              /* Object file type */
	u16 architecture;          /* Architecture */
	u32 obj_version;           /* Object file version */
	u32 entry;                 /* Entry point virtual address */
	u32 offset_program;        /* Program header table file offset */
	u32 offset_section;        /* Section header table file offset */
	u32 processor_flags;       /* Processor-specific flags */
	u16 hdr_size;              /* ELF header size in bytes */
	u16 size_program;          /* Program header table entry size */
	u16 num_program;           /* Program header table entry count */
	u16 size_section;          /* Section header table entry size */
	u16 num_section;           /* Section header table entry count */
	u16 section_strtbl_id;     /* Section header string table index */
} ElfHeader;

/* Section header.  */

typedef struct StructBE {
	u32 name_id; /* Section name (string tbl index) */
	u32 type; /* Section type */
	u32 flags; /* Section flags */
	u32 addr; /* Section virtual addr at execution */
	u32 offset; /* Section file offset */
	u32 size; /* Section size in bytes */
	u32 link; /* Link to another section */
	u32 info; /* Additional section information */
	u32 alignment; /* Section alignment */
	u32 num; /* Entry size if section holds table */
} ElfSection;

/* Special section indices.  */
#define SHN_UNDEF     0        /* Undefined section */
#define SHN_LORESERVE 0xff00   /* Start of reserved indices */
#define SHN_LOPROC    0xff00   /* Start of processor-specific */
#define SHN_BEFORE    0xff00   /* Order section before all others (Solaris). */
#define SHN_AFTER     0xff01   /* Order section after all others (Solaris). */
#define SHN_HIPROC    0xff1f   /* End of processor-specific */
#define SHN_LOOS      0xff20   /* Start of OS-specific */
#define SHN_HIOS      0xff3f   /* End of OS-specific */
#define SHN_ABS       0xfff1   /* Associated symbol is absolute */
#define SHN_COMMON    0xfff2   /* Associated symbol is common */
#define SHN_XINDEX    0xffff   /* Index is in extra table.  */
#define SHN_HIRESERVE 0xffff   /* End of reserved indices */

/* Legal values for sh_type (section type).  */
#define SHT_NULL           0   /* Section header table entry unused */
#define SHT_PROGBITS       1   /* Program data */
#define SHT_SYMTAB         2   /* Symbol table */
#define SHT_STRTAB         3   /* String table */
#define SHT_RELA           4   /* Relocation entries with addends */
#define SHT_HASH           5   /* Symbol hash table */
#define SHT_DYNAMIC        6   /* Dynamic linking information */
#define SHT_NOTE           7   /* Notes */
#define SHT_NOBITS         8   /* Program space with no data (bss) */
#define SHT_REL            9   /* Relocation entries, no addends */
#define SHT_SHLIB          10  /* Reserved */
#define SHT_DYNSYM         11  /* Dynamic linker symbol table */
#define SHT_INIT_ARRAY     14  /* Array of constructors */
#define SHT_FINI_ARRAY     15  /* Array of destructors */
#define SHT_PREINIT_ARRAY  16  /* Array of pre-constructors */
#define SHT_GROUP          17  /* Section group */
#define SHT_SYMTAB_SHNDX   18  /* Extended section indeces */
#define SHT_NUM            19  /* Number of defined types.  */
#define SHT_LOOS           0x60000000 /* Start OS-specific.  */
#define SHT_GNU_ATTRIBUTES 0x6ffffff5 /* Object attributes.  */
#define SHT_GNU_HASH       0x6ffffff6 /* GNU-style hash table.  */
#define SHT_GNU_LIBLIST    0x6ffffff7 /* Prelink library list */
#define SHT_CHECKSUM       0x6ffffff8 /* Checksum for DSO content.  */
#define SHT_LOSUNW         0x6ffffffa /* Sun-specific low bound.  */
#define SHT_SUNW_move      0x6ffffffa
#define SHT_SUNW_COMDAT    0x6ffffffb
#define SHT_SUNW_syminfo   0x6ffffffc
#define SHT_GNU_verdef     0x6ffffffd /* Version definition section.  */
#define SHT_GNU_verneed    0x6ffffffe /* Version needs section.  */
#define SHT_GNU_versym     0x6fffffff /* Version symbol table.  */
#define SHT_HISUNW         0x6fffffff /* Sun-specific high bound.  */
#define SHT_HIOS           0x6fffffff /* End OS-specific type */
#define SHT_LOPROC         0x70000000 /* Start of processor-specific */
#define SHT_HIPROC         0x7fffffff /* End of processor-specific */
#define SHT_LOUSER         0x80000000 /* Start of application-specific */
#define SHT_HIUSER         0x8fffffff /* End of application-specific */

/* Symbol table entry.  */

/* How to extract and insert information held in the st_info field.  */

#define ELF32_ST_BIND(val)        (((unsigned char)(val)) >> 4)
#define ELF32_ST_TYPE(val)        ((val) & 0xf)
#define ELF32_ST_INFO(bind, type) (((bind) << 4) + ((type) & 0xf))

/* Both ElfSymbol and Elf64_Sym use the same one-byte st_info field.  */
#define ELF64_ST_BIND(val)        ELF32_ST_BIND(val)
#define ELF64_ST_TYPE(val)        ELF32_ST_TYPE(val)
#define ELF64_ST_INFO(bind, type) ELF32_ST_INFO((bind), (type))

/* Legal values for ST_BIND subfield of st_info (symbol binding).  */

#define STB_LOCAL      0       /* Local symbol */
#define STB_GLOBAL     1       /* Global symbol */
#define STB_WEAK       2       /* Weak symbol */
#define STB_NUM        3       /* Number of defined types.  */
#define STB_LOOS       10      /* Start of OS-specific */
#define STB_GNU_UNIQUE 10 /* Unique symbol.  */
#define STB_HIOS       12      /* End of OS-specific */
#define STB_LOPROC     13      /* Start of processor-specific */
#define STB_HIPROC     15      /* End of processor-specific */

/* Legal values for ST_TYPE subfield of st_info (symbol type).  */

#define STT_NOTYPE    0        /* Symbol type is unspecified */
#define STT_OBJECT    1        /* Symbol is a data object */
#define STT_FUNC      2        /* Symbol is a code object */
#define STT_SECTION   3        /* Symbol associated with a section */
#define STT_FILE      4        /* Symbol's name is file name */
#define STT_COMMON    5        /* Symbol is a common data object */
#define STT_TLS       6        /* Symbol is thread-local data object*/
#define STT_NUM       7        /* Number of defined types.  */
#define STT_LOOS      10       /* Start of OS-specific */
#define STT_GNU_IFUNC 10 /* Symbol is indirect code object */
#define STT_HIOS      12       /* End of OS-specific */
#define STT_LOPROC    13       /* Start of processor-specific */
#define STT_HIPROC    15       /* End of processor-specific */

/* Symbol table indices are found in the hash buckets and chain table
   of a symbol hash table section.  This special index value indicates
   the end of a chain, meaning no further symbols are found in that bucket.  */

#define STN_UNDEF 0 /* End of a chain.  */

/* How to extract and insert information held in the st_other field.  */

#define ELF32_ST_VISIBILITY(o) ((o) & 0x03)

/* For ELF64 the definitions are the same.  */
#define ELF64_ST_VISIBILITY(o) ELF32_ST_VISIBILITY(o)

/* Symbol visibility specification encoded in the st_other field.  */
#define STV_DEFAULT   0        /* Default symbol visibility rules */
#define STV_INTERNAL  1        /* Processor specific hidden class */
#define STV_HIDDEN    2        /* GetSymInfo unavailable in other modules */
#define STV_PROTECTED 3 /* Not preemptible, not exported */

/* Relocation table entry without addend (in section of type SHT_REL).  */

typedef struct StructBE {
	u32 r_offset; /* Address */
	u32 r_info;                /* Relocation type and symbol index */
} Elf32_Rel;

/* Relocation table entry with addend (in section of type SHT_RELA).  */

typedef struct StructBE {
	u32 r_offset;              /* Address */
	u32 r_info;                /* Relocation type and symbol index */
	s32 r_addend;              /* Addend */
} Elf32_Rela;

/* How to extract and insert information held in the r_info field.  */

#define ELF32_R_SYM(val)        ((val) >> 8)
#define ELF32_R_TYPE(val)       ((val) & 0xff)
#define ELF32_R_INFO(sym, type) (((sym) << 8) + ((type) & 0xff))

/* MIPS R3000 specific definitions.  */

/* Legal values for e_flags field of ElfHeader.  */

#define EF_MIPS_NOREORDER   1 /* A .noreorder directive was used.  */
#define EF_MIPS_PIC         2  /* Contains PIC code.  */
#define EF_MIPS_CPIC        4  /* Uses PIC calling sequence.  */
#define EF_MIPS_XGOT        8
#define EF_MIPS_64BIT_WHIRL 16
#define EF_MIPS_ABI2        32
#define EF_MIPS_ABI_ON32    64
#define EF_MIPS_FP64        512 /* Uses FP64 (12 callee-saved).  */
#define EF_MIPS_NAN2008     1024 /* Uses IEEE 754-2008 NaN encoding.  */
#define EF_MIPS_ARCH        0xf0000000 /* MIPS architecture level.  */

/* Legal values for MIPS architecture level.  */

#define EF_MIPS_ARCH_1    0x00000000 /* -mips1 code.  */
#define EF_MIPS_ARCH_2    0x10000000 /* -mips2 code.  */
#define EF_MIPS_ARCH_3    0x20000000 /* -mips3 code.  */
#define EF_MIPS_ARCH_4    0x30000000 /* -mips4 code.  */
#define EF_MIPS_ARCH_5    0x40000000 /* -mips5 code.  */
#define EF_MIPS_ARCH_32   0x50000000 /* MIPS32 code.  */
#define EF_MIPS_ARCH_64   0x60000000 /* MIPS64 code.  */
#define EF_MIPS_ARCH_32R2 0x70000000 /* MIPS32r2 code.  */
#define EF_MIPS_ARCH_64R2 0x80000000 /* MIPS64r2 code.  */

/* The following are unofficial names and should not be used.  */

#define E_MIPS_ARCH_1  EF_MIPS_ARCH_1
#define E_MIPS_ARCH_2  EF_MIPS_ARCH_2
#define E_MIPS_ARCH_3  EF_MIPS_ARCH_3
#define E_MIPS_ARCH_4  EF_MIPS_ARCH_4
#define E_MIPS_ARCH_5  EF_MIPS_ARCH_5
#define E_MIPS_ARCH_32 EF_MIPS_ARCH_32
#define E_MIPS_ARCH_64 EF_MIPS_ARCH_64

/* Special section indices.  */

#define SHN_MIPS_ACOMMON    0xff00 /* Allocated common symbols.  */
#define SHN_MIPS_TEXT       0xff01 /* Allocated test symbols.  */
#define SHN_MIPS_DATA       0xff02 /* Allocated data symbols.  */
#define SHN_MIPS_SCOMMON    0xff03 /* Small common symbols.  */
#define SHN_MIPS_SUNDEFINED 0xff04 /* Small undefined symbols.  */

/* Legal values for sh_type field of Section.  */

#define SHT_MIPS_LIBLIST       0x70000000 /* Shared objects used in link.  */
#define SHT_MIPS_MSYM          0x70000001
#define SHT_MIPS_CONFLICT      0x70000002 /* Conflicting symbols.  */
#define SHT_MIPS_GPTAB         0x70000003 /* Global data area sizes.  */
#define SHT_MIPS_UCODE         0x70000004 /* Reserved for SGI/MIPS compilers */
#define SHT_MIPS_DEBUG         0x70000005 /* MIPS ECOFF debugging info.  */
#define SHT_MIPS_REGINFO       0x70000006 /* Register usage information.  */
#define SHT_MIPS_PACKAGE       0x70000007
#define SHT_MIPS_PACKSYM       0x70000008
#define SHT_MIPS_RELD          0x70000009
#define SHT_MIPS_IFACE         0x7000000b
#define SHT_MIPS_CONTENT       0x7000000c
#define SHT_MIPS_OPTIONS       0x7000000d /* Miscellaneous options.  */
#define SHT_MIPS_SHDR          0x70000010
#define SHT_MIPS_FDESC         0x70000011
#define SHT_MIPS_EXTSYM        0x70000012
#define SHT_MIPS_DENSE         0x70000013
#define SHT_MIPS_PDESC         0x70000014
#define SHT_MIPS_LOCSYM        0x70000015
#define SHT_MIPS_AUXSYM        0x70000016
#define SHT_MIPS_OPTSYM        0x70000017
#define SHT_MIPS_LOCSTR        0x70000018
#define SHT_MIPS_LINE          0x70000019
#define SHT_MIPS_RFDESC        0x7000001a
#define SHT_MIPS_DELTASYM      0x7000001b
#define SHT_MIPS_DELTAINST     0x7000001c
#define SHT_MIPS_DELTACLASS    0x7000001d
#define SHT_MIPS_DWARF         0x7000001e /* DWARF debugging information.  */
#define SHT_MIPS_DELTADECL     0x7000001f
#define SHT_MIPS_SYMBOL_LIB    0x70000020
#define SHT_MIPS_EVENTS        0x70000021 /* Event section.  */
#define SHT_MIPS_TRANSLATE     0x70000022
#define SHT_MIPS_PIXIE         0x70000023
#define SHT_MIPS_XLATE         0x70000024
#define SHT_MIPS_XLATE_DEBUG   0x70000025
#define SHT_MIPS_WHIRL         0x70000026
#define SHT_MIPS_EH_REGION     0x70000027
#define SHT_MIPS_XLATE_OLD     0x70000028
#define SHT_MIPS_PDR_EXCEPTION 0x70000029
#define SHT_MIPS_XHASH         0x7000002b

/* Legal values for sh_flags field of Section.  */

#define SHF_MIPS_GPREL   0x10000000 /* Must be in global data area.  */
#define SHF_MIPS_MERGE   0x20000000
#define SHF_MIPS_ADDR    0x40000000
#define SHF_MIPS_STRINGS 0x80000000
#define SHF_MIPS_NOSTRIP 0x08000000
#define SHF_MIPS_LOCAL   0x04000000
#define SHF_MIPS_NAMES   0x02000000
#define SHF_MIPS_NODUPE  0x01000000

/* Symbol tables.  */

/* MIPS specific values for `st_other'.  */
#define STO_MIPS_DEFAULT         0x0
#define STO_MIPS_INTERNAL        0x1
#define STO_MIPS_HIDDEN          0x2
#define STO_MIPS_PROTECTED       0x3
#define STO_MIPS_PLT             0x8
#define STO_MIPS_SC_ALIGN_UNUSED 0xff

/* MIPS specific values for `st_info'.  */
#define STB_MIPS_SPLIT_COMMON 13

/* MIPS relocs.  */

#define R_MIPS_NONE    0       /* No reloc */
#define R_MIPS_16      1       /* Direct 16 bit */
#define R_MIPS_32      2       /* Direct 32 bit */
#define R_MIPS_REL32   3       /* PC relative 32 bit */
#define R_MIPS_26      4       /* Direct 26 bit shifted */
#define R_MIPS_HI16    5       /* High 16 bit */
#define R_MIPS_LO16    6       /* Low 16 bit */
#define R_MIPS_GPREL16 7       /* GP relative 16 bit */
#define R_MIPS_LITERAL 8       /* 16 bit literal entry */
#define R_MIPS_GOT16   9       /* 16 bit GOT entry */
#define R_MIPS_PC16    10      /* PC relative 16 bit */
#define R_MIPS_CALL16  11      /* 16 bit GOT entry for function */
#define R_MIPS_GPREL32 12 /* GP relative 32 bit */

#define R_MIPS_SHIFT5          16
#define R_MIPS_SHIFT6          17
#define R_MIPS_64              18
#define R_MIPS_GOT_DISP        19
#define R_MIPS_GOT_PAGE        20
#define R_MIPS_GOT_OFST        21
#define R_MIPS_GOT_HI16        22
#define R_MIPS_GOT_LO16        23
#define R_MIPS_SUB             24
#define R_MIPS_INSERT_A        25
#define R_MIPS_INSERT_B        26
#define R_MIPS_DELETE          27
#define R_MIPS_HIGHER          28
#define R_MIPS_HIGHEST         29
#define R_MIPS_CALL_HI16       30
#define R_MIPS_CALL_LO16       31
#define R_MIPS_SCN_DISP        32
#define R_MIPS_REL16           33
#define R_MIPS_ADD_IMMEDIATE   34
#define R_MIPS_PJUMP           35
#define R_MIPS_RELGOT          36
#define R_MIPS_JALR            37
#define R_MIPS_TLS_DTPMOD32    38 /* Module number 32 bit */
#define R_MIPS_TLS_DTPREL32    39 /* Module-relative offset 32 bit */
#define R_MIPS_TLS_DTPMOD64    40 /* Module number 64 bit */
#define R_MIPS_TLS_DTPREL64    41 /* Module-relative offset 64 bit */
#define R_MIPS_TLS_GD          42 /* 16 bit GOT offset for GD */
#define R_MIPS_TLS_LDM         43 /* 16 bit GOT offset for LDM */
#define R_MIPS_TLS_DTPREL_HI16 44 /* Module-relative offset, high 16 bits */
#define R_MIPS_TLS_DTPREL_LO16 45 /* Module-relative offset, low 16 bits */
#define R_MIPS_TLS_GOTTPREL    46 /* 16 bit GOT offset for IE */
#define R_MIPS_TLS_TPREL32     47 /* TP-relative offset, 32 bit */
#define R_MIPS_TLS_TPREL64     48 /* TP-relative offset, 64 bit */
#define R_MIPS_TLS_TPREL_HI16  49 /* TP-relative offset, high 16 bits */
#define R_MIPS_TLS_TPREL_LO16  50 /* TP-relative offset, low 16 bits */
#define R_MIPS_GLOB_DAT        51
#define R_MIPS_COPY            126
#define R_MIPS_JUMP_SLOT       127
/* Keep this the last entry.  */
#define R_MIPS_NUM 128

#endif
#endif /* MIPS_ELF_H */