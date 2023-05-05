#pragma once

#include <cinttypes>
#include <vector>


#define EP_COUNT 3
#define MAP_COUNT 9


static const char* level_names[EP_COUNT][MAP_COUNT] = {
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


struct map_t
{
    std::vector<map_thing_t>        things;
    std::vector<map_linedefs_t>     linedefs;
    std::vector<map_sidedefs_t>     sidedefs;
    std::vector<map_vertex_t>       vertexes;
    std::vector<map_sectors_t>      map_sectors;
    std::vector<map_subsector_t>    map_subsectors;
    std::vector<map_node_t>         map_nodes;
    std::vector<map_seg_t>          map_segs;
    std::vector<subsector_t>        subsectors;
    std::vector<node_t>             nodes;
    int16_t bb[4];
};


extern map_t maps[EP_COUNT][MAP_COUNT];


void init_maps();

