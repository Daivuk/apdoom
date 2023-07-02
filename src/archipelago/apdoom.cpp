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
// *Source file for interfacing with archipelago*
//

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#ifdef _MSC_VER
#include <direct.h>
#endif
#else
#include <sys/types.h>
#endif


#include "apdoom_def.h"
#include "Archipelago.h"
#include <json/json.h>
#include <memory.h>
#include <chrono>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include <set>


#if defined(_WIN32)
static wchar_t *ConvertMultiByteToWide(const char *str, UINT code_page)
{
    wchar_t *wstr = NULL;
    int wlen = 0;

    wlen = MultiByteToWideChar(code_page, 0, str, -1, NULL, 0);

    if (!wlen)
    {
        errno = EINVAL;
        printf("ConvertMultiByteToWide: Failed to convert path to wide encoding.\n");
        return NULL;
    }

    wstr = (wchar_t*)malloc(sizeof(wchar_t) * wlen);

    if (!wstr)
    {
        printf("ERROR: ConvertMultiByteToWide: Failed to allocate new string.");
        return NULL;
    }

    if (MultiByteToWideChar(code_page, 0, str, -1, wstr, wlen) == 0)
    {
        errno = EINVAL;
        printf("ConvertMultiByteToWide: Failed to convert path to wide encoding.\n");
        free(wstr);
        return NULL;
    }

    return wstr;
}

wchar_t *AP_ConvertUtf8ToWide(const char *str)
{
    return ConvertMultiByteToWide(str, CP_UTF8);
}
#endif



//
// Create a directory
//

void AP_MakeDirectory(const char *path)
{
#ifdef _WIN32
    wchar_t *wdir;

    wdir = AP_ConvertUtf8ToWide(path);

    if (!wdir)
    {
        return;
    }

    _wmkdir(wdir);

    free(wdir);
#else
    mkdir(path, 0755);
#endif
}


FILE *AP_fopen(const char *filename, const char *mode)
{
#ifdef _WIN32
    FILE *file;
    wchar_t *wname = NULL;
    wchar_t *wmode = NULL;

    wname = AP_ConvertUtf8ToWide(filename);

    if (!wname)
    {
        return NULL;
    }

    wmode = AP_ConvertUtf8ToWide(mode);

    if (!wmode)
    {
        free(wname);
        return NULL;
    }

    file = _wfopen(wname, wmode);

    free(wname);
    free(wmode);

    return file;
#else
    return fopen(filename, mode);
#endif
}


int AP_FileExists(const char *filename)
{
    FILE *fstream;

    fstream = AP_fopen(filename, "r");

    if (fstream != NULL)
    {
        fclose(fstream);
        return true;
    }
    else
    {
        // If we can't open because the file is a directory, the 
        // "file" exists at least!

        return errno == EISDIR;
    }
}


ap_state_t ap_state;
int ap_is_in_game = 0;

static ap_settings_t ap_settings;
static AP_RoomInfo ap_room_info;
static std::vector<int64_t> ap_item_queue; // We queue when we're in the menu.
static bool ap_was_connected = false; // Got connected at least once. That means the state is valid
static std::set<int64_t> ap_progressive_locations;
static bool ap_initialized = false;
static std::vector<std::string> ap_cached_messages;
static std::string ap_save_dir_name;


void f_itemclr();
void f_itemrecv(int64_t item_id, bool notify_player);
void f_locrecv(int64_t loc_id);
void f_locprog(int64_t loc_id);
void f_difficulty(int);
void f_random_monsters(int);
void f_random_items(int);
void f_episode1(int);
void f_episode2(int);
void f_episode3(int);
void load_state();
void save_state();
void APSend(std::string msg);


std::string string_to_hex(const char* str)
{
    static const char hex_digits[] = "0123456789ABCDEF";

	std::string out;
	std::string in = str;

    out.reserve(in.length() * 2);
    for (unsigned char c : in)
    {
        out.push_back(hex_digits[c >> 4]);
        out.push_back(hex_digits[c & 15]);
    }

    return out;
}


