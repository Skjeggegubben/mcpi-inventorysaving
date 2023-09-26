#include <string>
#include <sstream> //For string split function
#include <fstream> //For file reading and writing
#include <thread> //For the timers
#include <chrono> //For the timers
#include <functional> //For the timers

#include <libreborn/libreborn.h>
#include <symbols/minecraft.h>
#include <mods/misc/misc.h>

using namespace std; //so that we don't need that darn std:: in front of everything!
using templateFunc = function<void()>; //For the setInterval in Timer class to work
typedef unsigned char uchar;

//Stuff for the manipulating of inventory data
typedef unsigned char *(*Inventory_t)(unsigned char *inventory, unsigned char *player, uint32_t is_creative);
static Inventory_t Inventory = (Inventory_t) 0x8e768;

//Stuff for the jacking trick that gets the IP
typedef bool (*RakNetInstance_connect_t)(uchar *self, char *ip, int host);
static RakNetInstance_connect_t RakNetInstance_connect = (RakNetInstance_connect_t) 0x735f0;
static void *RakNetInstance_connect_vtable_addr = (void*) 0x109af4;

//Declarations, declare the global vars and functions blah blah
ItemInstance *get_item_at_slot(int slot);
vector<string> split_str(string str, char theChar);
void showInfo(string text);
void saveStuff();
void loadStuff();
bool canDoStuff();
unsigned char *minecraft;
string server = "";
string inventoryString = "";



//Nice split_str function
vector<string> split_str(string str, char theChar) {
    stringstream ss(str);
    vector<string> ret;
    string tmp = "";
    while (getline(ss, tmp, theChar)) {
        ret.push_back(tmp);
    }
    return ret;
}



//Nice class with function for doing intervals
class Timer {
    bool clear = false;
public:
    void setTimeout(templateFunc function, int delay) {
        this->clear = false;
        thread t([=]() {
            if(this->clear) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            if(this->clear) return;
                function();
        });
        t.detach();
    }

	void setInterval(templateFunc function, int interval) {
        this->clear = false;
        thread t([=]() {
            while(true) {
                if(this->clear) return;
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                if(this->clear) return;
                    function();
            }
        });
        t.detach();
    }

    void stop() {
        this->clear = true;
    }
};
//Declare the timer vars for loading and saving
Timer loadTimer = Timer();
Timer saveTimer = Timer();
//Could be up at the top if separate class file?



//The functions that gets us the minecraft obj
void mcpi_callback(unsigned char *mcpi) {
    // Runs on every tick, sets the minecraft var. Should be prevented from running all the time?
    minecraft = mcpi; //Since according to TheBrokenRail, this will never change.
}
unsigned char *get_minecraft() {
    return minecraft;
}



//Gets inventory, needed for loading stuff into inventory
uchar *get_inventory() { 
    // Get minecraft and the player
    uchar *minecraft = get_minecraft();
    uchar *player = *(uchar **) (minecraft + Minecraft_player_property_offset);
    if (player != NULL) {
        // Get the player inventory
        uchar *inventory = *(uchar **) (player + Player_inventory_property_offset);
        return inventory;
    }
    // The player doesn't exist
    return NULL;
}



// Gets the item from the slot number, needed for saving all items
ItemInstance *get_item_at_slot(int slot) { 
    if (slot == -256) {
        return NULL;
    }
    uchar *inventory = get_inventory();
    if (inventory != NULL) {
        uchar *inventory_vtable = *(uchar **) inventory;
        FillingContainer_getItem_t FillingContainer_getItem = *(FillingContainer_getItem_t *) (inventory_vtable + FillingContainer_getItem_vtable_offset);
        ItemInstance *inventory_item = (*FillingContainer_getItem)(inventory, slot);
        return inventory_item;
    }
    return NULL;
}



