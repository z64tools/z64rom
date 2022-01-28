mips64_gcc     = tools/mips64-binutils/bin/mips64-gcc
mips64_ld      = tools/mips64-binutils/bin/mips64-ld
mips64_objdump = tools/mips64-binutils/bin/mips64-objdump
mips64_objcopy = tools/mips64-binutils/bin/mips64-objcopy
mips64_ar      = tools/mips64-binutils/bin/mips64-ar
novl           = tools/novl
elf2ld         = tools/elf2ld
z64rom         = $(shell find ./ -type f -name 'z64ro*')
setup_rom      = $(shell find ./ -type f -name '*.z64' -not -name 'build.z64')

PRNT_DGRY := \e[90;2m
PRNT_GRAY := \e[0;90m
PRNT_REDD := \e[0;91m
PRNT_GREN := \e[0;92m
PRNT_YELW := \e[0;93m
PRNT_BLUE := \e[0;94m
PRNT_PRPL := \e[0;95m
PRNT_CYAN := \e[0;96m
PRNT_RSET := \e[m

ElfDep        = rom/lib/codelib.bin
GccFlags_Warn = -Wall -Wno-builtin-declaration-mismatch -Wno-unused-variable
GccFlags      = -I$(CURDIR)/include -I$(CURDIR)/include/include -G 0 -O1 -fno-reorder-blocks -std=gnu99 -march=vr4300 -mabi=32 -mips3 -mno-explicit-relocs -mno-memcpy -mno-check-zero-division $(GccFlags_Warn)
GccFlags_Code = $(GccFlags) -mno-gpopt -fomit-frame-pointer
LdFlags       = -L$(CURDIR)/include/oot_mq_debug/ -L$(CURDIR)/include/common/ -T z64hdr.ld --emit-relocs
nOvlFlags     = -v -c -A

Actor_C   = $(shell find rom/actor/0x*/* -maxdepth 0 -type f -name '*.c' 2>/dev/null)
Actor_O   = $(foreach f,$(Actor_C:.c=.o),$f)
Actor_Elf = $(foreach f,$(Actor_C:.c=.elf),$f)

Library_C   = $(shell find lib/* -type f -name '*.c' 2>/dev/null)
Library_O   = $(foreach f,$(Library_C:.c=.o),rom/$f)
Library_Elf = $(foreach f,$(Library_C:.c=.elf),rom/$f)

# Make build directories
$(shell mkdir -p rom/ $(foreach dir, \
	$(dir $(Library_O)) \
	, $(dir)))

mips64_download = https://cdn.discordapp.com/attachments/889436307758129203/936660603454513162/mips64-binutils.7z
z64hdr_git      = https://github.com/z64tools/z64hdr.git

.PHONY: all rom clean dist-clean z64rom-clean dist-clean confirm setup actor library
.DEFAULT_GOAL := all

all: library actor
rom: all
	@$(z64rom)

clean:
	rm -f $(Actor_O)
	rm -f $(Actor_Elf)
	rm -f $(Library_O)
	rm -f $(Library_Bin)

dist-clean:
	@rm -f -R include
	@rm -f -R tools/mips64-binutils
	@rm -f -R tools/mips64-binutils.7z

z64rom-clean:
	@rm -f -R rom

setup:
	@echo "$(PRNT_GRAY)# $(PRNT_RSET)Setup is going to be downloading files and unpacking them automatically."
	@echo "$(PRNT_GRAY)# $(PRNT_RSET)You can check the links used for downloading in the $(PRNT_REDD)Makefile$(PRNT_RSET) by searching $(PRNT_REDD)mips64_download$(PRNT_RSET) and $(PRNT_REDD)z64hdr_git$(PRNT_RSET)"
	@echo -n "$(PRNT_GRAY)# $(PRNT_RSET)Are you sure you want to continue? $(PRNT_GRAY)[y/n]$(PRNT_RSET) " && read ans && if [ $${ans:-N} = y ]; then \
		if [ -f "tools/mips64-binutils.7z" ]; then \
			if md5sum --status -c tools/mips64-binutils.md5; then \
				echo "$(PRNT_REDD)Skipping$(PRNT_RSET):     Download mips64-binutils.7z"; \
			else \
				rm -f tools/mips64-binutils.7z; \
				echo "$(PRNT_YELW)Redownload$(PRNT_RSET):   mips64-binutils.7z"; \
				wget -O tools/mips64-binutils.7z $(mips64_download) -q --show-progress; \
			fi; \
		else \
			echo "$(PRNT_GREN)Download$(PRNT_RSET):     mips64-binutils.7z"; \
			wget -O tools/mips64-binutils.7z $(mips64_download) -q --show-progress; \
		fi; \
		if [ -d "tools/mips64-binutils/bin/" ]; then \
			echo "$(PRNT_REDD)Skipping$(PRNT_RSET):     Unpack mips64-binutils.7z"; \
		else \
			mkdir -p tools/mips64-binutils; \
			echo "$(PRNT_GREN)Unpack$(PRNT_RSET):       mips64-binutils.7z"; \
			7z x tools/mips64-binutils.7z -otools/mips64-binutils/ -aos > /dev/null 2>&1; \
		fi; \
		if [ -d "include/" ]; then \
			echo "$(PRNT_REDD)Skipping$(PRNT_RSET):     Download z64hdr"; \
		else \
			echo "$(PRNT_GREN)Download$(PRNT_RSET):     z64hdr"; \
			git clone $(z64hdr_git) include --quiet; \
		fi; \
		$(z64rom) $(setup_rom); \
	fi;

# # # # # # # # # # # # # # # # # # # #
# CODE ACTOR                          #
# # # # # # # # # # # # # # # # # # # #
	
actor: $(Actor_O) $(Actor_Elf)
	
rom/actor/%.o: rom/actor/%.c rom/actor/%.h
	@echo "$(PRNT_CYAN)Actor$(PRNT_RSET):   $<"
	@printf "ENTRY_POINT = 0x80800000;" > $(@D)/entry.ld
	@$(mips64_gcc) -c $(GccFlags) $< -o $@

rom/actor/%.elf: rom/actor/%.o $(ElfDep)
	@echo "$(PRNT_CYAN)Actor$(PRNT_RSET):   $<"
	@$(mips64_ld) -o $@ $< -L$(dir $@) $(LdFlags)
	@$(mips64_objdump) -drz $@ > $(@:.elf=.s)
	@printf "alloc_type = 0\n" > $(@D)/config.cfg
	@printf "vram_addr  = 0x80800000\n" >> $(@D)/config.cfg
	@printf "init_vars  = 0x%s\n" $$($(mips64_objdump) -t $@ | grep "_InitVars" | head -c 8) >> $(@D)/config.cfg
	@$(novl) $(nOvlFlags) 0x80800000 -o $(@D)/actor.zovl $@ -s

# # # # # # # # # # # # # # # # # # # #
# CODE LIBRARY                        #
# # # # # # # # # # # # # # # # # # # #

library: $(Library_O) $(Library_Bin)
	
rom/lib/%.o: lib/%.c
	@echo "$(PRNT_PRPL)Library$(PRNT_RSET): $<"
	@$(mips64_gcc) -c $(GccFlags_Code) $< -o $@

rom/lib/%.o: lib/%.c lib/%.h
	@echo "$(PRNT_PRPL)Library$(PRNT_RSET): $<"
	@$(mips64_gcc) -c $(GccFlags_Code) $< -o $@

rom/lib/codelib.bin: $(Library_O)
	@echo "$(PRNT_PRPL)Library$(PRNT_RSET): codelib.bin"
	@printf "ENTRY_POINT = 0x80600000;\n" > $(@D)/entry.ld
	@$(mips64_ld) -o rom/lib/codelib.elf $^ -L$(dir $@) $(LdFlags)
	@$(mips64_objdump) -drz rom/lib/codelib.elf > rom/lib/codelib.s
	@$(mips64_objdump) -x -t rom/lib/codelib.elf | grep -E '806.*[O,F].*[r,t,d][o,e,a][d,x,t].*\0000' > include/oot_mq_debug/temp_codelib.ld
	@$(elf2ld) include/oot_mq_debug/temp_codelib.ld include/oot_mq_debug/codelib.ld
	@$(mips64_objcopy) -R .MIPS.abiflags -O binary rom/lib/codelib.elf $@