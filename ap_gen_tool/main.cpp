//
// Copyright(C) 2023 David St-Louis
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// *Generates python files for Archipelago, also C and C++ headers for APDOOM*
//

#include <stdio.h>
#include <cinttypes>
#include <string.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <fstream>
#include <json/json.h>


//---- LINE TYPES ----
#define LT_NONE 0

// Doors types
#define LT_DR_DOOR_OPEN_WAIT_CLOSE_ALSO_MONSTERS 1
#define LT_DR_DOOR_OPEN_WAIT_CLOSE_FAST 117
#define LT_SR_DOOR_OPEN_WAIT_CLOSE 63
#define LT_SR_DOOR_OPEN_WAIT_CLOSE_FAST 114
#define LT_S1_DOOR_OPEN_WAIT_CLOSE 29
#define LT_S1_DOOR_OPEN_WAIT_CLOSE_FAST 111
#define LT_WR_DOOR_OPEN_WAIT_CLOSE 90
#define LT_WR_DOOR_OPEN_WAIT_CLOSE_FAST 105
#define LT_W1_DOOR_OPEN_WAIT_CLOSE_ALSO_MONSTERS 4
#define LT_W1_DOOR_OPEN_WAIT_CLOSE_FAST 108
#define LT_D1_DOOR_OPEN_STAY 31
#define LT_D1_DOOR_OPEN_STAY_FAST 118
#define LT_SR_DOOR_OPEN_STAY 61
#define LT_SR_DOOR_OPEN_STAY_FAST 115
#define LT_S1_DOOR_OPEN_STAY 103
#define LT_S1_DOOR_OPEN_STAY_FAST 112
#define LT_WR_DOOR_OPEN_STAY 86
#define LT_WR_DOOR_OPEN_STAY_FAST 106
#define LT_W1_DOOR_OPEN_STAY 2
#define LT_W1_DOOR_OPEN_STAY_FAST 109
#define LT_GR_DOOR_OPEN_STAY 46
#define LT_SR_DOOR_CLOSE_STAY 42
#define LT_SR_DOOR_CLOSE_STAY_FAST 116
#define LT_S1_DOOR_CLOSE_STAY 50
#define LT_S1_DOOR_CLOSE_STAY_FAST 113
#define LT_WR_DOOR_CLOSE_STAY 75
#define LT_WR_DOOR_CLOSE_STAY_FAST 107
#define LT_W1_DOOR_CLOSE_STAY 3
#define LT_W1_DOOR_CLOSE_FAST 110
#define LT_WR_DOOR_CLOSE_STAY_OPEN 76
#define LT_W1_DOOR_CLOSE_WAIT_OPEN 16

// Locked door types
#define LT_DR_DOOR_BLUE_OPEN_WAIT_CLOSE 26
#define LT_DR_DOOR_RED_OPEN_WAIT_CLOSE 28
#define LT_DR_DOOR_YELLOW_OPEN_WAIT_CLOSE 27
#define LT_D1_DOOR_BLUE_OPEN_STAY 32
#define LT_D1_DOOR_RED_OPEN_STAY 33
#define LT_D1_DOOR_YELLOW_OPEN_STAY 34
#define LT_SR_DOOR_BLUE_OPEN_STAY_FAST 99
#define LT_SR_DOOR_RED_OPEN_STAY_FAST 134
#define LT_SR_DOOR_YELLOW_OPEN_STAY_FAST 136
#define LT_S1_DOOR_BLUE_OPEN_STAY_FAST 133
#define LT_S1_DOOR_RED_OPEN_STAY_FAST 135
#define LT_S1_DOOR_YELLOW_OPEN_STAY_FAST 137

// Floor types
#define LT_SR_FLOOR_LOWER_TO_LOWEST_FLOOR 60
#define LT_S1_FLOOR_LOWER_TO_LOWEST_FLOOR 23
#define LT_WR_FLOOR_LOWER_TO_LOWEST_FLOOR 82
#define LT_W1_FLOOR_LOWER_TO_LOWEST_FLOOR 38
#define LT_WR_FLOOR_LOWER_TO_LOWEST_FLOOR_CHANGES_TEXTURE 84
#define LT_W1_FLOOR_LOWER_BY_LOWEST_FLOOR_CHANGES_TEXTURE 37
#define LT_SR_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR 69
#define LT_S1_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR 18
#define LT_WR_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR 128
#define LT_W1_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR 119
#define LT_SR_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR_FAST 132
#define LT_S1_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR_FAST 131
#define LT_WR_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR_FAST 129
#define LT_W1_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR_FAST 130
#define LT_SR_FLOOR_RAISE_TO_LOWEST_CEILING 64
#define LT_S1_FLOOR_RAISE_TO_LOWEST_CEILING 101
#define LT_WR_FLOOR_RAISE_TO_LOWEST_CEILING 91
#define LT_W1_FLOOR_RAISE_TO_LOWEST_CEILING 5
#define LT_G1_FLOOR_RAISE_TO_LOWEST_CEILING 24
#define LT_SR_FLOOR_RAISE_TO_8_BELLOW_LOWEST_CEILING_CRUSHES 65
#define LT_S1_FLOOR_RAISE_TO_8_BELLOW_LOWEST_CEILING_CRUSHES 55
#define LT_WR_FLOOR_RAISE_TO_8_BELLOW_LOWEST_CEILING_CRUSHES 94
#define LT_W1_FLOOR_RAISE_TO_8_BELLOW_LOWEST_CEILING_CRUSHES 56
#define LT_SR_FLOOR_LOWER_TO_HIGHEST_FLOOR 45
#define LT_S1_FLOOR_LOWER_TO_HIGHEST_FLOOR 102
#define LT_WR_FLOOR_LOWER_TO_HIGHEST_FLOOR 83
#define LT_W1_FLOOR_LOWER_TO_HIGHEST_FLOOR 19
#define LT_SR_FLOOR_LOWER_TO_8_ABOVE_HIGHEST_FLOOR 70
#define LT_S1_FLOOR_LOWER_TO_8_ABOVE_HIGHEST_FLOOR 71
#define LT_WR_FLOOR_LOWER_TO_8_ABOVE_HIGHEST_FLOOR 98
#define LT_W1_FLOOR_LOWER_BY_8_ABOVE_HIGHEST_FLOOR 36
#define LT_WR_FLOOR_RAISE_BY_24 92
#define LT_W1_FLOOR_RAISE_BY_24 58
#define LT_WR_FLOOR_RAISE_BY_24_CHANGES_TEXTURE 93
#define LT_W1_FLOOR_RAISE_BY_24_CHANGES_TEXTURE 59
#define LT_WR_FLOOR_RAISE_BY_SHORTEST_LOWER_TEXTURE 96
#define LT_W1_FLOOR_RAISE_BY_SHORTEST_LOWER_TEXTURE 30
#define LT_S1_FLOOR_RAISE_BY_512 140

// Ceiling types
#define LT_SR_CEILING_LOWER_TO_FLOOR 43
#define LT_S1_CEILING_LOWER_TO_FLOOR 41
#define LT_W1_CEILING_RAISE_TO_HIGHEST_CEILING 40
#define LT_WR_CEILING_LOWER_TO_8_ABOVE_FLOOR 72
#define LT_W1_CEILING_LOWER_TO_8_ABOVE_FLOOR 44

