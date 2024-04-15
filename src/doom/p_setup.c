//
// Copyright(C) 1993-1996 Id Software, Inc.
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
// DESCRIPTION:
//	Do all the WAD I/O, get map description,
//	set up initial state and misc. LUTs.
//



#include <math.h>
#include <stdlib.h>

#include "z_zone.h"

#include "deh_main.h"
#include "i_swap.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_misc.h" // [crispy] M_StringJoin()

#include "g_game.h"

#include "i_system.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"
#include "s_musinfo.h" // [crispy] S_ParseMusInfo()

#include "doomstat.h"

#include "p_extnodes.h" // [crispy] support extended node formats

#include "apdoom_c_def.h"
#include "apdoom2_c_def.h"
#include "apdoom_open_world_c_def.h"
#include "apdoom.h"

void	P_SpawnMapThing (mapthing_t*	mthing);

extern int open_world;


//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int		numvertexes;
vertex_t*	vertexes;

int		numsegs;
seg_t*		segs;

int		numsectors;
sector_t*	sectors;

int		numsubsectors;
subsector_t*	subsectors;

int		numnodes;
node_t*		nodes;

int		numlines;
line_t*		lines;

int		numsides;
side_t*		sides;

static int      totallines;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int		bmapwidth;
int		bmapheight;	// size in mapblocks
int32_t*	blockmap;	// int for larger maps // [crispy] BLOCKMAP limit
// offsets in blockmap are from here
int32_t*	blockmaplump; // [crispy] BLOCKMAP limit
// origin of block map
fixed_t		bmaporgx;
fixed_t		bmaporgy;
// for thing chains
mobj_t**	blocklinks;		


// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
byte*		rejectmatrix;


// Maintain single and multi player starting spots.
#define MAX_DEATHMATCH_STARTS	10

mapthing_t	deathmatchstarts[MAX_DEATHMATCH_STARTS];
mapthing_t*	deathmatch_p;
mapthing_t	playerstarts[MAXPLAYERS];
boolean     playerstartsingame[MAXPLAYERS];

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




//
// P_LoadVertexes
//
void P_LoadVertexes (int lump)
{
    byte*		data;
    int			i;
    mapvertex_t*	ml;
    vertex_t*		li;

    // Determine number of lumps:
    //  total lump length / vertex record length.
    numvertexes = W_LumpLength (lump) / sizeof(mapvertex_t);

    // Allocate zone memory for buffer.
    vertexes = Z_Malloc (numvertexes*sizeof(vertex_t),PU_LEVEL,0);	

    // Load data into cache.
    data = W_CacheLumpNum (lump, PU_STATIC);
	
    ml = (mapvertex_t *)data;
    li = vertexes;

    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    for (i=0 ; i<numvertexes ; i++, li++, ml++)
    {
	li->x = SHORT(ml->x)<<FRACBITS;
	li->y = SHORT(ml->y)<<FRACBITS;

	// [crispy] initialize vertex coordinates *only* used in rendering
	li->r_x = li->x;
	li->r_y = li->y;
	li->moved = false;
    }

    // Free buffer memory.
    W_ReleaseLumpNum(lump);
}

//
// GetSectorAtNullAddress
//
sector_t* GetSectorAtNullAddress(void)
{
    static boolean null_sector_is_initialized = false;
    static sector_t null_sector;

    if (!null_sector_is_initialized)
    {
        memset(&null_sector, 0, sizeof(null_sector));
        I_GetMemoryValue(0, &null_sector.floorheight, 4);
        I_GetMemoryValue(4, &null_sector.ceilingheight, 4);
        null_sector_is_initialized = true;
    }

    return &null_sector;
}

