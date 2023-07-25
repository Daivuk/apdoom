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
#include <onut/onut.h>
#include <onut/Strings.h>
#include <onut/Log.h>

#include "maps.h"
#include "ids_remap.h"

#include <algorithm>


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


struct level_sector_t
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
    std::vector<level_sector_t> sectors;
    int starting_sector = -1;
    bool keys[3] = {false};
    int location_count = 0;
    bool use_skull[3] = {false};
    map_t* map = nullptr;
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

const char* get_doom_type_name(int doom_type);

static std::string get_requirement_name(const std::string& level_name, int doom_type)
{
    switch (doom_type)
    {
        case 5: return level_name + " - Blue keycard";
        case 40: return level_name + " - Blue skull key";
        case 6: return level_name + " - Yellow keycard";
        case 39: return level_name + " - Yellow skull key";
        case 13: return level_name + " - Red keycard";
        case 38: return level_name + " - Red skull key";
        case 2005: return "Chainsaw";
        case 2001: return "Shotgun";
        case 2002: return "Chaingun";
        case 2003: return "Rocket launcher";
        case 2004: return "Plasma gun";
        case 2006: return "BFG9000";
    }
    return "ERROR";
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


std::string escape_csv(const std::string& str)
{
    std::string ret = str;
    for (int i = 0; i < (int)ret.size(); ++i)
    {
        auto c = ret[i];
        if (c == '"')
        {
            ret.insert(ret.begin() + i, '"');
            ++i;
        }
    }
    return "\"" + ret + "\"";
}


int generate()
{
    OLog("AP Gen Tool");

    if (OArguments.size() != 4) // Minimum effort validation
    {
        OLogE("Usage: ap_gen_tool.exe DOOM.WAD python_py_out_dir cpp_py_out_dir poptracker_data_dir\n  i.e: ap_gen_tool.exe DOOM.WAD C:\\github\\apdoom\\RunDir\\DOOM.WAD C:\\github\\Archipelago\\worlds\\doom_1993 C:\\github\\apdoom\\src\\archipelago  C:\\github\\apdoom\\data\\poptracker");
        return 1;
    }

    std::string py_out_dir = OArguments[1] + std::string("\\");
    std::string cpp_out_dir = OArguments[2] + std::string("\\");
    std::string pop_tracker_data_dir = OArguments[3] + std::string("\\");

    std::ifstream fregions(cpp_out_dir + "regions.json");
    Json::Value levels_json;
    fregions >> levels_json;
    fregions.close();

    ap_locations.reserve(1000);
    
    // Guns.
    add_item("Shotgun", 2001, 1, PROGRESSION, "Weapons");
    add_item("Rocket launcher", 2003, 1, PROGRESSION, "Weapons");
    add_item("Plasma gun", 2004, 1, PROGRESSION, "Weapons");
    add_item("Chainsaw", 2005, 1, PROGRESSION, "Weapons");
    add_item("Chaingun", 2002, 1, PROGRESSION, "Weapons");
    add_item("BFG9000", 2006, 1, PROGRESSION, "Weapons");

    // Make backpack progression item (Idea, gives more than one, with less increase each time)
    add_item("Backpack", 8, 1, PROGRESSION, "");

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

    std::vector<level_t*> levels;
    for (int ep = 0; ep < EP_COUNT; ++ep)
    {
        for (int lvl = 0; lvl < MAP_COUNT; ++lvl)
        {
            map_t* map = &maps[ep][lvl];
            level_t* level = new level_t();
            level->name[0] = 'E';
            level->name[1] = '1' + ep;
            level->name[2] = 'M';
            level->name[3] = '1' + lvl;
            level->name[4] = '\0';
            level->map = map;
            level->ep = ep + 1;
            level->lvl = lvl + 1;
            levels.push_back(level);
        }
    }
    
    // Keycard n such
    item_next_id = item_id_base + 200;
    const auto& episodes_json = levels_json["episodes"];
    int ep = 0;
    for (const auto& episode_json : episodes_json)
    {
        int lvl = 0;
        for (const auto& level_json : episode_json)
        {
            auto level = levels[ep * MAP_COUNT + lvl];

            level->sectors.resize(maps[ep][lvl].sectors.size());
            std::string lvl_prefix = level_names[ep][lvl] + std::string(" - ");
            int i = 0;

            for (const auto& thing : level->map->things)
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
                    case 2018: add_loc(lvl_prefix + "Armor", thing, level, i, thing.x, thing.y); break;
                    case 8: add_loc(lvl_prefix + "Backpack", thing, level, i, thing.x, thing.y); break;
                    case 2019: add_loc(lvl_prefix + "Mega Armor", thing, level, i, thing.x, thing.y); break;
                    case 2023: add_loc(lvl_prefix + "Berserk", thing, level, i, thing.x, thing.y); break;
                    case 2022: add_loc(lvl_prefix + "Invulnerability", thing, level, i, thing.x, thing.y); break;
                    case 2024: add_loc(lvl_prefix + "Partial invisibility", thing, level, i, thing.x, thing.y); break;
                    case 2013: add_loc(lvl_prefix + "Supercharge", thing, level, i, thing.x, thing.y); break;
                    case 2006: add_loc(lvl_prefix + "BFG9000", thing, level, i, thing.x, thing.y); break;
                    case 2002: add_loc(lvl_prefix + "Chaingun", thing, level, i, thing.x, thing.y); break;
                    case 2005: add_loc(lvl_prefix + "Chainsaw", thing, level, i, thing.x, thing.y); break;
                    case 2004: add_loc(lvl_prefix + "Plasma gun", thing, level, i, thing.x, thing.y); break;
                    case 2003: add_loc(lvl_prefix + "Rocket launcher", thing, level, i, thing.x, thing.y); break;
                    case 2001: add_loc(lvl_prefix + "Shotgun", thing, level, i, thing.x, thing.y); break;
                    case 2026: add_loc(lvl_prefix + "Computer area map", thing, level, i, thing.x, thing.y); break;
                }
                ++i;
            }

            const auto& regions_json = level_json["regions"];
            int region_i = 0;
            bool connects_to_exit = false;

            for (const auto& region_json : regions_json)
            {
                std::string region_name = level_names[ep][lvl] + std::string(" ") + region_json["name"].asString();

                const auto& rules_json = region_json["rules"];
                const auto& connections_json = rules_json["connections"];
                for (const auto& connection_json : connections_json)
                {
                    int target_region = connection_json["target_region"].asInt();
                    if (target_region == -2)
                    {
                        connects_to_exit = true;
                        ap_location_t complete_loc;
                        complete_loc.doom_thing_index = -1;
                        complete_loc.doom_type = -1;
                        complete_loc.ep = ep + 1;
                        complete_loc.lvl = lvl + 1;
                        complete_loc.x = -1;
                        complete_loc.y = -1;
                        complete_loc.name = lvl_prefix + "Exit";
                        complete_loc.region_name = region_name;
                        complete_loc.id = location_next_id++;
                        ap_locations.push_back(complete_loc);
                        break;
                    }
                    if (connects_to_exit) break;
                }
                if (connects_to_exit) break;
            }

            ++lvl;
        }
        ++ep;
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

    OLog(std::to_string(total_loc_count) + " locations\n" + std::to_string(total_item_count - 3) + " items");

    //--- Remap location's IDs for backward compatibility with 0.3.9
    {
        int64_t next_location_id = 0;
        std::vector<int> unmapped_locations;
        int i = 0;
        for (const auto &kv : LOCATIONS_TO_LEGACY_IDS)
            next_location_id = std::max(next_location_id, kv.second + 1);
        for (auto& location : ap_locations)
        {
            auto it = LOCATIONS_TO_LEGACY_IDS.find(location.name);
            if (it != LOCATIONS_TO_LEGACY_IDS.end())
                location.id = LOCATIONS_TO_LEGACY_IDS[location.name];
            else
                unmapped_locations.push_back(i);
            ++i;
        }
        for (auto unmapped_location : unmapped_locations)
            ap_locations[unmapped_location].id = next_location_id++;

        // Sort by id so it's clean in AP
        std::sort(ap_locations.begin(), ap_locations.end(), [](const ap_location_t& a, const ap_location_t& b) { return a.id < b.id; });
    }

    //--- Remap item's IDs for backward compatibility with 0.3.9
    {
        int64_t next_itemn_id = 0;
        std::vector<int> unmapped_items;
        int i = 0;
        for (const auto &kv : ITEMS_TO_LEGACY_IDS)
            next_itemn_id = std::max(next_itemn_id, kv.second + 1);
        for (auto& item : ap_items)
        {
            auto it = ITEMS_TO_LEGACY_IDS.find(item.name);
            if (it != ITEMS_TO_LEGACY_IDS.end())
                item.id = ITEMS_TO_LEGACY_IDS[item.name];
            else
                unmapped_items.push_back(i);
            ++i;
        }
        for (auto unmapped_item : unmapped_items)
            ap_items[unmapped_item].id = next_itemn_id++;

        // Sort by id so it's clean in AP
        std::sort(ap_items.begin(), ap_items.end(), [](const ap_item_t& a, const ap_item_t& b) { return a.id < b.id; });
    }

    // Fill in locations into level's sectors
    for (int i = 0, len = (int)ap_locations.size(); i < len; ++i)
    {
        auto& loc = ap_locations[i];
        for (const auto& level : levels)
        {
            if (loc.lvl == level->lvl &&
                loc.ep == level->ep &&
                loc.doom_thing_index >= 0)
            {
                auto subsector = point_in_subsector(loc.x, loc.y, &maps[loc.ep - 1][loc.lvl - 1]);
                if (subsector)
                {
                    level->sectors[subsector->sector].locations.push_back(i);
                    loc.sector = subsector->sector;
                }
                else
                {
                    OLogE("Cannot find sector for location: " + loc.name);
                }
                break;
            }
        }
    }

    //---------------------------------------------
    //-------- Generate the python files ----------
    //---------------------------------------------

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
        FILE* fout = fopen((py_out_dir + "Regions.py").c_str(), "w");
        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from typing import List\n");
        fprintf(fout, "from BaseClasses import TypedDict\n\n");

        fprintf(fout, "class RegionDict(TypedDict, total=False): \n");
        fprintf(fout, "    name: str\n");
        fprintf(fout, "    connects_to_hub: bool\n");
        fprintf(fout, "    episode: int\n");
        fprintf(fout, "    connections: List[str]\n\n\n");

        fprintf(fout, "regions:List[RegionDict] = [\n");

        // Regions
        const auto& episodes_json = levels_json["episodes"];
        int ep = 0;
        for (const auto& episode_json : episodes_json)
        {
            int lvl = 0;
            for (const auto& level_json : episode_json)
            {
                const auto& world_connections_json = level_json["world_rules"]["connections"];

                std::string level_name = level_names[ep][lvl];
                if (ep != 0 || lvl != 0) fprintf(fout, "\n");
                fprintf(fout, "    # %s\n", level_name.c_str());

                const auto& regions_json = level_json["regions"];
                int region_i = 0;
                for (const auto& region_json : regions_json)
                {
                    std::string region_name = level_name + " " + region_json["name"].asString();

                    // Create locations
                    const auto& sectors_json = region_json["sectors"];
                    for (const auto& sector_json : sectors_json)
                    {
                        int sectori = sector_json.asInt();
                        const auto& locs = levels[ep * MAP_COUNT + lvl]->sectors[sectori].locations;
                        for (auto loci : locs)
                        {
                            ap_locations[loci].region_name = region_name;
                        }
                    }
                    bool connects_to_exit = false;
                    bool connects_to_hub = false;

                    for (const auto& world_connection_json : world_connections_json)
                    {
                        auto target_region = world_connection_json["target_region"].asInt();
                        if (target_region == region_i) connects_to_hub = true;
                    }

                    // Build the region dicts
                    std::vector<std::string> connections;
                    const auto& rules_json = region_json["rules"];
                    const auto& connections_json = rules_json["connections"];
                    for (const auto& connection_json : connections_json)
                    {
                        int target_region = connection_json["target_region"].asInt();
                        if (target_region == -2)
                        {
                            connects_to_exit = true;
                            continue;
                        }
                        if (target_region == -1)
                        {
                            continue;
                        }

                        connections.push_back("\"" + level_name + " " + regions_json[target_region]["name"].asString() + "\"");
                    }
                    fprintf(fout, "    {\"name\":\"%s\",\n", region_name.c_str());
                    fprintf(fout, "     \"connects_to_hub\":%s,\n", connects_to_hub ? "True" : "False");
                    fprintf(fout, "     \"episode\":%i,\n", ep + 1);
                    if (connections.empty())
                    {
                        fprintf(fout, "     \"connections\":[]},\n");
                    }
                    else if (connections.size() == 1)
                    {
                        fprintf(fout, "     \"connections\":[%s]},\n", connections[0].c_str());
                    }
                    else
                    {
                        fprintf(fout, "     \"connections\":[\n");
                        fprintf(fout, "        %s", onut::join(connections, ",\n        ").c_str());
                        fprintf(fout, "]},\n");
                    }

                    ++region_i;
                }
                ++lvl;
            }
            ++ep;
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
        const auto& episodes_json = levels_json["episodes"];
        int ep = 0;
        for (const auto& episode_json : episodes_json)
        {
            int lvl = 0;
            for (const auto& level_json : episode_json)
            {
                const auto& locations_json = level_json["locations"];
                for (const auto& location_json : locations_json)
                {
                    auto idx = location_json["index"].asInt();
                    if (location_json["death_logic"].asBool())
                    {
                        for (const auto& location : ap_locations)
                        {
                            if (location.lvl - 1 == lvl && location.ep - 1 == ep && location.doom_thing_index == idx)
                            {
                                fprintf(fout, "    \"%s\",\n", location.name.c_str());
                            }
                        }
                    }
                }
                ++lvl;
            }
            ++ep;
        }
        fprintf(fout, "]\n");

        fclose(fout);
    }

    // Maps
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
                                (int)level->map->things.size());
                        int idx = 0;
                        for (const auto& thing : level->map->things)
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

    // Generate Rules.py from regions.json (Manually entered data)
    {
        FILE* fout = fopen((py_out_dir + "Rules.py").c_str(), "w");
        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from typing import TYPE_CHECKING\n");
        fprintf(fout, "from worlds.generic.Rules import set_rule\n\n");
        
        fprintf(fout, "if TYPE_CHECKING:\n");
        fprintf(fout, "    from . import DOOM1993World\n\n");

        const auto& episodes_json = levels_json["episodes"];
        int ep = 0;
        for (const auto& episode_json : episodes_json)
        {
            fprintf(fout, "\ndef set_episode%i_rules(player, world):\n", ep + 1);

            int lvl = 0;
            for (const auto& level_json : episode_json)
            {
                const auto& world_connections_json = level_json["world_rules"]["connections"];

                std::string level_name = level_names[ep][lvl];
                fprintf(fout, "    # %s\n", level_name.c_str());

                const auto& regions_json = level_json["regions"];
                int region_i = 0;
                bool has_rules = false;
                for (const auto& region_json : regions_json)
                {
                    std::string region_name = level_name + " " + region_json["name"].asString();

                    // Hub rules
                    for (const auto& world_connection_json : world_connections_json)
                    {
                        auto world_target_region = world_connection_json["target_region"].asInt();
                        if (world_target_region == region_i)
                        {
                            const auto& world_requirements_and_json = world_connection_json["requirements_and"];
                            const auto& world_requirements_or_json = world_connection_json["requirements_or"];

                            has_rules = true;

                            std::vector<std::string> ands = {"state.has(\"" + level_name + "\", player, 1)"};
                            std::vector<std::string> ors;

                            for (const auto& requirement_and_json: world_requirements_and_json)
                            {
                                int doom_type = requirement_and_json.asInt();
                                ands.push_back("state.has(\"" + get_requirement_name(level_name, doom_type) + "\", player, 1)");
                            }
                            for (const auto& requirement_or_json: world_requirements_or_json)
                            {
                                int doom_type = requirement_or_json.asInt();
                                ors.push_back("state.has(\"" + get_requirement_name(level_name, doom_type) + "\", player, 1)");
                            }

                            fprintf(fout, "    set_rule(world.get_entrance(\"Hub -> %s\", player), lambda state:\n", region_name.c_str());

                            if (ands.empty())
                                fprintf(fout, "        %s)\n", onut::join(ors, " or\n        ").c_str());
                            else if (ors.empty())
                                fprintf(fout, "        %s)\n", onut::join(ands, " and\n        ").c_str());
                            else
                                fprintf(fout, "       (%s) and\n       (%s))\n", onut::join(ands, " and\n        ").c_str(), onut::join(ors, " or\n        ").c_str());
                        }
                    }

                    std::vector<std::string> connections;
                    const auto& rules_json = region_json["rules"];
                    const auto& connections_json = rules_json["connections"];
                    for (const auto& connection_json : connections_json)
                    {
                        int target_region = connection_json["target_region"].asInt();
                        if (target_region == -2)
                        {
                            continue;
                        }
                        if (target_region == -1)
                        {
                            continue;
                        }

                        const auto& requirements_and_json = connection_json["requirements_and"];
                        const auto& requirements_or_json = connection_json["requirements_or"];

                        if (requirements_and_json.empty() && requirements_or_json.empty()) continue;

                        has_rules = true;

                        std::vector<std::string> ands;
                        std::vector<std::string> ors;

                        for (const auto& requirement_and_json: requirements_and_json)
                        {
                            int doom_type = requirement_and_json.asInt();
                            ands.push_back("state.has(\"" + get_requirement_name(level_name, doom_type) + "\", player, 1)");
                        }
                        for (const auto& requirement_or_json: requirements_or_json)
                        {
                            int doom_type = requirement_or_json.asInt();
                            ors.push_back("state.has(\"" + get_requirement_name(level_name, doom_type) + "\", player, 1)");
                        }

                        auto target_name = level_name + " " + regions_json[target_region]["name"].asCString();
                        fprintf(fout, "    set_rule(world.get_entrance(\"%s -> %s\", player), lambda state:\n", region_name.c_str(), target_name.c_str());
                        
                        if (ands.empty())
                            fprintf(fout, "        %s)\n", onut::join(ors, " or\n        ").c_str());
                        else if (ors.empty())
                            fprintf(fout, "        %s)\n", onut::join(ands, " and\n        ").c_str());
                        else
                            fprintf(fout, "       (%s) and       (%s))\n", onut::join(ands, " and\n        ").c_str(), onut::join(ors, " or\n        ").c_str());
                    }
                    ++region_i;
                }
                if (!has_rules)
                {
                    fprintf(fout, "    # No rules...\n\n");
                }
                else
                {
                    fprintf(fout, "\n");
                }
                ++lvl;
            }
            ++ep;
        }

        fprintf(fout, "\ndef set_rules(doom_1993_world: \"DOOM1993World\", included_episodes):\n");
        fprintf(fout, "    player = doom_1993_world.player\n");
        fprintf(fout, "    world = doom_1993_world.multiworld\n\n");
        ep = 0;
        for (const auto& episode_json : episodes_json)
        {
            fprintf(fout, "    if included_episodes[%i]:\n", ep);
            fprintf(fout, "        set_episode%i_rules(player, world)\n", ep + 1);
            ++ep;
        }

        fclose(fout);
    }

    // Generate location CSV that will be used for names
    {
        FILE* fout = fopen((pop_tracker_data_dir + "location_names.csv").c_str(), "w");
        fprintf(fout, "Map,Type,Index,Name,Description\n");
        int ep = 0;
        for (const auto& episode_json : episodes_json)
        {
            int lvl = 0;
            for (const auto& level_json : episode_json)
            {
                fprintf(fout, ",,,,\n");

                const auto& locations_json = level_json["locations"];
                for (const auto& location_json : locations_json)
                {
                    std::string level_name = level_names[ep][lvl];
                    int index = location_json["index"].asInt();

                    fprintf(fout, "%s,", level_name.c_str());
                    fprintf(fout, "%s,", get_doom_type_name(maps[ep][lvl].things[index].type));
                    fprintf(fout, "%i,", index);
                    fprintf(fout, "%s,", escape_csv(location_json["name"].asString()).c_str());
                    fprintf(fout, "%s,\n", escape_csv(location_json["description"].asString()).c_str());
                }
                ++lvl;
            }
            ++ep;
        }
        fclose(fout);
    }

    // Now for Poptracker, rasterize levels onto a 1024x1024

    // Clean up
    for (auto level : levels) delete level; // We don't need to
    return 0;
}
