#include "data.h"
#include "maps.h"

#include <onut/Files.h>
#include <onut/Json.h>
#include <onut/Log.h>
#include <json/json.h>


std::map<std::string, game_t> games;


void init_data()
{
    auto game_json_files = onut::findAllFiles("./games/", "json", false);
    for (const auto& game_json_file : game_json_files)
    {
        Json::Value game_json;
        if (!onut::loadJson(game_json, game_json_file)) continue;

        game_t game;

        game.name = game_json["name"].asString();
        game.world = game_json["world"].asString();
        game.codename = game_json["codename"].asString();
        game.classname = game_json["classname"].asString();
        game.wad_name = game_json["wad"].asString();
        game.item_ids = game_json["item_ids"].asInt64();
        game.loc_ids = game_json["loc_ids"].asInt64();
        game.check_sanity = game_json["check_sanity"].asBool();

        const auto& episodes_json = game_json["map_names"];
        if (!episodes_json.empty())
        {
            if (episodes_json[0].isArray()) // Episodic
            {
                game.ep_count = (int)episodes_json.size();
                game.episodes.resize(game.ep_count);
                int ep = 0;
                for (const auto& episode_json : episodes_json)
                {
                    game.episodes[ep].resize(episode_json.size());
                    int map = 0;
                    for (const auto& mapname_json : episode_json)
                    {
                        game.episodes[ep][map].name = mapname_json.asString();
                        ++map;
                    }
                    ++ep;
                }
            }
        }

        const auto& doom_types_ids = game_json["location_doom_types"].getMemberNames();
        for (const auto& doom_types_id : doom_types_ids)
        {
            game.location_doom_types[std::stoi(doom_types_id)] = game_json["location_doom_types"][doom_types_id].asString();
        }

        const auto& extra_connection_requirements_json = game_json["extra_connection_requirements"];
        for (const auto& extra_connection_requirement_json : extra_connection_requirements_json)
        {
            ap_item_def_t item;
            item.doom_type = extra_connection_requirement_json["doom_type"].asInt();
            item.name = extra_connection_requirement_json["name"].asString();
            item.sprite = extra_connection_requirement_json["sprite"].asString();
            game.extra_connection_requirements.push_back(item);
        }

        const auto& progressions_json = game_json["progressions"];
        for (const auto& progression_json : progressions_json)
        {
            ap_item_def_t item;
            item.doom_type = progression_json["doom_type"].asInt();
            item.name = progression_json["name"].asString();
            item.group = progression_json["group"].asString();
            item.sprite = progression_json["sprite"].asString();
            game.progressions.push_back(item);
        }

        const auto& fillers_json = game_json["fillers"];
        for (const auto& filler_json : fillers_json)
        {
            ap_item_def_t item;
            item.doom_type = filler_json["doom_type"].asInt();
            item.name = filler_json["name"].asString();
            item.group = filler_json["group"].asString();
            item.sprite = filler_json["sprite"].asString();
            game.fillers.push_back(item);
        }

        const auto& unique_progressions_json = game_json["unique_progressions"];
        for (const auto& progression_json : unique_progressions_json)
        {
            ap_item_def_t item;
            item.doom_type = progression_json["doom_type"].asInt();
            item.name = progression_json["name"].asString();
            item.group = progression_json["group"].asString();
            item.sprite = progression_json["sprite"].asString();
            game.unique_progressions.push_back(item);
        }

        const auto& unique_fillers_json = game_json["unique_fillers"];
        for (const auto& filler_json : unique_fillers_json)
        {
            ap_item_def_t item;
            item.doom_type = filler_json["doom_type"].asInt();
            item.name = filler_json["name"].asString();
            item.group = filler_json["group"].asString();
            item.sprite = filler_json["sprite"].asString();
            game.unique_fillers.push_back(item);
        }

        const auto& capacity_upgrades_json = game_json["capacity_upgrades"];
        for (const auto& capacity_upgrade_json : capacity_upgrades_json)
        {
            ap_item_def_t item;
            item.doom_type = capacity_upgrade_json["doom_type"].asInt();
            item.name = capacity_upgrade_json["name"].asString();
            item.group = capacity_upgrade_json["group"].asString();
            item.sprite = capacity_upgrade_json["sprite"].asString();
            game.capacity_upgrades.push_back(item);
        }

        const auto& keys_json = game_json["keys"];
        for (const auto& key_json : keys_json)
        {
            ap_key_def_t item;
            item.item.doom_type = key_json["doom_type"].asInt();
            item.item.name = key_json["name"].asString();
            item.item.group = key_json["group"].asString();
            item.item.sprite = key_json["sprite"].asString();
            item.key = key_json["key"].asInt();
            item.use_skull = key_json["use_skull"].asBool();
            item.region_name = key_json["region_name"].asString();
            item.color = Color(key_json["color"][0].asFloat(), key_json["color"][1].asFloat(), key_json["color"][2].asFloat());
            game.key_colors[item.key] = item.color;
            game.keys.push_back(item);
        }

        const auto& loc_remap_json = game_json["loc_remap"];
        const auto& loc_remap_names = game_json["loc_remap"].getMemberNames();
        for (const auto& loc_name : loc_remap_names)
        {
            game.loc_remap[loc_name] = loc_remap_json[loc_name].asInt64();
        }

        const auto& item_remap_json = game_json["item_remap"];
        const auto& item_remap_names = game_json["item_remap"].getMemberNames();
        for (const auto& item_name : item_remap_names)
        {
            game.item_remap[item_name] = item_remap_json[item_name].asInt64();
        }

        game.item_requirements.insert(game.item_requirements.end(), game.extra_connection_requirements.begin(), game.extra_connection_requirements.end());
        for (const auto& key : game.keys)
            game.item_requirements.push_back(key.item);
        game.item_requirements.insert(game.item_requirements.end(), game.progressions.begin(), game.progressions.end());
        game.item_requirements.insert(game.item_requirements.end(), game.unique_progressions.begin(), game.unique_progressions.end());

        init_maps(game);

        games[game.name] = game;
    }
}