int apdoom_init(ap_settings_t* settings)
{
	memset(&ap_state, 0, sizeof(ap_state));
	ap_state.player_state.health = 100;
	ap_state.player_state.ready_weapon = 1;
	ap_state.player_state.weapon_owned[0] = 1; // Fist
	ap_state.player_state.weapon_owned[1] = 1; // Pistol
	ap_state.player_state.ammo[0] = 50; // Clip
	ap_state.player_state.max_ammo[0] = 200;
	ap_state.player_state.max_ammo[1] = 50;
	ap_state.player_state.max_ammo[2] = 300;
	ap_state.player_state.max_ammo[3] = 50;
	for (int ep = 0; ep < AP_EPISODE_COUNT; ++ep)
		for (int map = 0; map < AP_LEVEL_COUNT; ++map)
			for (int k = 0; k < AP_CHECK_MAX; ++k)
				ap_state.level_states[ep][map].checks[k] = -1;

	ap_settings = *settings;

	AP_NetworkVersion version = {0, 4, 0};
	AP_SetClientVersion(&version);
    AP_Init(ap_settings.ip, ap_settings.game, ap_settings.player_name, ap_settings.passwd);
	AP_SetDeathLinkSupported(true);
	AP_SetItemClearCallback(f_itemclr);
	AP_SetItemRecvCallback(f_itemrecv);
	AP_SetLocationCheckedCallback(f_locrecv);
	AP_SetLocationIsProgressionCallback(f_locprog);
	AP_RegisterSlotDataIntCallback("difficulty", f_difficulty);
	AP_RegisterSlotDataIntCallback("random_monsters", f_random_monsters);
	AP_RegisterSlotDataIntCallback("random_pickups", f_random_items);
	AP_RegisterSlotDataIntCallback("episode1", f_episode1);
	AP_RegisterSlotDataIntCallback("episode2", f_episode2);
	AP_RegisterSlotDataIntCallback("episode3", f_episode3);
    AP_Start();

	// Block DOOM until connection succeeded or failed
	auto start_time = std::chrono::steady_clock::now();
	while (true)
	{
		bool should_break = false;
		switch (AP_GetConnectionStatus())
		{
			case AP_ConnectionStatus::Authenticated:
			{
				printf("AP: Authenticated\n");
				AP_GetRoomInfo(&ap_room_info);

				ap_was_connected = true;

				ap_save_dir_name = "AP_" + ap_room_info.seed_name + "_" + string_to_hex(ap_settings.player_name);

				// Create a directory where saves will go for this AP seed.
				if (!AP_FileExists(ap_save_dir_name.c_str()))
					AP_MakeDirectory(ap_save_dir_name.c_str());

				load_state();

				// If we don't have information about scouts, load them (We need this to display the proper icon for progressive vs non-progressive
				if (ap_progressive_locations.empty())
				{
					Json::Value packet;
					packet[0]["cmd"] = "LocationScouts";

					// const std::map<int /* ep */, std::map<int /* map */, std::map<int /* index */, int64_t /* loc id */>>> location_table = {
					for (const auto& kv1 : location_table)
					{
						if (!ap_state.episodes[kv1.first - 1])
							continue;
						for (const auto& kv2 : kv1.second)
						{
							for (const auto& kv3 : kv2.second)
							{
								if (kv3.first == -1) continue;
								packet[0]["locations"].append(kv3.second);
							}
						}
					}

					Json::FastWriter writer;
					APSend(writer.write(packet));
				}
				should_break = true;
				break;
			}
			case AP_ConnectionStatus::ConnectionRefused:
				printf("AP: Failed to connect\n");
				return 0;
		}
		if (should_break) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (std::chrono::steady_clock::now() - start_time > std::chrono::seconds(10))
		{
			printf("AP: Failed to connect\n");
			return 0;
		}
	}

	// Wait for location infos
	start_time = std::chrono::steady_clock::now();
	while (ap_progressive_locations.empty())
	{
		apdoom_update();
		
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (std::chrono::steady_clock::now() - start_time > std::chrono::seconds(10))
		{
			printf("AP: Failed to connect\n");
			return 0;
		}
	}

	ap_initialized = true;

	int ep_count = 0;
	for (int i = 0; i < AP_EPISODE_COUNT; ++i)
		if (ap_state.episodes[i])
			ep_count++;
	if (!ep_count)
		ap_state.episodes[0] = 1;

	return 1;
}


