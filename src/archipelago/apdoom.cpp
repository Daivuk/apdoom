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
#include "apdoom2_def.h"
#include "apheretic_def.h"
#include "apdoom_open_world_def.h"
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
        printf("APDOOM: ConvertMultiByteToWide: Failed to convert path to wide encoding.\n");
        return NULL;
    }

    wstr = (wchar_t*)malloc(sizeof(wchar_t) * wlen);

    if (!wstr)
    {
        printf("APDOOM: ConvertMultiByteToWide: Failed to allocate new string.");
        return NULL;
    }

    if (MultiByteToWideChar(code_page, 0, str, -1, wstr, wlen) == 0)
    {
        errno = EINVAL;
        printf("APDOOM: ConvertMultiByteToWide: Failed to convert path to wide encoding.\n");
        free(wstr);
        return NULL;
    }

    return wstr;
}

static wchar_t *AP_ConvertUtf8ToWide(const char *str)
{
    return ConvertMultiByteToWide(str, CP_UTF8);
}
#endif



//
// Create a directory
//

static void AP_MakeDirectory(const char *path)
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


static FILE *AP_fopen(const char *filename, const char *mode)
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


static int AP_FileExists(const char *filename)
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


enum class ap_game_t
{
	doom,
	doom2,
	heretic,
	doom_open_world
};


ap_state_t ap_state;
int ap_is_in_game = 0;
int ap_episode_count = -1;

static ap_game_t ap_game;
static int ap_weapon_count = -1;
static int ap_ammo_count = -1;
static int ap_powerup_count = -1;
static int ap_inventory_count = -1;
static int max_map_count = -1;
static ap_settings_t ap_settings;
static AP_RoomInfo ap_room_info;
static std::vector<int64_t> ap_item_queue; // We queue when we're in the menu.
static bool ap_was_connected = false; // Got connected at least once. That means the state is valid
static std::set<int64_t> ap_progressive_locations;
static bool ap_initialized = false;
static std::vector<std::string> ap_cached_messages;
static std::string ap_save_dir_name;
static std::vector<ap_notification_icon_t> ap_notification_icons;
static bool ap_check_sanity = false;


void f_itemclr();
void f_itemrecv(int64_t item_id, int player_id, bool notify_player);
void f_locrecv(int64_t loc_id);
void f_locinfo(std::vector<AP_NetworkItem> loc_infos);
void f_goal(int);
void f_difficulty(int);
void f_random_monsters(int);
void f_random_items(int);
void f_random_music(int);
void f_flip_levels(int);
void f_check_sanity(int);
void f_reset_level_on_death(int);
void f_episode1(int);
void f_episode2(int);
void f_episode3(int);
void f_episode4(int);
void f_episode5(int);
void f_two_ways_keydoors(int);
void load_state();
void save_state();
void APSend(std::string msg);


static int get_original_music_for_level(int ep, int map)
{
	switch (ap_game)
	{
		case ap_game_t::doom:
		case ap_game_t::doom_open_world:
		{
			int ep4_music[] = {
				// Song - Who? - Where?

				2 * 9 + 3 + 1, //mus_e3m4,        // American     e4m1
				2 * 9 + 1 + 1, //mus_e3m2,        // Romero       e4m2
				2 * 9 + 2 + 1, //mus_e3m3,        // Shawn        e4m3
				0 * 9 + 4 + 1, //mus_e1m5,        // American     e4m4
				1 * 9 + 6 + 1, //mus_e2m7,        // Tim          e4m5
				1 * 9 + 3 + 1, //mus_e2m4,        // Romero       e4m6
				1 * 9 + 5 + 1, //mus_e2m6,        // J.Anderson   e4m7 CHIRON.WAD
				1 * 9 + 4 + 1, //mus_e2m5,        // Shawn        e4m8
				0 * 9 + 8 + 1  //mus_e1m9,        // Tim          e4m9
			};

			if (ep == 4) return ep4_music[map - 1];
			return 1 + (ep - 1) * ap_get_map_count(ep) + (map - 1);
		}
		case ap_game_t::doom2:
		{
			return 52 + ap_index_to_map({ep - 1, map - 1}) - 1;
		}
		case ap_game_t::heretic:
		{
			return (ep - 1) * ap_get_map_count(ep) + (map - 1);
		}
	}

	// For now for doom and heretic
	return -1;
}


static std::vector<std::vector<ap_level_info_t>>& get_level_info_table()
{
	switch (ap_game)
	{
		case ap_game_t::doom: return ap_doom_level_infos;
		case ap_game_t::doom2: return ap_doom2_level_infos;
		case ap_game_t::heretic: return ap_heretic_level_infos;
		case ap_game_t::doom_open_world: return ap_doom_open_world_level_infos;
	}
}


int ap_get_map_count(int ep)
{
	--ep;
	auto& level_info_table = get_level_info_table();
	if (ep < 0 || ep >= (int)level_info_table.size()) return -1;
	return (int)level_info_table[ep].size();
}


ap_level_info_t* ap_get_level_info(ap_level_index_t idx)
{
	auto& level_info_table = get_level_info_table();
	if (idx.ep < 0 || idx.ep >= (int)level_info_table.size()) return nullptr;
	if (idx.map < 0 || idx.map >= (int)level_info_table[idx.ep].size()) return nullptr;
	return &level_info_table[idx.ep][idx.map];
}


ap_level_state_t* ap_get_level_state(ap_level_index_t idx)
{
	return &ap_state.level_states[idx.ep * max_map_count + idx.map];
}


static const std::map<int64_t, ap_item_t>& get_item_type_table()
{
	switch (ap_game)
	{
		case ap_game_t::doom: return ap_doom_item_table;
		case ap_game_t::doom2: return ap_doom2_item_table;
		case ap_game_t::heretic: return ap_heretic_item_table;
		case ap_game_t::doom_open_world: return ap_doom_open_world_item_table;
	}
}


static const std::map<int /* ep */, std::map<int /* map */, std::map<int /* index */, int64_t /* loc id */>>>& get_location_table()
{
	switch (ap_game)
	{
		case ap_game_t::doom: return ap_doom_location_table;
		case ap_game_t::doom2: return ap_doom2_location_table;
		case ap_game_t::heretic: return ap_heretic_location_table;
		case ap_game_t::doom_open_world: return ap_doom_open_world_location_table;
	}
}


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


