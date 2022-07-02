# z64rom

### For questions and feature requests [join z64tools discord](https://discord.gg/52DgAggYAT)!

### [Download z64rom here!](https://github.com/z64tools/z64rom/releases)

### Read the documentation [here](https://github.com/z64tools/z64rom/wiki)!

# Credits

**Documentation:** <br>
DezZival <br>
sklitte22

**Testers:** <br>
Zeldaboy14 <br>
Nokaubure <br>
sklitte22

**Special Thanks:** <br>
Sauraen <br>
Tharo <br>
Dragorn421 <br>
zel

**Tools:** <br>
[z64convert](https://github.com/z64me/z64convert) by [z64me](https://z64.me) <br>
[novl](https://github.com/z64me/nOvl) by [z64me](https://z64.me) <br>
[seq64](https://github.com/sauraen/seq64) by [Sauraen](https://github.com/sauraen/) <br>
Sequence Assembler (seqas) by [Zelda Reverse Engineering Team](https://zelda64.dev/) <br>
[z64audio](https://github.com/z64tools/z64audio) by [rankaisija](https://github.com/rankaisija64) <br>

# Development

## Compiling

Compiling works on Linux, native or WSL. The following instructions were tested on Ubuntu.

You need git, make, and possibly other standard tools.

You may need (?) the following packages to build:

    sudo apt-get install glib2.0-dev libelf-dev

Clone z64rom and its submodules:

    git clone --recurse-submodules git@github.com:z64tools/z64rom.git

Clone ExtLib and its submodules somewhere:

    git clone --recurse-submodules git@github.com:rankaisija64/ExtLib.git

Set the environment variable `C_INCLUDE_PATH` to the path to the ExtLib folder:

    export C_INCLUDE_PATH=/path/to/ExtLib

With the z64rom folder as working directory, run `make linux` or `make win32` to build for linux or for win32.

The result of the compilation is in `app_linux` or `app_win32`.

As an example, the following commands clone and build z64rom targeting linux:

    git clone --recurse-submodules git@github.com:z64tools/z64rom.git
    cd z64rom
    git clone --recurse-submodules git@github.com:rankaisija64/ExtLib.git
    export C_INCLUDE_PATH=$(pwd)/ExtLib/
    make linux