//The function that will be run only once, but repeats if game still not loaded, stops interval when success
void loadStuff(){
    if(canDoStuff() ) {        
        string thePath = string(getenv("HOME")) + "/.minecraft-pi/items_"+server+".txt";
        ifstream infile(thePath);
        if(!infile.is_open()){
            showInfo("Got no inventory for this server..");
            loadTimer.stop();
        } else {
            //showInfo("Trying to load inventory!");
            inventoryString = "";
            string theLine;
            uchar *inventory = get_inventory();
            
            while (infile >> theLine ) {
                if( theLine.length() > 0){
                    vector<string> data = split_str(theLine, ':');// split by : to get separate values
                    inventoryString += data[0] +":"+ data[1] +":"+ data[2] + "\n"; //set string so we can compare later for change
                    ItemInstance *item_instance = new ItemInstance;
                    ALLOC_CHECK(item_instance); //nice but I dunno what this actually does :S reserves memory space?
                    item_instance->id = stoi( data[0] );
                    item_instance->auxiliary = stoi( data[1] );
                    item_instance->count = stoi( data[2] );
                    (*FillingContainer_addItem)(inventory, item_instance);
                }
            }
            showInfo("Inventory for this server is loaded!");
            loadTimer.stop();
        }    
    }
}



//The function that will repeat every x ms to save the inventory to file if any changes in inventory
void saveStuff(){
    if(canDoStuff() ) {
        string tmpStr = ""; //make new string to put values into
        // get data from ALL slots
        for (int i = 9; i < 45; i++){
            ItemInstance *inventory_item = get_item_at_slot(i);
            if (inventory_item != NULL) {
                tmpStr += to_string(inventory_item->id) + ":" + to_string(inventory_item->auxiliary) + ":" + to_string(inventory_item->count) + "\n";
            } else tmpStr += "0:0:0\n";
        }
        if(tmpStr != inventoryString){ //Only save to file if there is a change!
            //showInfo("Saving inventory data for " + server );
            string thePath = string(getenv("HOME")) + "/.minecraft-pi/items_"+server+".txt";
            ofstream outfile;
            outfile.open(thePath, std::ios_base::trunc); // 'app' for append, 'trunc' overwrite
            outfile << tmpStr;
            inventoryString = tmpStr;
        } //else showInfo("No inventory change, not saving anything!");
    } else saveTimer.stop(); //if left the game, stop timer
}



//Function for printing some debug info directly to game screen
void showInfo(string text){ 
    unsigned char *gui = get_minecraft() + Minecraft_gui_property_offset;
    misc_add_message(gui, text.c_str() );
}



//Function with multiple safeguards to prevent doing stuff we can't do..
bool canDoStuff(){ 
    //If server string is still "" then it means either connected to local game, or not in world yet
    if (server == "") return false;
    // Get the level, use it to determine if connected to a server, maybe also ensures that game is loaded / still running? server will still hold the IP after leaving a game.. 
    unsigned char *level = *(unsigned char **) (get_minecraft() + Minecraft_level_property_offset);
    if (level) { // Check if on a server with clever offset value
        bool on_server = *(bool *) (level + 0x11);		
        if(!on_server) return false; //local game, saves and loads inventory by itself, so NO do stuff!        
        uchar *inventory = get_inventory();
        if (inventory == NULL) return false; //game is obviously not loaded yet / not still running, so NO do stuff!
    } else return false; //if NO level, we can't do stuff :\ Either left game, or still not entered world? 
    return true; //Yey, we can try do do stuff!
}



//Function that hi-jacks what happens when connecting to a server
static bool RakNetInstance_connect_injection(uchar *self, char *ip, int host) {
    //This will be executed only when connection attempt performed
	server = ip; //Get the server IP
    loadTimer.setInterval([&]() { loadStuff(); }, 5000); //try to load inventory 5 sec after connect, repeat until success 
    saveTimer.setInterval([&]() { saveStuff(); }, 60000); //try to save inventory every 60 sec after connnect.
	return RakNetInstance_connect(self, ip, host);
}



//The "main" function that starts when you run game?
__attribute__((constructor)) static void init() {
    misc_run_on_update(mcpi_callback);
    RakNetInstance_connect = *(RakNetInstance_connect_t *) RakNetInstance_connect_vtable_addr;
    patch_address(RakNetInstance_connect_vtable_addr, (void *) RakNetInstance_connect_injection);
    //Can't load or save or anything at this point in runtime, and user is currently eyeballing launcher screen
}
