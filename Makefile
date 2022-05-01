CFLAGS         := -s -flto -Wall -DEXTLIB=111 -pthread
CFLAGS_MAIN    := -s -Wall -pthread
OPT_WIN32      := -Ofast
OPT_LINUX      := -Ofast
SOURCE_C       := $(shell find src/* -type f -name '*.c')
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f)
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f)

RELEASE_EXECUTABLE_LINUX := app_linux/z64rom
RELEASE_EXECUTABLE_WIN32 := app_win32/z64rom.exe

SOURCE_nOVL_C            := $(shell find tools/nOVL/src/* -type f -name '*.c')

PRNT_DGRY := \e[90;2m
PRNT_GRAY := \e[0;90m
PRNT_REDD := \e[0;91m
PRNT_GREN := \e[0;92m
PRNT_YELW := \e[0;93m
PRNT_BLUE := \e[0;94m
PRNT_PRPL := \e[0;95m
PRNT_CYAN := \e[0;96m
PRNT_RSET := \e[m

HEADER := src/z64rom.h src/Audio.h

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(RELEASE_EXECUTABLE_WIN32)) \
	$(dir $(SOURCE_O_WIN32)) \
	$(dir $(RELEASE_EXECUTABLE_LINUX)) \
	$(dir $(SOURCE_O_LINUX)) \
	, $(dir)))
$(shell mkdir -p app_win32/tools/)
$(shell mkdir -p app_linux/tools/)

.PHONY: project-files-linux \
	    project-files-win32 \
		default \
		linux \
		win32 \
		all-release \
		linux-release \
		win32-release \
		clean-all \
		clean-sub \
		clean-release \
		clean

default: linux
all: linux win32
hdrup: hdrup.exe

include $(C_INCLUDE_PATH)/ExtLib.mk

linux: project-files-linux $(SOURCE_O_LINUX) $(RELEASE_EXECUTABLE_LINUX)
win32: project-files-win32 $(SOURCE_O_WIN32) $(RELEASE_EXECUTABLE_WIN32)

hdrup.exe: tools/hdrup.c
	@i686-w64-mingw32.static-gcc -o $@ $^ $(OPT_WIN32) $(CFLAGS) -DNDEBUG -lm -D_WIN32

# # # # # # # # # # # # # # # # # # # #
# PROJECT                             #
# # # # # # # # # # # # # # # # # # # #

# tools: project/tools/elf2ld \
# 	project/tools/cofi \
# 	project/tools/novl \
# 	project/tools/z64convert \
# 	project/tools/depper \
# 	project/tools/novl.exe

project-files-linux: project/tools/novl project/tools/z64audio project/tools/z64convert
	@mkdir -p             app_linux/src/actor
	@mkdir -p             app_linux/src/object
	@cp -r project/patch  app_linux/
	@cp -r project/src    app_linux/
	@cp -r project/tools  app_linux/
	@cp -r project/.[^.]* app_linux/
	@rm -f                app_linux/*/*/*.txt
	@rm -f                app_linux/*/*.txt
	@rm -f                app_linux/tools/z64audio.exe
	@rm -f                app_linux/tools/novl.exe

project-files-win32: project/tools/novl.exe project/tools/z64audio.exe project/tools/z64convert.exe
	@mkdir -p             app_win32/src/actor
	@mkdir -p             app_win32/src/object
	@cp -r project/patch  app_win32/
	@cp -r project/src    app_win32/
	@cp -r project/tools  app_win32/
	@cp -r project/.[^.]* app_win32/
	@cp tools/wget.exe    app_win32/tools/
	@rm -f                app_win32/*/*/*.txt
	@rm -f                app_win32/*/*.txt
	@rm -f                app_win32/tools/z64audio
	@rm -f                app_win32/tools/novl

# # # # # # # # # # # # # # # # # # # #
# CLEAN                               #
# # # # # # # # # # # # # # # # # # # #

clean-all: clean-sub clean-release clean

clean-sub:
	@$(MAKE) -C tools/z64audio clean --no-print-directory --silent
	@rm -f project/tools/z64audio
	@rm -f project/tools/z64audio.exe
	@rm -f project/tools/novl
	@rm -f project/tools/novl.exe
	@rm -f project/tools/z64convert
	@rm -f project/tools/z64convert.exe

clean: extlib_clean
	@rm -f -R bin
	@rm -f project/tools/elf2ld

clean-release:
	@rm -f -R app_linux
	@rm -f -R app_win32

binutils: app_linux/tools/mips64-binutils/bin/mips64-gcc app_win32/tools/mips64-binutils/bin/mips64-gcc.exe

app_linux/tools/mips64-binutils/bin/mips64-gcc: tools/mips64-binutils-linux.7z
	@echo "$(PRNT_RSET)[$(PRNT_YELW)$(notdir $@)$(PRNT_RSET)]"
	@7z x $< -oapp_linux/tools/mips64-binutils/ -aos > /dev/null 2>&1
	@touch $@

app_win32/tools/mips64-binutils/bin/mips64-gcc.exe: tools/mips64-binutils-win32.7z
	@echo "$(PRNT_RSET)[$(PRNT_YELW)$(notdir $@)$(PRNT_RSET)]"
	@7z x $< -oapp_win32/tools/mips64-binutils/ -aos > /dev/null 2>&1
	@touch $@
	
# # # # # # # # # # # # # # # # # # # #
# TOOLS                               #
# # # # # # # # # # # # # # # # # # # #

project/tools/novl: $(SOURCE_nOVL_C)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -o $@ -s -Os -DNOVL_DEBUG=1 -flto $^ -Wno-unused-result `pkg-config --cflags --libs libelf glib-2.0`
	
project/tools/novl.exe: $(SOURCE_nOVL_C)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ -Wall -DNDEBUG -Os -s -flto -mconsole tools/novl/src/*.c `i686-w64-mingw32.static-pkg-config --cflags --libs glib-2.0` -Itools/novl/libelf -D__LIBELF_INTERNAL__=1 -DHAVE_MEMCPY=1 -DHAVE_MEMCMP=1 -DHAVE_MEMMOVE=1 -DSTDC_HEADERS=1 tools/novl/libelf/*.c

project/tools/z64audio: tools/z64audio/z64audio.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64audio linux -j --no-print-directory --silent
	@cp $(<:.c=) $@

project/tools/z64audio.exe: tools/z64audio/z64audio.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64audio win32 -j --no-print-directory --silent
	@cp $(<:.c=.exe) $@
	
project/tools/z64convert: tools/z64convert/src/z64convert.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64convert lincli -j --no-print-directory --silent
	@cp tools/z64convert/z64convert-cli $@
	
project/tools/z64convert.exe: tools/z64convert/src/z64convert.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64convert wincli -j --no-print-directory --silent
	@cp tools/z64convert/z64convert-cli.exe $@

# # # # # # # # # # # # # # # # # # # #
# LINUX BUILD                         #
# # # # # # # # # # # # # # # # # # # #

bin/linux/src/lib/%.o: src/lib/%.c src/lib/%.h
bin/linux/src/lib/%.o: src/lib/%.c
	@echo "$(PRNT_RSET)[$(PRNT_YELW)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $<
	
bin/linux/%.o: %.c %.h $(HEADER) $(ExtLib_H)
bin/linux/%.o: %.c $(HEADER) $(ExtLib_H)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS)
	@objdump -drz $@ > $(@:.o=.s)

$(RELEASE_EXECUTABLE_LINUX): z64rom.c $(SOURCE_O_LINUX) $(ExtLib_Linux_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ -L$(C_INCLUDE_PATH) -L$(C_INCLUDE_PATH) -lm -ldl $(OPT_LINUX) $(CFLAGS_MAIN) -DNDEBUG

# # # # # # # # # # # # # # # # # # # #
# WINDOWS-32 BUILD                    #
# # # # # # # # # # # # # # # # # # # #

bin/win32/src/lib/%.o: src/lib/%.c src/lib/%.h
bin/win32/src/lib/%.o: src/lib/%.c
	@echo "$(PRNT_RSET)[$(PRNT_YELW)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $<
	
bin/win32/src/External.o: src/External.c $(ExtLib_H)
bin/win32/%.o: %.c %.h $(HEADER) $(ExtLib_H)
bin/win32/%.o: %.c $(HEADER) $(ExtLib_H)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -D_WIN32
	@i686-w64-mingw32.static-objdump -drz $@ > $(@:.o=.s)

bin/win32/icon.o: src/icon.rc src/icon.ico
	@i686-w64-mingw32.static-windres -o $@ $<

$(RELEASE_EXECUTABLE_WIN32): z64rom.c bin/win32/icon.o $(SOURCE_O_WIN32) $(ExtLib_Win32_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ -L$(C_INCLUDE_PATH) -lm $(OPT_WIN32) $(CFLAGS_MAIN) -DNDEBUG -D_WIN32
	@upx -9 --best --compress-icons=0 $@
	
