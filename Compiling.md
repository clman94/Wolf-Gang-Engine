# Compiling the Wolf-Gang Engine
This is the simple set of instructions to compiling this behemoth. (Note: These instructions may be a little vague and incomplete. Any suggestions are welcome.)

Some things you may need:
- cmake
- cmake-gui (It will make things easier)
- Your favorate C++ compiler (VS C++, GCC, etc...)
- An IDE. It's helpful. (Code::Blocks, VS, etc...)
It is recommended you have the latest version of your C++ compiler that supports the majority of the C++17 standard. This includes GCC-7.

### Step 1: Probably step 0

Clone the repository with all the submodules and enter the directory.
```bash
$ git clone https://github.com/clman94/Wolf-Gang-Engine.git ./WolfGangEngine --recursive
$ cd ./WolfGangEngine
```
### Step 2: Compiling SFML
Open up cmake-gui first of all.
```bash
$ cd ./3rdparty/sfml
$ cmake-gui ./
```
Select _Configure_ and let it do its thing. When its complete, Some highlighted red list will pop up.
Do the following:
- Deselect `BUILD_SHARED_LIBS`
- Select `SFML_USE_STATIC_STD_LIBS`

Then press the _Generate_ button and, again, let it do its thing. This will generate the needed configuration for whatever IDE or compiler you chose.
**Compile the sources for both Release and Debug.**

*Ensure that both the debug and release libs are on the top level of the `.3rdparty/sfml/lib` directory.*

### Step 3: Compiling Angelscript
Angelscript includes a nice directory littered with projects already setup for you in the `.3rdparty/AngelScript/sdk/angelscript/projects` directory. Just pick your favorite one and compile for both debug and release.

*Ensure that both the debug and release libs are on the top level of the `.3rdparty/AngelScript/sdk/angelscript/lib` directory.*

### Step 4: Setting up the engine
Cd to the top directory and open cmake-gui. Set the "Where to build the binaries" to a subdirectory called `./build`.
Hit configure and generate.

TODO: MORE DETAIL