void apdoom_shutdown()
{
	if (ap_was_connected)
		save_state();
}


void apdoom_save_state()
{
	if (ap_was_connected)
		save_state();
}


static void json_get_int(const Json::Value& json, int& out_or_default)
{
	if (json.isInt())
		out_or_default = json.asInt();
}


void load_state()
{
	std::string filename = ap_save_dir_name + "/apstate.json";
	std::ifstream f(filename);
	if (!f.is_open())
		return; // Could be no state yet, that's fine
	Json::Value json;
	f >> json;
	f.close();

	// Player state
	json_get_int(json["player"]["health"], ap_state.player_state.health);
	json_get_int(json["player"]["armor_points"], ap_state.player_state.armor_points);
	json_get_int(json["player"]["armor_type"], ap_state.player_state.armor_type);
	json_get_int(json["player"]["backpack"], ap_state.player_state.backpack);
	json_get_int(json["player"]["ready_weapon"], ap_state.player_state.ready_weapon);
	json_get_int(json["player"]["kill_count"], ap_state.player_state.kill_count);
	json_get_int(json["player"]["item_count"], ap_state.player_state.item_count);
	json_get_int(json["player"]["secret_count"], ap_state.player_state.secret_count);
	for (int i = 0; i < AP_NUM_POWERS; ++i)
		json_get_int(json["player"]["powers"][i], ap_state.player_state.powers[i]);
	for (int i = 0; i < AP_NUM_WEAPONS; ++i)
		json_get_int(json["player"]["weapon_owned"][i], ap_state.player_state.weapon_owned[i]);
	for (int i = 0; i < AP_NUM_AMMO; ++i)
		json_get_int(json["player"]["ammo"][i], ap_state.player_state.ammo[i]);
	for (int i = 0; i < AP_NUM_AMMO; ++i)
		json_get_int(json["player"]["max_ammo"][i], ap_state.player_state.max_ammo[i]);

	if (ap_state.player_state.backpack)
	{
		ap_state.player_state.max_ammo[0] = 200 * 2;
		ap_state.player_state.max_ammo[1] = 50 * 2;
		ap_state.player_state.max_ammo[2] = 300 * 2;
		ap_state.player_state.max_ammo[3] = 50 * 2;
	}


	// Level states
	for (int i = 0; i < AP_EPISODE_COUNT; ++i)
	{
		for (int j = 0; j < AP_LEVEL_COUNT; ++j)
		{
			json_get_int(json["episodes"][i][j]["completed"], ap_state.level_states[i][j].completed);
			json_get_int(json["episodes"][i][j]["keys0"], ap_state.level_states[i][j].keys[0]);
			json_get_int(json["episodes"][i][j]["keys1"], ap_state.level_states[i][j].keys[1]);
			json_get_int(json["episodes"][i][j]["keys2"], ap_state.level_states[i][j].keys[2]);
			json_get_int(json["episodes"][i][j]["check_count"], ap_state.level_states[i][j].check_count);
			json_get_int(json["episodes"][i][j]["has_map"], ap_state.level_states[i][j].has_map);
			json_get_int(json["episodes"][i][j]["unlocked"], ap_state.level_states[i][j].unlocked);

			int k = 0;
			for (const auto& json_check : json["episodes"][i][j]["checks"])
			{
				json_get_int(json_check, ap_state.level_states[i][j].checks[k++]);
			}
		}
	}

	// Item queue
	for (const auto& item_id_json : json["item_queue"])
	{
		ap_item_queue.push_back(item_id_json.asInt64());
	}

	json_get_int(json["ep"], ap_state.ep);
	json_get_int(json["map"], ap_state.map);
	int i = 0;
	for (auto& ep : ap_state.episodes)
		json_get_int(json["enabled_episodes"][i++], ep);

	for (const auto& prog_json : json["progressive_locations"])
	{
		ap_progressive_locations.insert(prog_json.asInt64());
	}
	
	json_get_int(json["victory"], ap_state.victory);
}


