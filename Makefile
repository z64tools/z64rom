CFLAGS          = -Wall -Wno-switch -DEXTLIB=153 -DNDEBUG
CFLAGS_MAIN     = -Wall -Wno-switch -DNDEBUG
OPT_WIN32      := -Ofast
OPT_LINUX      := -Ofast
SOURCE_C       := $(shell find src/* -type f -name '*.c')
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f)
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f)

RELEASE_EXECUTABLE_LINUX := app_linux/z64rom
RELEASE_EXECUTABLE_WIN32 := app_win32/z64rom.exe

TOOLS_WIN32    := project/tools/novl.exe project/tools/z64audio.exe project/tools/z64convert.exe project/tools/seqas.exe
TOOLS_LINUX    := $(foreach f,$(TOOLS_WIN32:.exe=),$f)

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

include $(C_INCLUDE_PATH)/ExtLib.mk

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
		clean \
		update-z64audio

default: linux
all: linux win32
	
PROJECT_FILES_L      = $(shell find project/* -type f -not -name '*.txt' -not -name '*.exe' -not -name '*.reg')
PROJECT_FILES_W      = $(shell find project/* -type f -not -name '*.txt' -not -iname 'z64convert' -not -iname 'z64audio' -not -iname 'novl' -not -iname 'seq64_console' -not -iname 'seqas')
PROJECT_FILES_L     += $(TOOLS_LINUX)
PROJECT_FILES_W     += $(TOOLS_WIN32)
PROJECT_FILES_LINUX := $(foreach f,$(PROJECT_FILES_L:project/%=%),app_linux/$f)
PROJECT_FILES_WIN32 := $(foreach f,$(PROJECT_FILES_W:project/%=%),app_win32/$f)

update-z64audio:
	@cd tools/z64audio/ && git pull https://github.com/z64tools/z64audio main && cd ../..

linux: project-files-linux $(RELEASE_EXECUTABLE_LINUX)
win32: project-files-win32 $(RELEASE_EXECUTABLE_WIN32)

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(RELEASE_EXECUTABLE_LINUX)) \
	$(dir $(PROJECT_FILES_LINUX)) \
	$(dir $(SOURCE_O_LINUX)) \
	\
	$(dir $(RELEASE_EXECUTABLE_WIN32)) \
	$(dir $(PROJECT_FILES_WIN32)) \
	$(dir $(SOURCE_O_WIN32)) \
	, $(dir)))
$(shell mkdir -p app_linux/tools/)
$(shell mkdir -p app_win32/tools/)

# # # # # # # # # # # # # # # # # # # #
# PROJECT                             #
# # # # # # # # # # # # # # # # # # # #

project-files-linux: $(PROJECT_FILES_LINUX)
project-files-win32: $(PROJECT_FILES_WIN32)

app_linux/%: project/% $(TOOLS_LINUX)
	@echo "$(PRNT_RSET)[copy $(PRNT_BLUE)$@$(PRNT_RSET)]"
	@rm -f $@
	@cp $< $@
app_win32/%: project/% $(TOOLS_WIN32)
	@echo "$(PRNT_RSET)[copy $(PRNT_BLUE)$@$(PRNT_RSET)]"
	@rm -f $@
	@cp $< $@

# # # # # # # # # # # # # # # # # # # #
# CLEAN                               #
# # # # # # # # # # # # # # # # # # # #

clean-all: clean-sub clean-release clean

clean-sub:
	@$(MAKE) -C tools/z64audio clean --no-print-directory --silent
	@rm -f $(TOOLS_WIN32) $(TOOLS_LINUX)

clean:
	@rm -f -R bin

clean-release:
	@rm -f -R app_linux
	@rm -f -R app_win32
	
# # # # # # # # # # # # # # # # # # # #
# TOOLS                               #
# # # # # # # # # # # # # # # # # # # #

project/tools/novl: $(SOURCE_nOVL_C)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -o $@ -s -Os -DNOVL_DEBUG=1 -flto $^ -Wno-unused-result `pkg-config --cflags --libs libelf glib-2.0`
project/tools/novl.exe: $(SOURCE_nOVL_C)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ tools/novl/src/*.c -Wall -Wno-unused-const-variable -DNDEBUG -Os -s -flto `i686-w64-mingw32.static-pkg-config --cflags --libs glib-2.0` -luuid -Itools/novl/libelf -D__LIBELF_INTERNAL__=1 -DHAVE_MEMCPY=1 -DHAVE_MEMCMP=1 -DHAVE_MEMMOVE=1 -DSTDC_HEADERS=1 tools/novl/libelf/*.c

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
	
project/tools/seqas: tools/seqas.cpp
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@g++ $^ -o $@ -I tools/ -Wall -Wextra -pedantic -std=c++17 -g -O2 -lstdc++ -lm
project/tools/seqas.exe: tools/seqas.cpp
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-g++ $^ -o $@ -I tools/ -Wall -Wextra -pedantic -std=c++17 -g -O2 -lstdc++ -lm

# # # # # # # # # # # # # # # # # # # #
# LINUX BUILD                         #
# # # # # # # # # # # # # # # # # # # #
	
bin/linux/%.o: %.c %.h $(HEADER) $(ExtLib_H)
bin/linux/%.o: %.c $(HEADER) $(ExtLib_H)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS)

$(RELEASE_EXECUTABLE_LINUX): $(SOURCE_O_LINUX) $(ExtLib_Linux_O) $(Zip_Linux_O) $(Xm_Linux_O) $(Audio_Linux_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ -lm -ldl -pthread $(OPT_LINUX) $(CFLAGS_MAIN)

# # # # # # # # # # # # # # # # # # # #
# WIN32 BUILD                         #
# # # # # # # # # # # # # # # # # # # #
	
bin/win32/%.o: %.c %.h $(HEADER) $(ExtLib_H)
bin/win32/%.o: %.c $(HEADER) $(ExtLib_H)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -D_WIN32
	@i686-w64-mingw32.static-objdump -drz $@ > $(@:.o=.s)

bin/win32/icon.o: src/icon.rc src/icon.ico
	@i686-w64-mingw32.static-windres -o $@ $<

$(RELEASE_EXECUTABLE_WIN32): bin/win32/icon.o $(SOURCE_O_WIN32) $(ExtLib_Win32_O) $(Zip_Win32_O) $(Xm_Win32_O) $(Audio_Win32_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ -lm -pthread $(OPT_WIN32) $(CFLAGS_MAIN) -D_WIN32
