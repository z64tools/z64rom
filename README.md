# This repo is archived. Newer fork: https://github.com/z64utils/z64rom

# z64rom

### [Download z64rom here!](https://github.com/z64tools/z64rom/releases)

### Read the documentation [here](https://github.com/z64tools/z64rom/wiki)!

# Credits

**Documentation:** <br>
DezZival <br>
sklitte22

**Testers:** <br>
logdebug <br>
Nokaubure <br>
Skawo <br>
sklitte22 <br>
Zeldaboy14 <br>
zfg

**Special Thanks:** <br>
Dragorn421 <br>
Sauraen <br>
Tharo <br>
zel

**Tools:** <br>
[z64convert](https://github.com/z64me/z64convert) by [z64me](https://z64.me) <br>
[novl](https://github.com/z64me/nOvl) by [z64me](https://z64.me) <br>
[seq64](https://github.com/sauraen/seq64) by [Sauraen](https://github.com/sauraen/) <br>
Sequence Assembler (seqas) by [Zelda Reverse Engineering Team](https://zelda64.dev/) <br>
[z64audio](https://github.com/z64tools/z64audio) by [rankaisija](https://github.com/rankaisija64) <br>

# Development

## Compiling

Compiling works on Linux, native or WSL.
You need git, make, and possibly other standard tools (`apt install build-essential git` ...).

You may need (?) the following packages to build:

```bash
sudo apt-get install glib2.0-dev libelf-dev libcurl4-gnutls-dev
```

Clone repository and all the submodules:

```bash
git clone --recurse-submodules https://github.com/z64tools/z64rom.git
```

`cd` inside z64rom directory and do the following:

```bash
# clones ExtLib to the current directory
# if the permission is denied run this: chmod u+x setup.sh
./setup.sh

# Magic happens here
make linux -j
```

This will output linux build into `app_linux/z64rom`
