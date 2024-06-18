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
#include "generate.h"
#include "data.h"

#include <algorithm>


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
    level_index_t idx;
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
    level_index_t idx;
    int doom_thing_index;
    int doom_type;
    std::vector<int64_t> required_items;
    int sector;
    int region = -1;
    std::string region_name;
    bool check_sanity = false;
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
    level_index_t idx;
    std::string name;
    std::vector<level_sector_t> sectors;
    int starting_sector = -1;
    bool keys[3] = {false};
    int location_count = 0;
    bool use_skull[3] = {false};
    map_t* map = nullptr;
    map_state_t* map_state = nullptr;
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


static std::string get_requirement_name(game_t* game, const std::string& level_name, int doom_type)
{
    for (const auto& item : game->unique_progressions)
        if (item.doom_type == doom_type)
            return level_name + " - " + item.name;

    for (const auto& item : game->keys)
        if (item.item.doom_type == doom_type)
            return level_name + " - " + item.item.name;

    for (const auto& requirement : game->item_requirements)
        if (requirement.doom_type == doom_type)
            return requirement.name;

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
    // Make sure it's not unreachable
    if (level->map_state->locations[index].unreachable) return;

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
    loc.idx = level->idx;
    loc.doom_thing_index = index;
    loc.doom_type = thing.type; // Index can be a risky one. We could replace the item by it's type if it's unique enough
    loc.x = x << 16;
    loc.y = y << 16;
    loc.check_sanity = level->map_state->locations[index].check_sanity;
    ap_locations.push_back(loc);

    level->location_count++;
    total_loc_count++;
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
        item.idx = level->idx;
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
    item.idx = level ? level->idx : level_index_t{"",-2,-2};
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


std::string convert_quoted_str(const std::string& str)
{
    bool has_single_quote = false;
    for (auto c : str)
    {
        if (c == '\'')
        {
            has_single_quote = true;
            break;
        }
    }
    if (has_single_quote)
        return "\"" + str + "\"";
    else
        return "'" + str + "'";
}


// This is a mess. Many refactors. Sorry...
int generate(game_t* game)
{
    OLog("AP Gen Tool");

    if (OArguments.size() != 3) // Minimum effort validation
    {
        OLogE("Usage: ap_gen_tool.exe python_py_out_dir cpp_py_out_dir poptracker_data_dir\n  i.e: ap_gen_tool.exe C:\\github\\Archipelago\\worlds C:\\github\\apdoom\\src\\archipelago C:\\github\\apdoom\\data\\poptracker");
        return 1;
    }

    std::string py_out_dir = OArguments[0] + "\\" + game->world + "\\";
    item_id_base = game->item_ids;
    item_next_id = item_id_base;
    location_next_id = game->loc_ids;

    total_item_count = 0;
    total_loc_count = 0;
    ap_items.clear();
    ap_locations.clear();
    item_name_groups.clear();
    level_to_keycards.clear();
    item_map.clear();

    std::string cpp_out_dir = OArguments[1] + std::string("\\");
    std::string pop_tracker_data_dir = OArguments[2] + std::string("\\");

    ap_locations.reserve(1000);
    ap_items.reserve(1000);

    for (const auto& def : game->progressions)
        add_item(def.name, def.doom_type, 1, PROGRESSION, def.group);

    // Backpack / Bag of Holding are handled kinda unusually because they used to be considered progression.
    for (const auto& def : game->capacity_upgrades)
    {
        if (def.doom_type == 8)
            add_item(def.name, def.doom_type, 0, USEFUL, def.group);
    }

    for (const auto& def : game->fillers)
        add_item(def.name, def.doom_type, 0, FILLER, def.group);
    
    std::vector<level_t*> levels;
    std::map<int, std::vector<level_t*>> levels_map;
    int ep = 0;
    for (auto& episode : game->episodes)
    {
        int map = 0;
        for (auto& meta : episode)
        {
            level_t* level = new level_t();
            level->idx = {game->name, ep, map};
            level->name = meta.name;
            level->map = &meta.map;
            level->map_state = &meta.state;
            levels.push_back(level);
            levels_map[ep].push_back(level);
            ++map;
        }
        ++ep;
    }

    auto get_level = [&levels_map](const level_index_t& idx) -> level_t*
    {
        return levels_map[idx.ep][idx.map];
    };
    
    // Keycard n such
    item_next_id = item_id_base + 200;
    for (auto level : levels)
    {
        auto map = level->map;
        level->sectors.resize(map->sectors.size());
        std::string lvl_prefix = level->name + std::string(" - ");

        for (int i = 0, len = (int)level->map->things.size(); i < len; ++i)
        {
            const auto& thing = level->map->things[i];
            if (thing.flags & 0x0010)
                continue; // Thing is not in single player
            if (game->location_doom_types.find(thing.type) == game->location_doom_types.end())
                continue; // Not a location

            bool added_key = false;
            for (const auto& key_def : game->keys)
            {
                if (key_def.item.doom_type == thing.type)
                {
                    level_to_keycards[(uintptr_t)level][0] = add_unique(lvl_prefix + key_def.item.name, thing, level, i, true, PROGRESSION, "Keys", thing.x, thing.y);
                    level->keys[key_def.key] = true;
                    level->use_skull[key_def.key] = key_def.use_skull;
                    added_key = true;
                    break;
                }
            }
            if (added_key)
            {
                continue;
            }

            //if (level->map_state->locations[i].unreachable)
            //    continue; // We don't include this location

            if (level->idx.ep == 0 && i == 43)
            {
                int tmp;
                tmp = 5;
            }

            auto loc_it = game->location_doom_types.find(thing.type);
            if (loc_it != game->location_doom_types.end())
            {
                add_loc(lvl_prefix + loc_it->second, thing, level, i, thing.x, thing.y);
            }
        }

        int region_i = 0;
        bool connects_to_exit = false;
        for (const auto& region : level->map_state->regions)
        {
            std::string region_name = level->name + std::string(" ") + region.name;

            for (const auto& connection : region.rules.connections)
            {
                if (connection.target_region == -2)
                {
                    connects_to_exit = true;
                    ap_location_t complete_loc;
                    complete_loc.doom_thing_index = -1;
                    complete_loc.doom_type = -1;
                    complete_loc.idx = level->idx;
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
    }

    // Lastly, add level items. We want to add more levels in the future and not shift all existing item IDs
    item_next_id = item_id_base + 400;
    for (auto level : levels)
    {
        std::string lvl_prefix = level->name + std::string(" - ");

        add_item(level->name, -1, 1, PROGRESSION, "Levels", 0, level);
        add_item(lvl_prefix + "Complete", -2, 1, PROGRESSION, "", 0, level);

        for (const auto& def : game->unique_progressions)
        {
            add_item(lvl_prefix + def.name, def.doom_type, 1, PROGRESSION, def.group, 0, level);
        }

        for (const auto& def : game->unique_fillers)
        {
            add_item(lvl_prefix + def.name, def.doom_type, 1, FILLER, def.group, 0, level);
        }
    }

    OLog(std::to_string(total_loc_count) + " locations\n" + std::to_string(total_item_count - 3) + " items");

    // Items unique to Archipelago: Capacity upgrades (excluding backpack / bag of holding)
    item_next_id = item_id_base + 600;
    for (const auto& def : game->capacity_upgrades)
    {
        if (def.doom_type != 8)
            add_item(def.name, def.doom_type, 0, USEFUL, def.group);
    }

    //--- Remap location's IDs
    {
        if (!game->loc_remap.empty())
        {
            int64_t next_location_id = 0;
            std::vector<int> unmapped_locations;
            int i = 0;
            for (const auto& kv : game->loc_remap)
                next_location_id = std::max(next_location_id, kv.second + 1);
            for (auto& location : ap_locations)
            {
                auto it = game->loc_remap.find(location.name);
                if (it != game->loc_remap.end())
                    location.id = game->loc_remap[location.name];
                else
                    unmapped_locations.push_back(i);
                ++i;
            }
            for (auto unmapped_location : unmapped_locations)
                ap_locations[unmapped_location].id = next_location_id++;
        }

        // Sort by id so it's clean in AP
        std::sort(ap_locations.begin(), ap_locations.end(), [](const ap_location_t& a, const ap_location_t& b) { return a.id < b.id; });
    }

    //--- Remap item's IDs
    {
        if (!game->item_remap.empty())
        {
            int64_t next_itemn_id = 0;
            std::vector<int> unmapped_items;
            int i = 0;
            for (const auto &kv : game->item_remap)
                next_itemn_id = std::max(next_itemn_id, kv.second + 1);
            for (auto& item : ap_items)
            {
                auto it = game->item_remap.find(item.name);
                if (it != game->item_remap.end())
                    item.id = game->item_remap[item.name];
                else
                    unmapped_items.push_back(i);
                ++i;
            }
            for (auto unmapped_item : unmapped_items)
                ap_items[unmapped_item].id = next_itemn_id++;

            // Sort by id so it's clean in AP
            std::sort(ap_items.begin(), ap_items.end(), [](const ap_item_t& a, const ap_item_t& b) { return a.id < b.id; });
        }
    }

    // Fill in locations into level's sectors
    for (int i = 0, len = (int)ap_locations.size(); i < len; ++i)
    {
        auto& loc = ap_locations[i];
        if (loc.doom_thing_index < 0) continue;
        auto level = get_level(loc.idx);
        auto subsector = point_in_subsector(loc.x, loc.y, get_map(loc.idx));
        if (subsector)
        {
            level->sectors[subsector->sector].locations.push_back(i);
            loc.sector = subsector->sector;
        }
        else
        {
            OLogE("Cannot find sector for location: " + loc.name);
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
    doom_type: int # Unique numerical id used to spawn the item. -1 is level item, -2 is level complete item. \n");
        fprintf(fout, "    episode: int # Relevant if that item targets a specific level, like keycard or map reveal pickup. \n");
        fprintf(fout, "    map: int \n\
\n\
\n\
");

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
            fprintf(fout, ",\n             'name': %s", convert_quoted_str(item.name).c_str());
            fprintf(fout, ",\n             'doom_type': %i", item.doom_type);
            fprintf(fout, ",\n             'episode': %i", item.idx.ep + 1);
            fprintf(fout, ",\n             'map': %i", item.idx.map + 1);
            fprintf(fout, "},\n");
        }
        fprintf(fout, "}\n\n\n");

        // item_name_groups
        fprintf(fout, "item_name_groups: Dict[str, Set[str]] = {\n");
        for (const auto& kv : item_name_groups)
        {
            fprintf(fout, "    %s: {", convert_quoted_str(kv.first).c_str());
            for (const auto& item_name : kv.second)
            {
                fprintf(fout, "%s, ", convert_quoted_str(item_name).c_str());
            }
            fprintf(fout, "},\n");
        }
        fprintf(fout, "}\n");

        fclose(fout);
    }

    // Generate Regions.py from regions.json (Manually entered data)
    {
        FILE* fout = fopen((py_out_dir + "Regions.py").c_str(), "w");
        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from typing import List\n");
        fprintf(fout, "from BaseClasses import TypedDict\n\n");

        fprintf(fout, "class ConnectionDict(TypedDict, total=False):\n");
        fprintf(fout, "    target: str\n");
        fprintf(fout, "    pro: bool\n\n");

        fprintf(fout, "class RegionDict(TypedDict, total=False):\n");
        fprintf(fout, "    name: str\n");
        fprintf(fout, "    connects_to_hub: bool\n");
        fprintf(fout, "    episode: int\n");
        fprintf(fout, "    connections: List[ConnectionDict]\n\n\n");

        fprintf(fout, "regions:List[RegionDict] = [\n");

        // Regions
        bool first = true;
        for (auto level : levels)
        {
            const std::string& level_name = level->name;
            if (!first) fprintf(fout, "\n");
            first = false;
            fprintf(fout, "    # %s\n", level_name.c_str());

            int region_i = 0;
            for (const auto& region : level->map_state->regions)
            {
                std::string region_name = level_name + " " + region.name;

                // Create locations
                for (auto sectori : region.sectors)
                {
                    const auto& locs = level->sectors[sectori].locations;
                    for (auto loci : locs)
                    {
                        ap_locations[loci].region_name = region_name;
                    }
                }
                bool connects_to_exit = false;
                bool connects_to_hub = false;

                for (const auto& world_connection : level->map_state->world_rules.connections)
                {
                    if (world_connection.target_region == region_i) connects_to_hub = true;
                }

                // Build the region dicts
                std::vector<std::string> connections;
                for (const auto& connection : region.rules.connections)
                {
                    if (connection.target_region == -2)
                    {
                        connects_to_exit = true;
                        continue;
                    }
                    if (connection.target_region == -1)
                    {
                        continue;
                    }
                    
                    auto pro = false;
                    for (auto req : connection.requirements_and)
                    {
                        if (req == -2)
                        {
                            pro = true;
                            break;
                        }
                    }
                    connections.push_back("{\"target\":\"" + level_name + " " + level->map_state->regions[connection.target_region].name + "\",\"pro\":" + (pro?"True":"False") + "}");
                }
                fprintf(fout, "    {\"name\":\"%s\",\n", region_name.c_str());
                fprintf(fout, "     \"connects_to_hub\":%s,\n", connects_to_hub ? "True" : "False");
                fprintf(fout, "     \"episode\":%i,\n", level->idx.ep + 1);
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
        }
        fprintf(fout, "]\n");

        fclose(fout);
    }
    
    // Locations
    {
        FILE* fout = fopen((py_out_dir + "Locations.py").c_str(), "w");

        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from typing import Dict, TypedDict, List, Set \n\
\n\
\n\
class LocationDict(TypedDict, total=False): \n\
    name: str \n");
        fprintf(fout, "    episode: int \n");
        if (game->check_sanity)
            fprintf(fout, "    check_sanity: bool \n");
        fprintf(fout, "    map: int \n\
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
            // Check from json if that location is not marked as "unreachable"

            fprintf(fout, "    %llu: {", loc.id);
            fprintf(fout, "'name': %s", convert_quoted_str(loc.name).c_str());
            fprintf(fout, ",\n             'episode': %i", loc.idx.ep + 1);
            if (game->check_sanity)
                fprintf(fout, ",\n             'check_sanity': %s", loc.check_sanity ? "True" : "False");
            fprintf(fout, ",\n             'map': %i", loc.idx.map + 1);
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
            location_name_groups[get_level_name(loc.idx)].insert(loc.name);
        }
        for (const auto& kv : location_name_groups)
        {
            fprintf(fout, "    %s: {", convert_quoted_str(kv.first).c_str());
            for (const auto& v : kv.second)
            {
                fprintf(fout, "\n        %s,", convert_quoted_str(v).c_str());
            }
            fprintf(fout, "\n    },\n");
        }
        fprintf(fout, "}\n\n\n");

        // Death logic locations
        fprintf(fout, "death_logic_locations = [\n");

        for (auto level : levels)
        {
            for (const auto& location_kv : level->map_state->locations)
            {
                auto idx = location_kv.first;
                if (location_kv.second.death_logic)
                {
                    for (const auto& location : ap_locations)
                    {
                        if (location.idx == level->idx && location.doom_thing_index == idx)
                        {
                            fprintf(fout, "    \"%s\",\n", location.name.c_str());
                        }
                    }
                }
            }
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
            fprintf(fout, "\n    %s,", convert_quoted_str(level->name).c_str());
        }
        fprintf(fout, "\n]\n");

        fclose(fout);
    }

    // Now generate apdoom_def.h so the game can map the IDs
    {
        FILE* fout = fopen((cpp_out_dir + "ap" + game->codename + "_def.h").c_str(), "w");
        
        fprintf(fout, "// This file is auto generated. More info: https://github.com/Daivuk/apdoom\n");
        fprintf(fout, "#pragma once\n\n");
        fprintf(fout, "#include \"apdoom.h\"\n");
        fprintf(fout, "#include \"apdoom_def_types.h\"\n");
        fprintf(fout, "#include <map>\n");
        fprintf(fout, "#include <string>\n");
        fprintf(fout, "#include <vector>\n\n\n");

        std::map<int /* ep */, std::map<int /* map */, std::map<int /* index */, int64_t /* loc id */>>> location_table;
        for (const auto& loc : ap_locations)
        {
            location_table[loc.idx.ep + 1][loc.idx.map + 1][loc.doom_thing_index] = loc.id;
        }

        // locations
        fprintf(fout, "const std::map<int /* ep */, std::map<int /* map */, std::map<int /* index */, int64_t /* loc id */>>> ap_%s_location_table = {\n", game->codename.c_str());
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
        fprintf(fout, "const std::map<int64_t, ap_item_t> ap_%s_item_table = {\n", game->codename.c_str());
        for (const auto& item : ap_items)
        {
            fprintf(fout, "    {%llu, {%i, %i, %i}},\n", item.id, item.doom_type, item.idx.ep + 1, item.idx.map + 1);
        }
        fprintf(fout, "};\n\n\n");

        // Level infos
        fprintf(fout, "std::vector<std::vector<ap_level_info_t>> ap_%s_level_infos = \n", game->codename.c_str());
        fprintf(fout, "{\n");
        for (int ep = 0; ep < game->ep_count; ++ep)
        {
            fprintf(fout, "    {\n");
            int map = 0;
            for (const auto& meta : game->episodes[ep])
            {
                auto level = get_level({game->name, ep, map});
                fprintf(fout, "        {\"%s\", {%s, %s, %s}, {%i, %i, %i}, %i, %i, {\n", 
                        level->name.c_str(),
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
                    bool check_sanity = false;
                    bool unreachable = true;
                    for (const auto& loc : ap_locations)
                    {
                        if (loc.idx == level->idx &&
                            loc.doom_thing_index == idx)
                        {
                            unreachable = false;
                            check_sanity = loc.check_sanity;
                            break;
                        }
                    }
                    fprintf(fout, "            {%i, %i, %i, %i},\n", thing.type, idx, check_sanity ? 1 : 0, unreachable ? 1 : 0);
                    ++idx;
                }
                fprintf(fout, "        }},\n");
                ++map;
            }
            fprintf(fout, "    },\n");
        }
        fprintf(fout, "};\n\n\n");

        // Item sprites (Used by notification icons)
        fprintf(fout, "const std::map<int, std::string> ap_%s_type_sprites = {\n", game->codename.c_str());
        for (const auto& item : game->progressions)
            fprintf(fout, "    {%i, \"%s\"},\n", item.doom_type, item.sprite.c_str());
        for (const auto& item : game->fillers)
            fprintf(fout, "    {%i, \"%s\"},\n", item.doom_type, item.sprite.c_str());
        for (const auto& item : game->unique_progressions)
            fprintf(fout, "    {%i, \"%s\"},\n", item.doom_type, item.sprite.c_str());
        for (const auto& item : game->unique_fillers)
            fprintf(fout, "    {%i, \"%s\"},\n", item.doom_type, item.sprite.c_str());
        for (const auto& item : game->capacity_upgrades)
            fprintf(fout, "    {%i, \"%s\"},\n", item.doom_type, item.sprite.c_str());
        for (const auto& item : game->keys)
            fprintf(fout, "    {%i, \"%s\"},\n", item.item.doom_type, item.item.sprite.c_str());
        fprintf(fout, "};\n");

        fclose(fout);
    }

    // We generate some stuff for doom also, C header.
    {
        FILE* fout = fopen((cpp_out_dir + "ap" + game->codename + "_c_def.h").c_str(), "w");
        
        fprintf(fout, "// This file is auto generated. More info: https://github.com/Daivuk/apdoom\n");
        fprintf(fout, "#ifndef _AP_%s_C_DEF_\n", game->codename.c_str());
        fprintf(fout, "#define _AP_%s_C_DEF_\n\n", game->codename.c_str());

        fprintf(fout, "static int is_%s_type_ap_location(int doom_type)\n", game->codename.c_str());
        fprintf(fout, "{\n");
        fprintf(fout, "    switch (doom_type)\n");
        fprintf(fout, "    {\n");

        for (const auto& location_doom_type_kv : game->location_doom_types)
        {
            fprintf(fout, "        case %i:\n", location_doom_type_kv.first);
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
        fprintf(fout, "    from . import %sWorld\n\n", game->classname.c_str());
        
        int prev_ep = -1;
        for (auto level : levels)
        {
            if (level->idx.ep != prev_ep)
            {
                prev_ep = level->idx.ep;
                fprintf(fout, "\ndef set_episode%i_rules(player, multiworld, pro):\n", level->idx.ep + 1);
            }

            const std::string& level_name = level->name;
            fprintf(fout, "    # %s\n", level_name.c_str());

            int region_i = 0;
            bool has_rules = false;
            for (const auto& region : level->map_state->regions)
            {
                std::string region_name = level_name + " " + region.name;

                // Hub rules
                for (const auto& world_connection : level->map_state->world_rules.connections)
                {
                    auto world_target_region = world_connection.target_region;
                    if (world_target_region == region_i)
                    {
                        has_rules = true;

                        std::vector<std::string> ands = {"state.has(\"" + level_name + "\", player, 1)"};
                        std::vector<std::string> ors;

                        for (auto doom_type: world_connection.requirements_and)
                        {
                            ands.push_back("state.has(\"" + get_requirement_name(game, level_name, doom_type) + "\", player, 1)");
                        }
                        for (auto doom_type: world_connection.requirements_or)
                        {
                            ors.push_back("state.has(\"" + get_requirement_name(game, level_name, doom_type) + "\", player, 1)");
                        }

                        fprintf(fout, "    set_rule(multiworld.get_entrance(\"Hub -> %s\", player), lambda state:\n", region_name.c_str());

                        if (ands.empty())
                            fprintf(fout, "        %s)\n", onut::join(ors, " or\n        ").c_str());
                        else if (ors.empty())
                            fprintf(fout, "        %s)\n", onut::join(ands, " and\n        ").c_str());
                        else
                            fprintf(fout, "       (%s) and\n       (%s))\n", onut::join(ands, " and\n        ").c_str(), onut::join(ors, " or\n        ").c_str());
                    }
                }

                std::vector<std::string> connections;
                for (const auto& connection : region.rules.connections)
                {
                    int target_region = connection.target_region;
                    if (target_region == -2)
                    {
                        continue;
                    }
                    if (target_region == -1)
                    {
                        continue;
                    }

                    int count = 0;
                    for (auto req : connection.requirements_and)
                        if (req >= 0)
                            ++count;
                    for (auto req : connection.requirements_or)
                        if (req >= 0)
                            ++count;
                    if (count == 0) continue;
                    
                    auto pro = false;
                    for (auto req : connection.requirements_and)
                    {
                        if (req == -2)
                        {
                            pro = true;
                            break;
                        }
                    }

                    has_rules = true;

                    std::vector<std::string> ands;
                    std::vector<std::string> ors;

                    std::string indent = "";
                    if (pro )
                    {
                        indent = "    ";
                        fprintf(fout, "    if pro:\n");
                    }

                    for (auto doom_type: connection.requirements_and)
                    {
                        if (doom_type >= 0)
                            ands.push_back("state.has(\"" + get_requirement_name(game, level_name, doom_type) + "\", player, 1)");
                    }
                    for (auto doom_type: connection.requirements_or)
                    {
                        if (doom_type >= 0)
                            ors.push_back("state.has(\"" + get_requirement_name(game, level_name, doom_type) + "\", player, 1)");
                    }

                    auto target_name = level_name + " " + level->map_state->regions[target_region].name;
                    fprintf(fout, "%s    set_rule(multiworld.get_entrance(\"%s -> %s\", player), lambda state:\n", indent.c_str(), region_name.c_str(), target_name.c_str());
                        
                    if (ands.empty())
                        fprintf(fout, "%s        %s)\n", indent.c_str(), onut::join(ors, " or\n        ").c_str());
                    else if (ors.empty())
                        fprintf(fout, "%s        %s)\n", indent.c_str(), onut::join(ands, " and\n        ").c_str());
                    else
                        fprintf(fout, "%s       (%s) and       (%s))\n", indent.c_str(), onut::join(ands, " and\n        ").c_str(), onut::join(ors, " or\n        ").c_str());
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
        }

        fprintf(fout, "\ndef set_rules(%s_world: \"%sWorld\", included_episodes, pro):\n", game->world.c_str(), game->classname.c_str());
        fprintf(fout, "    player = %s_world.player\n", game->world.c_str());
        fprintf(fout, "    multiworld = %s_world.multiworld\n\n", game->world.c_str());
        for (int ep = 0; ep < game->ep_count; ++ep)
        {
            fprintf(fout, "    if included_episodes[%i]:\n", ep);
            fprintf(fout, "        set_episode%i_rules(player, multiworld, pro)\n", ep + 1);
        }

        fclose(fout);
    }

    // Generate location CSV that will be used for names
    {
        FILE* fout = fopen((pop_tracker_data_dir + game->codename + "_location_names.csv").c_str(), "w");

        fprintf(fout, "Map,Type,Index,Name,Description\n");

        for (auto level : levels)
        {
            fprintf(fout, ",,,,\n");

            for (const auto& location_kv : level->map_state->locations)
            {
                const auto& location = location_kv.second;
                if (location.unreachable) continue;

                int index = location_kv.first;

                fprintf(fout, "%s,", level->name.c_str());
                fprintf(fout, "%s,", game->location_doom_types[level->map->things[index].type].c_str());
                fprintf(fout, "%i,", index);
                fprintf(fout, "%s,", escape_csv(location.name).c_str());
                fprintf(fout, "%s,\n", escape_csv(location.description).c_str());
            }
        }
        fclose(fout);
    }

    // TODO: Pop tracker logic

    // Clean up
    for (auto level : levels) delete level;
    return 0;
}
