CFLAGS          = -Wall -DEXTLIB=142 -DNDEBUG
CFLAGS_MAIN     = -Wall -DNDEBUG
OPT_WIN32      := -Ofast
OPT_LINUX      := -Ofast
SOURCE_C        = $(shell find src/* -type f -name '*.c')
SOURCE_C       += z64rom.c
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f)
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f)

RELEASE_EXECUTABLE_LINUX := app_linux/z64rom
RELEASE_EXECUTABLE_WIN32 := app_win32/z64rom.exe

TOOLS_WIN32    := project/tools/novl.exe project/tools/z64audio.exe project/tools/z64convert.exe project/tools/z64compress.exe project/tools/seqas.exe
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

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(RELEASE_EXECUTABLE_WIN32)) \
	$(dir $(SOURCE_O_WIN32)) \
	\
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
		clean \
		update-z64audio

default: linux
all: linux win32

update-z64audio:
	@cd tools/z64audio/ && git pull https://github.com/z64tools/z64audio main && cd ../..

linux: project-files-linux $(RELEASE_EXECUTABLE_LINUX)
win32: project-files-win32 $(RELEASE_EXECUTABLE_WIN32)

# # # # # # # # # # # # # # # # # # # #
# PROJECT                             #
# # # # # # # # # # # # # # # # # # # #

project-files-linux: $(TOOLS_LINUX)
	@mkdir -p             app_linux/src/actor
	@mkdir -p             app_linux/src/object
	@cp -r project/*      app_linux/
	@cp -r project/.[^.]* app_linux/
	@rm -f                app_linux/*/*/*.txt
	@rm -f                app_linux/*/*.txt
	@rm -f                app_linux/tools/z64convert.exe
	@rm -f                app_linux/tools/z64audio.exe
	@rm -f                app_linux/tools/novl.exe
	@rm -f                app_linux/tools/z64compress.exe
	@rm -f                app_linux/tools/seq64_console.exe
	@rm -f                app_linux/tools/seqas.exe
project-files-win32: $(TOOLS_WIN32)
	@mkdir -p             app_win32/src/actor
	@mkdir -p             app_win32/src/object
	@cp -r project/*      app_win32/
	@cp -r project/.[^.]* app_win32/
	@cp tools/wget.exe    app_win32/tools/
	@rm -f                app_win32/*/*/*.txt
	@rm -f                app_win32/*/*.txt
	@rm -f                app_win32/tools/z64convert
	@rm -f                app_win32/tools/z64audio
	@rm -f                app_win32/tools/novl
	@rm -f                app_win32/tools/z64compress
	@rm -f                app_win32/tools/seq64_console
	@rm -f                app_win32/tools/seqas

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

project/tools/z64compress:
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64compress linux -j --no-print-directory --silent
	@cp tools/z64compress/z64compress $@
	@$(MAKE) -C tools/z64compress clean --no-print-directory --silent
project/tools/z64compress.exe:
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64compress win32 -j --no-print-directory --silent
	@cp tools/z64compress/z64compress.exe $@
	@$(MAKE) -C tools/z64compress clean --no-print-directory --silent
	
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

bin/win32/icon.o: src/icon.rc src/icon.ico
	@i686-w64-mingw32.static-windres -o $@ $<

$(RELEASE_EXECUTABLE_WIN32): bin/win32/icon.o $(SOURCE_O_WIN32) $(ExtLib_Win32_O) $(Zip_Win32_O) $(Xm_Win32_O) $(Audio_Win32_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ -lm -pthread $(OPT_WIN32) $(CFLAGS_MAIN) -D_WIN32
