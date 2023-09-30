# mcpi-inventorysaving
Inventory saving mod for handling the saving and loading of inventory in mcpi when on dedicated/external server.
Saves item data to file with server ip in 1min. intervals during game play, loads only once when entering server. 

Made with lots of help from Bigjango, starting point was his old inventorysaving.cpp which could load inventory from a file on game start.

Compile your own and smoke it:

sudo apt install g++-arm-linux-gnueabihf\n
sudo apt install gcc-arm-linux-gnueabihf

mkdir build

cd build

cmake ..

make -j$(nproc)

cp libinventorysaving.so ~/.minecraft-pi/mods


Or download libinventorysaving.so from here and put in dir /mods
