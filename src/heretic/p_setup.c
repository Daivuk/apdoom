//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005-2014 Simon Howard
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

// P_main.c

#include <math.h>
#include <stdlib.h>

#include "doomdef.h"
#include "i_swap.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "p_local.h"
#include "s_sound.h"
#include "p_extnodes.h"

#include "apheretic_c_def.h"
#include "apdoom.h"

void P_SpawnMapThing(mapthing_t * mthing, int index);

int numvertexes;
vertex_t *vertexes;

int numsegs;
seg_t *segs;

int numsectors;
sector_t *sectors;

int numsubsectors;
subsector_t *subsectors;

int numnodes;
node_t *nodes;

int numlines;
line_t *lines;

int numsides;
side_t *sides;

int32_t *blockmap;	            // offsets in blockmap are from here // [crispy] BLOCKMAP limit
int32_t *blockmaplump;          // [crispy] BLOCKMAP limit
int bmapwidth, bmapheight;      // in mapblocks
fixed_t bmaporgx, bmaporgy;     // origin of block map
mobj_t **blocklinks;            // for thing chains

byte *rejectmatrix;             // for fast sight rejection

mapthing_t deathmatchstarts[10], *deathmatch_p;
mapthing_t playerstarts[MAXPLAYERS];
boolean playerstartsingame[MAXPLAYERS];

// [crispy] recalculate seg offsets
// adapted from prboom-plus/src/p_setup.c:474-482
fixed_t GetOffset(vertex_t *v1, vertex_t *v2)
{
    fixed_t dx, dy;
    fixed_t r;

    dx = (v1->x - v2->x)>>FRACBITS;
    dy = (v1->y - v2->y)>>FRACBITS;
    r = (fixed_t)(sqrt(dx*dx + dy*dy))<<FRACBITS;

    return r;
}