void save_state()
{
	std::string filename = ap_save_dir_name + "/apstate.json";
	std::ofstream f(filename);
	if (!f.is_open())
	{
		printf("Failed to save AP state.\n");
		return; // Ok that's bad. we won't save player state
	}

	// Player state
	Json::Value json;
	Json::Value json_player;
	json_player["health"] = ap_state.player_state.health;
	json_player["armor_points"] = ap_state.player_state.armor_points;
	json_player["armor_type"] = ap_state.player_state.armor_type;
	json_player["backpack"] = ap_state.player_state.backpack;
	json_player["ready_weapon"] = ap_state.player_state.ready_weapon;
	json_player["kill_count"] = ap_state.player_state.kill_count;
	json_player["item_count"] = ap_state.player_state.item_count;
	json_player["secret_count"] = ap_state.player_state.secret_count;

	Json::Value json_powers(Json::arrayValue);
	for (int i = 0; i < AP_NUM_POWERS; ++i)
		json_powers.append(ap_state.player_state.powers[i]);
	json_player["powers"] = json_powers;

	Json::Value json_weapon_owned(Json::arrayValue);
	for (int i = 0; i < AP_NUM_WEAPONS; ++i)
		json_weapon_owned.append(ap_state.player_state.weapon_owned[i]);
	json_player["weapon_owned"] = json_weapon_owned;

	Json::Value json_ammo(Json::arrayValue);
	for (int i = 0; i < AP_NUM_AMMO; ++i)
		json_ammo.append(ap_state.player_state.ammo[i]);
	json_player["ammo"] = json_ammo;

	Json::Value json_max_ammo(Json::arrayValue);
	for (int i = 0; i < AP_NUM_AMMO; ++i)
		json_max_ammo.append(ap_state.player_state.max_ammo[i]);
	json_player["max_ammo"] = json_max_ammo;

	json["player"] = json_player;

	// Level states
	Json::Value json_episodes(Json::arrayValue);
	for (int i = 0; i < AP_EPISODE_COUNT; ++i)
	{
		Json::Value json_levels(Json::arrayValue);
		for (int j = 0; j < AP_LEVEL_COUNT; ++j)
		{
			Json::Value json_level;

			json_level["completed"] = ap_state.level_states[i][j].completed;
			json_level["keys0"] = ap_state.level_states[i][j].keys[0];
			json_level["keys1"] = ap_state.level_states[i][j].keys[1];
			json_level["keys2"] = ap_state.level_states[i][j].keys[2];
			json_level["check_count"] = ap_state.level_states[i][j].check_count;
			json_level["has_map"] = ap_state.level_states[i][j].has_map;
			json_level["unlocked"] = ap_state.level_states[i][j].unlocked;

			Json::Value json_checks(Json::arrayValue);
			for (int k = 0; k < AP_CHECK_MAX; ++k)
			{
				if (ap_state.level_states[i][j].checks[k] == -1)
					continue;
				json_checks.append(ap_state.level_states[i][j].checks[k]);
			}
			json_level["checks"] = json_checks;

			json_levels.append(json_level);
		}
		json_episodes.append(json_levels);
	}
	json["episodes"] = json_episodes;

	// Item queue
	Json::Value json_item_queue(Json::arrayValue);
	for (auto item_id : ap_item_queue)
	{
		json_item_queue.append(item_id);
	}
	json["item_queue"] = json_item_queue;

	json["ep"] = ap_state.ep;
	json["map"] = ap_state.map;

	int i = 0;
	for (auto ep : ap_state.episodes)
		json["enabled_episodes"][i++] = ep ? true : false;

	// Progression items (So we don't scout everytime we connect)
	for (auto loc_id : ap_progressive_locations)
	{
		json["progressive_locations"].append(loc_id);
	}

	json["victory"] = ap_state.victory;

	f << json;
}


void f_itemclr()
{
	// Not sure what use this would have here.
}


