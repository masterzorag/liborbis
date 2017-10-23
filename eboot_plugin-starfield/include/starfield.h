#ifndef __STARFLD_H__
#define __STARFLD_H__

#include <sys/types.h>

typedef struct _STAR{
    float xpos, ypos;
    short zpos, speed;
    uint8_t color;
} STAR;
//__attribute__((aligned(16)));

#define NUMBER_OF_STARS 256*4     // max 2^16 for uint16_t

// compose ARGB color by components
#define ARGB(a, r, g, b) ( \
          (((a) &0xFF) <<24) | (((r) &0xFF) <<16) | \
          (((g) &0xFF) << 8) | (((b) &0xFF) << 0))

// extract single component form ARGB color
#define GET_A(color) ((color >>24) &0xFF)
#define GET_R(color) ((color >>16) &0xFF)
#define GET_G(color) ((color >> 8) &0xFF)
#define GET_B(color) ((color >> 0) &0xFF)

void init_Star(STAR *star, const uint16_t i);
void init_Starfield(void);
void draw_Starfield(void);

#endif // __STARFLD_H__