//
// P_LoadSegs
//
void P_LoadSegs (int lump)
{
    byte*		data;
    int			i;
    mapseg_t*		ml;
    seg_t*		li;
    line_t*		ldef;
    int			linedef;
    int			side;
    int                 sidenum;
	
    numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
    segs = Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0);	
    memset (segs, 0, numsegs*sizeof(seg_t));
    data = W_CacheLumpNum (lump,PU_STATIC);
	
    ml = (mapseg_t *)data;
    li = segs;
    for (i=0 ; i<numsegs ; i++, li++, ml++)
    {
	li->v1 = &vertexes[(unsigned short)SHORT(ml->v1)]; // [crispy] extended nodes
	li->v2 = &vertexes[(unsigned short)SHORT(ml->v2)]; // [crispy] extended nodes

	li->angle = (SHORT(ml->angle))<<FRACBITS;
//	li->offset = (SHORT(ml->offset))<<FRACBITS; // [crispy] recalculated below
	linedef = (unsigned short)SHORT(ml->linedef); // [crispy] extended nodes
	ldef = &lines[linedef];
	li->linedef = ldef;
	side = SHORT(ml->side);

	// e6y: check for wrong indexes
	if ((unsigned)linedef >= (unsigned)numlines)
	{
		I_Error("P_LoadSegs: seg %d references a non-existent linedef %d",
			i, (unsigned)linedef);
	}
	if ((unsigned)ldef->sidenum[side] >= (unsigned)numsides)
	{
		I_Error("P_LoadSegs: linedef %d for seg %d references a non-existent sidedef %d",
			linedef, i, (unsigned)ldef->sidenum[side]);
	}

	li->sidedef = &sides[ldef->sidenum[side]];
	li->frontsector = sides[ldef->sidenum[side]].sector;
	// [crispy] recalculate
	li->offset = GetOffset(li->v1, (ml->side ? ldef->v2 : ldef->v1));

        if (ldef-> flags & ML_TWOSIDED)
        {
            sidenum = ldef->sidenum[side ^ 1];

            // If the sidenum is out of range, this may be a "glass hack"
            // impassible window.  Point at side #0 (this may not be
            // the correct Vanilla behavior; however, it seems to work for
            // OTTAWAU.WAD, which is the one place I've seen this trick
            // used).

            if (sidenum < 0 || sidenum >= numsides)
            {
                // [crispy] linedef has two-sided flag set, but no valid second sidedef;
                // but since it has a midtexture, it is supposed to be rendered just
                // like a regular one-sided linedef
                if (li->sidedef->midtexture)
                {
                    li->backsector = 0;
                    fprintf(stderr, "P_LoadSegs: Linedef %d has two-sided flag set, but no second sidedef\n", linedef);
                }
                else
                li->backsector = GetSectorAtNullAddress();
            }
            else
            {
                li->backsector = sides[sidenum].sector;
            }
        }
        else
        {
	    li->backsector = 0;
        }
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

void P_SegLengths (boolean contrast_only)
{
    int i;
    const int rightangle = abs(finesine[(ANG60/2) >> ANGLETOFINESHIFT]);

    for (i = 0; i < numsegs; i++)
    {
	seg_t *const li = &segs[i];
	int64_t dx, dy;

	dx = li->v2->r_x - li->v1->r_x;
	dy = li->v2->r_y - li->v1->r_y;

	if (!contrast_only)
	{
		li->length = (uint32_t)(sqrt((double)dx*dx + (double)dy*dy)/2);

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

	// [crispy] smoother fake contrast
	if (!dy)
	    li->fakecontrast = -LIGHTBRIGHT;
	else
	if (abs(finesine[li->r_angle >> ANGLETOFINESHIFT]) < rightangle)
	    li->fakecontrast = -(LIGHTBRIGHT >> 1);
	else
	if (!dx)
	    li->fakecontrast = LIGHTBRIGHT;
	else
	if (abs(finecosine[li->r_angle >> ANGLETOFINESHIFT]) < rightangle)
	    li->fakecontrast = LIGHTBRIGHT >> 1;
	else
	    li->fakecontrast = 0;
    }
}

//
// P_LoadSubsectors
//
void P_LoadSubsectors (int lump)
{
    byte*		data;
    int			i;
    mapsubsector_t*	ms;
    subsector_t*	ss;
	
    numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
    subsectors = Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);	
    data = W_CacheLumpNum (lump,PU_STATIC);
	
    // [crispy] fail on missing subsectors
    if (!data || !numsubsectors)
	I_Error("P_LoadSubsectors: No subsectors in map!");

    ms = (mapsubsector_t *)data;
    memset (subsectors,0, numsubsectors*sizeof(subsector_t));
    ss = subsectors;
    
    for (i=0 ; i<numsubsectors ; i++, ss++, ms++)
    {
	ss->numlines = (unsigned short)SHORT(ms->numsegs); // [crispy] extended nodes
	ss->firstline = (unsigned short)SHORT(ms->firstseg); // [crispy] extended nodes
    }
	
    W_ReleaseLumpNum(lump);
}



//
// P_LoadSectors
//
void P_LoadSectors (int lump)
{
    byte*		data;
    int			i;
    mapsector_t*	ms;
    sector_t*		ss;
	
    // [crispy] fail on missing sectors
    if (lump >= numlumps)
	I_Error("P_LoadSectors: No sectors in map!");

    numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
    sectors = Z_Malloc (numsectors*sizeof(sector_t),PU_LEVEL,0);	
    memset (sectors, 0, numsectors*sizeof(sector_t));
    data = W_CacheLumpNum (lump,PU_STATIC);
	
    // [crispy] fail on missing sectors
    if (!data || !numsectors)
	I_Error("P_LoadSectors: No sectors in map!");

    ms = (mapsector_t *)data;

    if (gamemode == commercial)
    {
        // Doom II
        if (gamemap == 12)
        {
            ms[132].tag = 42;
            sprintf(ms[132].floorpic, "STEP1");
        }
    }

    ss = sectors;
    for (i=0 ; i<numsectors ; i++, ss++, ms++)
    {
	ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
	ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;
	ss->floorpic = R_FlatNumForName(ms->floorpic);
	ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
	ss->lightlevel = SHORT(ms->lightlevel);
	// [crispy] A11Y light level used for rendering
	ss->rlightlevel = ss->lightlevel;
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


//
// P_LoadNodes
//
void P_LoadNodes (int lump)
{
    byte*	data;
    int		i;
    int		j;
    int		k;
    mapnode_t*	mn;
    node_t*	no;
	
    numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
    nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);	
    data = W_CacheLumpNum (lump,PU_STATIC);
	
    // [crispy] warn about missing nodes
    if (!data || !numnodes)
    {
	if (numsubsectors == 1)
	    fprintf(stderr, "P_LoadNodes: No nodes in map, but only one subsector.\n");
	else
	    I_Error("P_LoadNodes: No nodes in map!");
    }

    mn = (mapnode_t *)data;
    no = nodes;
    
    for (i=0 ; i<numnodes ; i++, no++, mn++)
    {
	no->x = SHORT(mn->x)<<FRACBITS;
	no->y = SHORT(mn->y)<<FRACBITS;
	no->dx = SHORT(mn->dx)<<FRACBITS;
	no->dy = SHORT(mn->dy)<<FRACBITS;
	for (j=0 ; j<2 ; j++)
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

	    for (k=0 ; k<4 ; k++)
		no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
	}
    }
	
    W_ReleaseLumpNum(lump);
}


static unsigned long long hash_seed(unsigned char *str)
{
    unsigned long long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
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


static random_monster_def_t doom_random_monster_defs[] =
{
    { 3004, 20, 20*FRACUNIT, 56*FRACUNIT, 384, rmc_small }, // Former Human
    { 9, 30, 20*FRACUNIT, 56*FRACUNIT, 656, rmc_small }, // Former Human Sergeant
    { 3001, 60, 20*FRACUNIT, 56*FRACUNIT, 974, rmc_small }, // Imp

    { 3002, 150, 30*FRACUNIT, 56*FRACUNIT, 443, rmc_medium }, // Demon
    { 58, 150, 30*FRACUNIT, 56*FRACUNIT, 183, rmc_medium }, // SPECTRE
    { 3006, 100, 16*FRACUNIT, 56*FRACUNIT, 323, rmc_medium }, // Lost soul
    { 3005, 400, 31*FRACUNIT, 56*FRACUNIT, 193, rmc_medium }, // Cacodemon

    { 3003, 1000, 24*FRACUNIT, 64*FRACUNIT, 82, rmc_big }, // Baron of hell

    { 16, 4000, 40*FRACUNIT, 110*FRACUNIT, 5, rmc_boss, 1 }, // Cyberdemon
    { 7, 3000, 128*FRACUNIT, 100*FRACUNIT, 2, rmc_boss, 1 } // Spiderdemon
};


static random_monster_def_t doom2_random_monster_defs[] =
{
    { 3004, 20, 20*FRACUNIT, 56*FRACUNIT, 290, rmc_small }, // Former Human
    { 9, 30, 20*FRACUNIT, 56*FRACUNIT, 483, rmc_small }, // Former Human Sergeant
    { 3001, 60, 20*FRACUNIT, 56*FRACUNIT, 1223, rmc_small }, // Imp

    { 3002, 150, 30*FRACUNIT, 56*FRACUNIT, 331, rmc_medium }, // Demon
    { 58, 150, 30*FRACUNIT, 56*FRACUNIT, 175, rmc_medium }, // SPECTRE
    { 3006, 100, 16*FRACUNIT, 56*FRACUNIT, 197, rmc_medium }, // Lost soul
    { 3005, 400, 31*FRACUNIT, 56*FRACUNIT, 188, rmc_medium }, // Cacodemon

    { 3003, 1000, 24*FRACUNIT, 64*FRACUNIT, 31, rmc_big }, // Baron of hell

    { 16, 4000, 40*FRACUNIT, 110*FRACUNIT, 12, rmc_boss, 1 }, // Cyberdemon
    { 7, 3000, 128*FRACUNIT, 100*FRACUNIT, 6, rmc_boss, 1 }, // Spiderdemon

    { 84, 50, 20*FRACUNIT, 56*FRACUNIT, 156, rmc_small }, // Wolf SS
    { 65, 70, 20*FRACUNIT, 56*FRACUNIT, 273, rmc_small }, // Former Human Commando

    { 71, 400, 31*FRACUNIT, 56*FRACUNIT, 64, rmc_medium }, // Pain Elemental
    { 67, 600, 48*FRACUNIT, 64*FRACUNIT, 68, rmc_medium }, // Mancubus
    { 68, 500, 64*FRACUNIT, 64*FRACUNIT, 65, rmc_medium }, // Arachnotron
    { 66, 300, 20*FRACUNIT, 56*FRACUNIT, 90, rmc_medium }, // Revenant

    { 64, 700, 20*FRACUNIT, 56*FRACUNIT, 17, rmc_big }, // Archvile
    { 69, 500, 24*FRACUNIT, 64*FRACUNIT, 75, rmc_big } // Hell Knight
};


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


//
// P_LoadThings
//
void P_LoadThings (int lump)
{
    byte               *data;
    int			i, j;
    mapthing_t         *mt;
    mapthing_t          spawnthing;
    mapthing_t  spawnthing_player1_start;
    int			numthings;
    boolean		spawn;
    int bit;

    data = W_CacheLumpNum (lump,PU_STATIC);
    numthings = W_LumpLength (lump) / sizeof(mapthing_t);

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

#define E1M8_CUTOFF_OFFSET 6176

    int do_random_monsters = ap_state.random_monsters;
    if (gamemode == commercial && gamemap == 7) do_random_monsters = 0;

    if (do_random_monsters > 0)
    {
        random_monster_def_t* random_monster_defs = gamemode == commercial ? doom2_random_monster_defs : doom_random_monster_defs;
        int monster_def_count = gamemode == commercial ?
            sizeof(doom2_random_monster_defs) / sizeof(random_monster_def_t) : 
            sizeof(doom_random_monster_defs) / sizeof(random_monster_def_t);

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

            if (gameepisode == 1 && gamemap == 8 && gamemode != commercial)
                if (mt->y > E1M8_CUTOFF_OFFSET)
                    continue;

            for (int j = 0; j < monster_def_count; ++j)
            {
                if (random_monster_defs[j].dont_shuffle)
                    continue;
                if (random_monster_defs[j].doom_type == mt->type)
                {
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

        // Make sure we have at least 2 baron of hell in first episode boss level
        if (gameepisode == 1 && gamemap == 8 && gamemode != commercial)
        {
            int baron_count = 0;
            for (int i = 0; i < monster_count; ++i)
                if (monsters[i]->doom_type == 3003)
                    baron_count++;
            while (baron_count < 2)
            {
                int i = rand() % monster_count;
                if (monsters[i]->doom_type != 3003)
                {
                    monsters[i] = &random_monster_defs[7];
                    baron_count++;
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
                    case 2008: // 4 shotgun shells
                    case 2048: // box of bullets
                    case 2046: // box of rockets
                    case 2049: // box of shotgun shells
                    case 2007: // clip
                    case 2047: // energy cell
                    case 17: // energy cell pack
                    case 2010: // rocket
                    case 2015: // armor bonus
                    case 2014: // health bonus
                    case 2012: // medikit
                    case 2011: // Stimpack
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
                    case 2015: // armor bonus
                    case 2014: // health bonus
                        ratios[0]++;
                        total++;
                        break;

                    case 2011: // Stimpack
                    case 2008: // 4 shotgun shells
                    case 2007: // clip
                    case 2047: // energy cell
                    case 2010: // rocket
                        ratios[1]++;
                        total++;
                        break;

                    case 2048: // box of bullets
                    case 2046: // box of rockets
                    case 2049: // box of shotgun shells
                    case 17: // energy cell pack
                    case 2012: // medikit
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
                    case 2008: // 4 shotgun shells
                    case 2048: // box of bullets
                    case 2046: // box of rockets
                    case 2049: // box of shotgun shells
                    case 2007: // clip
                    case 2047: // energy cell
                    case 17: // energy cell pack
                    case 2010: // rocket
                    case 2015: // armor bonus
                    case 2014: // health bonus
                    case 2012: // medikit
                    case 2011: // Stimpack
                    {
                        int rnd = rand() % total;
                        if (rnd < ratios[0])
                        {
                            switch (rand()%2)
                            {
                                case 0: things_type_remap[i] = 2015; break; // armor bonus
                                case 1: things_type_remap[i] = 2014; break; // health bonus
                            }
                        }
                        else if (rnd < ratios[0] + ratios[1])
                        {
                            switch (rand()%5)
                            {
                                case 0: things_type_remap[i] = 2011; break; // Stimpack
                                case 1: things_type_remap[i] = 2008; break; // 4 shotgun shells
                                case 2: things_type_remap[i] = 2007; break; // clip
                                case 3: things_type_remap[i] = 2047; break; // energy cell
                                case 4: things_type_remap[i] = 2010; break; // rocket
                            }
                        }
                        else
                        {
                            switch (rand()%5)
                            {
                                case 0: things_type_remap[i] = 2048; break; // box of bullets
                                case 1: things_type_remap[i] = 2046; break; // box of rockets
                                case 2: things_type_remap[i] = 2049; break; // box of shotgun shells
                                case 3: things_type_remap[i] = 17; break; // energy cell pack
                                case 4: things_type_remap[i] = 2012; break; // medikit
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
	
    mt = (mapthing_t *)data;
    for (i=0 ; i<numthings ; i++, mt++)
    {
	spawn = true;

	// Do not spawn cool, new monsters if !commercial
	if (gamemode != commercial)
	{
	    switch (SHORT(mt->type))
	    {
	      case 68:	// Arachnotron
	      case 64:	// Archvile
	      case 88:	// Boss Brain
	      case 89:	// Boss Shooter
	      case 69:	// Hell Knight
	      case 67:	// Mancubus
	      case 71:	// Pain Elemental
	      case 65:	// Former Human Commando
	      case 66:	// Revenant
	      case 84:	// Wolf SS
		spawn = false;
		break;
	    }
	}
	if (spawn == false)
	    break;

	// Do spawn all other stuff. 
	spawnthing.x = SHORT(mt->x);
	spawnthing.y = SHORT(mt->y);
	spawnthing.angle = SHORT(mt->angle);
	spawnthing.type = SHORT(things_type_remap[i]);
	spawnthing.options = SHORT(mt->options);
	
    auto type_before = spawnthing.type;

        // Replace AP locations with AP item
        if ((gamemode != commercial && !open_world && is_doom_type_ap_location(spawnthing.type)) ||
            (gamemode != commercial && open_world && is_doom_open_world_type_ap_location(spawnthing.type)) ||
            (gamemode == commercial && is_doom2_type_ap_location(spawnthing.type)))
        {
            // Validate that the location index matches what we have in our data. If it doesn't then the WAD is not the same, we can't continue
            int ret = ap_validate_doom_location(ap_make_level_index(gameepisode, gamemap), spawnthing.type, i);
            if (ret == -1)
            {
                I_Error("WAD file doesn't match the one used to generate the logic.\nTo make sure it works as intended, get DOOM.WAD or DOOM2.WAD from the steam releases.");
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


//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs (int lump)
{
    byte*		data;
    int			i;
    maplinedef_t*	mld;
    line_t*		ld;
    vertex_t*		v1;
    vertex_t*		v2;
    int warn, warn2; // [crispy] warn about invalid linedefs
	
    numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
    lines = Z_Malloc (numlines*sizeof(line_t),PU_LEVEL,0);	
    memset (lines, 0, numlines*sizeof(line_t));
    data = W_CacheLumpNum (lump,PU_STATIC);
	
    mld = (maplinedef_t *)data;
    ld = lines;
    warn = warn2 = 0; // [crispy] warn about invalid linedefs



    // [AP] If the multiworld was generacted with 2 way keydoors, we need to fix those doors to be 2 ways
    if (gamemode == commercial)
    {
        // Doom II
        if (gamemap == 2)
        {
            mld[390].special = 1;
        }
        else if (gamemap == 12)
        {
            mld[271].special = 62;
            mld[271].tag = 42;
            mld[271].flags &= ~0x0010; // LINE_FLAGS_LOWER_UNPEGGED;
        }
    }
    else
    {
        // Ultimate Doom
        if (gameepisode == 2 && gamemap == 6 && ap_state.two_ways_keydoors)
            mld[620].special = 27; // Yellow keycard
        else if (gameepisode == 3 && gamemap == 9 && ap_state.two_ways_keydoors)
            mld[195].special = 32; // Blue keycard

        // [AP] Can be softlocked if coming back to that level after boss is dead, make sure to disable it's triggers that closes the doors
        else if (gameepisode == 2 && gamemap == 8)
        {
            for (i = 140; i <= 143; ++i)
            {
                mld[i].special = 0;
                mld[i].tag = 0;
            }
        }

        // [AP] We can get stuck and not able to come back to the HUB. Make sure the entrance door can be re-openned from the other side
        else if (gameepisode == 4 && gamemap == 8)
            mld[96].special = 61; // Stay open
    }



    for (i=0 ; i<numlines ; i++, mld++, ld++)
    {
	ld->flags = (unsigned short)SHORT(mld->flags); // [crispy] extended nodes
	ld->special = SHORT(mld->special);
	// [crispy] warn about unknown linedef types
	if ((unsigned short) ld->special > 141 && ld->special != 271 && ld->special != 272)
	{
	    fprintf(stderr, "P_LoadLineDefs: Unknown special %d at line %d.\n", ld->special, i);
	    warn++;
	}
	ld->tag = SHORT(mld->tag);
	// [crispy] warn about special linedefs without tag
	if (ld->special && !ld->tag)
	{
	    switch (ld->special)
	    {
		case 1:	// Vertical Door
		case 26:	// Blue Door/Locked
		case 27:	// Yellow Door /Locked
		case 28:	// Red Door /Locked
		case 31:	// Manual door open
		case 32:	// Blue locked door open
		case 33:	// Red locked door open
		case 34:	// Yellow locked door open
		case 117:	// Blazing door raise
		case 118:	// Blazing door open
		case 271:	// MBF sky transfers
		case 272:
		case 48:	// Scroll Wall Left
		case 85:	// [crispy] [JN] (Boom) Scroll Texture Right
		case 11:	// s1 Exit level
		case 51:	// s1 Secret exit
		case 52:	// w1 Exit level
		case 124:	// w1 Secret exit
		    break;
		default:
		    fprintf(stderr, "P_LoadLineDefs: Special linedef %d without tag.\n", i);
		    warn2++;
		    break;
	    }
	}
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
	    if (FixedDiv (ld->dy , ld->dx) > 0)
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

	// [crispy] calculate sound origin of line to be its midpoint
	ld->soundorg.x = ld->bbox[BOXLEFT] / 2 + ld->bbox[BOXRIGHT] / 2;
	ld->soundorg.y = ld->bbox[BOXTOP] / 2 + ld->bbox[BOXBOTTOM] / 2;

	ld->sidenum[0] = SHORT(mld->sidenum[0]);
	ld->sidenum[1] = SHORT(mld->sidenum[1]);

	// [crispy] substitute dummy sidedef for missing right side
	if (ld->sidenum[0] == NO_INDEX)
	{
	    ld->sidenum[0] = 0;
	    fprintf(stderr, "P_LoadLineDefs: linedef %d without first sidedef!\n", i);
	}

	if (ld->sidenum[0] != NO_INDEX) // [crispy] extended nodes
	    ld->frontsector = sides[ld->sidenum[0]].sector;
	else
	    ld->frontsector = 0;

	if (ld->sidenum[1] != NO_INDEX) // [crispy] extended nodes
	    ld->backsector = sides[ld->sidenum[1]].sector;
	else
	    ld->backsector = 0;
    }

    // [crispy] warn about unknown linedef types
    if (warn)
    {
	fprintf(stderr, "P_LoadLineDefs: Found %d line%s with unknown linedef type.\n", warn, (warn > 1) ? "s" : "");
    }
    // [crispy] warn about special linedefs without tag
    if (warn2)
    {
	fprintf(stderr, "P_LoadLineDefs: Found %d special linedef%s without tag.\n", warn2, (warn2 > 1) ? "s" : "");
    }
    if (warn || warn2)
    {
	fprintf(stderr, "THIS MAP MAY NOT WORK AS EXPECTED!\n");
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadSideDefs
//
void P_LoadSideDefs (int lump)
{
    byte*		data;
    int			i;
    mapsidedef_t*	msd;
    side_t*		sd;
	
    numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
    sides = Z_Malloc (numsides*sizeof(side_t),PU_LEVEL,0);	
    memset (sides, 0, numsides*sizeof(side_t));
    data = W_CacheLumpNum (lump,PU_STATIC);
	
    msd = (mapsidedef_t *)data;

    if (gamemode == commercial)
    {
        // Doom II
        if (gamemap == 2)
        {
            sprintf(msd[584].midtexture, "%s", "SHAWN2");
            sprintf(msd[589].midtexture, "%s", "SHAWN2");
        }
        else if (gamemap == 12)
        {
            memcpy(msd[366].bottomtexture, "SUPPORT2", 8);
            memcpy(msd[316].bottomtexture, "SUPPORT2", 8);
        }
    }

    sd = sides;
    for (i=0 ; i<numsides ; i++, msd++, sd++)
    {
	sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
	sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;
	sd->toptexture = R_TextureNumForName(msd->toptexture);
	sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
	sd->midtexture = R_TextureNumForName(msd->midtexture);
	sd->sector = &sectors[SHORT(msd->sector)];
	// [crispy] smooth texture scrolling
	sd->basetextureoffset = sd->textureoffset;
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadBlockMap
//
boolean P_LoadBlockMap (int lump)
{
    int i;
    int count;
    int lumplen;
    short *wadblockmaplump;

    // [crispy] (re-)create BLOCKMAP if necessary
    if (M_CheckParm("-blockmap") ||
        lump >= numlumps ||
        (lumplen = W_LumpLength(lump)) < 8 ||
        (count = lumplen / 2) >= 0x10000)
    {
	return false;
    }
	
    // [crispy] remove BLOCKMAP limit
    // adapted from boom202s/P_SETUP.C:1025-1076
    wadblockmaplump = Z_Malloc(lumplen, PU_LEVEL, NULL);
    W_ReadLump(lump, wadblockmaplump);
    blockmaplump = Z_Malloc(sizeof(*blockmaplump) * count, PU_LEVEL, NULL);
    blockmap = blockmaplump + 4;

    blockmaplump[0] = SHORT(wadblockmaplump[0]);
    blockmaplump[1] = SHORT(wadblockmaplump[1]);
    blockmaplump[2] = (int32_t)(SHORT(wadblockmaplump[2])) & 0xffff;
    blockmaplump[3] = (int32_t)(SHORT(wadblockmaplump[3])) & 0xffff;

    // Swap all short integers to native byte ordering.
  
    for (i=4; i<count; i++)
    {
	short t = SHORT(wadblockmaplump[i]);
	blockmaplump[i] = (t == -1) ? -1l : (int32_t) t & 0xffff;
    }

    Z_Free(wadblockmaplump);
		
    // Read the header

    bmaporgx = blockmaplump[0]<<FRACBITS;
    bmaporgy = blockmaplump[1]<<FRACBITS;
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];
	
    // Clear out mobj chains

    count = sizeof(*blocklinks) * bmapwidth * bmapheight;
    blocklinks = Z_Malloc(count, PU_LEVEL, 0);
    memset(blocklinks, 0, count);

    // [crispy] (re-)create BLOCKMAP if necessary
    fprintf(stderr, ")\n");
    return true;
}



//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void P_GroupLines (void)
{
    line_t**		linebuffer;
    int			i;
    int			j;
    line_t*		li;
    sector_t*		sector;
    subsector_t*	ss;
    seg_t*		seg;
    fixed_t		bbox[4];
    int			block;
	
    // look up sector number for each subsector
    ss = subsectors;
    for (i=0 ; i<numsubsectors ; i++, ss++)
    {
	seg = &segs[ss->firstline];
	ss->sector = seg->sidedef->sector;
    }

    // count number of lines in each sector
    li = lines;
    totallines = 0;
    for (i=0 ; i<numlines ; i++, li++)
    {
	totallines++;
	li->frontsector->linecount++;

	if (li->backsector && li->backsector != li->frontsector)
	{
	    li->backsector->linecount++;
	    totallines++;
	}
    }

    // build line tables for each sector	
    linebuffer = Z_Malloc (totallines*sizeof(line_t *), PU_LEVEL, 0);

    for (i=0; i<numsectors; ++i)
    {
        // Assign the line buffer for this sector

        sectors[i].lines = linebuffer;
        linebuffer += sectors[i].linecount;

        // Reset linecount to zero so in the next stage we can count
        // lines into the list.

        sectors[i].linecount = 0;
    }

    // Assign lines to sectors

    for (i=0; i<numlines; ++i)
    { 
        li = &lines[i];

        if (li->frontsector != NULL)
        {
            sector = li->frontsector;

            sector->lines[sector->linecount] = li;
            ++sector->linecount;
        }

        if (li->backsector != NULL && li->frontsector != li->backsector)
        {
            sector = li->backsector;

            sector->lines[sector->linecount] = li;
            ++sector->linecount;
        }
    }
    
    // Generate bounding boxes for sectors
	
    sector = sectors;
    for (i=0 ; i<numsectors ; i++, sector++)
    {
	M_ClearBox (bbox);

	for (j=0 ; j<sector->linecount; j++)
	{
            li = sector->lines[j];

            M_AddToBox (bbox, li->v1->x, li->v1->y);
            M_AddToBox (bbox, li->v2->x, li->v2->y);
	}

	// set the degenmobj_t to the middle of the bounding box
	sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
	sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;
		
	// adjust bounding box to map blocks
	block = (bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block >= bmapheight ? bmapheight-1 : block;
	sector->blockbox[BOXTOP]=block;

	block = (bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block < 0 ? 0 : block;
	sector->blockbox[BOXBOTTOM]=block;

	block = (bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block >= bmapwidth ? bmapwidth-1 : block;
	sector->blockbox[BOXRIGHT]=block;

	block = (bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block < 0 ? 0 : block;
	sector->blockbox[BOXLEFT]=block;
    }
	
}

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

// Pad the REJECT lump with extra data when the lump is too small,
// to simulate a REJECT buffer overflow in Vanilla Doom.

static void PadRejectArray(byte *array, unsigned int len)
{
    unsigned int i;
    unsigned int byte_num;
    byte *dest;
    unsigned int padvalue;

    // Values to pad the REJECT array with:

    unsigned int rejectpad[4] =
    {
        0,                                    // Size
        0,                                    // Part of z_zone block header
        50,                                   // PU_LEVEL
        0x1d4a11                              // DOOM_CONST_ZONEID
    };

    rejectpad[0] = ((totallines * 4 + 3) & ~3) + 24;

    // Copy values from rejectpad into the destination array.

    dest = array;

    for (i=0; i<len && i<sizeof(rejectpad); ++i)
    {
        byte_num = i % 4;
        *dest = (rejectpad[i / 4] >> (byte_num * 8)) & 0xff;
        ++dest;
    }

    // We only have a limited pad size.  Print a warning if the
    // REJECT lump is too small.

    if (len > sizeof(rejectpad))
    {
        fprintf(stderr, "PadRejectArray: REJECT lump too short to pad! (%u > %i)\n",
                        len, (int) sizeof(rejectpad));

        // Pad remaining space with 0 (or 0xff, if specified on command line).

        if (M_CheckParm("-reject_pad_with_ff"))
        {
            padvalue = 0xff;
        }
        else
        {
            padvalue = 0x00;
        }

        memset(array + sizeof(rejectpad), padvalue, len - sizeof(rejectpad));
    }
}

static void P_LoadReject(int lumpnum)
{
    int minlength;
    int lumplen;

    // Calculate the size that the REJECT lump *should* be.

    minlength = (numsectors * numsectors + 7) / 8;

    // If the lump meets the minimum length, it can be loaded directly.
    // Otherwise, we need to allocate a buffer of the correct size
    // and pad it with appropriate data.

    lumplen = W_LumpLength(lumpnum);

    if (lumplen >= minlength)
    {
        rejectmatrix = W_CacheLumpNum(lumpnum, PU_LEVEL);
    }
    else
    {
        rejectmatrix = Z_Malloc(minlength, PU_LEVEL, &rejectmatrix);
        W_ReadLump(lumpnum, rejectmatrix);

        PadRejectArray(rejectmatrix + lumplen, minlength - lumplen);
    }
}

// [crispy] log game skill in plain text
const char *skilltable[] =
{
    "Nothing",
    "Baby",
    "Easy",
    "Normal",
    "Hard",
    "Nightmare"
};

// [crispy] factor out map lump name and number finding into a separate function
int P_GetNumForMap (int episode, int map, boolean critical)
{
    char lumpname[9];

    // find map name
    if ( gamemode == commercial)
    {
	if (map<10)
	    DEH_snprintf(lumpname, 9, "map0%i", map);
	else
	    DEH_snprintf(lumpname, 9, "map%i", map);
    }
    else
    {
	lumpname[0] = 'E';
	lumpname[1] = '0' + episode;
	lumpname[2] = 'M';
	lumpname[3] = '0' + map;
	lumpname[4] = 0;
    }

    // [crispy] special-casing for E1M10 "Sewers" support
    if (crispy->havee1m10 && episode == 1 && map == 10)
    {
	DEH_snprintf(lumpname, 9, "E1M10");
    }

    // [crispy] NRFTL / The Master Levels
    if (crispy->havenerve && episode == 2 && map <= 9)
    {
	strcat(lumpname, "N");
    }
    if (crispy->havemaster && episode == 3 && map <= 21)
    {
	strcat(lumpname, "M");
    }

    return critical ? W_GetNumForName(lumpname) : W_CheckNumForName(lumpname);
}

// pointer to the current map lump info struct
lumpinfo_t *maplumpinfo;

//
// P_SetupLevel
//
void
P_SetupLevel
( int		episode,
  int		map,
  int		playermask,
  skill_t	skill)
{
    int		i;
    char	lumpname[9];
    int		lumpnum;
    boolean	crispy_validblockmap;
    mapformat_t	crispy_mapformat;
	
    totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
    // [crispy] count spawned monsters
    extrakills = 0;
    wminfo.partime = 180;
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	players[i].killcount = players[i].secretcount 
	    = players[i].itemcount = 0;
    }

    // [crispy] NRFTL / The Master Levels
    if (crispy->havenerve || crispy->havemaster)
    {
        if (crispy->havemaster && episode == 3)
        {
            gamemission = pack_master;
        }
        else
        if (crispy->havenerve && episode == 2)
        {
            gamemission = pack_nerve;
        }
        else
        {
            gamemission = doom2;
        }
    }
    else
    {
        if (gamemission == pack_master)
        {
            episode = gameepisode = 3;
        }
        else
        if (gamemission == pack_nerve)
        {
            episode = gameepisode = 2;
        }
    }

    // Initial height of PointOfView
    // will be set by player think.
    players[consoleplayer].viewz = 1; 

    // [crispy] stop demo warp mode now
    if (crispy->demowarp == map)
    {
	crispy->demowarp = 0;
	nodrawers = false;
	singletics = false;
    }

    // [crispy] don't load map's default music if loaded from a savegame with MUSINFO data
    if (!musinfo.from_savegame)
    {
    // Make sure all sounds are stopped before Z_FreeTags.
    S_Start ();			
    }
    musinfo.from_savegame = false;

    Z_FreeTags (PU_LEVEL, PU_PURGELEVEL-1);

    // UNUSED W_Profile ();
    P_InitThinkers ();

    // if working with a devlopment map, reload it
    W_Reload ();

// [crispy] factor out map lump name and number finding into a separate function
/*
    // find map name
    if ( gamemode == commercial)
    {
	if (map<10)
	    DEH_snprintf(lumpname, 9, "map0%i", map);
	else
	    DEH_snprintf(lumpname, 9, "map%i", map);
    }
    else
    {
	lumpname[0] = 'E';
	lumpname[1] = '0' + episode;
	lumpname[2] = 'M';
	lumpname[3] = '0' + map;
	lumpname[4] = 0;
    }

    lumpnum = W_GetNumForName (lumpname);
*/
    lumpnum = P_GetNumForMap (episode, map, true);
	
    maplumpinfo = lumpinfo[lumpnum];
    strncpy(lumpname, maplumpinfo->name, 8);

    leveltime = 0;
    leveltimesinceload = 0;
    oldleveltime = 0;
	
    // [crispy] better logging
    {
	extern int savedleveltime;
	const int ltime = savedleveltime / TICRATE,
	          ttime = (totalleveltimes + savedleveltime) / TICRATE;
	char *rfn_str;

	rfn_str = M_StringJoin(
	    respawnparm ? " -respawn" : "",
	    fastparm ? " -fast" : "",
	    nomonsters ? " -nomonsters" : "",
	    NULL);

	fprintf(stderr, "P_SetupLevel: %s (%s) %s%s %d:%02d:%02d/%d:%02d:%02d ",
	    maplumpinfo->name, W_WadNameForLump(maplumpinfo),
	    skilltable[BETWEEN(0,5,(int) skill+1)], rfn_str,
	    ltime/3600, (ltime%3600)/60, ltime%60,
	    ttime/3600, (ttime%3600)/60, ttime%60);

	free(rfn_str);
    }
    // [crispy] check and log map and nodes format
    crispy_mapformat = P_CheckMapFormat(lumpnum);

    // note: most of this ordering is important	
    crispy_validblockmap = P_LoadBlockMap (lumpnum+ML_BLOCKMAP); // [crispy] (re-)create BLOCKMAP if necessary
    P_LoadVertexes (lumpnum+ML_VERTEXES);
    P_LoadSectors (lumpnum+ML_SECTORS);
    P_LoadSideDefs (lumpnum+ML_SIDEDEFS);

    if (crispy_mapformat & MFMT_HEXEN)
	P_LoadLineDefs_Hexen (lumpnum+ML_LINEDEFS);
    else
    P_LoadLineDefs (lumpnum+ML_LINEDEFS);
    // [crispy] (re-)create BLOCKMAP if necessary
    if (!crispy_validblockmap)
    {
	extern void P_CreateBlockMap (void);
	P_CreateBlockMap();
    }
    if (crispy_mapformat & (MFMT_ZDBSPX | MFMT_ZDBSPZ))
	P_LoadNodes_ZDBSP (lumpnum+ML_NODES, crispy_mapformat & MFMT_ZDBSPZ);
    else
    if (crispy_mapformat & MFMT_DEEPBSP)
    {
	P_LoadSubsectors_DeePBSP (lumpnum+ML_SSECTORS);
	P_LoadNodes_DeePBSP (lumpnum+ML_NODES);
	P_LoadSegs_DeePBSP (lumpnum+ML_SEGS);
    }
    else
    {
    P_LoadSubsectors (lumpnum+ML_SSECTORS);
    P_LoadNodes (lumpnum+ML_NODES);
    P_LoadSegs (lumpnum+ML_SEGS);
    }

    P_GroupLines ();
    P_LoadReject (lumpnum+ML_REJECT);

    // [crispy] remove slime trails
    P_RemoveSlimeTrails();
    // [crispy] fix long wall wobble
    P_SegLengths(false);
    // [crispy] blinking key or skull in the status bar
    memset(st_keyorskull, 0, sizeof(st_keyorskull));

    bodyqueslot = 0;
    deathmatch_p = deathmatchstarts;
    if (crispy_mapformat & MFMT_HEXEN)
	P_LoadThings_Hexen (lumpnum+ML_THINGS);
    else
    P_LoadThings (lumpnum+ML_THINGS);
    
    // if deathmatch, randomly spawn the active players
    if (deathmatch)
    {
	for (i=0 ; i<MAXPLAYERS ; i++)
	    if (playeringame[i])
	    {
		players[i].mo = NULL;
		G_DeathMatchSpawnPlayer (i);
	    }
			
    }
    // [crispy] support MUSINFO lump (dynamic music changing)
    if (gamemode != shareware)
    {
	S_ParseMusInfo(lumpname);
    }

    // clear special respawning que
    iquehead = iquetail = 0;		
	
    // set up world state
    P_SpawnSpecials ();
	
    // build subsector connect matrix
    //	UNUSED P_ConnectSubsectors ();

    // preload graphics
    if (precache)
	R_PrecacheLevel ();

    //printf ("free memory: 0x%x\n", Z_FreeMemory());

}



//
// P_Init
//
void P_Init (void)
{
    P_InitSwitchList ();
    P_InitPicAnims ();
    R_InitSprites (sprnames);
}