void f_itemrecv(int64_t item_id, bool notify_player)
{
	auto it = item_doom_type_table.find(item_id);
	if (it == item_doom_type_table.end())
		return; // Skip
	ap_item_t item = it->second;

	switch (item.doom_type)
	{
		// Is it a key?
		case 5:
		case 40:
			ap_state.level_states[item.ep - 1][item.map - 1].keys[0] = 1;
			break;
		case 6:
		case 39:
			ap_state.level_states[item.ep - 1][item.map - 1].keys[1] = 1;
			break;
		case 13:
		case 38:
			ap_state.level_states[item.ep - 1][item.map - 1].keys[2] = 1;
			break;

		// Map
		case 2026:
			ap_state.level_states[item.ep - 1][item.map - 1].has_map = 1;
			break;

		// Backpack
		case 8:
			ap_state.player_state.backpack = 1;
			ap_state.player_state.max_ammo[0] = 200 * 2;
			ap_state.player_state.max_ammo[1] = 50 * 2;
			ap_state.player_state.max_ammo[2] = 300 * 2;
			ap_state.player_state.max_ammo[3] = 50 * 2;
            break;

		// Is it a weapon?
        case 2001:
			ap_state.player_state.weapon_owned[2] = 1;
            break;
        case 2002:
			ap_state.player_state.weapon_owned[3] = 1;
            break;
        case 2003:
			ap_state.player_state.weapon_owned[4] = 1;	
            break;
        case 2004:
			ap_state.player_state.weapon_owned[5] = 1;	
            break;
        case 2006:
			ap_state.player_state.weapon_owned[6] = 1;	
            break;
        case 2005:
			ap_state.player_state.weapon_owned[7] = 1;	
            break;
	}

	// Is it a level?
	if (item.doom_type == -1)
	{
		ap_state.level_states[item.ep - 1][item.map - 1].unlocked = 1;
	}


	if (!notify_player) return;

	if (!ap_is_in_game)
	{
		ap_item_queue.push_back(item_id);
		return;
	}

	// Give item to player
	ap_settings.give_item_callback(item.doom_type, item.ep, item.map);
}


void f_locrecv(int64_t loc_id)
{
	// Not much to do here
}


void f_locprog(int64_t loc_id)
{
	ap_progressive_locations.insert(loc_id);
}


void f_difficulty(int difficulty)
{
	ap_state.difficulty = difficulty;
}


void f_random_monsters(int random_monsters)
{
	ap_state.random_monsters = random_monsters;
}


void f_random_items(int random_items)
{
	ap_state.random_items = random_items;
}


void f_episode1(int ep)
{
	ap_state.episodes[0] = ep;
}


void f_episode2(int ep)
{
	ap_state.episodes[1] = ep;
}


void f_episode3(int ep)
{
	ap_state.episodes[2] = ep;
}


const char* apdoom_get_seed()
{
	return ap_save_dir_name.c_str();
}


void apdoom_check_location(int ep, int map, int index)
{
	auto it1 = location_table.find(ep);
	if (it1 == location_table.end()) return;

	auto it2 = it1->second.find(map);
	if (it2 == it1->second.end()) return;

	auto it3 = it2->second.find(index);
	if (it3 == it2->second.end()) return;

	int64_t id = it3->second;

	ap_state.level_states[ep - 1][map - 1].checks[ap_state.level_states[ep - 1][map - 1].check_count] = index;
	ap_state.level_states[ep - 1][map - 1].check_count++;

	AP_SendItem(id);
}


int apdoom_is_location_progression(int ep, int map, int index)
{
	auto it1 = location_table.find(ep);
	if (it1 == location_table.end()) return 0;

	auto it2 = it1->second.find(map);
	if (it2 == it1->second.end()) return 0;

	auto it3 = it2->second.find(index);
	if (it3 == it2->second.end()) return 0;

	int64_t id = it3->second;

	return (ap_progressive_locations.find(id) != ap_progressive_locations.end()) ? 1 : 0;
}


void apdoom_check_victory()
{
	if (ap_state.victory) return;

	for (int ep = 0; ep < AP_EPISODE_COUNT; ++ep)
	{
		if (!ap_state.episodes[ep]) continue;
		for (int map = 0; map < AP_LEVEL_COUNT; ++map)
		{
			if (!ap_state.level_states[ep][map].completed) return;
		}
	}

	AP_StoryComplete();
	ap_settings.victory_callback();
}


