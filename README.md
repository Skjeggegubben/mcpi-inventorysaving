# mcpi-inventorysaving
Inventory saving for mcpi when on dedicated/external server.

Made with lots of help from Bigjango, starting point was his old inventorysaving.cpp which could load inventory from a file on game start.

Compile your own and smoke it:
mkdir build
cd build
cmake ..
make -j$(nproc)
cp libinventorysaving.so ~/.minecraft-pi/mods


Or download libinventorysaving.so from here and put in dir /mods