// Platform types
#define LT_SR_FLOOR_RAISE_BY_24_CHANGES_TEXTURE 66
#define LT_S1_FLOOR_RAISE_BY_24_CHANGES_TEXTURE 15
#define LT_SR_FLOOR_RAISE_BY_32_CHANGES_TEXTURE 67
#define LT_S1_FLOOR_RAISE_BY_32_CHANGES_TEXTURE 14
#define LT_SR_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR_CHANGES_TEXTURE 68
#define LT_S1_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR_CHANGES_TEXTURE 20
#define LT_WR_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR_CHANGES_TEXTURE 95
#define LT_W1_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR_CHANGES_TEXTURE 22
#define LT_G1_FLOOR_RAISE_TO_NEXT_HIGHER_FLOOR_CHANGES_TEXTURE 47
#define LT_WR_FLOOR_START_MOVING_UP_AND_DOWN 87
#define LT_W1_FLOOR_START_MOVING_UP_AND_DOWN 53
#define LT_WR_FLOOR_STOP_MOVING 89
#define LT_W1_FLOOR_STOP_MOVING 54
#define LT_SR_LIFT_LOWER_WAIT_RAISE 62
#define LT_S1_LIFT_LOWER_WAIT_RAISE 21
#define LT_WR_LIFT_LOWER_WAIT_RAISE_ALSO_MONSTERS 88
#define LT_W1_LIFT_LOWER_WAIT_RAISE 10
#define LT_SR_LIFT_LOWER_WAIT_RAISE_FAST 123
#define LT_S1_LIFT_LOWER_WAIT_RAISE_FAST 122
#define LT_WR_LIFT_LOWER_WAIT_RAISE_FAST 120
#define LT_W1_LIFT_LOWER_WAIT_RAISE_FAST 121

// Crusher types
#define LT_S1_CEILING_LOWER_TO_8_ABOVE_FLOOR_PERPETUAL_SLOW_CHRUSHER_DAMAGE 49
#define LT_WR_CRUSHER_START_WITH_SLOW_DAMAGE 73
#define LT_W1_CRUSHER_START_WITH_SLOW_DAMAGE 25
#define LT_WR_CRUSHER_START_WITH_FAST_DAMAGE 77
#define LT_W1_CRUSHER_START_WITH_FAST_DAMAGE 6
#define LT_W1_CRUSHER_START_WITH_SLOW_DAMAGE_SILENT 141
#define LT_WR_CRUSHER_STOP 74
#define LT_W1_CRUSHER_STOP 57

// Stairs types
#define LT_S1_STAIRS_RAISE_BY_8 7
#define LT_W1_STAIRS_RAISE_BY_8 8
#define LT_S1_STAIRS_RAISE_BY_16_FAST 127
#define LT_W1_STAIRS_RAISE_BY_16_FAST 100

// Lighting types
#define LT_SR_LIGHT_CHANGE_TO_35 139
#define LT_WR_LIGHT_CHANGE_TO_35 79
#define LT_W1_LIGHT_CHANGE_TO_35 35
#define LT_SR_LIGHT_CHANGE_TO_255 138
#define LT_WR_LIGHT_CHANGE_TO_255 81
#define LT_W1_LIGHT_CHANGE_TO_255 13
#define LT_WR_LIGHT_CHANGE_TO_BRIGHTEST_ADGACENT 80
#define LT_W1_LIGHT_CHANGE_TO_BRIGHTEST_ADJACENT 12
#define LT_W1_LIGHT_CHANGE_TO_DARKEST_ADJACENT 104
#define LT_W1_LIGHT_START_BLINKING 17

// Exit types
#define LT_S1_EXIT_LEVEL 11
#define LT_W1_EXIT_LEVEL 52
#define LT_S1_EXIT_LEVEL_GOES_TO_SECRET_LEVEL 51
#define LT_W1_EXIT_LEVEL_GOES_TO_SECRET_LEVEL 124

// Teleporter types
#define LT_WR_TELEPORT_ALSO_MONSTERS 97
#define LT_W1_TELEPORT_ALSO_MONSTERS 39
#define LT_WR_TELEPORT_MONSTERS_ONLY 126
#define LT_W1_TELEPORT_MONSTERS_ONLY 125

// Donut types
#define LT_S1_FLOOR_RAISE_DONUT_CHANGES_TEXTURE 9

// Animated textures
#define LT_SCROLL_TEXTURE_LEFT 48


struct map_header_t
{
    char identification[4];
    int32_t num_lumps;
    int32_t directory_offset;
};


struct map_directory_t
{
    int32_t offset;
    int32_t size;
    char name[8];
};


struct map_thing_t
{
    int16_t x;
    int16_t y;
    int16_t direction;
    int16_t type;
    int16_t flags;
};


struct map_linedefs_t
{
    int16_t start_vertex;
    int16_t end_vertex;
    int16_t flags;
    int16_t special_type;
    int16_t sector_tag;
    int16_t front_sidedef;
    int16_t back_sidedef;
};


struct map_sidedefs_t
{
    int16_t x_offset;
    int16_t y_offset;
    char    upper_texture[8];
    char    lower_texture[8];
    char    middle_texture[8];
    int16_t sector;
};


struct map_vertex_t
{
    int16_t x;
    int16_t y;
};


struct map_sectors_t
{
    int16_t floor_height;
    int16_t ceiling_height;
    char    floor_texture[8];
    char    ceiling_texture[8];
    int16_t light_level;
    int16_t type;
    int16_t tag;
};


struct map_subsector_t
{
    uint16_t numsegs;
    uint16_t firstseg;
};


struct map_node_t
{
    // Partition line from (x,y) to x+dx,y+dy)
    int16_t x;
    int16_t y;
    int16_t dx;
    int16_t dy;

    // Bounding box for each child,
    // clip against view frustum.
    int16_t bbox[2][4];

    // If NF_SUBSECTOR its a subsector,
    // else it's a node of another subtree.
    uint16_t children[2];
};


struct map_seg_t
{
    uint16_t v1;
    uint16_t v2;
    int16_t angle;
    uint16_t linedef;
    int16_t side;
    int16_t offset;
};


struct node_t
{
    int x;
    int y;
    int dx;
    int dy;
    int bbox[2][4];
    int children[2];
};


struct subsector_t
{
    int sector;
};

struct region_t
{
    std::vector<int64_t> required_items;
    std::vector<int64_t> locations;
};


enum item_classification_t
{
    FILLER          = 0b0000, // aka trash, as in filler items like ammo, currency etc,
    PROGRESSION     = 0b0001, // Item that is logically relevant
    USEFUL          = 0b0010, // Item that is generally quite useful, but not required for anything logical
    TRAP            = 0b0100, // detrimental or entirely useless (nothing) item
    SKIP_BALANCING  = 0b1000,
    PROGRESSION_SKIP_BALANCING = 0b1001
};


struct ap_item_t
{
    int64_t id;
    std::string name;
    int ep;
    int lvl;
    int doom_type;
    int count;
    int progression_count;
    bool is_key;
    item_classification_t classification;
};


struct ap_location_t
{
    int64_t id;
    std::string name;
    int x, y;
    int ep;
    int lvl;
    int doom_thing_index;
    int doom_type;
    std::vector<int64_t> required_items;
    int sector;
    int region = -1;
    std::string region_name;
};


struct connection_t
{
    int sector;
    std::vector<int64_t> required_items; // OR
};


struct sector_t
{
    bool visited = false;
    std::vector<connection_t> connections;
    std::vector<int> locations;
};


struct level_t
{
    char name[9];
    int ep;
    int lvl;
    std::vector<map_thing_t>        things;
    std::vector<map_linedefs_t>     linedefs;
    std::vector<map_sidedefs_t>     sidedefs;
    std::vector<map_vertex_t>       vertexes;
    std::vector<map_sectors_t>      map_sectors;
    std::vector<sector_t>           sectors;
    std::vector<map_subsector_t>    map_subsectors;
    std::vector<map_node_t>         map_nodes;
    std::vector<map_seg_t>          map_segs;
    std::vector<subsector_t>        subsectors;
    std::vector<node_t>             nodes;
    int starting_sector = -1;
    bool keys[3] = {false};
    int location_count = 0;
    bool use_skull[3] = {false};
};


int64_t item_id_base = 350000;
int64_t item_next_id = item_id_base;
int64_t location_next_id = 351000;

int total_item_count = 0;
int total_loc_count = 0;
std::vector<ap_item_t> ap_items;
std::vector<ap_location_t> ap_locations;
std::map<std::string, std::set<std::string>> item_name_groups;
std::map<uintptr_t, std::map<int, int64_t>> level_to_keycards;
std::map<std::string, ap_item_t*> item_map;

#define EP_COUNT 3
#define MAP_COUNT 9