unsigned long long hash_seed(unsigned char *str)
{
    unsigned long long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/*
=================
=
= P_LoadVertexes
=
=================
*/

void P_LoadVertexes(int lump)
{
    byte *data;
    int i;
    mapvertex_t *ml;
    vertex_t *li;

    numvertexes = W_LumpLength(lump) / sizeof(mapvertex_t);
    vertexes = Z_Malloc(numvertexes * sizeof(vertex_t), PU_LEVEL, 0);
    data = W_CacheLumpNum(lump, PU_STATIC);

    ml = (mapvertex_t *) data;
    li = vertexes;
    for (i = 0; i < numvertexes; i++, li++, ml++)
    {
        li->x = SHORT(ml->x) << FRACBITS;
        li->y = SHORT(ml->y) << FRACBITS;

        // [crispy] initialize pseudovertexes with actual vertex coordinates
        li->r_x = li->x;
        li->r_y = li->y;
        li->moved = false;
    }

    W_ReleaseLumpNum(lump);
}


/*
=================
=
= P_LoadSegs
=
=================
*/

void P_LoadSegs(int lump)
{
    byte *data;
    int i;
    mapseg_t *ml;
    seg_t *li;
    line_t *ldef;
    int linedef, side;

    numsegs = W_LumpLength(lump) / sizeof(mapseg_t);
    segs = Z_Malloc(numsegs * sizeof(seg_t), PU_LEVEL, 0);
    memset(segs, 0, numsegs * sizeof(seg_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    ml = (mapseg_t *) data;
    li = segs;
    for (i = 0; i < numsegs; i++, li++, ml++)
    {
        li->v1 = &vertexes[(unsigned short)SHORT(ml->v1)]; // [crispy] extended nodes
        li->v2 = &vertexes[(unsigned short)SHORT(ml->v2)]; // [crispy] extended nodes

        li->angle = (SHORT(ml->angle)) << 16;
        li->offset = (SHORT(ml->offset)) << 16;
        linedef = (unsigned short)SHORT(ml->linedef); // [crispy] extended nodes
        ldef = &lines[linedef];
        li->linedef = ldef;
        side = SHORT(ml->side);
        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;
        // [crispy] recalculate
        li->offset = GetOffset(li->v1, (ml->side ? ldef->v2 : ldef->v1));
        if (ldef->flags & ML_TWOSIDED)
            li->backsector = sides[ldef->sidenum[side ^ 1]].sector;
        else
            li->backsector = 0;
    }

    W_ReleaseLumpNum(lump);
}

// [crispy] fix long wall wobble

static angle_t anglediff(angle_t a, angle_t b)
{
    if (b > a)
        return anglediff(b, a);

    if (a - b < ANG180)
        return a - b;
    else // [crispy] wrap around
        return b - a;
}

static void P_SegLengths(void)
{
    int i;

    for (i = 0; i < numsegs; i++)
    {
        seg_t *const li = &segs[i];
        int64_t dx, dy;

        dx = li->v2->r_x - li->v1->r_x;
        dy = li->v2->r_y - li->v1->r_y;

        li->length = (uint32_t)(sqrt((double)dx * dx + (double)dy * dy) / 2);

        // [crispy] re-calculate angle used for rendering
        viewx = li->v1->r_x;
        viewy = li->v1->r_y;
        li->r_angle = R_PointToAngleCrispy(li->v2->r_x, li->v2->r_y);
        // [crispy] more than just a little adjustment?
        // back to the original angle then
        if (anglediff(li->r_angle, li->angle) > ANG60/2)
        {
            li->r_angle = li->angle;
        }
    }
}

/*
=================
=
= P_LoadSubsectors
=
=================
*/

void P_LoadSubsectors(int lump)
{
    byte *data;
    int i;
    mapsubsector_t *ms;
    subsector_t *ss;

    numsubsectors = W_LumpLength(lump) / sizeof(mapsubsector_t);
    subsectors = Z_Malloc(numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
    data = W_CacheLumpNum(lump, PU_STATIC);

    ms = (mapsubsector_t *) data;
    memset(subsectors, 0, numsubsectors * sizeof(subsector_t));
    ss = subsectors;
    for (i = 0; i < numsubsectors; i++, ss++, ms++)
    {
        ss->numlines = (unsigned short)SHORT(ms->numsegs); // [crispy] extended nodes
        ss->firstline = (unsigned short)SHORT(ms->firstseg); // [crispy] extended nodes
    }

    W_ReleaseLumpNum(lump);
}


/*
=================
=
= P_LoadSectors
=
=================
*/

void P_LoadSectors(int lump)
{
    byte *data;
    int i;
    mapsector_t *ms;
    sector_t *ss;

    numsectors = W_LumpLength(lump) / sizeof(mapsector_t);
    sectors = Z_Malloc(numsectors * sizeof(sector_t), PU_LEVEL, 0);
    memset(sectors, 0, numsectors * sizeof(sector_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    ms = (mapsector_t *) data;

    if (gameepisode == 1 && gamemap == 7)
    {
        ms[74].tag = 3333;
        sprintf(ms[74].floorpic, "FLOOR28");
    }

    ss = sectors;
    for (i = 0; i < numsectors; i++, ss++, ms++)
    {
        ss->floorheight = SHORT(ms->floorheight) << FRACBITS;
        ss->ceilingheight = SHORT(ms->ceilingheight) << FRACBITS;
        ss->floorpic = R_FlatNumForName(ms->floorpic);
        ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
        ss->lightlevel = SHORT(ms->lightlevel);
        ss->special = SHORT(ms->special);
        ss->tag = SHORT(ms->tag);
        ss->thinglist = NULL;

        // [crispy] WiggleFix: [kb] for R_FixWiggle()
        ss->cachedheight = 0;

        // [AM] Sector interpolation.  Even if we're
        //      not running uncapped, the renderer still
        //      uses this data.
        ss->oldfloorheight = ss->floorheight;
        ss->interpfloorheight = ss->floorheight;
        ss->oldceilingheight = ss->ceilingheight;
        ss->interpceilingheight = ss->ceilingheight;
        // [crispy] inhibit sector interpolation during the 0th gametic
        ss->oldgametic = -1;
    }

    W_ReleaseLumpNum(lump);
}


/*
=================
=
= P_LoadNodes
=
=================
*/

void P_LoadNodes(int lump)
{
    byte *data;
    int i, j, k;
    mapnode_t *mn;
    node_t *no;

    numnodes = W_LumpLength(lump) / sizeof(mapnode_t);
    nodes = Z_Malloc(numnodes * sizeof(node_t), PU_LEVEL, 0);
    data = W_CacheLumpNum(lump, PU_STATIC);

    mn = (mapnode_t *) data;
    no = nodes;
    for (i = 0; i < numnodes; i++, no++, mn++)
    {
        no->x = SHORT(mn->x) << FRACBITS;
        no->y = SHORT(mn->y) << FRACBITS;
        no->dx = SHORT(mn->dx) << FRACBITS;
        no->dy = SHORT(mn->dy) << FRACBITS;
        for (j = 0; j < 2; j++)
        {
            no->children[j] = (unsigned short)SHORT(mn->children[j]); // [crispy] extended nodes

            // [crispy] add support for extended nodes
            // from prboom-plus/src/p_setup.c:937-957
            if (no->children[j] == NO_INDEX)
                no->children[j] = -1;
            else
            if (no->children[j] & NF_SUBSECTOR_VANILLA)
            {
                no->children[j] &= ~NF_SUBSECTOR_VANILLA;

                if (no->children[j] >= numsubsectors)
                    no->children[j] = 0;

                no->children[j] |= NF_SUBSECTOR;
            }

            for (k = 0; k < 4; k++)
                no->bbox[j][k] = SHORT(mn->bbox[j][k]) << FRACBITS;
        }
    }

    W_ReleaseLumpNum(lump);
}


typedef enum
{
    // Not size, but difficulty
    rmc_small,
    rmc_medium,
    rmc_big,
    rmc_boss,

    NUM_RMC
} random_monster_cat_t;


typedef struct
{
    int doom_type;
    int hit_points;
    int radius;
    int height;
    int frequency; // Globally in entire game. This is used for random balanced
    random_monster_cat_t category;
    int dont_shuffle;
} random_monster_def_t;


// I feel like these ratios should be per episode. Espideo 4-5 squewing things up a bit
static random_monster_def_t random_monster_defs[] =
{
    { 66, 40, 16*FRACUNIT, 36*FRACUNIT, 1350, rmc_small }, // Gargoyle
    { 5, 80, 16*FRACUNIT, 36*FRACUNIT, 626, rmc_small }, // Fire gargoyle
    { 68, 80, 22*FRACUNIT, 62*FRACUNIT, 560, rmc_small }, // Golem
    { 69, 80, 22*FRACUNIT, 62*FRACUNIT, 137, rmc_small }, // Golem ghost
    { 45, 100, 22*FRACUNIT, 62*FRACUNIT, 292, rmc_small }, // Nitrogolem
    { 46, 100, 22*FRACUNIT, 62*FRACUNIT, 153, rmc_small }, // Nitrogolem ghost

    { 64, 200, 24*FRACUNIT, 78*FRACUNIT, 193, rmc_medium }, // Undead Warrior
    { 65, 200, 24*FRACUNIT, 78*FRACUNIT, 55, rmc_medium }, // Undead Warrior ghost
    { 15, 180, 16*FRACUNIT, 68*FRACUNIT, 476, rmc_medium }, // Disciple of D'Sparil
    { 90, 150, 20*FRACUNIT, 64*FRACUNIT, 652, rmc_medium }, // Sabreclaw
    { 70, 220, 32*FRACUNIT, 74*FRACUNIT, 248, rmc_medium }, // Weredragon
    { 92, 280, 22*FRACUNIT, 70*FRACUNIT, 284, rmc_medium }, // Ophidian

    { 6, 700, 50*FRACUNIT, 72*FRACUNIT, 108, rmc_big }, // Iron lich (Changed this radius from 40 -> 50 otherwise it gets stuck in stairs)

    { 9, 3000, 28*FRACUNIT, 100*FRACUNIT, 22, rmc_boss, 1 }, // Maulotaur
    { 7, 2000, 28*FRACUNIT, 100*FRACUNIT, 1, rmc_boss, 1 } // D'Sparil 28x100 [Boss, keep them there]
};
static const int monster_def_count = sizeof(random_monster_defs) / sizeof(random_monster_def_t);


typedef struct
{
    int index;
    random_monster_def_t* og_monster;
    random_monster_def_t* monster;
    fixed_t fit_radius;
    fixed_t fit_height;
} monster_spawn_def_t;


extern fixed_t tmbbox[4];
extern int tmflags;
extern fixed_t tmx;
extern fixed_t tmy;
fixed_t tmradius;
int tmtype;


boolean PIT_CheckLine_NoFlags(line_t* ld)
{
    if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
     || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
     || tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
     || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
        return true;

    if (P_BoxOnLineSide (tmbbox, ld) != -1)
        return true;
		
    // A line has been hit
    
    // The moving thing's destination position will cross
    // the given line.
    // If this should not be allowed, return false.
    // If the line is special, keep track of it
    // to process later if the move is proven ok.
    // NOTE: specials are NOT sorted by order,
    // so two special lines that are only 8 pixels apart
    // could be crossed in either order.

    //if (tmtype == 6)
    //    return false; // Iron lich are too big
    
    if (!ld->backsector)
	    return false;		// one sided line
		
	if (ld->flags & ML_BLOCKING)
	    return false;	// explicitly blocking everything

	if (ld->flags & ML_BLOCKMONSTERS)
	    return false;	// block monsters only

    if (tmradius <= 20)
        return true; // Smallest unit

    // Check if the back sector can step up/down
    float floor = ld->frontsector->floorheight;
    float back_floor = ld->backsector->floorheight;
    
    return abs(floor - back_floor) <= 20 * FRACUNIT;
}


boolean check_position(fixed_t x, fixed_t y, fixed_t radius)
{
    int			xl;
    int			xh;
    int			yl;
    int			yh;
    int			bx;
    int			by;
    subsector_t*	newsubsec;
	
    tmx = x;
    tmy = y;
    tmradius = radius;
	
    tmbbox[BOXTOP] = y + radius;
    tmbbox[BOXBOTTOM] = y - radius;
    tmbbox[BOXRIGHT] = x + radius;
    tmbbox[BOXLEFT] = x - radius;

    newsubsec = R_PointInSubsector(x, y);
    validcount++;
    
    // check lines
    xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockLinesIterator(bx, by, PIT_CheckLine_NoFlags))
                return false;

    return true;
}


void get_fit_dimensions(fixed_t x, fixed_t y, fixed_t* fit_radius, fixed_t* fit_height)
{
    static const fixed_t radius_checks[] = {
        128*FRACUNIT,
        64*FRACUNIT,
        48*FRACUNIT,
        40*FRACUNIT,
        31*FRACUNIT,
        30*FRACUNIT,
        24*FRACUNIT,
        20*FRACUNIT,
        16*FRACUNIT
    };

    static const fixed_t height_checks[] = {
        110*FRACUNIT,
        100*FRACUNIT,
        64*FRACUNIT,
        56*FRACUNIT
    };

    subsector_t* ss = R_PointInSubsector(x, y);

    for (int i = 0, len = sizeof(height_checks) / sizeof(fixed_t); i < len; ++i)
    {
        fixed_t sector_height = ss->sector->ceilingheight - ss->sector->floorheight;
        if (sector_height >= height_checks[i] || i == len - 1)
        {
            *fit_height = height_checks[i];
            break;
        }
    }

    for (int i = 0, len = sizeof(radius_checks) / sizeof(fixed_t); i < len; ++i)
    {
        if (check_position(x, y, radius_checks[i]) || i == len - 1)
        {
            *fit_radius = radius_checks[i];
            break;
        }
    }
}


/*
=================
=
= P_LoadThings
=
=================
*/

void P_LoadThings(int lump)
{
    byte *data;
    int i, j;
    mapthing_t spawnthing;
    mapthing_t spawnthing_player1_start;
    mapthing_t *mt;
    int numthings;
    int bit;

    data = W_CacheLumpNum(lump, PU_STATIC);
    numthings = W_LumpLength(lump) / sizeof(mapthing_t);

    // Generate unique random seed from ap seed + level
    const char* ap_seed = apdoom_get_seed();
    unsigned long long seed = hash_seed(ap_seed);
    seed += gameepisode * 9 + gamemap;
    srand(seed);

    int things_type_remap[1024] = {0};

    mt = (mapthing_t *)data;
    for (i = 0; i < numthings; i++, mt++)
    {
        things_type_remap[i] = mt->type;
    }
    
#define E1M8_CUTOFF_OFFSET -1984

    int do_random_monsters = ap_state.random_monsters;
    if (do_random_monsters > 0)
    {
        // Make sure at the right difficulty level
        if (gameskill == sk_baby)
            bit = 1;
        else if (gameskill == sk_nightmare)
            bit = 4;
        else
            bit = 1<<(gameskill-1);

        random_monster_def_t* monsters[1024] = {0};
        int monster_count = 0;
        monster_spawn_def_t spawns[1024] = {0};
        int spawn_count = 0;

        // Collect spawn points
        mt = (mapthing_t *)data;
        for (i = 0; i < numthings; i++, mt++)
        {
            if (!(mt->options & bit))
                continue;

            for (int j = 0; j < monster_def_count; ++j)
            {
                if (random_monster_defs[j].dont_shuffle)
                    continue;

                if (gameepisode == 1 && gamemap == 8)
                    if (mt->y < E1M8_CUTOFF_OFFSET)
                        continue;

                if (random_monster_defs[j].doom_type == mt->type)
                {
                    tmtype = mt->type;
                    get_fit_dimensions(mt->x * FRACUNIT, mt->y * FRACUNIT, &spawns[spawn_count].fit_radius, &spawns[spawn_count].fit_height);
                    spawns[spawn_count].og_monster = &random_monster_defs[j];
                    spawns[spawn_count++].index = i;
                    break;
                }
            }
        }

        if (ap_state.random_monsters == 1) // Shuffle
        {
            // Collect monsters
            for (int i = 0; i < spawn_count; ++i)
            {
                monster_spawn_def_t* spawn = &spawns[i];
                monsters[monster_count++] = spawn->og_monster;
            }
        }
        else if (ap_state.random_monsters == 2) // Random balanced
        {
            int ratios[NUM_RMC] = {0};
            random_monster_def_t* defs_by_rmc[NUM_RMC][20];
            int defs_by_rmc_count[NUM_RMC] = {0};
            int rmc_ratios[NUM_RMC] = {0};
            for (int i = 0; i < monster_def_count; ++i)
            {
                random_monster_def_t* monster = &random_monster_defs[i];
                defs_by_rmc[monster->category][defs_by_rmc_count[monster->category]++] = monster;
                rmc_ratios[monster->category] += monster->frequency;
            }

            int total = 0;
            for (int i = 0; i < spawn_count; ++i)
            {
                ratios[spawns[i].og_monster->category]++;
                total++;
            }

            while (monster_count < spawn_count)
            {
                int rnd = rand() % total;
                for (int i = 0; i < NUM_RMC; ++i)
                {
                    if (rnd < ratios[i])
                    {
                        rnd = rand() % rmc_ratios[i];
                        for (int j = 0; j < defs_by_rmc_count[i]; ++j)
                        {
                            if (rnd < defs_by_rmc[i][j]->frequency)
                            {
                                monsters[monster_count++] = defs_by_rmc[i][j];
                                break;
                            }
                            rnd -= defs_by_rmc[i][j]->frequency;
                        }
                        break;
                    }
                    rnd -= ratios[i];
                }
            }
        }
        else if (ap_state.random_monsters == 3) // Random chaotic
        {
            int total = 0;
            for (int i = 0; i < monster_def_count; ++i)
            {
                random_monster_def_t* monster = &random_monster_defs[i];
                if (monster->dont_shuffle) continue;
                total += monster->frequency;
            }

            while (monster_count < spawn_count)
            {
                int rnd = rand() % total;
                for (int i = 0; i < monster_def_count; ++i)
                {
                    random_monster_def_t* monster = &random_monster_defs[i];
                    if (monster->dont_shuffle) continue;
                    if (rnd < monster->frequency)
                    {
                        monsters[monster_count++] = monster;
                        break;
                    }
                    rnd -= monster->frequency;
                }
            }
        }

        // Make sure we have at least 2 iron lynch in first episode boss level
        if (gameepisode == 1 && gamemap == 8)
        {
            int iron_lynch_count = 0;
            for (int i = 0; i < monster_count; ++i)
                if (monsters[i]->doom_type == 6)
                    iron_lynch_count++;
            while (iron_lynch_count < 2)
            {
                int i = rand() % monster_count;
                if (monsters[i]->doom_type != 6)
                {
                    monsters[i] = &random_monster_defs[12];
                    iron_lynch_count++;
                }
            }
        }

        // Make sure we have at least 1 iron lynch in E4M8
        if (gameepisode == 4 && gamemap == 8)
        {
            int iron_lynch_count = 0;
            for (int i = 0; i < monster_count; ++i)
                if (monsters[i]->doom_type == 6)
                    iron_lynch_count++;
            while (iron_lynch_count < 1)
            {
                int i = rand() % monster_count;
                if (monsters[i]->doom_type != 6)
                {
                    monsters[i] = &random_monster_defs[12];
                    iron_lynch_count++;
                }
            }
        }
        
        // Randomly pick them until empty, and place them in different spots
        for (i = 0; i < spawn_count; i++)
        {
            int idx = rand() % monster_count;
            spawns[i].monster = monsters[idx];
            monsters[idx] = monsters[monster_count - 1];
            monster_count--;
        }

        // Go through again, and make sure they fit
        for (i = 0; i < spawn_count; i++)
        {
            monster_spawn_def_t* spawn1 = &spawns[i];
            if (spawn1->monster->height > spawn1->fit_height ||
                spawn1->monster->radius > spawn1->fit_radius)
            {
                // He doesn't fit here, find another monster randomly that would fit here, then swap
                int tries = 1000;
                while (tries--)
                {
                    int j = rand() % spawn_count;
                    if (j == i) continue;
                    monster_spawn_def_t* spawn2 = &spawns[j];
                    if (spawn1->monster->height <= spawn2->fit_height &&
                        spawn1->monster->radius <= spawn2->fit_radius &&
                        spawn2->monster->height <= spawn1->fit_height &&
                        spawn2->monster->radius <= spawn1->fit_radius)
                    {
                        random_monster_def_t* tmp = spawn1->monster;
                        spawn1->monster = spawn2->monster;
                        spawn2->monster = tmp;
                        break;
                    }
                }
            }
        }

        // Do the final remapping
        for (i = 0; i < spawn_count; i++)
        {
            monster_spawn_def_t* spawn = &spawns[i];
            things_type_remap[spawn->index] = spawn->monster->doom_type;
        }
    }

    if (ap_state.random_items > 0)
    {
        // Make sure at the right difficulty level
        if (gameskill == sk_baby)
            bit = 1;
        else if (gameskill == sk_nightmare)
            bit = 4;
        else
            bit = 1<<(gameskill-1);

        if (ap_state.random_items == 1) // Shuffle
        {
            int items[1024];
            int item_count = 0;
            int indices[1024];
            int index_count = 0;

            // Collect all items
            mt = (mapthing_t *)data;
            for (i = 0; i < numthings; i++, mt++)
            {
                if (mt->options & 16)
                    continue; // Multiplayer item
                if (!(mt->options & bit))
                    continue;

                switch (mt->type)
                {
                    case 54: // Claw Orb
                    case 12: // Crystal Geode
                    case 55: // Energy Orb
                    case 18: // Ethereal Arrows
                    case 22: // Flame Orb
                    case 21: // Greater Runes
                    case 23: // Inferno Orb
                    case 20: // Lesser Runes
                    case 13: // Mace Spheres
                    case 16: // Pile of Mace Spheres
                    case 19: // Quiver of Ethereal Arrows
                    case 10: // Wand Crystal
                    case 81: // Crystal Vial
                    case 82: // Quartz Flask
                    {
                        items[item_count++] = mt->type;
                        indices[index_count++] = i;
                        break;
                    }
                }
            }

            // Randomly pick them until empty, and place them in different spots
            mt = (mapthing_t *)data;
            for (i = 0; i < index_count; i++)
            {
                int idx = rand() % item_count;
                things_type_remap[indices[i]] = items[idx];
                items[idx] = items[item_count - 1];
                item_count--;
            }
        }
        else if (ap_state.random_items == 2) // Random balanced
        {
            int ratios[3] = {0, 0, 0};
            int total = 0;

            // Make sure at the right difficulty level
            if (gameskill == sk_baby)
                bit = 1;
            else if (gameskill == sk_nightmare)
                bit = 4;
            else
                bit = 1<<(gameskill-1);

            // Calculate ratios
            mt = (mapthing_t *)data;
            for (i = 0; i < numthings; i++, mt++)
            {
                if (mt->options & 16)
                    continue; // Multiplayer item

                switch (mt->type)
                {
                    // Tiny
                    case 81: // Crystal Vial
                    case 10: // Wand Crystal
                    case 18: // Ethereal Arrows
                        ratios[0]++;
                        total++;
                        break;

                    // Small
                    case 54: // Claw Orb
                    case 22: // Flame Orb
                    case 20: // Lesser Runes
                    case 13: // Mace Spheres
                        ratios[1]++;
                        total++;
                        break;

                    // Big
                    case 12: // Crystal Geode
                    case 55: // Energy Orb
                    case 21: // Greater Runes
                    case 23: // Inferno Orb
                    case 16: // Pile of Mace Spheres
                    case 19: // Quiver of Ethereal Arrows
                    case 82: // Quartz Flask
                        ratios[2]++;
                        total++;
                        break;
                }
            }

            // Randomly pick items based on ratio
            mt = (mapthing_t *)data;
            for (i = 0; i < numthings; i++, mt++)
            {
                switch (mt->type)
                {
                    case 54: // Claw Orb
                    case 12: // Crystal Geode
                    case 55: // Energy Orb
                    case 18: // Ethereal Arrows
                    case 22: // Flame Orb
                    case 21: // Greater Runes
                    case 23: // Inferno Orb
                    case 20: // Lesser Runes
                    case 13: // Mace Spheres
                    case 16: // Pile of Mace Spheres
                    case 19: // Quiver of Ethereal Arrows
                    case 10: // Wand Crystal
                    case 81: // Crystal Vial
                    {
                        int rnd = rand() % total;
                        if (rnd < ratios[0])
                        {
                            switch (rand()%3)
                            {
                                case 0: things_type_remap[i] = 81; break; // Crystal Vial
                                case 1: things_type_remap[i] = 10; break; // Wand Crystal
                                case 2: things_type_remap[i] = 18; break; // Ethereal Arrows
                            }
                        }
                        else if (rnd < ratios[0] + ratios[1])
                        {
                            switch (rand()%4)
                            {
                                case 0: things_type_remap[i] = 54; break; // Claw Orb
                                case 1: things_type_remap[i] = 22; break; // Flame Orb
                                case 2: things_type_remap[i] = 20; break; // Lesser Runes
                                case 3: things_type_remap[i] = 13; break; // Mace Spheres
                            }
                        }
                        else
                        {
                            switch (rand()%7)
                            {
                                case 0: things_type_remap[i] = 12; break; // Crystal Geode
                                case 1: things_type_remap[i] = 55; break; // Energy Orb
                                case 2: things_type_remap[i] = 21; break; // Greater Runes
                                case 3: things_type_remap[i] = 23; break; // Inferno Orb
                                case 4: things_type_remap[i] = 16; break; // Pile of Mace Spheres
                                case 5: things_type_remap[i] = 19; break; // Quiver of Ethereal Arrows
                                case 6: things_type_remap[i] = 82; break; // Quartz Flask
                            }
                        }
                        break;
                    }
                }
            }
        }
    }


    mt = (mapthing_t *) data;
    for (i = 0; i < numthings; i++, mt++)
    {
        spawnthing.x = SHORT(mt->x);
        spawnthing.y = SHORT(mt->y);
        spawnthing.angle = SHORT(mt->angle);
        spawnthing.type = SHORT(things_type_remap[i]);
        spawnthing.options = SHORT(mt->options);

        // [AP] Rarely, but sometimes we need to move the spawning at the bottom of a clift.
        if (spawnthing.type == 1 && gamemap == 3 && gameepisode == 3) spawnthing.y = -1088;
        if (spawnthing.type == 1 && gamemap == 5 && gameepisode == 5) spawnthing.x = 320;
        
        int type_before = spawnthing.type;

        // Replace AP locations with AP item
        if (is_heretic_type_ap_location(spawnthing.type))
        {
            // Validate that the location index matches what we have in our data. If it doesn't then the WAD is not the same, we can't continue
            int ret = ap_validate_doom_location(ap_make_level_index(gameepisode, gamemap), spawnthing.type, i);
            if (ret == -1)
            {
                I_Error("WAD file doesn't match the one used to generate the logic.\nTo make sure it works as intended, get HERETIC.WAD from the steam releases.");
            }
            else if (ret == 0)
            {
                continue; // Skip it
            }
            else if (ret == 1)
            {
                if (apdoom_is_location_progression(ap_make_level_index(gameepisode, gamemap), i))
                    spawnthing.type = 20001;
                else
                    spawnthing.type = 20000;
                int skip = 0;
                ap_level_state_t* level_state = ap_get_level_state(ap_make_level_index(gameepisode, gamemap));
                for (j = 0; j < level_state->check_count; ++j)
                {
                    if (level_state->checks[j] == i)
                    {
                        skip = 1;
                        break;
                    }
                }
                if (skip)
                    continue;
            }
        }

        // [AP] On player start 1, put level select teleport "HUB"
        if (spawnthing.type == 1)
            spawnthing_player1_start = spawnthing;

	    P_SpawnMapThing(&spawnthing, i);
    }
    
    // [AP] Spawn level select teleport "HUB"
    spawnthing_player1_start.type = 20002;
    P_SpawnMapThing(&spawnthing_player1_start, i);

    if (!deathmatch)
    {
        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (playeringame[i] && !playerstartsingame[i])
            {
                I_Error("P_LoadThings: Player %d start missing (vanilla crashes here)", i + 1);
            }
            playerstartsingame[i] = false;
        }
    }

    W_ReleaseLumpNum(lump);
}



/*
=================
=
= P_LoadLineDefs
=
= Also counts secret lines for intermissions
=================
*/

void P_LoadLineDefs(int lump)
{
    byte *data;
    int i;
    maplinedef_t *mld;
    line_t *ld;
    vertex_t *v1, *v2;

    numlines = W_LumpLength(lump) / sizeof(maplinedef_t);
    lines = Z_Malloc(numlines * sizeof(line_t), PU_LEVEL, 0);
    memset(lines, 0, numlines * sizeof(line_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    mld = (maplinedef_t *) data;
    ld = lines;


    // [AP] Add special line changes
    if (gameepisode == 1 && gamemap == 7)
    {
        mld[779].special = 62;
        mld[779].tag = 3333;
        mld[779].flags &= ~0x0010; // LINE_FLAGS_LOWER_UNPEGGED;
    }


    for (i = 0; i < numlines; i++, mld++, ld++)
    {
        ld->flags = (unsigned short)SHORT(mld->flags); // [crispy] extended nodes
        ld->special = SHORT(mld->special);
        ld->tag = SHORT(mld->tag);
        v1 = ld->v1 = &vertexes[(unsigned short)SHORT(mld->v1)]; // [crispy] extended nodes
        v2 = ld->v2 = &vertexes[(unsigned short)SHORT(mld->v2)]; // [crispy] extended nodes
        ld->dx = v2->x - v1->x;
        ld->dy = v2->y - v1->y;
        if (!ld->dx)
            ld->slopetype = ST_VERTICAL;
        else if (!ld->dy)
            ld->slopetype = ST_HORIZONTAL;
        else
        {
            if (FixedDiv(ld->dy, ld->dx) > 0)
                ld->slopetype = ST_POSITIVE;
            else
                ld->slopetype = ST_NEGATIVE;
        }

        if (v1->x < v2->x)
        {
            ld->bbox[BOXLEFT] = v1->x;
            ld->bbox[BOXRIGHT] = v2->x;
        }
        else
        {
            ld->bbox[BOXLEFT] = v2->x;
            ld->bbox[BOXRIGHT] = v1->x;
        }
        if (v1->y < v2->y)
        {
            ld->bbox[BOXBOTTOM] = v1->y;
            ld->bbox[BOXTOP] = v2->y;
        }
        else
        {
            ld->bbox[BOXBOTTOM] = v2->y;
            ld->bbox[BOXTOP] = v1->y;
        }
        ld->sidenum[0] = SHORT(mld->sidenum[0]);
        ld->sidenum[1] = SHORT(mld->sidenum[1]);
        if (ld->sidenum[0] != NO_INDEX) // [crispy] extended nodes
            ld->frontsector = sides[ld->sidenum[0]].sector;
        else
            ld->frontsector = 0;
        if (ld->sidenum[1] != NO_INDEX) // [crispy] extended nodes
            ld->backsector = sides[ld->sidenum[1]].sector;
        else
            ld->backsector = 0;
    }

    W_ReleaseLumpNum(lump);
}


/*
=================
=
= P_LoadSideDefs
=
=================
*/

void P_LoadSideDefs(int lump)
{
    byte *data;
    int i;
    mapsidedef_t *msd;
    side_t *sd;

    numsides = W_LumpLength(lump) / sizeof(mapsidedef_t);
    sides = Z_Malloc(numsides * sizeof(side_t), PU_LEVEL, 0);
    memset(sides, 0, numsides * sizeof(side_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    msd = (mapsidedef_t *) data;
    
    if (gameepisode == 1 && gamemap == 7)
    {
        sprintf(msd[1119].bottomtexture, "%s", "METL2");
        sprintf(msd[1533].bottomtexture, "%s", "METL2");
        sprintf(msd[2223].bottomtexture, "%s", "METL2");
    }

    sd = sides;
    for (i = 0; i < numsides; i++, msd++, sd++)
    {
        sd->textureoffset = SHORT(msd->textureoffset) << FRACBITS;
        sd->rowoffset = SHORT(msd->rowoffset) << FRACBITS;
        sd->toptexture = R_TextureNumForName(msd->toptexture);
        sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
        sd->midtexture = R_TextureNumForName(msd->midtexture);
        sd->sector = &sectors[SHORT(msd->sector)];
        // [crispy] smooth texture scrolling
        sd->basetextureoffset = sd->textureoffset;
    }

    W_ReleaseLumpNum(lump);
}



/*
=================
=
= P_LoadBlockMap
=
=================
*/

void P_LoadBlockMap(int lump)
{
    int i, count;
    int lumplen;
    short *wadblockmaplump;

    lumplen = W_LumpLength(lump);

    count = lumplen / 2; // [crispy] remove BLOCKMAP limit

    // [crispy] remove BLOCKMAP limit
    wadblockmaplump = Z_Malloc(lumplen, PU_LEVEL, NULL);
    W_ReadLump(lump, wadblockmaplump);
    blockmaplump = Z_Malloc(sizeof(*blockmaplump) * count, PU_LEVEL, NULL);
    blockmap = blockmaplump + 4;

    blockmaplump[0] = SHORT(wadblockmaplump[0]);
    blockmaplump[1] = SHORT(wadblockmaplump[1]);
    blockmaplump[2] = (int32_t)(SHORT(wadblockmaplump[2])) & 0xffff;
    blockmaplump[3] = (int32_t)(SHORT(wadblockmaplump[3])) & 0xffff;

    // Swap all short integers to native byte ordering:

    // count = lumplen / 2; // [crispy] moved up
    for (i=4; i<count; i++)
    {
        short t = SHORT(wadblockmaplump[i]);
        blockmaplump[i] = (t == -1) ? -1l : (int32_t) t & 0xffff;
    }

    Z_Free(wadblockmaplump);

    bmaporgx = blockmaplump[0] << FRACBITS;
    bmaporgy = blockmaplump[1] << FRACBITS;
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];

// clear out mobj chains
    count = sizeof(*blocklinks) * bmapwidth * bmapheight;
    blocklinks = Z_Malloc(count, PU_LEVEL, 0);
    memset(blocklinks, 0, count);
}




/*
=================
=
= P_GroupLines
=
= Builds sector line lists and subsector sector numbers
= Finds block bounding boxes for sectors
=================
*/

void P_GroupLines(void)
{
    line_t **linebuffer;
    int i, j, total;
    line_t *li;
    sector_t *sector;
    subsector_t *ss;
    seg_t *seg;
    fixed_t bbox[4];
    int block;

// look up sector number for each subsector
    ss = subsectors;
    for (i = 0; i < numsubsectors; i++, ss++)
    {
        seg = &segs[ss->firstline];
        ss->sector = seg->sidedef->sector;
    }

// count number of lines in each sector
    li = lines;
    total = 0;
    for (i = 0; i < numlines; i++, li++)
    {
        total++;
        li->frontsector->linecount++;
        if (li->backsector && li->backsector != li->frontsector)
        {
            li->backsector->linecount++;
            total++;
        }
    }

// build line tables for each sector    
    linebuffer = Z_Malloc(total * sizeof(line_t *), PU_LEVEL, 0);
    sector = sectors;
    for (i = 0; i < numsectors; i++, sector++)
    {
        M_ClearBox(bbox);
        sector->lines = linebuffer;
        li = lines;
        for (j = 0; j < numlines; j++, li++)
        {
            if (li->frontsector == sector || li->backsector == sector)
            {
                *linebuffer++ = li;
                M_AddToBox(bbox, li->v1->x, li->v1->y);
                M_AddToBox(bbox, li->v2->x, li->v2->y);
            }
        }
        if (linebuffer - sector->lines != sector->linecount)
            I_Error("P_GroupLines: miscounted");

        // set the degenmobj_t to the middle of the bounding box
        sector->soundorg.x = (bbox[BOXRIGHT] + bbox[BOXLEFT]) / 2;
        sector->soundorg.y = (bbox[BOXTOP] + bbox[BOXBOTTOM]) / 2;

        // adjust bounding box to map blocks
        block = (bbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block >= bmapheight ? bmapheight - 1 : block;
        sector->blockbox[BOXTOP] = block;

        block = (bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXBOTTOM] = block;

        block = (bbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block >= bmapwidth ? bmapwidth - 1 : block;
        sector->blockbox[BOXRIGHT] = block;

        block = (bbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXLEFT] = block;
    }

}

//=============================================================================

// [crispy] remove slime trails
// mostly taken from Lee Killough's implementation in mbfsrc/P_SETUP.C:849-924,
// with the exception that not the actual vertex coordinates are modified,
// but separate coordinates that are *only* used in rendering,
// i.e. r_bsp.c:R_AddLine()

static void P_RemoveSlimeTrails(void)
{
    int i;

    for (i = 0; i < numsegs; i++)
    {
	const line_t *l = segs[i].linedef;
	vertex_t *v = segs[i].v1;

	// [crispy] ignore exactly vertical or horizontal linedefs
	if (l->dx && l->dy)
	{
	    do
	    {
		// [crispy] vertex wasn't already moved
		if (!v->moved)
		{
		    v->moved = true;
		    // [crispy] ignore endpoints of linedefs
		    if (v != l->v1 && v != l->v2)
		    {
			// [crispy] move the vertex towards the linedef
			// by projecting it using the law of cosines
			int64_t dx2 = (l->dx >> FRACBITS) * (l->dx >> FRACBITS);
			int64_t dy2 = (l->dy >> FRACBITS) * (l->dy >> FRACBITS);
			int64_t dxy = (l->dx >> FRACBITS) * (l->dy >> FRACBITS);
			int64_t s = dx2 + dy2;

			// [crispy] MBF actually overrides v->x and v->y here
			v->r_x = (fixed_t)((dx2 * v->x + dy2 * l->v1->x + dxy * (v->y - l->v1->y)) / s);
			v->r_y = (fixed_t)((dy2 * v->y + dx2 * l->v1->y + dxy * (v->x - l->v1->x)) / s);

			// [crispy] wait a minute... moved more than 8 map units?
			// maybe that's a linguortal then, back to the original coordinates
			if (abs(v->r_x - v->x) > 8*FRACUNIT || abs(v->r_y - v->y) > 8*FRACUNIT)
			{
			    v->r_x = v->x;
			    v->r_y = v->y;
			}
		    }
		}
	    // [crispy] if v doesn't point to the second vertex of the seg already, point it there
	    } while ((v != segs[i].v2) && (v = segs[i].v2));
	}
    }
}

/*
=================
=
= P_SetupLevel
=
=================
*/
extern int leveltimesinceload;
void P_SetupLevel(int episode, int map, int playermask, skill_t skill)
{
    int i;
    int parm;
    char lumpname[9];
    int lumpnum;
    mobj_t *mobj;
    mapformat_t crispy_mapformat;

    totalkills = totalitems = totalsecret = 0;
    for (i = 0; i < MAXPLAYERS; i++)
    {
        players[i].killcount = players[i].secretcount
            = players[i].itemcount = 0;
    }
    players[consoleplayer].viewz = 1;   // will be set by player think

    // [crispy] stop demo warp mode now
    if (crispy->demowarp == map)
    {
        crispy->demowarp = 0;
        nodrawers = false;
        singletics = false;
    }

    S_Start();                  // make sure all sounds are stopped before Z_FreeTags

    Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);

    P_InitThinkers();

//
// look for a regular (development) map first
//
    lumpname[0] = 'E';
    lumpname[1] = '0' + episode;
    lumpname[2] = 'M';
    lumpname[3] = '0' + map;
    lumpname[4] = 0;
    leveltime = 0;
    leveltimesinceload = 0;
    oldleveltime = 0;  // [crispy] Track if game is running

    lumpnum = W_GetNumForName(lumpname);

    // [crispy] check and log map and nodes format
    crispy_mapformat = P_CheckMapFormat(lumpnum);

// note: most of this ordering is important     
    P_LoadBlockMap(lumpnum + ML_BLOCKMAP);
    P_LoadVertexes(lumpnum + ML_VERTEXES);
    P_LoadSectors(lumpnum + ML_SECTORS);
    P_LoadSideDefs(lumpnum + ML_SIDEDEFS);

    P_LoadLineDefs(lumpnum + ML_LINEDEFS);

    if (crispy_mapformat & (MFMT_ZDBSPX | MFMT_ZDBSPZ))
    {
        P_LoadNodes_ZDBSP(lumpnum + ML_NODES, crispy_mapformat & MFMT_ZDBSPZ);
    }
    else if (crispy_mapformat & MFMT_DEEPBSP)
    {
        P_LoadSubsectors_DeePBSP(lumpnum + ML_SSECTORS);
        P_LoadNodes_DeePBSP(lumpnum + ML_NODES);
        P_LoadSegs_DeePBSP(lumpnum + ML_SEGS);
    }
    else
    {
    P_LoadSubsectors(lumpnum + ML_SSECTORS);
    P_LoadNodes(lumpnum + ML_NODES);
    P_LoadSegs(lumpnum + ML_SEGS);
    }

    rejectmatrix = W_CacheLumpNum(lumpnum + ML_REJECT, PU_LEVEL);
    P_GroupLines();

    // [crispy] remove slime trails
    P_RemoveSlimeTrails();

    // [crispy] fix long wall wobble
    P_SegLengths();

    bodyqueslot = 0;
    deathmatch_p = deathmatchstarts;
    P_InitAmbientSound();
    P_InitMonsters();
    P_OpenWeapons();
    P_LoadThings(lumpnum + ML_THINGS);
    P_CloseWeapons();

//
// if deathmatch, randomly spawn the active players
//
    TimerGame = 0;
    if (deathmatch)
    {
        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (playeringame[i])
            {                   // must give a player spot before deathmatchspawn
                mobj = P_SpawnMobj(playerstarts[i].x << 16,
                                   playerstarts[i].y << 16, 0, MT_PLAYER);
                players[i].mo = mobj;
                G_DeathMatchSpawnPlayer(i);
                P_RemoveMobj(mobj);
            }
        }

        //!
        // @arg <n>
        // @category net
        // @vanilla
        //
        // For multiplayer games: exit each level after n minutes.
        //

        parm = M_CheckParmWithArgs("-timer", 1);
        if (parm)
        {
            TimerGame = atoi(myargv[parm + 1]) * 35 * 60;
        }
    }

// set up world state
    P_SpawnSpecials();

// build subsector connect matrix
//      P_ConnectSubsectors ();

// preload graphics
    if (precache)
        R_PrecacheLevel();

//printf ("free memory: 0x%x\n", Z_FreeMemory());

}


/*
=================
=
= P_Init
=
=================
*/

void P_Init(void)
{
    P_InitSwitchList();
    P_InitPicAnims();
    P_InitTerrainTypes();
    P_InitLava();
    R_InitSprites(sprnames);
}
