#include "z64rom.h"
#include "z64elf.h"
#include "tools.h"

void GetSymInfo(const char* symbol) {
	Elf64* elf = Elf64_Load("rom/lib_user/z_lib_user.elf");
	
	if (!vldt_hex(symbol)) {
		u32 addr = Elf64_FindSym(elf, symbol);
		
		info("Symbol: "PRNT_GRAY "%s", symbol);
		
		if (addr != NULL_SYM)
			info("%s = " PRNT_YELW "%08X", symbol, addr);
		else
			warn("%s = " PRNT_YELW "None", symbol);
		
		return;
	}
	
	List list = List_New();
	List_Alloc(&list, 32000);
	
	nested(void, call, (void* arg, const char* name, ElfSymbol * sym)) {
		if (sym->visibility == ELF64_VISIBILITY_LOCAL)
			return;
		
		switch (sym->type) {
			case ELF64_TYPE_VAR:
			case ELF64_TYPE_FUNC:
				if ((sym->value & 0xFF000000) == 0x80000000)
					List_Add(arg, x_fmt("0x%08X %s%s", sym->value, sym->type == ELF64_TYPE_FUNC ? "F" : " ", name));
				break;
		}
	};
	
	Elf64_ReadSyms(elf, &list, (void*)call);
	List_Sort(&list);
	
	u32 infoaddr = shex(symbol);
	info("Address: "PRNT_GRAY "%08X", infoaddr);
	
	for (int i = 0; i < list.num; i++) {
		u32 addr = shex(x_strndup(list.item[i] + 2, 8));
		u32 next = (i + 1 < list.num) ? shex(x_strndup(list.item[i + 1] + 2, 8)) : 0xFFFFFFFF;
		
		if (infoaddr >= addr && infoaddr < next) {
			u32 diff = infoaddr - addr;
			
			info("%s = " PRNT_YELW "%08X " PRNT_GRAY "%s", list.item[i] + 12, addr, diff ? x_fmt("( %X )", diff) : "");
			
			if (list.item[i][11] == 'F') {
				char* dump = sys_exes(x_fmt("%s -d %s",
						Tools_Get(mips64_objdump),
						"rom/lib_user/z_lib_user.elf"));
				
				char* start = strstr(dump, x_fmt("%08x:\t", addr));
				bool first = true;
				
				while (start && (first || (start = strline(start, 1)))) {
					first = false;
					if (start[0] == '\n' || start[0] == '\0' || start[0] == '\r')
						break;
					
					u32 offset = shex(x_strndup(start, 8)) - addr;
					char* asmline = x_cpyline(start + 9 + 8 + 3, 0);
					char* instruction = x_cpyword(asmline, 0);
					
					asmline += strlen(instruction) + 1;
					
					asmline = x_rep(asmline, "<", "" PRNT_REDD "<");
					asmline = x_rep(asmline, ">", ">" PRNT_RSET "");
					asmline = x_rep(asmline, "(", " " PRNT_GRAY "(");
					asmline = x_rep(asmline, ")", ")" PRNT_RSET "");
					asmline = x_rep(asmline, ",", ", ");
					
					if (offset == diff)
						warn(PRNT_REDD "%08X" PRNT_RSET " " PRNT_BLUE "%-8s" PRNT_RSET " %s", offset, instruction, asmline);
					else
						info("%08X " PRNT_BLUE "%-8s" PRNT_RSET " %s", offset, instruction, asmline);
				}
			}
			
			return;
		}
	}
	
	warn("%s = " PRNT_YELW "None", symbol);
}