const char* level_names[EP_COUNT][MAP_COUNT] = {
    {
        "Hangar (E1M1)",
        "Nuclear Plant (E1M2)",
        "Toxin Refinery (E1M3)",
        "Command Control (E1M4)",
        "Phobos Lab (E1M5)",
        "Central Processing (E1M6)",
        "Computer Station (E1M7)",
        "Phobos Anomaly (E1M8)",
        "Military Base (E1M9)"
    },
    {
        "Deimos Anomaly (E2M1)",
        "Containment Area (E2M2)",
        "Refinery (E2M3)",
        "Deimos Lab (E2M4)",
        "Command Center (E2M5)",
        "Halls of the Damned (E2M6)",
        "Spawning Vats (E2M7)",
        "Tower of Babel (E2M8)",
        "Fortress of Mystery (E2M9)"
    },
    {
        "Hell Keep (E3M1)",
        "Slough of Despair (E3M2)",
        "Pandemonium (E3M3)",
        "House of Pain (E3M4)",
        "Unholy Cathedral (E3M5)",
        "Mt. Erebus (E3M6)",
        "Limbo (E3M7)",
        "Dis (E3M8)",
        "Warrens (E3M9)"
    }
};


void generate_regions(level_t* level);
void connect_neighbors(level_t* level);


int FixedMul(int a, int b)
{
    return ((int64_t) a * (int64_t) b) >> 16;
}


int point_on_side(int x, int y, node_t* node)
{
    int	dx;
    int	dy;
    int	left;
    int	right;
	
    if (!node->dx)
    {
        if (x <= node->x)
        return node->dy > 0;
	
        return node->dy < 0;
    }
    if (!node->dy)
    {
        if (y <= node->y)
        return node->dx < 0;
	
        return node->dx > 0;
    }
	
    dx = (x - node->x);
    dy = (y - node->y);
	
    // Try to quickly decide by looking at sign bits.
    if ( (node->dy ^ node->dx ^ dx ^ dy)&0x80000000 )
    {
        if  ( (node->dy ^ dx) & 0x80000000 )
        {
            // (left is negative)
            return 1;
        }
        return 0;
    }

    left = FixedMul(node->dy >> 16, dx);
    right = FixedMul(dy, node->dx >> 16);
	
    if (right < left)
    {
        // front side
        return 0;
    }

    // back side
    return 1;			
}


subsector_t* point_in_subsector(int x, int y, level_t* level)
{
    node_t*	node;
    int side;
    int nodenum;

    // single subsector is a special case
    if (level->nodes.empty())
        return level->subsectors.data();
		
    nodenum = (int)level->nodes.size() - 1;

    while (!(nodenum & 0x80000000))
    {
        node = &level->nodes[nodenum];
        side = point_on_side(x, y, node);
        nodenum = node->children[side];
    }
	
    return &level->subsectors[nodenum & ~0x80000000];
}


bool loc_name_taken(const std::string& name)
{
    for (const auto& loc : ap_locations)
    {
        if (loc.name == name) return true;
    }
    return false;
}

void add_loc(const std::string& name, const map_thing_t& thing, level_t* level, int index, int x, int y)
{
    int count = 0;
    std::string loc_name = name;
    while (loc_name_taken(loc_name))
    {
        ++count;
        loc_name = name + " " + std::to_string(count + 1);
    }

    ap_location_t loc;
    loc.id = location_next_id++;
    loc.name = loc_name;
    loc.ep = level->ep;
    loc.lvl = level->lvl;
    loc.doom_thing_index = index;
    loc.doom_type = thing.type; // Index can be a risky one. We could replace the item by it's type if it's unique enough
    loc.x = x << 16;
    loc.y = y << 16;
    ap_locations.push_back(loc);

    level->location_count++;
    total_loc_count++;
}

void add_loc(const std::string& name, level_t* level)
{
    int count = 0;
    std::string loc_name = name;
    while (loc_name_taken(loc_name))
    {
        ++count;
        loc_name = name + " " + std::to_string(count + 1);
    }

    ap_location_t loc;
    loc.id = location_next_id++;
    loc.name = loc_name;
    loc.ep = level->ep;
    loc.lvl = level->lvl;
    loc.doom_thing_index = -1;
    loc.doom_type = -1;
    loc.x = -1;
    loc.y = -1;
    ap_locations.push_back(loc);
}

int64_t add_unique(const std::string& name, const map_thing_t& thing, level_t* level, int index, bool is_key, item_classification_t classification, const std::string& group_name, int x, int y)
{
    int64_t ret = 0;
    bool duplicated_item = false;
    for (const auto& other_item : ap_items)
    {
        if (other_item.name == name)
        {
            duplicated_item = true;
            ret = other_item.id;
            break;
        }
    }

    if (!duplicated_item)
    {
        ap_item_t item;
        item.id = item_next_id++;
        item.name = name;
        item.ep = level->ep;
        item.lvl = level->lvl;
        item.doom_type = thing.type;
        item.count = 1;
        total_item_count++;
        item.is_key = is_key;
        item.classification = classification;
        item.progression_count = 0;
        ap_items.push_back(item);
        if (!group_name.empty())
        {
            item_name_groups[group_name].insert(name);
        }
        ret = item.id;
    }
    add_loc(name, thing, level, index, x, y);
    return ret;
}

ap_item_t& add_item(const std::string& name, int doom_type, int count, item_classification_t classification, const std::string& group_name, int progression_count = 0, level_t* level = nullptr)
{
    ap_item_t item;
    item.id = item_next_id++;
    item.name = name;
    item.ep = level ? level->ep : -1;
    item.lvl = level ? level->lvl : -1;
    item.doom_type = doom_type;
    item.count = count;
    item.progression_count = progression_count;
    total_item_count += count;
    item.is_key = false;
    item.classification = classification;
    if (!group_name.empty())
    {
        item_name_groups[group_name].insert(name);
    }
    ap_items.push_back(item);
    return ap_items.back();
}


template<typename T>
static bool try_load_lump(const char *lump_name, 
                          FILE *f, 
                          const map_directory_t &dir_entry, 
                          std::vector<T> &elements)
{
    if (strncmp(dir_entry.name, lump_name, 8) == 0)
    {
        auto count = dir_entry.size / sizeof(T);
        elements.resize(count);
        fseek(f, dir_entry.offset, SEEK_SET);
        fread(elements.data(), sizeof(T), count, f);
        return true;
    }
    return false;
}