game_t* get_game(const level_index_t& idx)
{
    auto it = games.find(idx.game_name);
    if (it == games.end()) return nullptr;
    return &it->second;
}


meta_t* get_meta(const level_index_t& idx, active_source_t source)
{
    auto game = get_game(idx);
    if (!game) return nullptr;
    if (idx.ep < 0 || idx.ep >= (int)game->episodes.size()) return nullptr;
    if (idx.map < 0 || idx.map >= (int)game->episodes[idx.ep].size()) return nullptr;
    if (source == active_source_t::current ||
        source == active_source_t::target) return &game->episodes[idx.ep][idx.map];
    return nullptr;
}

map_state_t* get_state(const level_index_t& idx, active_source_t source)
{
    auto game = get_game(idx);
    if (!game) return nullptr;
    if (idx.ep < 0 || idx.ep >= (int)game->episodes.size()) return nullptr;
    if (idx.map < 0 || idx.map >= (int)game->episodes[idx.ep].size()) return nullptr;
    if (source == active_source_t::current ||
        source == active_source_t::target) return &game->episodes[idx.ep][idx.map].state;
    return nullptr;
}

map_t* get_map(const level_index_t& idx)
{
    auto game = get_game(idx);
    if (!game) return nullptr;
    if (idx.ep < 0 || idx.ep >= (int)game->episodes.size()) return nullptr;
    if (idx.map < 0 || idx.map >= (int)game->episodes[idx.ep].size()) return nullptr;
    return &game->episodes[idx.ep][idx.map].map;
}

const std::string& get_level_name(const level_index_t& idx)
{
    auto game = get_game(idx);
    static std::string err_str = "ERROR";
    if (!game) return err_str;
    if (idx.ep < 0 || idx.ep >= (int)game->episodes.size()) return nullptr;
    if (idx.map < 0 || idx.map >= (int)game->episodes[idx.ep].size()) return nullptr;
    return game->episodes[idx.ep][idx.map].name;
}
