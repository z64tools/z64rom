mips64_gcc     = tools/mips64-binutils/bin/mips64-gcc
mips64_ld      = tools/mips64-binutils/bin/mips64-ld
mips64_objdump = tools/mips64-binutils/bin/mips64-objdump
mips64_objcopy = tools/mips64-binutils/bin/mips64-objcopy
mips64_ar      = tools/mips64-binutils/bin/mips64-ar
novl           = tools/novl
elf2ld         = tools/elf2ld
z64rom         = ./z64rom.exe
setup_rom      = $(shell find ./ -type f -name '*.z64' -not -name 'build.z64')

PRDR_GRAY := \e[90;0m
PRDR_REDD := \e[91;0m
PRDR_GREN := \e[92;0m
PRDR_YELW := \e[93;0m
PRDR_BLUE := \e[94;0m
PRDR_PRPL := \e[95;0m
PRDR_CYAN := \e[96;0m

PRNT_GRAY := \e[0;90m
PRNT_REDD := \e[0;91m
PRNT_GREN := \e[0;92m
PRNT_YELW := \e[0;93m
PRNT_BLUE := \e[0;94m
PRNT_PRPL := \e[0;95m
PRNT_CYAN := \e[0;96m

PRNT_RSET := \e[m

Linkers        = include/z_lib_user.ld include/objects.ld
GccFlags_Warn = -Wall -Wno-builtin-declaration-mismatch -Wno-unused-variable
GccFlags      = -Iinclude/z64hdr -Iinclude/z64hdr/include -Iinclude/ -Isrc/lib_user -G 0 -O1 -fno-reorder-blocks -std=gnu99 -march=vr4300 -mabi=32 -mips3 -mno-explicit-relocs -mno-memcpy -mno-check-zero-division $(GccFlags_Warn)
GccFlags_Code = $(GccFlags) -mno-gpopt -fomit-frame-pointer
LdFlags       = -Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/ -T z64hdr.ld -T objects.ld -T z_lib_user.ld --emit-relocs
nOvlFlags     = -v -c -A

EntryDir = $(@D)/.temp/$(notdir $(basename $<))

Object_objex   = $(shell cd src && find object/0x*/* -type f -name '*.objex' 2>/dev/null)
Object_zobj   = $(foreach f,$(Object_objex:.objex=.zobj),rom/$(f))

Actor_C   = $(shell cd src && find actor/0x*/* -type f -name '*.c' 2>/dev/null)
Actor_H   = $(shell find src/actor/0x*/* -maxdepth 0 -type f -name '*.h' '*.c' 2>/dev/null)
Actor_O   = $(foreach f,$(Actor_C:.c=.o),rom/$(f))
Actor_Elf = $(foreach f,$(Actor_C:.c=.elf),rom/$(f))

Library_C   = $(shell cd src && find lib_user/* -type f -name '*.c' 2>/dev/null)
Library_O   = $(foreach f,$(Library_C:.c=.o),rom/$(f))
Library_Elf = $(foreach f,$(Library_C:.c=.elf),rom/$(f))

FileChoose_C   = $(shell cd src && find system/GameState_FileChoose/* -type f -name '*.c' 2>/dev/null)
FileChoose_O   = $(foreach f,$(FileChoose_C:.c=.o),rom/$(f))

Code_C   = $(shell cd src && find lib_code/* -maxdepth 2 -type f -name '*.c' 2>/dev/null)
Code_O   = $(foreach f,$(Code_C:.c=.o),rom/$(f))
Code_Elf = $(foreach f,$(Code_C:.c=.elf),rom/$(f))

Code_S   = $(shell cd src && find lib_code/* -maxdepth 2 -type f -name '*.s' 2>/dev/null)
Code_SElf = $(foreach f,$(Code_S:.s=.elf),rom/$(f))

# Make build directories
$(shell mkdir -p include/)
$(shell mkdir -p rom/ $(foreach dir, \
	$(dir $(Library_O)) \
	$(dir $(Code_O)) \
	$(dir $(Actor_O)) \
	$(dir $(FileChoose_O)) \
	$(dir $(Object_zobj)) \
	, $(dir)))

mips64_download = https://cdn.discordapp.com/attachments/889436307758129203/936660603454513162/mips64-binutils.7z
z64hdr_git      = https://github.com/z64tools/z64hdr.git

.PHONY: all rom update clean clean-dist clean-rom clean-dist confirm setup actor library code check object system
.DEFAULT_GOAL := all

all:
	@$(MAKE) object --no-print-directory -s
	@$(MAKE) header_depper --no-print-directory -s
	@$(MAKE) src --no-print-directory -s

src: library code system actor
object: $(Object_zobj)

pack:
	@rm -f pack.7z
	@mkdir -p .pack/src/
	@mkdir -p .pack/rom/
	@rsync -av --exclude=".*" --exclude="*.txt" src/* .pack/src/
	@rsync -av --exclude=".*" --exclude="*.elf" --exclude="*.o" rom/* .pack/rom/
	@7z a pack ./.pack/*

rom: all
	@$(z64rom)

clean:
	@rm -f -R $(dir $(Actor_O))
	@rm -f -R $(dir $(Object_zobj))
	@rm -f -R $(dir $(FileChoose_O))
	@rm -f $(Linkers)
	@rm -f -R rom/lib_user
	@rm -f -R rom/lib_code

clean-rom:
	@rm -f -R rom

clean-dist: clean clean-rom
	@rm -f -R include
	@rm -f -R tools/mips64-binutils
	@rm -f -R tools/mips64-binutils.7z

# Update if the included file is newer. Mainly used for object changes
header_depper: $(Actor_H)
	@tools/depper $^

setup:
	@ if [ -f "tools/.download-permission" ]; then \
		make setup-no-ask --no-print-directory -s; \
	else \
		echo "$(PRNT_GRAY)# $(PRNT_RSET)Setup is going to be downloading files and unpacking them automatically."; \
		echo "$(PRNT_GRAY)# $(PRNT_RSET)You can check the links used for downloading in the $(PRNT_REDD)Makefile$(PRNT_RSET) by searching $(PRNT_REDD)mips64_download$(PRNT_RSET) and $(PRNT_REDD)z64hdr_git$(PRNT_RSET)"; \
		echo -n "$(PRNT_GRAY)# $(PRNT_RSET)Are you sure you want to continue? $(PRNT_GRAY)[y/n]$(PRNT_RSET) " && read ans && if [ $${ans:-N} = y ]; then \
			touch tools/.download-permission; \
			make setup-no-ask --no-print-directory -s; \
		fi; \
	fi; \

setup-no-ask:
	@ if [ -f "tools/mips64-binutils.7z" ]; then \
		if ! md5sum --status -c tools/mips64-binutils.md5; then \
			rm -f tools/mips64-binutils.7z; \
			echo "$(PRNT_YELW)Redownload$(PRNT_RSET):   mips64-binutils.7z"; \
			wget -O tools/mips64-binutils.7z $(mips64_download) -q --show-progress; \
		fi; \
	else \
		echo "$(PRNT_GREN)Download$(PRNT_RSET):     mips64-binutils.7z"; \
		wget -O tools/mips64-binutils.7z $(mips64_download) -q --show-progress; \
	fi; \
	if [ ! -d "tools/mips64-binutils/bin/" ]; then \
		mkdir -p tools/mips64-binutils; \
		echo "$(PRNT_GREN)Unpack$(PRNT_RSET):       mips64-binutils.7z"; \
		7z x tools/mips64-binutils.7z -otools/mips64-binutils/ -aos > /dev/null 2>&1; \
	fi; \
	if [ ! -d "include/z64hdr" ]; then \
		echo "$(PRNT_GREN)Download$(PRNT_RSET):     z64hdr"; \
		cd include && git clone $(z64hdr_git) --quiet; \
		cd z64hdr; \
		git checkout temp-update; \
		cd ..; \
		cd ..; \
	fi; \
	if [ ! -d "rom/actor/" ]; then \
		$(z64rom) $(setup_rom); \
	fi; \

update:
	@echo "$(PRNT_YELW)Update$(PRNT_RSET):       z64hdr"
	@cd include/z64hdr && git pull --quiet

# # # # # # # # # # # # # # # # # # # #
# CODE OBJECT                         #
# # # # # # # # # # # # # # # # # # # #

ObjexSettings = $(shell sed -n 1p $(dir $<)conf.txt)

rom/object/%.zobj: src/object/%.objex
	@echo "$(PRNT_GREN)Object$(PRNT_RSET):  $(notdir $@)"
	@mkdir -p include/object/
	@tools/z64convert --silent --in $< --out $@ $(ObjexSettings) --header include/object/$(notdir $(basename $<))_tmp.h --linker include/object/$(notdir $(basename $<))_tmp.ld
	@tools/cofi include/object/$(notdir $(basename $<))_tmp.ld include/object/$(notdir $(basename $<)).ld
	@tools/cofi include/object/$(notdir $(basename $<))_tmp.h include/object/$(notdir $(basename $<)).h

include/objects.ld: $(Object_zobj)
	@echo "$(PRNT_YELW)Linker$(PRNT_RSET):  $(notdir $@)"
	@-cat include/object/*.ld > $(@:.ld=_tmp.ld);
	@-tools/cofi __temp__include/objects.ld $@;
	@if [ ! -f "include/objects.ld" ]; then \
		touch include/objects.ld; \
	fi; \

# # # # # # # # # # # # # # # # # # # #
# CODE ACTOR                          #
# # # # # # # # # # # # # # # # # # # #

actor: $(Actor_O) $(Actor_Elf)

rom/actor/%.o: src/actor/%.c src/actor/%.h
	@echo "$(PRNT_CYAN)Actor$(PRNT_RSET):   $(notdir $@)"
	@printf "ENTRY_POINT = 0x80800000;" > $(@D)/entry.ld
	@$(mips64_gcc) -c $(GccFlags) $< -o $@

rom/actor/%.elf: rom/actor/%.o $(Linkers)
	@echo "$(PRNT_BLUE)Actor$(PRNT_RSET):   $(notdir $@)"
	@$(mips64_ld) -o $@ $< -L$(@D) $(LdFlags)
	@$(mips64_objdump) -drz $@ > $(@:.elf=.s)
	@printf "alloc_type = 0\n" > $(@D)/config.cfg
	@printf "vram_addr  = 0x80800000\n" >> $(@D)/config.cfg
	@printf "init_vars  = 0x%s\n" $$($(mips64_objdump) -t $@ | grep "_InitVars" | head -c 8) >> $(@D)/config.cfg
	@$(novl) $(nOvlFlags) 0x80800000 -o $(@D)/actor.zovl $@ -s

# # # # # # # # # # # # # # # # # # # #
# CODE GAMESTATE                      #
# # # # # # # # # # # # # # # # # # # #

system: $(FileChoose_O) rom/system/GameState_FileChoose/state.elf

rom/system/%.o: src/system/%.c
	@echo "$(PRNT_CYAN)State$(PRNT_RSET):   $(notdir $@)"
	@$(mips64_gcc) -c $(GccFlags) $< -o $@

rom/system/GameState_FileChoose/state.elf: $(FileChoose_O) $(Linkers)
	@echo "$(PRNT_BLUE)State$(PRNT_RSET):   $(notdir $@)"
	@printf "ENTRY_POINT = 0x80800000;" > $(@D)/entry.ld
	$(mips64_ld) -o $@ $(FileChoose_O) -L$(@D) $(LdFlags)
	@$(mips64_objdump) -drz $@ > $(@:.elf=.s)
	@$(mips64_objcopy) -R .MIPS.abiflags -O binary $@ $(@:.elf=.bin)
	
	@printf "vram_addr  = 0x80800000\n" > $(@D)/config.cfg
	@printf "init_func  = 0x%s\n" $$($(mips64_objdump) -t $@ | grep "_StateInit" | head -c 8) >> $(@D)/config.cfg
	@printf "dest_func  = 0x%s\n" $$($(mips64_objdump) -t $@ | grep "_StateDestroy" | head -c 8) >> $(@D)/config.cfg
	@$(novl) $(nOvlFlags) 0x80800000 -o $(@D)/actor.zovl $@ -s
	
# # # # # # # # # # # # # # # # # # # #
# LIB_USER                            #
# # # # # # # # # # # # # # # # # # # #

library: $(Library_O) include/z_lib_user.ld

rom/lib_user/%.o: src/lib_user/%.c include/objects.ld
	@echo "$(PRNT_CYAN)Library$(PRNT_RSET): $(notdir $@)"
	@$(mips64_gcc) -c $(GccFlags_Code) $< -o $@

rom/lib_user/%.o: src/lib_user/%.c src/lib_user/%.h include/objects.ld
	@echo "$(PRNT_CYAN)Library$(PRNT_RSET): $(notdir $@)"
	@$(mips64_gcc) -c $(GccFlags_Code) $< -o $@

z_code_lib_elf = rom/lib_user/z_lib_user.elf
z_code_lib_s   = rom/lib_user/z_lib_user.s
z_code_lib_bin = rom/lib_user/z_lib_user.bin

include/z_lib_user.ld: $(Library_O)
	@echo "$(PRNT_YELW)Linker$(PRNT_RSET):  $(notdir $@)"
	@printf "ENTRY_POINT = 0x80600000;\n" > rom/lib_user/entry.ld
	@$(mips64_ld) -o $(z_code_lib_elf) $^ -Lrom/lib_user -Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/ -T z64hdr.ld -T objects.ld --emit-relocs
	@$(mips64_objdump) -drz $(z_code_lib_elf) > $(z_code_lib_s)
	@$(mips64_objcopy) -R .MIPS.abiflags -O binary $(z_code_lib_elf) $(z_code_lib_bin)
	
	@$(mips64_objdump) -x -t $(z_code_lib_elf) | grep -E '806.*[O,F].*[r,t,d,b][o,e,a,s][d,x,t,s].*\0000' > include/__temp__z_code_lib.ld
	@$(elf2ld) include/__temp__z_code_lib.ld include/z_lib_user.ld

# # # # # # # # # # # # # # # # # # # #
# LIB_CODE                            #
# # # # # # # # # # # # # # # # # # # #

code: $(Code_O) $(Code_Elf) $(Code_SElf)

rom/lib_code/%.o: src/lib_code/%.c src/lib_code/%.cfg
	@echo "$(PRNT_CYAN)Code$(PRNT_RSET):    $(notdir $@)"
	@$(mips64_gcc) -c $(GccFlags_Code) $< -o $@
	@cp $(<:.c=.cfg) $(@:.o=.cfg)

rom/lib_code/%.elf: rom/lib_code/%.o $(Linkers)
	@echo "$(PRNT_BLUE)Code$(PRNT_RSET):    $(notdir $@)"
	@mkdir -p $(EntryDir)
	@printf "ENTRY_POINT = %s;\n" $$(grep 'ram' $(@:.elf=.cfg) | cut -d " " -f 3) > $(EntryDir)/entry.ld
	@$(mips64_ld) -o $@ $< -L$(EntryDir)/ $(LdFlags)
	@$(mips64_objdump) -drz $@ > $(@:.elf=.s)
	@$(mips64_objcopy) -R .MIPS.abiflags -O binary $@ $(@:.elf=.bin)

rom/lib_code/%.elf: src/lib_code/%.s $(Linkers)
	@cp $(<:.s=.cfg) $(@:.elf=.cfg)
	@echo "$(PRNT_BLUE)Asm$(PRNT_RSET):     $(notdir $@)"
	@mkdir -p $(EntryDir)
	@printf "ENTRY_POINT = %s;\n" $$(grep 'ram' $(@:.elf=.cfg) | cut -d " " -f 3) > $(EntryDir)/entry.ld
	@$(mips64_gcc) -x assembler-with-cpp -c $< -o $(@:.elf=.o) -L$(EntryDir)/ -Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/ -T z64hdr.ld -T objects.ld -T z_lib_user.ld
	@$(mips64_ld) -o $@ $(@:.elf=.o) -L$(EntryDir)/ $(LdFlags)
	@$(mips64_objcopy) -R .MIPS.abiflags -O binary $@ $(@:.elf=.bin)