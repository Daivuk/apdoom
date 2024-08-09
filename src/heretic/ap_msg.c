#include "ap_msg.h"
#include <inttypes.h>
#include "i_timer.h"
#include "doomdef.h"
#include "i_video.h"
#include <stdlib.h>


#define HU_APMSGTIMEOUT	    (5*TICRATE)
#define HU_MAXLINES		    4
#define HU_MAXLINELENGTH	80
#define HU_MAX_LINE_BUFFER  500


typedef struct
{
    char message[HU_MAXLINELENGTH];
    boolean on;
    int counter;
    int y;
} ap_message_t;


static char ap_message_buffer[HU_MAX_LINE_BUFFER][HU_MAXLINELENGTH + 1];
static ap_message_t ap_messages[HU_MAXLINES];
static int ap_message_buffer_count = 0;
static int ap_message_anim = 0;


void HU_AddAPLine(const char* line, int len)
{
    char baked_line[HU_MAXLINELENGTH + 1];
    memcpy(baked_line, line, len);
    baked_line[len] = '\0';

    // Add to buffer
    if (ap_message_buffer_count >= HU_MAX_LINE_BUFFER)
        return; // No more room
    memcpy(ap_message_buffer[ap_message_buffer_count], baked_line, len + 1);
    ap_message_buffer_count++;
}


void HU_AddAPMessage(const char* message)
{
    int len = strlen(message);

    int i = 0;
    int j = 0;
    char baked_line[HU_MAXLINELENGTH + 1];
    baked_line[HU_MAXLINELENGTH] = '\0';
    int word_start = 0;
    while (i < len)
    {
        if (message[j] == ' ')
        {
            word_start = j;
        }
        if (message[j] != '\n')
        {
            int w = MN_TextAWidth_len(message + i, j - i);
            if (w >= ORIGWIDTH + WIDESCREENDELTA - 8 && word_start == i)
            {
                if (j > i && word_start == i) --j;
            }
            else
            {
                if (w < ORIGWIDTH + WIDESCREENDELTA - 8 && (j - i) + 2 < HU_MAXLINELENGTH && j < len)
                {
                    j++;
                    continue;
                }
                if (j < len) j = word_start;
            }
        }
        else
        {
            j++;
            word_start = j;
        }
        memcpy(baked_line + 2, message + i, j - i);
        baked_line[0] = '~'; baked_line[1] = '2'; // Always make sure to use white
        HU_AddAPLine(baked_line, (j - i) + 2);
        i = j;
        word_start = j;
        while (message[i] == ' ')
        {
            i++;
            j++;
            word_start++;
        }
    }
}


void HU_DrawAPMessages()
{
    for (int i = 0; i < 4; ++i)
    {
        if (i == 0 && ap_message_anim > 0 && HU_GetActiveAPMessageCount() == 4) continue;
        if (ap_messages[i].on)
            MN_DrTextA(ap_messages[i].message, 0, ap_messages[i].y);
    }
}


boolean HU_HasAPMessageRoom()
{
    for (int i = 0; i < 4; ++i)
        if (!ap_messages[i].on)
            return true;
    return false;
}


int HU_GetActiveAPMessageCount()
{
    int active_count = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (ap_messages[i].on)
            active_count++;
    }
    return active_count;
}


void HU_UpdateAPMessagePosition(int i)
{
    ap_messages[i].y = 3 * 10 - i * 10 - 4 * 10 + HU_GetActiveAPMessageCount() * 10 + ap_message_anim;
}


void HU_TickAPMessages()
{
    while (HU_HasAPMessageRoom() && ap_message_buffer_count && ap_message_anim == 0)
    {
        // Shift currents
        for (int i = 3; i > 0; --i)
        {
            memcpy(&ap_messages[i], &ap_messages[i - 1], sizeof(ap_message_t));
        }
	    memcpy(ap_messages[0].message, ap_message_buffer[0], HU_MAXLINELENGTH);
	    ap_messages[0].on = true;
	    ap_messages[0].counter = HU_APMSGTIMEOUT;

        // Shift buffers
        for (int i = 1; i < ap_message_buffer_count; ++i)
        {
            memcpy(ap_message_buffer[i - 1], ap_message_buffer[i], HU_MAXLINELENGTH + 1);
        }
        ap_message_buffer_count--;
    }

    if (ap_message_anim == 0)
    {
        for (int i = 3; i >= 0; --i)
        {
            if (ap_messages[i].counter)
            {
                ap_messages[i].counter -= MAX(1, ap_message_buffer_count / 6);
                if (ap_messages[i].counter <= 0)
                {
                    ap_messages[i].counter = 0;
                    ap_messages[i].on = false;
                    ap_message_anim = 8;
                    break;
                }
            }
        }
    }

    // Animate their position
    for (int i = 0; i < 4; ++i)
        HU_UpdateAPMessagePosition(i);

    if (ap_message_anim > 0)
    {
        ap_message_anim -= MIN(4, MAX(1, ap_message_buffer_count / 10));
        if (ap_message_anim < 0) ap_message_anim = 0;
    }
}


void HU_ClearAPMessages()
{
    ap_message_buffer_count = 0;
    for (int i = 0; i < HU_MAXLINES; ++i)
        ap_messages[i].on = false;
}