static const int doom_max_ammos[] = {200, 50, 300, 50};
static const int doom2_max_ammos[] = {200, 50, 300, 50};
static const int heretic_max_ammos[] = {100, 50, 200, 200, 20, 150};


static unsigned long long hash_seed(const char *str)
{
    unsigned long long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}


const int* get_max_ammos()
{
	switch (ap_game)
	{
		case ap_game_t::doom: return doom_max_ammos;
		case ap_game_t::doom2: return doom2_max_ammos;
		case ap_game_t::heretic: return heretic_max_ammos;
		case ap_game_t::doom_open_world: return doom_max_ammos;
	}
}


int validate_doom_location(ap_level_index_t idx, int index)
{
    ap_level_info_t* level_info = ap_get_level_info(idx);
    if (index >= level_info->thing_count) return 0;
	if (level_info->thing_infos[index].unreachable) return 0;
    return level_info->thing_infos[index].check_sanity == 0 || ap_state.check_sanity == 1;
}


int apdoom_init(ap_settings_t* settings)
{
	printf("%s\n", APDOOM_VERSION_FULL_TEXT);

	ap_notification_icons.reserve(4096); // 1MB. A bit exessive, but I got a crash with invalid strings and I cannot figure out why. Let's not take any chances...
	memset(&ap_state, 0, sizeof(ap_state));

	if (strcmp(settings->game, "DOOM 1993") == 0)
	{
		ap_game = ap_game_t::doom;
		ap_weapon_count = 9;
		ap_ammo_count = 4;
		ap_powerup_count = 6;
		ap_inventory_count = 0;
	}
	else if (strcmp(settings->game, "DOOM II") == 0)
	{
		ap_game = ap_game_t::doom2;
		ap_weapon_count = 9;
		ap_ammo_count = 4;
		ap_powerup_count = 6;
		ap_inventory_count = 0;
	}
	else if (strcmp(settings->game, "Heretic") == 0)
	{
		ap_game = ap_game_t::heretic;
		ap_weapon_count = 9;
		ap_ammo_count = 6;
		ap_powerup_count = 9;
		ap_inventory_count = 14;
	}
	else if (strcmp(settings->game, "DOOM Open World") == 0)
	{
		ap_game = ap_game_t::doom_open_world;
		ap_weapon_count = 9;
		ap_ammo_count = 4;
		ap_powerup_count = 6;
		ap_inventory_count = 0;
	}
	else
	{
		printf("APDOOM: Invalid game: %s\n", settings->game);
		return 0;
	}

	const auto& level_info_table = get_level_info_table();
	ap_episode_count = (int)level_info_table.size();
	max_map_count = 0; // That's really the map count
	for (const auto& episode_level_info : level_info_table)
	{
		max_map_count = max(max_map_count, (int)episode_level_info.size());
	}

	printf("APDOOM: Initializing Game: \"%s\", Server: %s, Slot: %s\n", settings->game, settings->ip, settings->player_name);

	ap_state.level_states = new ap_level_state_t[ap_episode_count * max_map_count];
	ap_state.episodes = new int[ap_episode_count];
	ap_state.player_state.powers = new int[ap_powerup_count];
	ap_state.player_state.weapon_owned = new int[ap_weapon_count];
	ap_state.player_state.ammo = new int[ap_ammo_count];
	ap_state.player_state.max_ammo = new int[ap_ammo_count];
	ap_state.player_state.inventory = ap_inventory_count ? new ap_inventory_slot_t[ap_inventory_count] : nullptr;

	memset(ap_state.level_states, 0, sizeof(ap_level_state_t) * ap_episode_count * max_map_count);
	memset(ap_state.episodes, 0, sizeof(int) * ap_episode_count);
	memset(ap_state.player_state.powers, 0, sizeof(int) * ap_powerup_count);
	memset(ap_state.player_state.weapon_owned, 0, sizeof(int) * ap_weapon_count);
	memset(ap_state.player_state.ammo, 0, sizeof(int) * ap_ammo_count);
	memset(ap_state.player_state.max_ammo, 0, sizeof(int) * ap_ammo_count);
	if (ap_inventory_count)
		memset(ap_state.player_state.inventory, 0, sizeof(ap_inventory_slot_t) * ap_inventory_count);

	ap_state.player_state.health = 100;
	ap_state.player_state.ready_weapon = 1;
	ap_state.player_state.weapon_owned[0] = 1; // Fist
	ap_state.player_state.weapon_owned[1] = 1; // Pistol
	ap_state.player_state.ammo[0] = 50; // Clip
	auto max_ammos = get_max_ammos();
	for (int i = 0; i < ap_ammo_count; ++i)
		ap_state.player_state.max_ammo[i] = max_ammos[i];
	for (int ep = 0; ep < ap_episode_count; ++ep)
	{
		int map_count = ap_get_map_count(ep + 1);
		for (int map = 0; map < map_count; ++map)
		{
			for (int k = 0; k < AP_CHECK_MAX; ++k)
			{
				ap_state.level_states[ep * max_map_count + map].checks[k] = -1;
			}
			auto level_info = ap_get_level_info(ap_level_index_t{ep, map});
			level_info->sanity_check_count = 0;
			for (int k = 0; k < level_info->thing_count; ++k)
			{
				if (level_info->thing_infos[k].check_sanity)
					level_info->sanity_check_count++;
			}
		}
	}

	ap_settings = *settings;

	if (ap_settings.override_skill)
		ap_state.difficulty = ap_settings.skill;
	if (ap_settings.override_monster_rando)
		ap_state.random_monsters = ap_settings.monster_rando;
	if (ap_settings.override_item_rando)
		ap_state.random_items = ap_settings.item_rando;
	if (ap_settings.override_music_rando)
		ap_state.random_music = ap_settings.music_rando;
	if (ap_settings.override_flip_levels)
		ap_state.flip_levels = ap_settings.flip_levels;
	if (ap_settings.override_reset_level_on_death)
		ap_state.reset_level_on_death = ap_settings.reset_level_on_death;

	AP_NetworkVersion version = {0, 4, 1};
	AP_SetClientVersion(&version);
    AP_Init(ap_settings.ip, ap_settings.game, ap_settings.player_name, ap_settings.passwd);
	AP_SetDeathLinkSupported(ap_settings.force_deathlink_off ? false : true);
	AP_SetItemClearCallback(f_itemclr);
	AP_SetItemRecvCallback(f_itemrecv);
	AP_SetLocationCheckedCallback(f_locrecv);
	AP_SetLocationInfoCallback(f_locinfo);
	AP_RegisterSlotDataIntCallback("goal", f_goal);
	AP_RegisterSlotDataIntCallback("difficulty", f_difficulty);
	AP_RegisterSlotDataIntCallback("random_monsters", f_random_monsters);
	AP_RegisterSlotDataIntCallback("random_pickups", f_random_items);
	AP_RegisterSlotDataIntCallback("random_music", f_random_music);
	AP_RegisterSlotDataIntCallback("flip_levels", f_flip_levels);
	AP_RegisterSlotDataIntCallback("check_sanity", f_check_sanity);
	AP_RegisterSlotDataIntCallback("reset_level_on_death", f_reset_level_on_death);
	AP_RegisterSlotDataIntCallback("episode1", f_episode1);
	AP_RegisterSlotDataIntCallback("episode2", f_episode2);
	AP_RegisterSlotDataIntCallback("episode3", f_episode3);
	AP_RegisterSlotDataIntCallback("episode4", f_episode4);
	AP_RegisterSlotDataIntCallback("episode5", f_episode5);
	AP_RegisterSlotDataIntCallback("two_ways_keydoors", f_two_ways_keydoors);
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
				printf("APDOOM: Authenticated\n");
				AP_GetRoomInfo(&ap_room_info);

				printf("APDOOM: Room Info:\n");
				printf("  Network Version: %i.%i.%i\n", ap_room_info.version.major, ap_room_info.version.minor, ap_room_info.version.build);
				printf("  Tags:\n");
				for (const auto& tag : ap_room_info.tags)
					printf("    %s\n", tag.c_str());
				printf("  Password required: %s\n", ap_room_info.password_required ? "true" : "false");
				printf("  Permissions:\n");
				for (const auto& permission : ap_room_info.permissions)
					printf("    %s = %i:\n", permission.first.c_str(), permission.second);
				printf("  Hint cost: %i\n", ap_room_info.hint_cost);
				printf("  Location check points: %i\n", ap_room_info.location_check_points);
				printf("  Data package checksums:\n");
				for (const auto& kv : ap_room_info.datapackage_checksums)
					printf("    %s = %s:\n", kv.first.c_str(), kv.second.c_str());
				printf("  Seed name: %s\n", ap_room_info.seed_name.c_str());
				printf("  Time: %f\n", ap_room_info.time);
				
				ap_was_connected = true;
				ap_save_dir_name = "AP_" + ap_room_info.seed_name + "_" + string_to_hex(ap_settings.player_name);

				// Create a directory where saves will go for this AP seed.
				printf("APDOOM: Save directory: %s\n", ap_save_dir_name.c_str());
				if (!AP_FileExists(ap_save_dir_name.c_str()))
				{
					printf("  Doesn't exist, creating...\n");
					AP_MakeDirectory(ap_save_dir_name.c_str());
				}

				load_state();
				should_break = true;
				break;
			}
			case AP_ConnectionStatus::ConnectionRefused:
				printf("APDOOM: Failed to connect, connection refused\n");
				return 0;
		}
		if (should_break) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (std::chrono::steady_clock::now() - start_time > std::chrono::seconds(10))
		{
			printf("APDOOM: Failed to connect, timeout 10s\n");
			return 0;
		}
	}

	// If none episode is selected, select the first one.
	int ep_count = 0;
	for (int i = 0; i < ap_episode_count; ++i)
		if (ap_state.episodes[i])
			ep_count++;
	if (!ep_count)
	{
		printf("APDOOM: No episode selected, selecting episode 1\n");
		ap_state.episodes[0] = 1;
	}

	// Seed for random features
	auto ap_seed = apdoom_get_seed();
	unsigned long long seed = hash_seed(ap_seed);
	srand(seed);

	// Randomly flip levels based on the seed
	if (ap_state.flip_levels == 1)
	{
		printf("APDOOM: All levels flipped\n");
		for (int ep = 0; ep < ap_episode_count; ++ep)
		{
			int map_count = ap_get_map_count(ep + 1);
			for (int map = 0; map < map_count; ++map)
				ap_state.level_states[ep * max_map_count + map].flipped = 1;
		}
	}
	else if (ap_state.flip_levels == 2)
	{
		printf("APDOOM: Levels randomly flipped\n");
		for (int ep = 0; ep < ap_episode_count; ++ep)
		{
			int map_count = ap_get_map_count(ep + 1);
			for (int map = 0; map < map_count; ++map)
				ap_state.level_states[ep * max_map_count + map].flipped = rand() % 2;
		}
	}

	// Map original music to every level to start
	for (int ep = 0; ep < ap_episode_count; ++ep)
	{
		int map_count = ap_get_map_count(ep + 1);
		for (int map = 0; map < map_count; ++map)
			ap_state.level_states[ep * max_map_count + map].music = get_original_music_for_level(ep + 1, map + 1);
	}

	// Randomly shuffle music 
	if (ap_state.random_music > 0)
	{
		// Collect music for all selected levels
		std::vector<int> music_pool;
		for (int ep = 0; ep < ap_episode_count; ++ep)
		{
			if (ap_state.episodes[ep] || ap_state.random_music == 2)
			{
				int map_count = ap_get_map_count(ep + 1);
				for (int map = 0; map < map_count; ++map)
					music_pool.push_back(ap_state.level_states[ep * max_map_count + map].music);
			}
		}

		// Shuffle
		printf("APDOOM: Random Music:\n");
		for (int ep = 0; ep < ap_episode_count; ++ep)
		{
			if (ap_state.episodes[ep])
			{
				int map_count = ap_get_map_count(ep + 1);
				for (int map = 0; map < map_count; ++map)
				{
					int rnd = rand() % (int)music_pool.size();
					int mus = music_pool[rnd];
					music_pool.erase(music_pool.begin() + rnd);
					ap_state.level_states[ep * max_map_count + map].music = mus;

					switch (ap_game)
					{
						case ap_game_t::doom:
							printf("  E%iM%i = E%iM%i\n", ep + 1, map + 1, ((mus - 1) / max_map_count) + 1, ((mus - 1) % max_map_count) + 1);
							break;
						case ap_game_t::doom2:
							printf("  MAP%02i = MAP%02i\n", map + 1, mus);
							break;
						case ap_game_t::heretic:
							printf("  E%iM%i = E%iM%i\n", ep + 1, map + 1, (mus / max_map_count) + 1, (mus % max_map_count) + 1);
							break;
						case ap_game_t::doom_open_world:
							printf("  E%iM%i = E%iM%i\n", ep + 1, map + 1, ((mus - 1) / max_map_count) + 1, ((mus - 1) % max_map_count) + 1);
							break;
					}
				}
			}
		}
	}

	// Scout locations to see which are progressive
	if (ap_progressive_locations.empty())
	{
		std::vector<int64_t> location_scouts;

		const auto& loc_table = get_location_table();
		for (const auto& kv1 : loc_table)
		{
			if (!ap_state.episodes[kv1.first - 1])
				continue;
			for (const auto& kv2 : kv1.second)
			{
				for (const auto& kv3 : kv2.second)
				{
					if (kv3.first == -1) continue;
					if (kv3.second == 371349)
					{
						int tmp;
						tmp = 5;
					}
					if (validate_doom_location({kv1.first - 1, kv2.first - 1}, kv3.first))
					{
						location_scouts.push_back(kv3.second);
					}
				}
			}
		}
		
		printf("APDOOM: Scouting for %i locations...\n", (int)location_scouts.size());
		AP_SendLocationScouts(location_scouts, 0);

		// Wait for location infos
		start_time = std::chrono::steady_clock::now();
		while (ap_progressive_locations.empty())
		{
			apdoom_update();
		
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if (std::chrono::steady_clock::now() - start_time > std::chrono::seconds(10))
			{
				printf("APDOOM: Timeout waiting for LocationScouts. 10s\n  Do you have a VPN active?\n  Checks will all look non-progression.");
				break;
			}
		}
	}
	else
	{
		printf("APDOOM: Scout locations cached loaded\n");
	}
	
	printf("APDOOM: Initialized\n");
	ap_initialized = true;
	return 1;
}


