#include "maps.h"

#include <onut/onut.h>
#include <onut/Dialogs.h>

#include <stdio.h>


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


map_t maps[EP_COUNT][MAP_COUNT];


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


subsector_t* point_in_subsector(int x, int y, map_t* map)
{
    node_t*	node;
    int side;
    int nodenum;

    // single subsector is a special case
    if (map->nodes.empty())
        return map->subsectors.data();
		
    nodenum = (int)map->nodes.size() - 1;

    while (!(nodenum & 0x80000000))
    {
        node = &map->nodes[nodenum];
        side = point_on_side(x, y, node);
        nodenum = node->children[side];
    }
	
    return &map->subsectors[nodenum & ~0x80000000];
}


void init_maps()
{
    // Load DOOM.WAD
    FILE* f = fopen(OArguments[0].c_str(), "rb");
    if (!f)
    {
        onut::showMessageBox("Error", "Cannot open file: " + OArguments[0]);
        exit(1); // Hard kill
    }
    
    // Read header
    map_header_t header;
    fread(&header, sizeof(header), 1, f);
    if (strncmp(header.identification, "PWAD", 4) != 0 && strncmp(header.identification, "IWAD", 4) != 0)
    {
        onut::showMessageBox("Error", "Invalid IWAD or PWAD");
        exit(1); // Hard kill
    }
    
    // Read directory
    std::vector<map_directory_t> directory(header.num_lumps);
    fseek(f, header.directory_offset, SEEK_SET);
    fread(directory.data(), sizeof(map_directory_t), header.num_lumps, f);

    // loop directory and find levels, then load them all. YOLO
    for (int i = 0, len = (int)directory.size(); i < len; ++i)
    {
        const auto &dir_entry = directory[i];
        if (strlen(dir_entry.name) == 4 && dir_entry.name[0] == 'E' && dir_entry.name[2] == 'M')
        {
            // That's a level!
            auto ep = dir_entry.name[1] - '0';
            auto lvl = dir_entry.name[3] - '0';
            if (ep < 1 || ep > 3 || lvl < 1 || lvl > 9)
            {
                // ok.. not a level
                continue;
            }

            auto map = &maps[ep - 1][lvl - 1];

            ++i;
            for (; i < len; ++i)
            {
                const auto &dir_entry = directory[i];
                try_load_lump("THINGS", f, dir_entry, map->things);
                try_load_lump("LINEDEFS", f, dir_entry, map->linedefs);
                try_load_lump("SIDEDEFS", f, dir_entry, map->sidedefs);
                try_load_lump("VERTEXES", f, dir_entry, map->vertexes);
                try_load_lump("SECTORS", f, dir_entry, map->map_sectors);
                try_load_lump("SSECTORS", f, dir_entry, map->map_subsectors);
                try_load_lump("NODES", f, dir_entry, map->map_nodes);
                try_load_lump("SEGS", f, dir_entry, map->map_segs);
                if (strncmp(dir_entry.name, "BLOCKMAP", 8) == 0)
                {
                    break;
                }
            }

            map->subsectors.resize(map->map_subsectors.size());
            map->nodes.resize(map->map_nodes.size());
            for (int j = 0, len = (int)map->map_nodes.size(); j < len; ++j)
            {
                map->nodes[j].x = (int)map->map_nodes[j].x << 16;
                map->nodes[j].y = (int)map->map_nodes[j].y << 16;
                map->nodes[j].dx = (int)map->map_nodes[j].dx << 16;
                map->nodes[j].dy = (int)map->map_nodes[j].dy << 16;
                for (int jj = 0; jj < 2; ++jj)
                {
                    map->nodes[j].children[jj] = (uint16_t)(int16_t)map->map_nodes[j].children[jj];
                    if (map->nodes[j].children[jj] == (uint16_t)-1)
                        map->nodes[j].children[jj] = -1;
                    else if (map->nodes[j].children[jj] & 0x8000)
                    {
                        map->nodes[j].children[jj] &= ~0x8000;
                        if (map->nodes[j].children[jj] >= (int)map->map_subsectors.size())
                            map->nodes[j].children[jj] = 0;
                        map->nodes[j].children[jj] |= 0x80000000;
                    }
                    for (int k = 0; k < 4; ++k)
                        map->nodes[j].bbox[jj][k] = (int)map->map_nodes[j].bbox[jj][k] << 16;
                }
            }

            for (int j = 0, len = (int)map->map_subsectors.size(); j < len; ++j)
            {
                const auto& map_seg = map->map_segs[map->map_subsectors[j].firstseg];
                const auto& map_sidedef = map->sidedefs[map_seg.linedef];
                map->subsectors[j].sector = map_sidedef.sector;
            }

            map->bb[0] = map->vertexes[0].x;
            map->bb[1] = map->vertexes[0].y;
            map->bb[2] = map->vertexes[0].x;
            map->bb[3] = map->vertexes[0].y;
            for (int v = 1, vlen = (int)map->vertexes.size(); v < vlen; ++v)
            {
                map->bb[0] = std::min(map->bb[0], map->vertexes[v].x);
                map->bb[1] = std::min(map->bb[1], map->vertexes[v].y);
                map->bb[2] = std::max(map->bb[2], map->vertexes[v].x);
                map->bb[3] = std::max(map->bb[3], map->vertexes[v].y);
            }
        }
    }

    // close file
    fclose(f);
}