void apdoom_send_message(const char* msg)
{
	Json::Value say_packet;
	say_packet[0]["cmd"] = "Say";
	say_packet[0]["text"] = msg;
	Json::FastWriter writer;
	APSend(writer.write(say_packet));
}


void apdoom_on_death()
{
	AP_DeathLinkSend();
}


void apdoom_clear_death()
{
	AP_DeathLinkClear();
}


int apdoom_should_die()
{
	return AP_DeathLinkPending() ? 1 : 0;
}


/*
    black: "000000"
    red: "EE0000"
    green: "00FF7F"  # typically a location
    yellow: "FAFAD2"  # typically other slots/players
    blue: "6495ED"  # typically extra info (such as entrance)
    magenta: "EE00EE"  # typically your slot/player
    cyan: "00EEEE"  # typically regular item
    slateblue: "6D8BE8"  # typically useful item
    plum: "AF99EF"  # typically progression item
    salmon: "FA8072"  # typically trap item
    white: "FFFFFF"  # not used, if you want to change the generic text color change color in Label

    (byte *) &cr_none, // 0 (RED)
    (byte *) &cr_dark, // 1 (DARK RED)
    (byte *) &cr_gray, // 2 (WHITE) normal text
    (byte *) &cr_green, // 3 (GREEN) location
    (byte *) &cr_gold, // 4 (YELLOW) player
    (byte *) &cr_red, // 5 (RED, same as cr_none)
    (byte *) &cr_blue, // 6 (BLUE) extra info such as Entrance
    (byte *) &cr_red2blue, // 7 (BLUE) items
    (byte *) &cr_red2green // 8 (DARK EDGE GREEN)
*/
void apdoom_update()
{
	if (ap_initialized)
	{
		if (!ap_cached_messages.empty())
		{
			for (const auto& cached_msg : ap_cached_messages)
				ap_settings.message_callback(cached_msg.c_str());
			ap_cached_messages.clear();
		}
	}

	while (AP_IsMessagePending())
	{
		AP_Message* msg = AP_GetLatestMessage();

		std::string colored_msg;

		if (msg->messageParts.empty())
		{
			colored_msg = "~2" + msg->text;
		}
		else
		{
			for (const auto& message_part : msg->messageParts)
			{
				switch (message_part.type)
				{
					case AP_NormalText:
						colored_msg += "~2" + message_part.text;
						break;
					case AP_LocationText:
						colored_msg += "~3" + message_part.text;
						break;
					case AP_ItemText:
						colored_msg += "~9" + message_part.text;
						break;
					case AP_PlayerText:
						colored_msg += "~4" + message_part.text;
						break;
				}
			}
		}

		printf("AP: %s\n", msg->text.c_str());

		if (ap_initialized)
			ap_settings.message_callback(colored_msg.c_str());
		else
			ap_cached_messages.push_back(colored_msg);

		//switch (msg->type)
		//{
		//	case AP_MessageType::Plaintext:
		//	{
		//		break;
		//	}
		//	case AP_MessageType::ItemSend:
		//	{
		//		AP_ItemSendMessage* item_send_msg = (AP_ItemSendMessage*)msg;
		//		break;
		//	}
		//	case AP_MessageType::ItemRecv:
		//	{
		//		AP_ItemRecvMessage* item_recv_msg = (AP_ItemRecvMessage*)msg;
		//		break;
		//	}
		//	case AP_MessageType::Hint:
		//	{
		//		AP_HintMessage* hint_msg = (AP_HintMessage*)msg;
		//		break;
		//	}
		//	case AP_MessageType::Countdown:
		//	{
		//		AP_CountdownMessage* countdown_msg = (AP_CountdownMessage*)msg;
		//		break;
		//	}
		//}

		AP_ClearLatestMessage();
	}

	// Check if we're in game, then dequeue the items
	if (ap_is_in_game)
	{
		while (!ap_item_queue.empty())
		{
			auto item_id = ap_item_queue.front();
			ap_item_queue.erase(ap_item_queue.begin());
			f_itemrecv(item_id, true);
		}
	}
}