static bool is_loc_checked(ap_level_index_t idx, int index)
{
	auto level_state = ap_get_level_state(idx);
	for (int i = 0; i < level_state->check_count; ++i)
	{
		if (level_state->checks[i] == index) return true;
	}
	return false;
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


static void json_get_bool_or(const Json::Value& json, int& out_or_default)
{
	if (json.isInt())
		out_or_default |= json.asInt();
}


const char* get_weapon_name(int weapon)
{
	switch (weapon)
	{
		case 0: return "Fist";
		case 1: return "Pistol";
		case 2: return "Shotgun";
		case 3: return "Chaingun";
		case 4: return "Rocket launcher";
		case 5: return "Plasma gun";
		case 6: return "BFG9000";
		case 7: return "Chainsaw";
		case 8: return "Super shotgun";
		default: return "UNKNOWN";
	}
}


const char* get_power_name(int weapon)
{
	switch (weapon)
	{
		case 0: return "Invulnerability";
		case 1: return "Strength";
		case 2: return "Invisibility";
		case 3: return "Hazard suit";
		case 4: return "Computer area map";
		case 5: return "Infrared";
		default: return "UNKNOWN";
	}
}


const char* get_ammo_name(int weapon)
{
	switch (weapon)
	{
		case 0: return "Bullets";
		case 1: return "Shells";
		case 2: return "Cells";
		case 3: return "Rockets";
		default: return "UNKNOWN";
	}
}


void load_state()
{
	printf("APDOOM: Load sate\n");

	std::string filename = ap_save_dir_name + "/apstate.json";
	std::ifstream f(filename);
	if (!f.is_open())
	{
		printf("  None found.\n");
		return; // Could be no state yet, that's fine
	}
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
	for (int i = 0; i < ap_powerup_count; ++i)
		json_get_int(json["player"]["powers"][i], ap_state.player_state.powers[i]);
	for (int i = 0; i < ap_weapon_count; ++i)
		json_get_bool_or(json["player"]["weapon_owned"][i], ap_state.player_state.weapon_owned[i]);
	for (int i = 0; i < ap_ammo_count; ++i)
		json_get_int(json["player"]["ammo"][i], ap_state.player_state.ammo[i]);
	for (int i = 0; i < ap_inventory_count; ++i)
	{
		const auto& inventory_slot = json["player"]["inventory"][i];
		json_get_int(inventory_slot["type"], ap_state.player_state.inventory[i].type);
		json_get_int(inventory_slot["count"], ap_state.player_state.inventory[i].count);
	}
	
	if (ap_state.player_state.backpack)
	{
		auto max_ammos = get_max_ammos();
		for (int i = 0; i < ap_ammo_count; ++i)
			ap_state.player_state.max_ammo[i] = max_ammos[i] * (ap_state.player_state.backpack ? 2 : 1);
	}

	printf("  Player State:\n");
	printf("    Health %i:\n", ap_state.player_state.health);
	printf("    Armor points %i:\n", ap_state.player_state.armor_points);
	printf("    Armor type %i:\n", ap_state.player_state.armor_type);
	printf("    Backpack %s:\n", ap_state.player_state.backpack ? "true" : "false");
	printf("    Ready weapon: %s\n", get_weapon_name(ap_state.player_state.ready_weapon));
	printf("    Kill count %i:\n", ap_state.player_state.kill_count);
	printf("    Item count %i:\n", ap_state.player_state.item_count);
	printf("    Secret count %i:\n", ap_state.player_state.secret_count);
	printf("    Active powerups:\n");
	for (int i = 0; i < ap_powerup_count; ++i)
		if (ap_state.player_state.powers[i])
			printf("    %s\n", get_power_name(i));
	printf("    Owned weapons:\n");
	for (int i = 0; i < ap_weapon_count; ++i)
		if (ap_state.player_state.weapon_owned[i])
			printf("      %s\n", get_weapon_name(i));
	printf("    Ammo:\n");
	for (int i = 0; i < ap_ammo_count; ++i)
		printf("      %s = %i\n", get_ammo_name(i), ap_state.player_state.ammo[i]);

	// Level states
	for (int i = 0; i < ap_episode_count; ++i)
	{
		int map_count = ap_get_map_count(i + 1);
		for (int j = 0; j < map_count; ++j)
		{
			auto level_state = ap_get_level_state(ap_level_index_t{i, j});
			json_get_bool_or(json["episodes"][i][j]["completed"], level_state->completed);
			json_get_bool_or(json["episodes"][i][j]["keys0"], level_state->keys[0]);
			json_get_bool_or(json["episodes"][i][j]["keys1"], level_state->keys[1]);
			json_get_bool_or(json["episodes"][i][j]["keys2"], level_state->keys[2]);
			//json_get_bool_or(json["episodes"][i][j]["check_count"], level_state->check_count);
			json_get_bool_or(json["episodes"][i][j]["has_map"], level_state->has_map);
			json_get_bool_or(json["episodes"][i][j]["unlocked"], level_state->unlocked);
			json_get_bool_or(json["episodes"][i][j]["special"], level_state->special);

			//int k = 0;
			//for (const auto& json_check : json["episodes"][i][j]["checks"])
			//{
			//	json_get_bool_or(json_check, level_state->checks[k++]);
			//}
		}
	}

	// Item queue
	for (const auto& item_id_json : json["item_queue"])
	{
		ap_item_queue.push_back(item_id_json.asInt64());
	}

	json_get_int(json["ep"], ap_state.ep);
	printf("  Enabled episodes: ");
	int first = 1;
	for (int i = 0; i < ap_episode_count; ++i)
	{
		json_get_int(json["enabled_episodes"][i++], ap_state.episodes[i]);
		if (ap_state.episodes[i])
		{
			if (!first) printf(", ");
			first = 0;
			printf("%i", i);
		}
	}
	printf("\n");
	json_get_int(json["map"], ap_state.map);
	printf("  Episode: %i\n", ap_state.ep);
	printf("  Map: %i\n", ap_state.map);

	for (const auto& prog_json : json["progressive_locations"])
	{
		ap_progressive_locations.insert(prog_json.asInt64());
	}
	
	json_get_bool_or(json["victory"], ap_state.victory);
	printf("  Victory state: %s\n", ap_state.victory ? "true" : "false");
}


static Json::Value serialize_level(int ep, int map)
{
	auto level_state = ap_get_level_state(ap_level_index_t{ep - 1, map - 1});

	Json::Value json_level;

	json_level["completed"] = level_state->completed;
	json_level["keys0"] = level_state->keys[0];
	json_level["keys1"] = level_state->keys[1];
	json_level["keys2"] = level_state->keys[2];
	json_level["check_count"] = level_state->check_count;
	json_level["has_map"] = level_state->has_map;
	json_level["unlocked"] = level_state->unlocked;
	json_level["special"] = level_state->special;

	Json::Value json_checks(Json::arrayValue);
	for (int k = 0; k < AP_CHECK_MAX; ++k)
	{
		if (level_state->checks[k] == -1)
			continue;
		json_checks.append(level_state->checks[k]);
	}
	json_level["checks"] = json_checks;

	return json_level;
}


std::vector<ap_level_index_t> get_level_indices()
{
	std::vector<ap_level_index_t> ret;

	for (int i = 0; i < ap_episode_count; ++i)
	{
		int map_count = ap_get_map_count(i + 1);
		for (int j = 0; j < map_count; ++j)
		{
			ret.push_back({i + 1, j + 1});
		}
	}

	return ret;
}


void save_state()
{
	std::string filename = ap_save_dir_name + "/apstate.json";
	std::ofstream f(filename);
	if (!f.is_open())
	{
		printf("Failed to save AP state.\n");
#if WIN32
		MessageBoxA(nullptr, "Failed to save player state. That's bad.", "Error", MB_OK);
#endif
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
	for (int i = 0; i < ap_powerup_count; ++i)
		json_powers.append(ap_state.player_state.powers[i]);
	json_player["powers"] = json_powers;

	Json::Value json_weapon_owned(Json::arrayValue);
	for (int i = 0; i < ap_weapon_count; ++i)
		json_weapon_owned.append(ap_state.player_state.weapon_owned[i]);
	json_player["weapon_owned"] = json_weapon_owned;

	Json::Value json_ammo(Json::arrayValue);
	for (int i = 0; i < ap_ammo_count; ++i)
		json_ammo.append(ap_state.player_state.ammo[i]);
	json_player["ammo"] = json_ammo;

	Json::Value json_inventory(Json::arrayValue);
	for (int i = 0; i < ap_inventory_count; ++i)
	{
		if (ap_state.player_state.inventory[i].type == 9) // Don't include wings to player inventory, they are per level
			continue;
		Json::Value json_inventory_slot;
		json_inventory_slot["type"] = ap_state.player_state.inventory[i].type;
		json_inventory_slot["count"] = ap_state.player_state.inventory[i].count;
		json_inventory.append(json_inventory_slot);
	}
	json_player["inventory"] = json_inventory;

	json["player"] = json_player;

	// Level states
	Json::Value json_episodes(Json::arrayValue);
	for (int i = 0; i < ap_episode_count; ++i)
	{
		Json::Value json_levels(Json::arrayValue);
		int map_count = ap_get_map_count(i + 1);
		for (int j = 0; j < map_count; ++j)
		{
			json_levels.append(serialize_level(i + 1, j + 1));
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
	for (int i = 0; i < ap_episode_count; ++i)
		json["enabled_episodes"][i++] = ap_state.episodes[i] ? true : false;
	json["map"] = ap_state.map;

	// Progression items (So we don't scout everytime we connect)
	for (auto loc_id : ap_progressive_locations)
	{
		json["progressive_locations"].append(loc_id);
	}

	json["victory"] = ap_state.victory;

	json["version"] = APDOOM_VERSION_FULL_TEXT;

	f << json;
}


void f_itemclr()
{
	// Not sure what use this would have here.
}


static const std::map<int, int> doom_keys_map = {{5, 0}, {40, 0}, {6, 1}, {39, 1}, {13, 2}, {38, 2}};
static const std::map<int, int> doom2_keys_map = {{5, 0}, {40, 0}, {6, 1}, {39, 1}, {13, 2}, {38, 2}};
static const std::map<int, int> heretic_keys_map = {{80, 0}, {73, 1}, {79, 2}};


const std::map<int, int>& get_keys_map()
{
	switch (ap_game)
	{
		case ap_game_t::doom: return doom_keys_map;
		case ap_game_t::doom2: return doom2_keys_map;
		case ap_game_t::heretic: return heretic_keys_map;
		case ap_game_t::doom_open_world: return doom_keys_map;
	}
}


int get_map_doom_type()
{
	switch (ap_game)
	{
		case ap_game_t::doom: return 2026;
		case ap_game_t::doom2: return 2026;
		case ap_game_t::heretic: return 35;
		case ap_game_t::doom_open_world: return 2026;
	}
}


static const std::map<int, int> doom_weapons_map = {{2001, 2}, {2002, 3}, {2003, 4}, {2004, 5}, {2006, 6}, {2005, 7}};
static const std::map<int, int> doom2_weapons_map = {{2001, 2}, {2002, 3}, {2003, 4}, {2004, 5}, {2006, 6}, {2005, 7}, {82, 8}};
static const std::map<int, int> heretic_weapons_map = {{2005, 7}, {2001, 2}, {53, 3}, {2003, 5}, {2002, 6}, {2004, 4}};


const std::map<int, int>& get_weapons_map()
{
	switch (ap_game)
	{
		case ap_game_t::doom: return doom_weapons_map;
		case ap_game_t::doom2: return doom2_weapons_map;
		case ap_game_t::heretic: return heretic_weapons_map;
		case ap_game_t::doom_open_world: return doom_weapons_map;
	}
}


const std::map<int, std::string>& get_sprites()
{
	switch (ap_game)
	{
		case ap_game_t::doom: return ap_doom_type_sprites;
		case ap_game_t::doom2: return ap_doom2_type_sprites;
		case ap_game_t::heretic: return ap_heretic_type_sprites;
		case ap_game_t::doom_open_world: return ap_doom_open_world_type_sprites;
	}
}


std::string get_exmx_name(const std::string& name)
{
	auto pos = name.find_first_of('(');
	if (pos == std::string::npos) return name;
	return name.substr(pos);
}


void f_itemrecv(int64_t item_id, int player_id, bool notify_player)
{
	const auto& item_type_table = get_item_type_table();
	auto it = item_type_table.find(item_id);
	if (it == item_type_table.end())
		return; // Skip
	ap_item_t item = it->second;
	ap_level_index_t idx = {item.ep - 1, item.map - 1};
	ap_level_info_t* level_info = ap_get_level_info(idx);

	std::string notif_text;

	auto level_state = ap_get_level_state(idx);

	// Key?
	const auto& keys_map = get_keys_map();
	auto key_it = keys_map.find(item.doom_type);
	if (key_it != keys_map.end())
	{
		level_state->keys[key_it->second] = 1;
		notif_text = get_exmx_name(level_info->name);
	}

	// Map?
	if (item.doom_type == get_map_doom_type())
	{
		level_state->has_map = 1;
		notif_text = get_exmx_name(level_info->name);
	}

	// Backpack?
	if (item.doom_type == 8)
	{
		ap_state.player_state.backpack = 1;
		auto max_ammos = get_max_ammos();
		for (int i = 0; i < ap_ammo_count; ++i)
			ap_state.player_state.max_ammo[i] = max_ammos[i] * 2;
	}

	// Weapon?
	const auto& weapons_map = get_weapons_map();
	auto weapon_it = weapons_map.find(item.doom_type);
	if (weapon_it != weapons_map.end())
		ap_state.player_state.weapon_owned[weapon_it->second] = 1;

	// Ignore inventory items, the game will add them up

	// Is it a level?
	if (item.doom_type == -1)
	{
		level_state->unlocked = 1;
		notif_text = get_exmx_name(level_info->name);
	}

	// Level complete?
	if (item.doom_type == -2)
		level_state->completed = 1;


	if (!notify_player) return;

	if (!ap_is_in_game)
	{
		ap_item_queue.push_back(item_id);
		return;
	}

	// Give item to player
	ap_settings.give_item_callback(item.doom_type, item.ep, item.map);

	// Add notification icon
	const auto& sprite_map = get_sprites();
	auto sprite_it = sprite_map.find(item.doom_type);
	if (sprite_it != sprite_map.end())
	{
		ap_notification_icon_t notif;
		snprintf(notif.sprite, 9, "%s", sprite_it->second.c_str());
		notif.t = 0;
		notif.text[0] = '\0'; // For now
		if (notif_text != "")
		{
			snprintf(notif.text, 260, "%s", notif_text.c_str());
		}
		notif.xf = AP_NOTIF_SIZE / 2 + AP_NOTIF_PADDING;
		notif.yf = -200.0f + AP_NOTIF_SIZE / 2;
		notif.state = AP_NOTIF_STATE_PENDING;
		notif.velx = 0.0f;
		notif.vely = 0.0f;
		notif.x = (int)notif.xf;
		notif.y = (int)notif.yf;
		ap_notification_icons.push_back(notif);
	}
}


bool find_location(int64_t loc_id, int &ep, int &map, int &index)
{
	ep = -1;
	map = -1;
	index = -1;

	const auto& loc_table = get_location_table();
	for (const auto& loc_map_table : loc_table)
	{
		for (const auto& loc_index_table : loc_map_table.second)
		{
			for (const auto& loc_index : loc_index_table.second)
			{
				if (loc_index.second == loc_id)
				{
					ep = loc_map_table.first;
					map = loc_index_table.first;
					index = loc_index.first;
					break;
				}
			}
			if (ep != -1) break;
		}
		if (ep != -1) break;
	}
	return (ep > 0);
}


void f_locrecv(int64_t loc_id)
{
	// Find where this location is
	int ep = -1;
	int map = -1;
	int index = -1;
	if (!find_location(loc_id, ep, map, index))
	{
		printf("APDOOM: In f_locrecv, loc id not found: %i\n", (int)loc_id);
		return; // Loc not found
	}

	ap_level_index_t idx = {ep - 1, map - 1};

	// Make sure we didn't already check it
	if (is_loc_checked(idx, index)) return;
	if (index < 0) return;

	auto level_state = ap_get_level_state(idx);
	level_state->checks[level_state->check_count] = index;
	level_state->check_count++;
}


void f_locinfo(std::vector<AP_NetworkItem> loc_infos)
{
	for (const auto& loc_info : loc_infos)
	{
		if (loc_info.flags & 1)
			ap_progressive_locations.insert(loc_info.location);
	}
}


void f_goal(int goal)
{
	ap_state.goal = goal;
}


void f_difficulty(int difficulty)
{
	if (ap_settings.override_skill)
		return;

	ap_state.difficulty = difficulty;
}


void f_random_monsters(int random_monsters)
{
	if (ap_settings.override_monster_rando)
		return;

	ap_state.random_monsters = random_monsters;
}


void f_random_items(int random_items)
{
	if (ap_settings.override_item_rando)
		return;

	ap_state.random_items = random_items;
}


void f_random_music(int random_music)
{
	if (ap_settings.override_music_rando)
		return;

	ap_state.random_music = random_music;
}


void f_flip_levels(int flip_levels)
{
	if (ap_settings.override_flip_levels)
		return;

	ap_state.flip_levels = flip_levels;
}


void f_check_sanity(int check_sanity)
{
	ap_state.check_sanity = check_sanity;
}


void f_reset_level_on_death(int reset_level_on_death)
{
	if (ap_settings.override_reset_level_on_death)
		return;

	ap_state.reset_level_on_death = reset_level_on_death;
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


void f_episode4(int ep)
{
	ap_state.episodes[3] = ep;
}


void f_episode5(int ep)
{
	ap_state.episodes[4] = ep;
}


void f_two_ways_keydoors(int two_ways_keydoors)
{
	ap_state.two_ways_keydoors = two_ways_keydoors;
}


const char* apdoom_get_seed()
{
	return ap_save_dir_name.c_str();
}


void apdoom_check_location(ap_level_index_t idx, int index)
{
	int64_t id = 0;
	const auto& loc_table = get_location_table();

	auto it1 = loc_table.find(idx.ep + 1);
	if (it1 == loc_table.end()) return;

	auto it2 = it1->second.find(idx.map + 1);
	if (it2 == it1->second.end()) return;

	auto it3 = it2->second.find(index);
	if (it3 == it2->second.end()) return;

	id = it3->second;

	if (index >= 0)
	{
		if (is_loc_checked(idx, index))
		{
			printf("APDOOM: Location already checked\n");
		}
		else
		{ // We get back from AP
			//auto level_state = ap_get_level_state(ep, map);
			//level_state->checks[level_state->check_count] = index;
			//level_state->check_count++;
		}
	}
	AP_SendItem(id);
}


int apdoom_is_location_progression(ap_level_index_t idx, int index)
{
	const auto& loc_table = get_location_table();

	auto it1 = loc_table.find(idx.ep + 1);
	if (it1 == loc_table.end()) return 0;

	auto it2 = it1->second.find(idx.map + 1);
	if (it2 == it1->second.end()) return 0;

	auto it3 = it2->second.find(index);
	if (it3 == it2->second.end()) return 0;

	int64_t id = it3->second;

	return (ap_progressive_locations.find(id) != ap_progressive_locations.end()) ? 1 : 0;
}

void apdoom_complete_level(ap_level_index_t idx)
{
	//if (ap_state.level_states[ep - 1][map - 1].completed) return; // Already completed
    ap_get_level_state(idx)->completed = 1;
	apdoom_check_location(idx, -1); // -1 is complete location
}


ap_level_index_t ap_make_level_index(int ep /* 1-based */, int map /* 1-based */)
{
	if (ap_game != ap_game_t::doom2) return { ep - 1, map - 1 };

	// In Doom2, every map is ep = 1
	ap_level_index_t ret = { 0, map - 1 };
	const auto& table = get_level_info_table();
	while (ret.map >= (int)table[ret.ep].size())
	{
		ret.map -= (int)table[ret.ep].size();
		ret.ep++;
	}

	return ret;
}


int ap_index_to_ep(ap_level_index_t idx)
{
	if (ap_game != ap_game_t::doom2) return idx.ep + 1;
	return 1;
}


int ap_index_to_map(ap_level_index_t idx)
{
	if (ap_game != ap_game_t::doom2) return idx.map + 1;

	const auto& table = get_level_info_table();
	for (int ep = 0; ep < idx.ep; ++ep)
	{
		idx.map += (int)table[ep].size();
	}
	return idx.map + 1;
}


void apdoom_check_victory()
{
	if (ap_state.victory) return;

	if (ap_state.goal == 1 && (ap_game == ap_game_t::doom || ap_game == ap_game_t::doom_open_world || ap_game == ap_game_t::heretic))
	{
		for (int ep = 0; ep < ap_episode_count; ++ep)
		{
			if (!ap_state.episodes[ep]) continue;
			if (!ap_get_level_state(ap_level_index_t{ep, 7})->completed) return;
		}
	}
	else
	{
		// All levels
		for (int ep = 0; ep < ap_episode_count; ++ep)
		{
			if (!ap_state.episodes[ep]) continue;
		
			int map_count = ap_get_map_count(ep + 1);
			for (int map = 0; map < map_count; ++map)
			{
				if (!ap_get_level_state(ap_level_index_t{ep, map})->completed) return;
			}
		}
	}

	ap_state.victory = 1;

	AP_StoryComplete();
	ap_settings.victory_callback();
}


void apdoom_send_message(const char* msg)
{
	std::string smsg = msg;
	if (strnicmp(msg, "!hint ", 6) == 0)
	{
		// Make the hint easier.
		// Find an E#M# in the text
		for (size_t i = 6; i < smsg.size() - 4; ++i)
		{
			ap_level_info_t* level_info = nullptr;

			if (toupper(smsg[i]) == 'E' &&
				toupper(smsg[i + 2]) == 'M' &&
				(smsg[i + 1] - '0') >= 1 && (smsg[i + 1] - '0') <= 9 &&
				(smsg[i + 3] - '0') >= 1 && (smsg[i + 3] - '0') <= 9)
			{
				// Find the level name
				int ep = smsg[i + 1] - '1';
				int map = smsg[i + 3] - '1';
				level_info = ap_get_level_info(ap_level_index_t{ep, map});
			}

			if (toupper(smsg[i]) == 'M' &&
				toupper(smsg[i + 1]) == 'A' &&
				toupper(smsg[i + 2]) == 'P' &&
				(smsg[i + 3] - '0') >= 0 && (smsg[i + 3] - '0') <= 3 &&
				(smsg[i + 4] - '0') >= 0 && (smsg[i + 4] - '0') <= 9)
			{
				// Find the level name
				int ep = 1;
				int map = smsg[i + 4] + (smsg[i + 3] - '0') * 10;
				level_info = ap_get_level_info(ap_make_level_index(ep, map));
			}

			if (level_info)
			{
				std::string level_name = level_info->name;

				// Check what it's looking for. Computer area map? map scroll?
				if (smsg.find("map") != std::string::npos || smsg.find("MAP") != std::string::npos)
				{
					switch (ap_game)
					{
						case ap_game_t::doom:
						case ap_game_t::doom2:
						case ap_game_t::doom_open_world:
							smsg = "!hint " + level_name + " - Computer area map";
							break;
						case ap_game_t::heretic:
							smsg = "!hint " + level_name + " - Map scroll";
							break;
					}
				}
				else if (smsg.find("blue") != std::string::npos || smsg.find("BLUE") != std::string::npos)
				{
					switch (ap_game)
					{
						case ap_game_t::doom:
						case ap_game_t::doom2:
						case ap_game_t::doom_open_world:
							smsg = "!hint " + level_name + " - Blue " + (level_info->use_skull[0] ? "skull key" : "keycard");
							break;
						case ap_game_t::heretic:
							smsg = "!hint " + level_name + " - Blue key";
							break;
					}
				}
				else if (smsg.find("yellow") != std::string::npos || smsg.find("YELLOW") != std::string::npos)
				{
					switch (ap_game)
					{
						case ap_game_t::doom:
						case ap_game_t::doom2:
						case ap_game_t::doom_open_world:
							smsg = "!hint " + level_name + " - Yellow " + (level_info->use_skull[1] ? "skull key" : "keycard");
							break;
						case ap_game_t::heretic:
							smsg = "!hint " + level_name + " - Yellow key";
							break;
					}
				}
				else if (smsg.find("red") != std::string::npos || smsg.find("RED") != std::string::npos)
				{
					switch (ap_game)
					{
						case ap_game_t::doom:
						case ap_game_t::doom2:
						case ap_game_t::doom_open_world:
							smsg = "!hint " + level_name + " - Red " + (level_info->use_skull[0] ? "skull key" : "keycard");
							break;
						case ap_game_t::heretic:
							break;
					}
				}
				else if (smsg.find("green") != std::string::npos || smsg.find("GREEN") != std::string::npos)
				{
					switch (ap_game)
					{
						case ap_game_t::doom:
						case ap_game_t::doom2:
						case ap_game_t::doom_open_world:
							break;
						case ap_game_t::heretic:
							smsg = "!hint " + level_name + " - Green key";
							break;
					}
				}

				break;
			}
		}
	}

	Json::Value say_packet;
	say_packet[0]["cmd"] = "Say";
	say_packet[0]["text"] = smsg;
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


const ap_notification_icon_t* ap_get_notification_icons(int* count)
{
	*count = (int)ap_notification_icons.size();
	return ap_notification_icons.data();
}


int ap_get_highest_episode()
{
	int highest = 0;
	for (int i = 0; i < ap_episode_count; ++i)
		if (ap_state.episodes[i])
			highest = i;
	return highest;
}


int ap_validate_doom_location(ap_level_index_t idx, int doom_type, int index)
{
	ap_level_info_t* level_info = ap_get_level_info(idx);
    if (index >= level_info->thing_count) return -1;
	if (level_info->thing_infos[index].doom_type != doom_type) return -1;
	if (level_info->thing_infos[index].unreachable) return 0;
    return level_info->thing_infos[index].check_sanity == 0 || ap_state.check_sanity == 1;
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

		switch (msg->type)
		{
			case AP_MessageType::ItemSend:
			{
				AP_ItemSendMessage* o_msg = static_cast<AP_ItemSendMessage*>(msg);
				colored_msg = "~9" + o_msg->item + "~2 was sent to ~4" + o_msg->recvPlayer;
				break;
			}
			case AP_MessageType::ItemRecv:
			{
				AP_ItemRecvMessage* o_msg = static_cast<AP_ItemRecvMessage*>(msg);
				colored_msg = "~2Received ~9" + o_msg->item + "~2 from ~4" + o_msg->sendPlayer;
				break;
			}
			case AP_MessageType::Hint:
			{
				AP_HintMessage* o_msg = static_cast<AP_HintMessage*>(msg);
				colored_msg = "~9" + o_msg->item + "~2 from ~4" + o_msg->sendPlayer + "~2 to ~4" + o_msg->recvPlayer + "~2 at ~3" + o_msg->location + (o_msg->checked ? " (Checked)" : " (Unchecked)");
				break;
			}
			default:
			{
				colored_msg = "~2" + msg->text;
				break;
			}
		}

		printf("APDOOM: %s\n", msg->text.c_str());

		if (ap_initialized)
			ap_settings.message_callback(colored_msg.c_str());
		else
			ap_cached_messages.push_back(colored_msg);

		AP_ClearLatestMessage();
	}

	// Check if we're in game, then dequeue the items
	if (ap_is_in_game)
	{
		while (!ap_item_queue.empty())
		{
			auto item_id = ap_item_queue.front();
			ap_item_queue.erase(ap_item_queue.begin());
			f_itemrecv(item_id, 0, true);
		}
	}

	// Update notification icons
	float previous_y = 2.0f;
	for (auto it = ap_notification_icons.begin(); it != ap_notification_icons.end();)
	{
		auto& notification_icon = *it;

		if (notification_icon.state == AP_NOTIF_STATE_PENDING && previous_y > -100.0f)
		{
			notification_icon.state = AP_NOTIF_STATE_DROPPING;
		}
		if (notification_icon.state == AP_NOTIF_STATE_PENDING)
		{
			++it;
			continue;
		}

		if (notification_icon.state == AP_NOTIF_STATE_DROPPING)
		{
			notification_icon.vely += 0.15f + (float)(ap_notification_icons.size() / 4) * 0.25f;
			if (notification_icon.vely > 8.0f) notification_icon.vely = 8.0f;
			notification_icon.yf += notification_icon.vely;
			if (notification_icon.yf >= previous_y - AP_NOTIF_SIZE - AP_NOTIF_PADDING)
			{
				notification_icon.yf = previous_y - AP_NOTIF_SIZE - AP_NOTIF_PADDING;
				notification_icon.vely *= -0.3f / ((float)(ap_notification_icons.size() / 4) * 0.05f + 1.0f);

				notification_icon.t += ap_notification_icons.size() / 4 + 1; // Faster the more we have queued (4 can display on screen)
				if (notification_icon.t > 350 * 3 / 4) // ~7.5sec
				{
					notification_icon.state = AP_NOTIF_STATE_HIDING;
				}
			}
		}

		if (notification_icon.state == AP_NOTIF_STATE_HIDING)
		{
			notification_icon.velx -= 0.14f + (float)(ap_notification_icons.size() / 4) * 0.1f;
			notification_icon.xf += notification_icon.velx;
			if (notification_icon.xf < -AP_NOTIF_SIZE / 2)
			{
				it = ap_notification_icons.erase(it);
				continue;
			}
		}

		notification_icon.x = (int)notification_icon.xf;
		notification_icon.y = (int)notification_icon.yf;
		previous_y = notification_icon.yf;

		++it;
	}
}
