#include "ap_notif.h"
#include "apdoom.h"
#include "v_video.h"
#include "z_zone.h"
#include "doomdef.h"
#include "i_video.h"
#include <stdlib.h>
#include "i_swap.h"
#include "hu_lib.h"


typedef struct cached_icon_s
{
    patch_t* patch;
    pixel_t* pixels;
    struct cached_icon_s* next;
} cached_icon_t;

static cached_icon_t* cached_icon_start = 0;
static cached_icon_t* cached_icon_end = 0;

#define ICON_BLOCK_SIZE (AP_NOTIF_SIZE - 4)


cached_icon_t* get_cached_icon(patch_t* patch)
{
    cached_icon_t* cached_icon = cached_icon_start;
    while (cached_icon)
    {
        if (cached_icon->patch == patch) return cached_icon;
        cached_icon = cached_icon->next;
    }

    // Not cached yet, cache a new one
    cached_icon = (cached_icon_t*)malloc(sizeof(cached_icon_t));
    if (cached_icon_end) cached_icon_end->next = cached_icon;
    cached_icon_end = cached_icon;
    if (!cached_icon_start) cached_icon_start = cached_icon;
    cached_icon->patch = patch;
    cached_icon->pixels = malloc(ICON_BLOCK_SIZE * ICON_BLOCK_SIZE);
    cached_icon->next = 0;


    pixel_t* raw = malloc(patch->width * patch->height);
    memset(raw, 0, patch->width * patch->height);

    for (int x = 0; x < patch->width; ++x)
    {
        column_t* column = (column_t *)((byte *)patch + LONG(patch->columnofs[x]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            byte* source = (byte *)column + 3;

            for (int y = 0; y < column->length; ++y)
            {
                int k = (y + column->topdelta) * patch->width + x;
                raw[k] = *source++;
            }

            column = (column_t *)((byte *)column + column->length + 4);
        }
    }


    // Scale down raw into cached icon (I inverted src and dst, too lazy to change)
    int max_size = MAX(patch->width, patch->height);
    float scale = 1.0f;
    if (max_size > ICON_BLOCK_SIZE)
        scale = (float)max_size / (float)ICON_BLOCK_SIZE;
    int offsetx = (patch->width - (int)((float)ICON_BLOCK_SIZE * scale)) / 2;
    int offsety = (patch->height - (int)((float)ICON_BLOCK_SIZE * scale)) / 2;
    for (int srcy = 0; srcy < ICON_BLOCK_SIZE; ++srcy)
    {
        for (int srcx = 0; srcx < ICON_BLOCK_SIZE; ++srcx)
        {
            int srck = srcy * ICON_BLOCK_SIZE + srcx;
            int dstx = (int)((float)srcx * scale) + offsetx;
            int dsty = (int)((float)srcy * scale) + offsety;
            if (dstx < 0 || dstx >= patch->width ||
                dsty < 0 || dsty >= patch->height)
            {
                cached_icon->pixels[srck] = 0;
                continue; // Skip, outside source patch
            }
            int dstk = dsty * patch->width + dstx;
            cached_icon->pixels[srck] = raw[dstk];
        }
    }

    free(raw);
    return cached_icon;
}


void ap_notif_draw(void)
{
    int notif_count;
    const ap_notification_icon_t* notifs = ap_get_notification_icons(&notif_count);

    for (int i = 0; i < notif_count; ++i)
    {
        const ap_notification_icon_t* notif = notifs + i;
        if (notif->state == AP_NOTIF_STATE_PENDING) continue;

        patch_t* patch = W_CacheLumpName(notif->sprite, PU_CACHE);
        if (!patch) continue;
        cached_icon_t* cached_icon = get_cached_icon(patch);

        int center_y = 172 + notif->y;

        V_DrawPatch(notif->x - AP_NOTIF_SIZE / 2 - WIDESCREENDELTA, 
                    center_y - AP_NOTIF_SIZE / 2, 
                    W_CacheLumpName("NOTIFBG", PU_CACHE));
        V_DrawScaledBlockTransparency(
            notif->x - ICON_BLOCK_SIZE / 2 - WIDESCREENDELTA,
            center_y - ICON_BLOCK_SIZE / 2,
            ICON_BLOCK_SIZE, ICON_BLOCK_SIZE,
            cached_icon->pixels);

        if (notif->text[0])
            HUlib_drawText(notif->text,
                           notif->x + AP_NOTIF_SIZE / 2 + 3 - WIDESCREENDELTA,
                           center_y - 5);
    }
}