int main(int argc, char** argv)
{
    printf("AP Gen Tool\n");

    if (argc != 4) // Minimum effort validation
    {
        printf("Usage: ap_gen_tool.exe DOOM.WAD python_py_out_dir cpp_py_out_dir\n  i.e: ap_gen_tool.exe DOOM.WAD C:\\github\\apdoom\\RunDir\\DOOM.WAD C:\\github\\Archipelago\\worlds\\doom_1993 C:\\github\\apdoom\\src\\archipelago");
        return 1;
    }

    std::string py_out_dir = argv[2] + std::string("\\");
    std::string cpp_out_dir = argv[3] + std::string("\\");

    FILE* f = fopen(argv[1], "rb");
    if (!f)
    {
        printf("Cannot open file: %s\n", argv[1]);
        return 1;
    }

    // Read header
    map_header_t header;
    fread(&header, sizeof(header), 1, f);
    if (strncmp(header.identification, "PWAD", 4) != 0 && strncmp(header.identification, "IWAD", 4) != 0)
    {
        printf("Invalid IWAD or PWAD\n");
        return 1;
    }
    
    // Read directory
    std::vector<map_directory_t> directory(header.num_lumps);
    fseek(f, header.directory_offset, SEEK_SET);
    fread(directory.data(), sizeof(map_directory_t), header.num_lumps, f);

    // loop directory and find levels, then load them all. YOLO
    int max_thing_per_level = 0;
    std::vector<level_t*> levels;
    for (int i = 0, len = (int)directory.size(); i < len; ++i)
    {
        const auto &dir_entry = directory[i];
        if (strlen(dir_entry.name) == 4 && dir_entry.name[0] == 'E' && dir_entry.name[2] == 'M')
        {
            // That's a level!
            level_t* level = new level_t();
            level->ep = dir_entry.name[1] - '0';
            level->lvl = dir_entry.name[3] - '0';
            if (level->ep < 1 || level->ep > 3 || level->lvl < 1 || level->lvl > 9)
            {
                // ok.. not a level
                delete level;
                continue;
            }

            strncpy(level->name, dir_entry.name, 8);

            ++i;
            for (; i < len; ++i)
            {
                const auto &dir_entry = directory[i];
                try_load_lump("THINGS", f, dir_entry, level->things);
                try_load_lump("LINEDEFS", f, dir_entry, level->linedefs);
                try_load_lump("SIDEDEFS", f, dir_entry, level->sidedefs);
                try_load_lump("VERTEXES", f, dir_entry, level->vertexes);
                try_load_lump("SECTORS", f, dir_entry, level->map_sectors);
                try_load_lump("SSECTORS", f, dir_entry, level->map_subsectors);
                try_load_lump("NODES", f, dir_entry, level->map_nodes);
                try_load_lump("SEGS", f, dir_entry, level->map_segs);
                if (strncmp(dir_entry.name, "BLOCKMAP", 8) == 0)
                {
                    break;
                }
            }

            max_thing_per_level = std::max(max_thing_per_level, (int)level->things.size());

            level->sectors.resize(level->map_sectors.size());
            level->subsectors.resize(level->map_subsectors.size());
            level->nodes.resize(level->map_nodes.size());
            for (int j = 0, len = (int)level->map_nodes.size(); j < len; ++j)
            {
                level->nodes[j].x = (int)level->map_nodes[j].x << 16;
                level->nodes[j].y = (int)level->map_nodes[j].y << 16;
                level->nodes[j].dx = (int)level->map_nodes[j].dx << 16;
                level->nodes[j].dy = (int)level->map_nodes[j].dy << 16;
                for (int jj = 0; jj < 2; ++jj)
                {
                    level->nodes[j].children[jj] = (uint16_t)(int16_t)level->map_nodes[j].children[jj];
                    if (level->nodes[j].children[jj] == (uint16_t)-1)
                        level->nodes[j].children[jj] = -1;
                    else if (level->nodes[j].children[jj] & 0x8000)
                    {
                        level->nodes[j].children[jj] &= ~0x8000;
                        if (level->nodes[j].children[jj] >= (int)level->map_subsectors.size())
                            level->nodes[j].children[jj] = 0;
                        level->nodes[j].children[jj] |= 0x80000000;
                    }
                    for (int k = 0; k < 4; ++k)
                        level->nodes[j].bbox[jj][k] = (int)level->map_nodes[j].bbox[jj][k] << 16;
                }
            }

            for (int j = 0, len = (int)level->map_subsectors.size(); j < len; ++j)
            {
                const auto& map_seg = level->map_segs[level->map_subsectors[j].firstseg];
                const auto& map_sidedef = level->sidedefs[map_seg.linedef];
                level->subsectors[j].sector = map_sidedef.sector;
            }

            levels.push_back(level);
        }
    }

    // close file
    fclose(f);


    int armor_count = 0;
    int megaarmor_count = 0;
    int berserk_count = 0;
    int invulnerability_count = 0;
    int partial_invisibility_count = 0;
    int supercharge_count = 0;

    // Guns.
    add_item("Shotgun", 2001, 1, PROGRESSION, "Weapons");
    add_item("Rocket launcher", 2003, 1, PROGRESSION, "Weapons");
    add_item("Plasma gun", 2004, 1, PROGRESSION, "Weapons");
    add_item("Chainsaw", 2005, 1, PROGRESSION, "Weapons");
    add_item("Chaingun", 2002, 1, PROGRESSION, "Weapons");
    add_item("BFG9000", 2006, 1, PROGRESSION, "Weapons");

    // Make backpack progression item (Idea, gives more than one, with less increase each time)
    add_item("Backpack", 8, 1, PROGRESSION, "");

    // Fillers
    printf("Armor: %i\n", armor_count);
    printf("Mega Armor: %i\n", megaarmor_count);
    printf("Berserk: %i\n", berserk_count);
    printf("Invulnerability: %i\n", invulnerability_count);
    printf("Partial invisibility: %i\n", partial_invisibility_count);
    printf("Supercharge: %i\n", supercharge_count);

    add_item("Armor", 2018, 0, FILLER, "Powerups");
    add_item("Mega Armor", 2019, 0, FILLER, "Powerups");
    add_item("Berserk", 2023, 0, FILLER, "Powerups");
    add_item("Invulnerability", 2022, 0, FILLER, "Powerups");
    add_item("Partial invisibility", 2024, 0, FILLER, "Powerups");
    add_item("Supercharge", 2013, 0, FILLER, "Powerups");

    // Junk items
    add_item("Medikit", 2012, 0, FILLER, "");
    add_item("Box of bullets", 2048, 0, FILLER, "Ammos");
    add_item("Box of rockets", 2046, 0, FILLER, "Ammos");
    add_item("Box of shotgun shells", 2049, 0, FILLER, "Ammos");
    add_item("Energy cell pack", 17, 0, FILLER, "Ammos");

    // Keycard n such
    item_next_id = item_id_base + 200;
    for (auto level : levels)
    {
        std::string lvl_prefix = level_names[level->ep - 1][level->lvl - 1] + std::string(" - "); //"E" + std::to_string(level->ep) + "M" + std::to_string(level->lvl) + " ";
        int i = 0;

        for (const auto& thing : level->things)
        {
            if (thing.flags & 0x0010)
            {
                ++i;
                continue; // Thing is not in single player
            }
            switch (thing.type)
            {
                // Uniques
                case 5:
                    level_to_keycards[(uintptr_t)level][0] = add_unique(lvl_prefix + "Blue keycard", thing, level, i, true, PROGRESSION, "Keys", thing.x, thing.y);
                    level->keys[0] = true;
                    break;
                case 40:
                    level_to_keycards[(uintptr_t)level][0] = add_unique(lvl_prefix + "Blue skull key", thing, level, i, true, PROGRESSION, "Keys", thing.x, thing.y);
                    level->keys[0] = true;
                    level->use_skull[0] = true;
                    break;
                case 6:
                    level_to_keycards[(uintptr_t)level][1] = add_unique(lvl_prefix + "Yellow keycard", thing, level, i, true, PROGRESSION, "Keys", thing.x, thing.y);
                    level->keys[1] = true;
                    break;
                case 39:
                    level_to_keycards[(uintptr_t)level][1] = add_unique(lvl_prefix + "Yellow skull key", thing, level, i, true, PROGRESSION, "Keys", thing.x, thing.y);
                    level->keys[1] = true;
                    level->use_skull[1] = true;
                    break;
                case 13:
                    level_to_keycards[(uintptr_t)level][2] = add_unique(lvl_prefix + "Red keycard", thing, level, i, true, PROGRESSION, "Keys", thing.x, thing.y);
                    level->keys[2] = true;
                    break;
                case 38:
                    level_to_keycards[(uintptr_t)level][2] = add_unique(lvl_prefix + "Red skull key", thing, level, i, true, PROGRESSION, "Keys", thing.x, thing.y);
                    level->keys[2] = true;
                    level->use_skull[2] = true;
                    break;

                // Locations
                case 2018: add_loc(lvl_prefix + "Armor", thing, level, i, thing.x, thing.y); ++armor_count; break;
                case 8: add_loc(lvl_prefix + "Backpack", thing, level, i, thing.x, thing.y); break;
                case 2019: add_loc(lvl_prefix + "Mega Armor", thing, level, i, thing.x, thing.y); ++megaarmor_count; break;
                case 2023: add_loc(lvl_prefix + "Berserk", thing, level, i, thing.x, thing.y); ++berserk_count; break;
                case 2022: add_loc(lvl_prefix + "Invulnerability", thing, level, i, thing.x, thing.y); ++invulnerability_count; break;
                case 2024: add_loc(lvl_prefix + "Partial invisibility", thing, level, i, thing.x, thing.y); ++partial_invisibility_count; break;
                case 2013: add_loc(lvl_prefix + "Supercharge", thing, level, i, thing.x, thing.y); ++supercharge_count; break;
                case 2006: add_loc(lvl_prefix + "BFG9000", thing, level, i, thing.x, thing.y); break;
                case 2002: add_loc(lvl_prefix + "Chaingun", thing, level, i, thing.x, thing.y); break;
                case 2005: add_loc(lvl_prefix + "Chainsaw", thing, level, i, thing.x, thing.y); break;
                case 2004: add_loc(lvl_prefix + "Plasma gun", thing, level, i, thing.x, thing.y); break;
                case 2003: add_loc(lvl_prefix + "Rocket launcher", thing, level, i, thing.x, thing.y); break;
                case 2001: add_loc(lvl_prefix + "Shotgun", thing, level, i, thing.x, thing.y); break;
                case 2026: add_loc(lvl_prefix + "Computer area map", thing, level, i, thing.x, thing.y); break;

#if 0
                // Player spawn
                case 1:
                {
                    auto subsector = point_in_subsector(thing.x << 16, thing.y << 16, level);
                    if (subsector)
                    {
                        level->starting_sector = subsector->sector;
                    }
                    else
                    {
                        printf("Player start is not in sector: %s\n", lvl_prefix.c_str());
                    }
                    break;
                }
#endif
            }
            ++i;
        }
    }

    // Lastly, add level items. We want to add more levels in the future and not shift all existing item IDs
    item_next_id = item_id_base + 400;
    for (auto level : levels)
    {
        const char* lvl_name = level_names[level->ep - 1][level->lvl - 1];
        std::string lvl_prefix = lvl_name + std::string(" - ");

        add_item(lvl_name, -1, 1, PROGRESSION, "Levels", 0, level);
        add_item(lvl_prefix + "Complete", -2, 1, PROGRESSION, "", 0, level);
        add_item(lvl_prefix + "Computer area map", 2026, 1, FILLER, "", 0, level);
    }

    printf("%i locations\n%i items\n", total_loc_count, total_item_count - 3 /* Early items */);

#if 0
    // Fill in locations into level's sectors
    for (int i = 0, len = (int)ap_locations.size(); i < len; ++i)
    {
        auto& loc = ap_locations[i];
        for (const auto& level : levels)
        {
            if (loc.lvl == level->lvl &&
                loc.ep == level->ep)
            {
                auto subsector = point_in_subsector(loc.x, loc.y, level);
                if (subsector)
                {
                    level->sectors[subsector->sector].locations.push_back(i);
                    loc.sector = subsector->sector;
                }
                else
                {
                    printf("Cannot find sector for location: %s\n", loc.name.c_str());
                }
                break;
            }
        }
    }

    // Collect sector neighbors
    for (auto level : levels)
        connect_neighbors(level);

    // Regions
    generate_regions(levels[0]);
#endif

    //--- Generate the python files
    std::ifstream fregions(cpp_out_dir + "regions.json");
    Json::Value levels_json;
    fregions >> levels_json;
    fregions.close();

    // Items
    {
        FILE* fout = fopen((py_out_dir + "Items.py").c_str(), "w");
        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from BaseClasses import ItemClassification\n\
from typing import TypedDict, Dict, Set \n\
\n\
\n\
class ItemDict(TypedDict, total=False): \n\
    classification: ItemClassification \n\
    count: int \n\
    name: str \n\
    doom_type: int # Unique numerical id used to spawn the item. -1 is level item, -2 is level complete item. \n\
    episode: int # Relevant if that item targets a specific level, like keycard or map reveal pickup. \n\
    map: int \n\
\n\
\n\
");
        //     progression_count: int \n\

        fprintf(fout, "item_table: Dict[int, ItemDict] = {\n");
        for (const auto& item : ap_items)
        {
            fprintf(fout, "    %llu: {", item.id);
            switch (item.classification)
            {
                case FILLER: fprintf(fout, "'classification': ItemClassification.filler"); break;
                case PROGRESSION: fprintf(fout, "'classification': ItemClassification.progression"); break;
                case USEFUL: fprintf(fout, "'classification': ItemClassification.useful"); break;
                case TRAP: fprintf(fout, "'classification': ItemClassification.trap"); break;
                case SKIP_BALANCING: fprintf(fout, "'classification': ItemClassification.skip_balancing"); break;
                case PROGRESSION_SKIP_BALANCING: fprintf(fout, "'classification': ItemClassification.progression_skip_balancing"); break;
            }
            fprintf(fout, ",\n             'count': %i", item.count);
            //fprintf(fout, ",\n             'progression_count': %i", item.progression_count);
            fprintf(fout, ",\n             'name': '%s'", item.name.c_str());
            fprintf(fout, ",\n             'doom_type': %i", item.doom_type);
            fprintf(fout, ",\n             'episode': %i", item.ep);
            fprintf(fout, ",\n             'map': %i", item.lvl);
            fprintf(fout, "},\n");
        }
        fprintf(fout, "}\n\n\n");

        // item_name_groups
        fprintf(fout, "item_name_groups: Dict[str, Set[str]] = {\n");
        for (const auto& kv : item_name_groups)
        {
            fprintf(fout, "    '%s': {", kv.first.c_str());
            for (const auto& item_name : kv.second)
            {
                fprintf(fout, "'%s', ", item_name.c_str());
            }
            fprintf(fout, "},\n");
        }
        fprintf(fout, "}\n");

        fclose(fout);
    }

#if 1 // Regions.py
    // Generate Regions.py from regions.json (Manually entered data)
    {
        std::ifstream fregions(cpp_out_dir + "regions.json");
        Json::Value levels_json;
        fregions >> levels_json;
        fregions.close();

        FILE* fout = fopen((py_out_dir + "Regions.py").c_str(), "w");
        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from typing import List\n");
        fprintf(fout, "from BaseClasses import Region, Entrance\n\n");

        fprintf(fout, "regions:List[Region] = [\n");

        // Regions
        auto level_names = levels_json.getMemberNames();
        for (const auto& level_name : level_names)
        {
            const auto& level_json = levels_json[level_name];
            auto region_names = level_json["Regions"].getMemberNames();
            for (const auto& region_name : region_names)
            {
                fprintf(fout, "    \"%s %s\",\n", level_name.c_str(), region_name.c_str());
                const auto& region_json = level_json["Regions"][region_name];
                const auto& locs_json = level_json["Regions"][region_name]["locations"];
                for (const auto& loc_json : locs_json)
                {
                    bool found = false;
                    for (auto& loc : ap_locations)
                    {
                        if (loc.name == level_name + " - " + loc_json.asString())
                        {
                            loc.region_name = level_name + " " + region_name;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        printf("Location not found: %s\n", (level_name + " - " + loc_json.asString()).c_str());
                }
                if (region_json["connects_to_exit"].asBool())
                {
                    ap_location_t complete_loc;
                    complete_loc.doom_thing_index = -1;
                    complete_loc.doom_type = -1;
                    complete_loc.ep = level_json["episode"].asInt();
                    complete_loc.lvl = level_json["map"].asInt();
                    complete_loc.x = -1;
                    complete_loc.y = -1;
                    complete_loc.name = level_name + " - Exit";
                    complete_loc.region_name = level_name + " " + region_name;
                    complete_loc.id = location_next_id++;
                    ap_locations.push_back(complete_loc);
                }
            }
        }
        fprintf(fout, "]\n");

        fclose(fout);
    }
#endif
    
    // Locations
    {
        FILE* fout = fopen((py_out_dir + "Locations.py").c_str(), "w");

        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from typing import Dict, TypedDict, List, Set \n\
\n\
\n\
class LocationDict(TypedDict, total=False): \n\
    name: str \n\
    episode: int \n\
    map: int \n\
    index: int # Thing index as it is stored in the wad file. \n\
    doom_type: int # In case index end up unreliable, we can use doom type. Maps have often only one of each important things. \n\
    region: str \n\
\n\
\n\
");

        // Location table
        fprintf(fout, "location_table: Dict[int, LocationDict] = {\n");
        for (const auto& loc : ap_locations)
        {
            fprintf(fout, "    %llu: {", loc.id);
            fprintf(fout, "'name': '%s'", loc.name.c_str());
            fprintf(fout, ",\n             'episode': %i", loc.ep);
            fprintf(fout, ",\n             'map': %i", loc.lvl);
            fprintf(fout, ",\n             'index': %i", loc.doom_thing_index);
            fprintf(fout, ",\n             'doom_type': %i", loc.doom_type);
            fprintf(fout, ",\n             'region': \"%s\"", loc.region_name.c_str());
            fprintf(fout, "},\n");
        }
        fprintf(fout, "}\n\n\n");

        // name groups
        fprintf(fout, "location_name_groups: Dict[str, Set[str]] = {\n");
        std::map<std::string, std::set<std::string>> location_name_groups;
        for (const auto& loc : ap_locations)
        {
            location_name_groups[level_names[loc.ep - 1][loc.lvl - 1]].insert(loc.name);
        }
        for (const auto& kv : location_name_groups)
        {
            fprintf(fout, "    '%s': {", kv.first.c_str());
            for (const auto& v : kv.second)
            {
                fprintf(fout, "\n        '%s',", v.c_str());
            }
            fprintf(fout, "\n    },\n");
        }
        fprintf(fout, "}\n\n\n");

        // Death logic locations
        fprintf(fout, "death_logic_locations = [\n");
        for (const auto& level_json : levels_json)
        {
            auto level_name = level_names[level_json["episode"].asInt() - 1][level_json["map"].asInt() - 1];
            for (const auto& loc_json : level_json["death_logic_locations"])
            {
                fprintf(fout, "    \"%s - %s\",\n", level_name, loc_json.asCString());
            }
        }
        fprintf(fout, "]\n");
        //death_logic_locations

        fclose(fout);
    }

    // Events
    {
        FILE* fout = fopen((py_out_dir + "Maps.py").c_str(), "w");

        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from typing import List\n\n\n");

        fprintf(fout, "map_names: List[str] = [");
        for (auto level : levels)
        {
            fprintf(fout, "\n    '%s',", level_names[level->ep - 1][level->lvl - 1]);
        }
        fprintf(fout, "\n]\n");

        fclose(fout);
    }

    // Now generate apdoom_def.h so the game can map the IDs
    {
        FILE* fout = fopen((cpp_out_dir + "apdoom_def.h").c_str(), "w");
        
        fprintf(fout, "// This file is auto generated. More info: https://github.com/Daivuk/apdoom\n");
        fprintf(fout, "#pragma once\n\n");
        fprintf(fout, "#include \"apdoom.h\"\n");
        fprintf(fout, "#include <map>\n\n\n");

        std::map<int /* ep */, std::map<int /* map */, std::map<int /* index */, int64_t /* loc id */>>> location_table;
        for (const auto& loc : ap_locations)
        {
            location_table[loc.ep][loc.lvl][loc.doom_thing_index] = loc.id;
        }

        // locations
        fprintf(fout, "const std::map<int /* ep */, std::map<int /* map */, std::map<int /* index */, int64_t /* loc id */>>> location_table = {\n");
        for (const auto& kv1 : location_table)
        {
            fprintf(fout, "    {%i, {\n", kv1.first);
            for (const auto& kv2 : kv1.second)
            {
                fprintf(fout, "        {%i, {\n", kv2.first);
                for (const auto& kv3 : kv2.second)
                {
                    fprintf(fout, "            {%i, %lli},\n", kv3.first, kv3.second);
                }
                fprintf(fout, "        }},\n");
            }
            fprintf(fout, "    }},\n");
        }
        fprintf(fout, "};\n\n\n");
        
        // items
        fprintf(fout, "// Map item id\n");
        fprintf(fout, "struct ap_item_t\n");
        fprintf(fout, "{\n");
        fprintf(fout, "    int doom_type;\n");
        fprintf(fout, "    int ep; // If doom_type is a keycard\n");
        fprintf(fout, "    int map; // If doom_type is a keycard\n");
        fprintf(fout, "};\n\n");
        fprintf(fout, "const std::map<int64_t, ap_item_t> item_doom_type_table = {\n");
        for (const auto& item : ap_items)
        {
            fprintf(fout, "    {%llu, {%i, %i, %i}},\n", item.id, item.doom_type, item.ep, item.lvl);
        }
        fprintf(fout, "};\n\n\n");

        // Level infos
        fprintf(fout, "ap_level_info_t ap_level_infos[AP_EPISODE_COUNT][AP_LEVEL_COUNT] = \n");
        fprintf(fout, "{\n");
        for (int ep = 0; ep < EP_COUNT; ++ep)
        {
            fprintf(fout, "    {\n");
            for (int map = 0; map < MAP_COUNT; ++map)
            {
                for (auto level : levels)
                {
                    if (level->ep - 1== ep && level->lvl - 1 == map)
                    {
                        int64_t loc_id = 0;
                        for (const auto& loc : ap_locations)
                        {
                            if (loc.ep == level->ep && loc.lvl == level->lvl)
                            {
                                if (loc.doom_type == -1)
                                {
                                    loc_id = loc.id;
                                    break;
                                }
                            }
                        }
                        fprintf(fout, "        {{%s, %s, %s}, {%i, %i, %i}, %i, %i, {\n", 
                                level->keys[0] ? "true" : "false", 
                                level->keys[1] ? "true" : "false", 
                                level->keys[2] ? "true" : "false", 
                                level->use_skull[0] ? 1 : 0, 
                                level->use_skull[1] ? 1 : 0, 
                                level->use_skull[2] ? 1 : 0, 
                                level->location_count,
                                (int)level->things.size());
                        int idx = 0;
                        for (const auto& thing : level->things)
                        {
                            fprintf(fout, "            {%i, %i},\n", thing.type, idx);
                            ++idx;
                        }
                        fprintf(fout, "        }},\n");
                    }
                }
            }
            fprintf(fout, "    },\n");
        }
        fprintf(fout, "};\n");

        fclose(fout);
    }

    // We generate some stuff for doom also, C header.
    {
        FILE* fout = fopen((cpp_out_dir + "apdoom_c_def.h").c_str(), "w");
        
        fprintf(fout, "// This file is auto generated. More info: https://github.com/Daivuk/apdoom\n");
        fprintf(fout, "#ifndef _AP_DOOM_C_DEF_\n");
        fprintf(fout, "#define _AP_DOOM_C_DEF_\n\n");

        fprintf(fout, "static int is_doom_type_ap_location(int doom_type)\n");
        fprintf(fout, "{\n");
        fprintf(fout, "    switch (doom_type)\n");
        fprintf(fout, "    {\n");

        std::set<int> doom_types;
        for (const auto& loc : ap_locations)
        {
            doom_types.insert(loc.doom_type);
        }
        for (auto doom_type : doom_types)
        {
            fprintf(fout, "        case %i:\n", doom_type);
        }
        
        fprintf(fout, "            return 1;\n");
        fprintf(fout, "    }\n\n");
        fprintf(fout, "    return 0;\n");
        fprintf(fout, "}\n\n");

        fprintf(fout, "#endif\n");
        fclose(fout);
    }

    // Generate regions.json without all the rules, filling all locations for now.
    // (I should have just import JsonCPP, this is dumb...)
#if 0
    {
        FILE* fout = fopen((cpp_out_dir + "regions.json").c_str(), "w");
        fprintf(fout, "{\n");

        for (auto level : levels)
        {
            const auto level_name = level_names[level->ep - 1][level->lvl - 1];
            int substr_len = strlen(level_name) + 3;
            fprintf(fout, "    \"%s\": {\n", level_name);
            fprintf(fout, "        \"episode\": %i, \"map\": %i,\n", level->ep, level->lvl); // For extra context
            fprintf(fout, "        \"Regions\": {\n");
            fprintf(fout, "            \"Main\": {\n");
            fprintf(fout, "                \"locations\": [");
            bool keys[3] = {false};
            bool first = true;
            for (const auto& loc : ap_locations)
            {
                if (loc.ep == level->ep && loc.lvl == level->lvl)
                {
                    if (!first) fprintf(fout, ", ");
                    first = false;
                    fprintf(fout, "\"%s\"", loc.name.substr(substr_len).c_str());
                    if (loc.name.find("keycard") != std::string::npos || 
                        loc.name.find("skull key") != std::string::npos)
                    {
                        if (loc.name.find("Blue") != std::string::npos) keys[0] = true;
                        else if (loc.name.find("Yellow") != std::string::npos) keys[1] = true;
                        else if (loc.name.find("Red") != std::string::npos) keys[2] = true;
                    }
                }
            }
            fprintf(fout, "],\n");
            fprintf(fout, "                \"connects_to_hub\": true,\n");
            fprintf(fout, "                \"connects_to_exit\": true,\n");
            fprintf(fout, "                \"required_items_or\": []\n");
               
            if (keys[0] || keys[1] || keys[2])
            {
                fprintf(fout, "            },\n");

                if (keys[0])
                {
                    fprintf(fout, "            \"Blue\": {\n                \"locations\": [],\n                \"required_items_or\": [\"Blue keycard\"]\n            }");
                    if (keys[1] || keys[2]) fprintf(fout, ",\n");
                    else fprintf(fout, "\n");
                }

                if (keys[1])
                {
                    fprintf(fout, "            \"Yellow\": {\n                \"locations\": [],\n                \"required_items_or\": [\"Yellow keycard\"]\n            }");
                    if (keys[2]) fprintf(fout, ",\n");
                    else fprintf(fout, "\n");
                }

                if (keys[2])
                    fprintf(fout, "            \"Red\": {\n                \"locations\": [],\n                \"required_items_or\": [\"Red keycard\"]\n            }\n");
            }
            else
            {
                fprintf(fout, "            }\n");
            }
            fprintf(fout, "        }\n");
            if (level == levels.back())
                fprintf(fout, "    }\n");
            else
                fprintf(fout, "    },\n");
        }

        fprintf(fout, "}\n");
        fclose(fout);
    }
#else

    // Generate Rules.py from regions.json (Manually entered data)
    {
        FILE* fout = fopen((py_out_dir + "Rules.py").c_str(), "w");
        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from typing import TYPE_CHECKING\n");

        fprintf(fout, "from worlds.generic.Rules import set_rule\n\n");
        
        fprintf(fout, "if TYPE_CHECKING:\n");
        fprintf(fout, "    from . import DOOM1993World\n\n\n");

        fprintf(fout, "def set_rules(doom_1993_world: \"DOOM1993World\"):\n");
        fprintf(fout, "    player = doom_1993_world.player\n");
        fprintf(fout, "    world = doom_1993_world.multiworld\n\n");

        // This was not such a good idea. We want to forbid other game's progression items too!
        
        //fprintf(fout, "    progression_items = [\n");
        //for (const auto& item : ap_items)
        //{
        //    if (item.classification == PROGRESSION || item.progression_count)
        //    {
        //        fprintf(fout, "        \"%s\",\n", item.name.c_str());
        //    }
        //}
        //fprintf(fout, "    ]\n\n");

        //fprintf(fout, "    # Specific Case for E1M4, item you have 1 shot to get. Lets not put progressive in there.\n");
        //fprintf(fout, "    forbid_items(world.get_location(\"Command Control - Supercharge\", player), progression_items)\n\n");

        //fprintf(fout, "    # Specific Case for E3M4, 2 items here might require you die if you're not fast enough. Lets not put progressive in there.\n");
        //fprintf(fout, "    forbid_items(world.get_location(\"House of Pain - Chaingun\", player), progression_items)\n");
        //fprintf(fout, "    forbid_items(world.get_location(\"House of Pain - Invulnerability\", player), progression_items)\n\n");

        //fprintf(fout, "    # Specific Case for E3M3, item behind one way elevator. You can die to get it.\n");
        //fprintf(fout, "    forbid_items(world.get_location(\"Pandemonium - Mega Armor\", player), progression_items)\n\n");

        std::vector<std::string> map_items = {
            "Blue keycard", "Blue skull key", "Yellow keycard", "Yellow skull key", "Red keycard", "Red skull key"
        };

        for (auto level : levels)
        {
            auto level_name = level_names[level->ep - 1][level->lvl - 1];
            const auto& level_json = levels_json[level_name];
            fprintf(fout, "    # %s - E%iM%i\n", level_name, level_json["episode"].asInt(), level_json["map"].asInt());
            auto region_names = level_json["Regions"].getMemberNames();
            for (const auto& region_name : region_names)
            {
                const auto& region_json = level_json["Regions"][region_name];
                const auto& locs_json = level_json["Regions"][region_name]["locations"];

                std::vector<std::string> required_items_or;
                for (const auto& required_item_json : region_json["required_items_or"])
                {
                    required_items_or.push_back(required_item_json.asString());
                }

                std::vector<std::string> required_items_and;
                for (const auto& required_item_json : region_json["required_items_and"])
                {
                    required_items_and.push_back(required_item_json.asString());
                }
#if 0 // Rules by locations
                for (const auto& loc_json : locs_json)
                {
                    // We guarantee shotgun for any level that is not first of each episode
                    if (level->lvl > 1)
                    {
                        fprintf(fout, "    set_rule(world.get_location(\"%s - %s\", player), lambda state: (state.has(\"%s\", player, 1) and state.has(\"Shotgun\", player, 1))", level_name, loc_json.asCString(), level_name);
                    }
                    else
                    {
                        fprintf(fout, "    set_rule(world.get_location(\"%s - %s\", player), lambda state: state.has(\"%s\", player, 1)", level_name, loc_json.asCString(), level_name);
                    }
                    for (const auto& required_item_and : required_items_and)
                    {
                        if (std::find(map_items.begin(), map_items.end(), required_item_and) != map_items.end())
                            fprintf(fout, "and state.has(\"%s - %s\", player, 1)", level_name, required_item_and.c_str());
                        else
                            fprintf(fout, "and state.has(\"%s\", player, 1)", required_item_and.c_str());
                    }
                    if (!required_items_or.empty())
                    {
                        fprintf(fout, " and (");
                    }
                    bool first = true;
                    for (const auto& required_item_or : required_items_or)
                    {
                        if (!first) fprintf(fout, " or ");
                        first = false;
                        if (std::find(map_items.begin(), map_items.end(), required_item_or) != map_items.end())
                            fprintf(fout, "state.has(\"%s - %s\", player, 1)", level_name, required_item_or.c_str());
                        else
                            fprintf(fout, "state.has(\"%s\", player, 1)", required_item_or.c_str());
                    }
                    if (!required_items_or.empty())
                    {
                        fprintf(fout, ")");
                    }
                    fprintf(fout, ")\n");
                }

                if (region_json["connects_to_exit"].asBool())
                {
                    if (level->lvl > 1)
                    {
                        fprintf(fout, "    set_rule(world.get_location(\"%s - Complete\", player), lambda state: (state.has(\"%s\", player, 1) and state.has(\"Shotgun\", player, 1))", level_name, level_name);
                    }
                    else
                    {
                        fprintf(fout, "    set_rule(world.get_location(\"%s - Complete\", player), lambda state: state.has(\"%s\", player, 1)", level_name, level_name);
                    }

                    // Gotta love some duplicated code
                    for (const auto& required_item_and : required_items_and)
                    {
                        if (std::find(map_items.begin(), map_items.end(), required_item_and) != map_items.end())
                            fprintf(fout, "and state.has(\"%s - %s\", player, 1)", level_name, required_item_and.c_str());
                        else
                            fprintf(fout, "and state.has(\"%s\", player, 1)", required_item_and.c_str());
                    }
                    if (!required_items_or.empty())
                    {
                        fprintf(fout, " and (");
                    }
                    bool first = true;
                    for (const auto& required_item_or : required_items_or)
                    {
                        if (!first) fprintf(fout, " or ");
                        first = false;
                        if (std::find(map_items.begin(), map_items.end(), required_item_or) != map_items.end())
                            fprintf(fout, "state.has(\"%s - %s\", player, 1)", level_name, required_item_or.c_str());
                        else
                            fprintf(fout, "state.has(\"%s\", player, 1)", required_item_or.c_str());
                    }
                    if (!required_items_or.empty())
                    {
                        fprintf(fout, ")");
                    }
                    fprintf(fout, ")\n");
                }
#else // Rules by regions
                fprintf(fout, "    set_rule(world.get_entrance(\"Mars -> %s %s\", player), lambda state: state.has(\"%s\", player, 1)", level_name, region_name.c_str(), level_name);
                if (level->lvl > 1)
                    fprintf(fout, " and state.has(\"Shotgun\", player, 1)");
                if (level->lvl > 3)
                    fprintf(fout, " and state.has(\"Chaingun\", player, 1)");
                for (const auto& required_item_and : required_items_and)
                {
                    if (std::find(map_items.begin(), map_items.end(), required_item_and) != map_items.end())
                        fprintf(fout, "and state.has(\"%s - %s\", player, 1)", level_name, required_item_and.c_str());
                    else
                        fprintf(fout, "and state.has(\"%s\", player, 1)", required_item_and.c_str());
                }
                if (!required_items_or.empty())
                {
                    fprintf(fout, " and (");
                }
                bool first = true;
                for (const auto& required_item_or : required_items_or)
                {
                    if (!first) fprintf(fout, " or ");
                    first = false;
                    if (std::find(map_items.begin(), map_items.end(), required_item_or) != map_items.end())
                        fprintf(fout, "state.has(\"%s - %s\", player, 1)", level_name, required_item_or.c_str());
                    else
                        fprintf(fout, "state.has(\"%s\", player, 1)", required_item_or.c_str());
                }
                if (!required_items_or.empty())
                {
                    fprintf(fout, ")");
                }
                fprintf(fout, ")\n");
#endif
            }
            fprintf(fout, "\n");
        }

        fclose(fout);
    }
#endif

    // Clean up
    for (auto level : levels) delete level; // We don't need to
    return 0;
}


bool is_linedef_door(int16_t type, int& keycard)
{
    keycard = -1;

    if (type == LT_DR_DOOR_BLUE_OPEN_WAIT_CLOSE ||
        type == LT_D1_DOOR_BLUE_OPEN_STAY ||
        type == LT_SR_DOOR_BLUE_OPEN_STAY_FAST ||
        type == LT_S1_DOOR_BLUE_OPEN_STAY_FAST)
    {
        keycard = 0;
        return true;
    }
    if (type == LT_DR_DOOR_YELLOW_OPEN_WAIT_CLOSE ||
        type == LT_D1_DOOR_YELLOW_OPEN_STAY ||
        type == LT_SR_DOOR_YELLOW_OPEN_STAY_FAST ||
        type == LT_S1_DOOR_YELLOW_OPEN_STAY_FAST)
    {
        keycard = 1;
        return true;
    }
    if (type == LT_DR_DOOR_RED_OPEN_WAIT_CLOSE ||
        type == LT_D1_DOOR_RED_OPEN_STAY ||
        type == LT_SR_DOOR_RED_OPEN_STAY_FAST ||
        type == LT_S1_DOOR_RED_OPEN_STAY_FAST)
    {
        keycard = 2;
        return true;
    }

    return
        type == LT_DR_DOOR_OPEN_WAIT_CLOSE_ALSO_MONSTERS ||
        type == LT_DR_DOOR_OPEN_WAIT_CLOSE_FAST ||
        type == LT_SR_DOOR_OPEN_WAIT_CLOSE ||
        type == LT_SR_DOOR_OPEN_WAIT_CLOSE_FAST ||
        type == LT_S1_DOOR_OPEN_WAIT_CLOSE ||
        type == LT_S1_DOOR_OPEN_WAIT_CLOSE_FAST ||
        type == LT_WR_DOOR_OPEN_WAIT_CLOSE ||
        type == LT_WR_DOOR_OPEN_WAIT_CLOSE_FAST ||
        type == LT_W1_DOOR_OPEN_WAIT_CLOSE_ALSO_MONSTERS ||
        type == LT_W1_DOOR_OPEN_WAIT_CLOSE_FAST ||
        type == LT_D1_DOOR_OPEN_STAY ||
        type == LT_D1_DOOR_OPEN_STAY_FAST ||
        type == LT_SR_DOOR_OPEN_STAY ||
        type == LT_SR_DOOR_OPEN_STAY_FAST ||
        type == LT_S1_DOOR_OPEN_STAY ||
        type == LT_S1_DOOR_OPEN_STAY_FAST ||
        type == LT_WR_DOOR_OPEN_STAY ||
        type == LT_WR_DOOR_OPEN_STAY_FAST ||
        type == LT_W1_DOOR_OPEN_STAY ||
        type == LT_W1_DOOR_OPEN_STAY_FAST ||
        type == LT_GR_DOOR_OPEN_STAY;
}


int64_t get_keycard_for_level(level_t* level, int keycard)
{
    return level_to_keycards[(uintptr_t)level][keycard];
}


void connect_neighbors(level_t* level)
{
    for (int i = 0, len = (int)level->linedefs.size(); i < len; ++i)
    {
        const auto& linedef = level->linedefs[i];
        if (linedef.back_sidedef == -1) continue; // One-sided
        if (linedef.front_sidedef == -1) continue; // One-sided, but only back? its possible, but bad level design
        if (linedef.flags & 0x0001) continue; // Blocks players

        auto front_sectori = level->sidedefs[linedef.front_sidedef].sector;
        auto back_sectori = level->sidedefs[linedef.back_sidedef].sector;

        auto& front_map_sector = level->map_sectors[front_sectori];
        auto& back_map_sector = level->map_sectors[back_sectori];

        auto& front_sector = level->sectors[front_sectori];
        auto& back_sector = level->sectors[back_sectori];

        // Front to back
        {
            auto can_step = front_map_sector.floor_height >= back_map_sector.floor_height - 24;
            auto can_fit = back_map_sector.ceiling_height - front_map_sector.floor_height >= 56;
            int keycard;
            auto is_door = is_linedef_door(linedef.special_type, keycard);

            if ((can_step && can_fit) || is_door)
            {
                connection_t connection_front_to_back;
                if (keycard != -1) connection_front_to_back.required_items.push_back(get_keycard_for_level(level, keycard));
                connection_front_to_back.sector = back_sectori;
                front_sector.connections.push_back(connection_front_to_back);
            }
        }

        // back to front
        {
            auto can_step = back_map_sector.floor_height >= front_map_sector.floor_height - 24;
            auto can_fit = front_map_sector.ceiling_height - back_map_sector.floor_height >= 56;
            int keycard;
            auto is_door = is_linedef_door(linedef.special_type, keycard);

            if ((can_step && can_fit) || is_door)
            {
                connection_t connection_back_to_front;
                if (keycard != -1) connection_back_to_front.required_items.push_back(get_keycard_for_level(level, keycard));
                connection_back_to_front.sector = front_sectori;
                back_sector.connections.push_back(connection_back_to_front);
            }
        }
    }
}


// That's the tough one!
void generate_regions(level_t* level)
{
    if (level->starting_sector == -1)
    {
        printf("Level has no starting sector: %s\n", level->name);
        return;
    }

    std::set<int> to_check = {level->starting_sector};

    std::vector<region_t> regions;
    int regioni = 0;

    region_t region;
    while (!to_check.empty())
    {
        auto sectori = *to_check.begin();
        to_check.erase(to_check.begin());

        auto& sector = level->sectors[sectori];
        sector.visited = true;

        region.locations.insert(region.locations.end(), sector.locations.begin(), sector.locations.end());
        for (auto& loc : sector.locations)
            ap_locations[loc].region = regioni;

        for (const auto& connection : sector.connections)
        {
            if (level->sectors[connection.sector].visited) continue; // Already visited
            if (to_check.count(connection.sector)) continue; // Already to check
            to_check.insert(connection.sector);
        }
    }
    regions.push_back(region);

    // Print missed regions
    for (const auto& loc : ap_locations)
    {
        if (loc.ep == level->ep && loc.lvl == level->lvl)
        {
            if (loc.region == -1)
            {
                printf("Unreachable location: %s\n", loc.name.c_str());
            }
        }
    }
}
