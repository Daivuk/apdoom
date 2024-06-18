#pragma once


#include <onut/Maths.h>
#include <onut/Vector2.h>
#include <onut/Texture.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include "maps.h"


struct rule_connection_t
{
    int target_region = -1;
    std::vector<int> requirements_or;
    std::vector<int> requirements_and;

    bool operator==(const rule_connection_t& other) const
    {
        return 
            target_region == other.target_region &&
            requirements_or == other.requirements_or &&
            requirements_and == other.requirements_and;
    }
};


struct rule_region_t
{
    int x = 0, y = 0;
    std::vector<rule_connection_t> connections;

    bool operator==(const rule_region_t& other) const
    {
        return 
            x == other.x &&
            y == other.y &&
            connections == other.connections;
    }
};

struct bb_t
{
    int x1, y1, x2, y2;
    int region = -1;

    int overlaps(const bb_t& other) const
    {
        auto d1 = other.x2 - x1;
        if (d1 < 0) return 0;

        auto d2 = x2 - other.x1;
        if (d2 < 0) return 0;

        auto d3 = other.y2 - y1;
        if (d3 < 0) return 0;

        auto d4 = y2 - other.y1;
        if (d4 < 0) return 0;

        return onut::max(d1, d2, d3, d4);

        //return (x1 <= other.x2 && x2 >= other.x1 && 
        //        y1 <= other.y2 && y2 >= other.y1);
    }

    bb_t operator+(const Vector2& v) const
    {
        return {
            (int)(x1 + v.x),
            (int)(y1 + v.y),
            (int)(x2 + v.x),
            (int)(y2 + v.y)
        };
    }

    Vector2 center() const
    {
        return {
            (float)(x1 + x2) * 0.5f,
            (float)(y1 + y2) * 0.5f
        };
    }

    bool operator==(const bb_t& other) const
    {
        return 
            x1 == other.x1 &&
            y1 == other.y1 &&
            x2 == other.x2 &&
            y2 == other.y2 &&
            region == other.region;
    }
};


struct region_t
{
    std::string name;
    std::set<int> sectors;
    Color tint = Color::White;
    rule_region_t rules;

    bool operator==(const region_t& other) const
    {
        return 
            name == other.name &&
            sectors == other.sectors &&
            tint == other.tint &&
            rules == other.rules;
    }
};


struct location_t
{
    bool death_logic = false;
    bool unreachable = false; // Check will be removed (Unreachable area)
    bool check_sanity = false; // Removed, but option to put it back
    std::string name;
    std::string description;

    bool operator==(const location_t& other) const
    {
        return 
            death_logic == other.death_logic &&
            unreachable == other.unreachable &&
            name == other.name &&
            description == other.description;
    }
};


struct map_state_t
{
    Vector2 pos;
    float angle = 0.0f;
    int selected_bb = -1;
    int selected_region = -1;
    int selected_location = -1;
    std::vector<bb_t> bbs;
    std::vector<region_t> regions;
    rule_region_t world_rules;
    rule_region_t exit_rules;
    std::set<int> accesses;
    std::map<int, location_t> locations;
    bool different = false;
    int check_sanity_count;

    bool operator==(const map_state_t& other) const
    {
        return 
            bbs == other.bbs &&
            regions == other.regions &&
            world_rules == other.world_rules &&
            exit_rules == other.exit_rules &&
            accesses == other.accesses &&
            locations == other.locations;
    }
};


struct map_view_t
{
    Vector2 cam_pos;
    float cam_zoom = 0.25f;
};


struct map_history_t
{
    std::vector<map_state_t> history;
    int history_point = 0;
};


struct meta_t // Bad name, but everything about a level
{
    std::string name;
    map_t map; // As loaded from the wad
    map_state_t state; // What we play with
    map_state_t state_new; // For diffing
    map_view_t view; // Camera zoom/position
    map_history_t history; // History of map_state_t for undo/redo (It's infinite!)
};


struct ap_item_def_t
{
    int doom_type = -1;
    std::string name;
    std::string group;
    std::string sprite;
    OTextureRef icon;
};


struct ap_key_def_t
{
    ap_item_def_t item;
    int key = -1;
    bool use_skull = false; // Only relevent for doom games
    std::string region_name;
    Color color;
};


struct game_t
{
    std::string name; // Official name used on websites. i.e. "DOOM 1993"
    std::string world; // World folder name. i.e. "doom_1993", which will result in a world placed in "Archipelago/worlds/doom_1993/"
    std::string codename; // Used for C and C++ function/struct names. i.e. "doom", which will result in things like "ap_doom_location_table[]"
    std::string classname; // Class name prefixed to classes in Archipelago. i.e. "DOOM1993", which result in "DOOM1993World", "DOOM1993Web", etc.
    std::string wad_name; // Wad file to load and analyze for this game. i.e. "DOOM.WAD"
    int64_t item_ids = 0;
    int64_t loc_ids = 0;
    std::map<int, std::string> location_doom_types;
    std::vector<ap_item_def_t> extra_connection_requirements;
    std::vector<ap_item_def_t> progressions;
    std::vector<ap_item_def_t> fillers;
    std::vector<ap_item_def_t> unique_progressions;
    std::vector<ap_item_def_t> unique_fillers;
    std::vector<ap_item_def_t> capacity_upgrades;
    std::vector<ap_key_def_t> keys;
    std::map<std::string, int64_t> loc_remap;
    std::map<std::string, int64_t> item_remap;

    Color key_colors[3];
    int ep_count = -1;
    std::vector<std::vector<meta_t>> episodes;
    std::vector<ap_item_def_t> item_requirements;
    bool check_sanity = false;
    std::map<int, int> total_doom_types; // Count of every doom types in the game
};


struct level_index_t
{
    std::string game_name;
    int ep = -1;
    int map = -1;

    bool operator==(const level_index_t& other) const
    {
        return game_name == other.game_name && ep == other.ep && map == other.map;
    }

    bool operator!() const
    {
        return ep < 0 && map < 0;
    }
};


enum class active_source_t
{
    current,
    target
};


extern std::map<std::string, game_t> games;


void init_data();
game_t* get_game(const level_index_t& idx);
meta_t* get_meta(const level_index_t& idx, active_source_t source = active_source_t::current);
map_state_t* get_state(const level_index_t& idx, active_source_t source = active_source_t::current);
map_t* get_map(const level_index_t& idx);
const std::string& get_level_name(const level_index_t& idx);
