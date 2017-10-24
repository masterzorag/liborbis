#include <stdio.h>
#include <string.h>
#include <orbis2d.h>  // orbis2dDrawPixelColor()

#include "orbisXbmFont.h"


static uint32_t fading_color[SIMPLE_GRADIENT_STEPS];  // precomputed gradient [0-7]


/***********************************************************************
* simple gradient (ARGB)
*
* uint32_t *a     = pointer to start color
* uint32_t *b     = pointer to end color
* uint8_t steps   = number of steps we split fade
* uint8_t step    = which step we compute and return
***********************************************************************/
static uint32_t simple_gradient(const uint32_t *a, const uint32_t *b, const uint8_t steps, const uint8_t step)
{
    uint8_t fr[4], to[4];
    float st[4];

    fr[0] = GET_A(*a), fr[1] = GET_R(*a), fr[2] = GET_G(*a), fr[3] = GET_B(*a),
    to[0] = GET_A(*b), to[1] = GET_R(*b), to[2] = GET_G(*b), to[3] = GET_B(*b);

    st[0] = ((to[0] - fr[0]) / (float)(steps -1));
    st[1] = ((to[1] - fr[1]) / (float)(steps -1));
    st[2] = ((to[2] - fr[2]) / (float)(steps -1));
    st[3] = ((to[3] - fr[3]) / (float)(steps -1));

    return ARGB((int)fr[0] + (int)(st[0] * step),
                (int)fr[1] + (int)(st[1] * step),
                (int)fr[2] + (int)(st[2] * step),
                (int)fr[3] + (int)(st[3] * step));
}


/***********************************************************************
* update_gradient
*
* precompute palette to use in print_text() and setup colors range
*
* uint32_t *a     = pointer to start color
* uint32_t *b     = pointer to end color
***********************************************************************/
void update_gradient(const uint32_t *a, const uint32_t *b)
{
    for(uint8_t i = 0; i < SIMPLE_GRADIENT_STEPS; i++)
    {
        fading_color[i] = (*a == *b) ? *a : simple_gradient(a, b, SIMPLE_GRADIENT_STEPS, i);
    }
}


/***********************************************************************
* compute x to align text into canvas
*
* const char *str = referring string
* uint8_t align.  = RIGHT / CENTER (1/2)
***********************************************************************/
uint16_t get_aligned_x(const char *str, const uint8_t alignment)
{
    uint16_t len = (strlen(str) * FONT_W);  // monospaced font

    return (uint16_t)((ATTR_WIDTH - len) / alignment);
}


/***********************************************************************
* print text, with bitmap data from xbm_font.h
*
* int32_t x       = start x coordinate into canvas
* int32_t y       = start y coordinate into canvas
* const char *str = string to print
***********************************************************************/
int32_t print_text(int32_t x, int32_t y, const char *str)
{
    char *bit = NULL;
    uint8_t *c, i, j, tx = 0, ty = 0;

    while(*str != '\0')
    {
        c = (uint8_t*)str++;        // address the current char

        if(*c < LOWER_ASCII_CODE
        || *c > UPPER_ASCII_CODE)
        { x += FONT_W; continue; }  // skipped, move one char in canvas

        bit = xbmFont[*c - LOWER_ASCII_CODE];

        // dump bits map (bytes_per_line 2, size 32 char of 8 bit)
        for(i = 0; i < ((FONT_W * FONT_H) / BITS_IN_BYTE); i++)
        {
            for(j = 0; j < BITS_IN_BYTE; j++)
            {
                if(bit[i] & (1 << j))  // least significant bit first
                {
                    /* draw to screen via liborbis2d */
                    orbis2dDrawPixelColor(
                        (x + tx * BITS_IN_BYTE + j),
                        (y + ty),
                        fading_color[ty /2]);  // paint FG pixel with precomputed fading color

                    // displace shadow by (+SHADOW_PX, +SHADOW_PX)
                    orbis2dDrawPixelColor(
                        (x + tx * BITS_IN_BYTE + j) + SHADOW_PX,
                        (y + ty) + SHADOW_PX,
                        0x80000000);  // paint SHADOW pixel with fixed color
                }
            }
            tx++;
            if(tx == (FONT_W / BITS_IN_BYTE))
                tx = 0, ty++;        // step to decrease gradient
        }
        // glyph painted, move one char right in text
        x += FONT_W, ty = 0;
    }

    return x;
}
