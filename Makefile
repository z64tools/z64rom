CFLAGS         := -s -flto -Wall
OPT_WIN32      := -Os
OPT_LINUX      := -Ofast
SOURCE_C       := $(shell find lib/* -type f -name '*.c')
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f)
SOURCE_O_RELEASE_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/ndebug/$f)
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f)
SOURCE_O_RELEASE_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/ndebug/$f)

RELEASE_EXECUTABLE_LINUX := release_linux/z64rom
DEBUG_EXECUTABLE_LINUX   := release_linux/z64rom-debug
RELEASE_EXECUTABLE_WIN32 := release_win32/z64rom.exe
DEBUG_EXECUTABLE_WIN32   := release_win32/z64rom-debug.exe

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

HEADER := lib/z64rom.h lib/ExtLib.h lib/Audio.h

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(SOURCE_O_WIN32)) \
	$(dir $(RELEASE_EXECUTABLE_WIN32)) \
	$(dir $(SOURCE_O_RELEASE_WIN32)) \
	$(dir $(SOURCE_O_LINUX)) \
	$(dir $(RELEASE_EXECUTABLE_LINUX)) \
	$(dir $(SOURCE_O_RELEASE_LINUX)), $(dir)))
$(shell mkdir -p release_win32/tools/)
$(shell mkdir -p release_linux/tools/)

.PHONY: project-files-linux \
	    project-files-win32 \
		clean \
		tools \
		clean-release \
		default \
		linux \
		win32 \
		all-release \
		linux-release \
		win32-release

default: linux
all: linux win32
all-release: linux-release win32-release

linux:         project-files-linux $(SOURCE_O_LINUX)         $(DEBUG_EXECUTABLE_LINUX)
linux-release: project-files-linux $(SOURCE_O_RELEASE_LINUX) $(RELEASE_EXECUTABLE_LINUX)
win32:         project-files-win32 $(SOURCE_O_WIN32)         $(DEBUG_EXECUTABLE_WIN32)
win32-release: project-files-win32 $(SOURCE_O_RELEASE_WIN32) $(RELEASE_EXECUTABLE_WIN32)

# # # # # # # # # # # # # # # # # # # #
# PROJECT                             #
# # # # # # # # # # # # # # # # # # # #

tools: project/tools/elf2ld project/tools/cofi project/tools/novl project/tools/z64convert project/tools/depper

project-files-linux: tools project/tools/z64audio
	@mkdir -p             release_linux/src/actor
	@mkdir -p             release_linux/src/object
	@cp -r project/patch  release_linux/
	@cp -r project/src    release_linux/
	@cp -R project/*      release_linux/
	@rm -f                release_linux/*/*/*.txt
	@rm -f                release_linux/tools/z64audio.exe

project-files-win32: tools project/tools/z64audio.exe
	@mkdir -p             release_win32/src/actor
	@mkdir -p             release_win32/src/object
	@cp -r project/patch  release_win32/
	@cp -r project/src    release_win32/
	@cp -R project/*      release_win32/
	@rm -f                release_win32/*/*/*.txt
	@rm -f                release_win32/tools/z64audio

# # # # # # # # # # # # # # # # # # # #
# CLEAN                               #
# # # # # # # # # # # # # # # # # # # #

clean-all: clean-sub clean

clean-sub:
	@$(MAKE) -C tools/z64audio clean --no-print-directory --silent
	@rm -f project/tools/z64audio
	@rm -f project/tools/z64audio.exe
	@rm -f project/tools/novl

clean:
	@rm -f -R bin
	@rm -f -R release_linux
	@rm -f -R release_win32
	@rm -f project/tools/elf2ld
	
# # # # # # # # # # # # # # # # # # # #
# TOOLS                               #
# # # # # # # # # # # # # # # # # # # #

project/tools/novl: $(SOURCE_nOVL_C)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -o $@ -s -Os -DNOVL_DEBUG=1 -flto $^ -Wno-unused-result `pkg-config --cflags --libs libelf glib-2.0`

project/tools/z64audio: tools/z64audio/z64audio.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64audio linux -j --no-print-directory --silent
	@cp $(<:.c=) $@
	
project/tools/z64convert: tools/z64convert/src/z64convert.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64convert lincli -j --no-print-directory --silent
	@cp tools/z64convert/z64convert-cli $@

project/tools/z64audio.exe: tools/z64audio/z64audio.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64audio win32 -j --no-print-directory --silent
	@cp $(<:.c=.exe) $@
	
project/tools/%: tools/%.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -o $@ $< lib/ExtLib.c $(OPT_LINUX) $(CFLAGS) -DNDEBUG

# # # # # # # # # # # # # # # # # # # #
# LINUX BUILD                         #
# # # # # # # # # # # # # # # # # # # #

bin/linux/ndebug/%.o: %.c %.h $(HEADER)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS) -DNDEBUG
	
bin/linux/ndebug/%.o: %.c $(HEADER)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS) -DNDEBUG -Wno-missing-braces

bin/linux/%.o: %.c %.h $(HEADER)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS)
	
bin/linux/%.o: %.c $(HEADER)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS) -Wno-missing-braces

$(RELEASE_EXECUTABLE_LINUX): z64rom.c $(SOURCE_O_RELEASE_LINUX)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ $(OPT_LINUX) $(CFLAGS) -DNDEBUG -lm

$(DEBUG_EXECUTABLE_LINUX): z64rom.c $(SOURCE_O_LINUX)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ $(OPT_LINUX) $(CFLAGS) -lm

# # # # # # # # # # # # # # # # # # # #
# WINDOWS-32 BUILD                    #
# # # # # # # # # # # # # # # # # # # #

bin/win32/ndebug/%.o: %.c %.h $(HEADER)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -DNDEBUG -D_WIN32
	
bin/win32/ndebug/%.o: %.c $(HEADER)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -DNDEBUG -D_WIN32 -Wno-missing-braces
	
bin/win32/%.o: %.c %.h $(HEADER)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -D_WIN32
	
bin/win32/%.o: %.c $(HEADER)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -D_WIN32 -Wno-missing-braces

bin/win32/icon.o: lib/icon.rc lib/icon.ico
	@i686-w64-mingw32.static-windres -o $@ $<

$(RELEASE_EXECUTABLE_WIN32): z64rom.c bin/win32/icon.o $(SOURCE_O_RELEASE_WIN32)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(OPT_WIN32) $(CFLAGS) -DNDEBUG -lm -D_WIN32

$(DEBUG_EXECUTABLE_WIN32): z64rom.c bin/win32/icon.o $(SOURCE_O_WIN32)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(OPT_WIN32) $(CFLAGS) -lm -D_WIN32
	
