#include "SMSlib.h" // we're including the library with the functions we will use
#include "./build/bank2.h"  // we're including the assets we created before
#include "./map/map1_l1.h"

#define BG_TILES 0
#define METATILE_GRAPHICS 16     // in tiles, we have 4x4 tiles within a meta-tile of 32x32 pixels
#define METATILE_WIDTH 32        // in pixel
#define METATILE_HEIGHT 32       // in pixel
#define VIRTUALSCREEN_WIDTH 256  // in pixel
#define VIRTUALSCREEN_HEIGHT 224 // in pixel
#define SCREEN_WIDTH 256         // in pixel
#define SCREEN_HEIGHT 192        // in pixel
#define MAP_WIDTH 7872           // in pixel
#define MAP_HEIGHT 512           // in pixel
#define SPRITE_TILES 256
#define START_X 0        // in metatile index
#define START_Y 8        // in metatile index
#define START_X_OFFSET 3 // metatile_x offset
#define START_Y_OFFSET 0 // metatile_y offset
#define SPEED 5

unsigned int g_metatile_x = START_X;
unsigned int g_metatile_y = START_Y;
unsigned char g_metatile_x_offset = START_X_OFFSET;
unsigned char g_metatile_y_offset = START_Y_OFFSET;          // must be divisible by 4
unsigned char g_metatile_x_offset_max = METATILE_WIDTH / 8;  // number of tile columns within each metatile
unsigned char g_metatile_y_offset_max = METATILE_HEIGHT / 8; // number of tile rows within each metatile
unsigned int g_old_scrollh = 0;
unsigned int g_new_scrollh = 0;
unsigned int g_old_scrollv = 0;
unsigned int g_new_scrollv = 0;

enum Direction
{
    RIGHT,
    LEFT,
    UP,
    DOWN
};
struct Coordinates
{
    unsigned int x;
    unsigned int y;
};
struct Player
{
    struct Coordinates coords;
    enum Direction direction;
    unsigned int speed;
    unsigned int is_moving;
    unsigned int walking_animation_right;
    unsigned int walking_animation_left;
    unsigned int running_animation_right;
    unsigned int running_animation_left;
    unsigned int running_ix;
    unsigned int running_frames;
};

void initPlayer(struct Player *p)
{
    p->speed = SPEED;
    p->coords.x = 128; // map coordinates in pixel
    p->coords.y = 112; // map coordinates in pixel
    p->is_moving = 0;
    p->direction = RIGHT;
    p->walking_animation_right = 0;
    p->walking_animation_left = 1;
    p->running_animation_right = 2;
    p->running_animation_left = 8;
    p->running_ix = 0;
    p->running_frames = 6;
}

// used to increment or decrement metatile_offset index by an offset up to a given maximum value
// e.g. updateMetaTileOffset(x=0,-4,16) gives x=12,8,4,0,12,8,...
// emits 1 if x is reset or wraps
unsigned char updateMetaTileOffset(unsigned char *metatile_offset_ix, signed char metatile_offset_step, unsigned char metatile_offset_max)
{
    *metatile_offset_ix += metatile_offset_step;
    // this should happen when incrementing
    if (*metatile_offset_ix == metatile_offset_max)
    {
        *metatile_offset_ix = 0; // reset g_metatile_y_offset

        return 1;
    }
    // this should happen when decrementing (because of unsigend integer will wrap when 0 gets subtracted)
    if (*metatile_offset_ix > metatile_offset_max)
    {
        *metatile_offset_ix = (metatile_offset_max + metatile_offset_step); // wrap to maximum allowable value

        return 1;
    }

    return 0;
}

void loadMetaTileMapColumn(unsigned char x, unsigned int metatile_x, unsigned int metatile_y, unsigned char metatile_x_offset, unsigned char metatile_y_offset)
{
    unsigned char y = 0; // tile row index
    unsigned int metatile_ix = metatile_x + metatile_y * (MAP_WIDTH / METATILE_WIDTH);
    // go through all metatile rows in tilemap
    for (unsigned char metatile_row = 0; metatile_row <= (VIRTUALSCREEN_HEIGHT / METATILE_HEIGHT); metatile_row++)
    {
        // go on until last tile row in metatile or if the tilemap is full
        unsigned char metatile_y_offset_reset = 0;
        while ((!metatile_y_offset_reset) && (y < VIRTUALSCREEN_HEIGHT / 8))
        {
            SMS_setNextTileatXY(x, y);
            SMS_setTile(bg__tilemap__mtm[map[metatile_ix] * METATILE_GRAPHICS + metatile_y_offset + metatile_x_offset]);
            y++;                                                                                                            // increase tile row index
            metatile_y_offset_reset = updateMetaTileOffset(&metatile_y_offset, g_metatile_y_offset_max, METATILE_GRAPHICS); // update metatile_y_offset
        }
        metatile_ix += (MAP_WIDTH / METATILE_WIDTH); // go to next metatile row
    }
}

void loadMetaTileMapRow(unsigned char y, unsigned int metatile_x, unsigned int metatile_y, unsigned char metatile_x_offset, unsigned char metatile_y_offset)
{
    unsigned char x = 0; // tile col index
    unsigned int metatile_ix = metatile_x + metatile_y * (MAP_WIDTH / METATILE_WIDTH);
    // go through all tile columns in metatile
    for (unsigned char metatile_col = 0; metatile_col <= (VIRTUALSCREEN_WIDTH / METATILE_WIDTH); metatile_col++)
    {
        SMS_setNextTileatXY(x, y);
        // go on until last tile column in metatile or if the tilemap is full
        unsigned char metatile_x_offset_reset = 0;
        while ((!metatile_x_offset_reset) && (x < VIRTUALSCREEN_WIDTH / 8))
        {
            SMS_setTile(bg__tilemap__mtm[map[metatile_ix] * METATILE_GRAPHICS + metatile_y_offset + metatile_x_offset]);
            x++;                                                                                            // increase tile col index
            metatile_x_offset_reset = updateMetaTileOffset(&metatile_x_offset, 1, g_metatile_x_offset_max); // update metatile_x_offset
        }
        metatile_ix++; // go to next metatile col
    }
}

void drawScreenByColumn(void)
{
    for (unsigned char x = 0; x < (VIRTUALSCREEN_WIDTH / 8); x++)
    {
        if ((g_metatile_x_offset == 0) && (x != 0))
            g_metatile_x++; // increase metatile_x

        loadMetaTileMapColumn(x, g_metatile_x, g_metatile_y, g_metatile_x_offset, g_metatile_y_offset);
        updateMetaTileOffset(&g_metatile_x_offset, 1, g_metatile_x_offset_max);
    }
}

void drawScreenByRow(void)
{
    for (unsigned char y = 0; y < (VIRTUALSCREEN_HEIGHT / 8); y++)
    {
        if ((g_metatile_y_offset == 0) && (y != 0))
            g_metatile_y++; // increase metatile_y

        loadMetaTileMapRow(y, g_metatile_x, g_metatile_y, g_metatile_x_offset, g_metatile_y_offset);
        updateMetaTileOffset(&g_metatile_y_offset, g_metatile_y_offset_max, METATILE_GRAPHICS);
    }
    // reset metatile_y and its offset
    g_metatile_y = START_Y;
    g_metatile_y_offset = START_Y_OFFSET;
}

void drawPlayer(struct Coordinates *c)
{
    SMS_addSprite(c->x, c->y, SPRITE_TILES);
    SMS_addSprite(c->x + 8, c->y, SPRITE_TILES + 2);
    SMS_addSprite(c->x + 16, c->y, SPRITE_TILES + 4);
    SMS_addSprite(c->x, c->y + 16, SPRITE_TILES + 6);
    SMS_addSprite(c->x + 8, c->y + 16, SPRITE_TILES + 8);
    SMS_addSprite(c->x + 16, c->y + 16, SPRITE_TILES + 10);
}

void loadAssets(void)
{
    SMS_loadBGPalette(bg__palette__bin);
    SMS_loadPSGaidencompressedTiles(bg__tiles__psgcompr, BG_TILES);
    drawScreenByRow();
    SMS_loadSpritePalette(tails__palette__bin);
}

void main(void)
{
    unsigned int ks;
    unsigned char x;
    struct Player player;
    initPlayer(&player);
    SMS_setSpriteMode(SPRITEMODE_TALL);
    loadAssets();
    SMS_VDPturnOnFeature(VDPFEATURE_LEFTCOLBLANK); // hide the leftmost column
    SMS_initSprites();
    SMS_displayOn();
    for (;;)
    {
        SMS_waitForVBlank();
        UNSAFE_SMS_copySpritestoSAT();
        SMS_initSprites();
        UNSAFE_SMS_loadNTiles(&tails__tiles__bin[0 * 12 * 32], SPRITE_TILES, 12);
        drawPlayer(&player.coords);
        ks = SMS_getKeysStatus();
        if (ks)
        {
            // START CHECK FOR HORIZONTAL MOVEMENT...
            if ((ks & PORT_A_KEY_RIGHT) && (g_metatile_x < ((MAP_WIDTH / METATILE_WIDTH)-8)))
            {
                player.direction = RIGHT;
                player.is_moving = 1;
                g_new_scrollh += player.speed;
                SMS_setBGScrollX(-g_new_scrollh);
                if (((g_old_scrollh % 8) == 0) || (((g_new_scrollh % 8) != 0) && ((g_new_scrollh % 8) < (g_old_scrollh % 8))))
                {
                    x = ((VIRTUALSCREEN_WIDTH / 8) + (g_new_scrollh / 8)) % (VIRTUALSCREEN_WIDTH / 8);
                    loadMetaTileMapColumn(x, g_metatile_x + (VIRTUALSCREEN_WIDTH / METATILE_WIDTH), g_metatile_y, g_metatile_x_offset, g_metatile_y_offset);
                    unsigned char metatile_x_offset_reset = updateMetaTileOffset(&g_metatile_x_offset, 1, g_metatile_x_offset_max);
                    if (metatile_x_offset_reset)
                        g_metatile_x++; // increase metatile_x
                }
                g_old_scrollh = g_new_scrollh;
            }
            else if ((ks & PORT_A_KEY_LEFT) && (g_metatile_x > 0))
            {
                player.direction = LEFT;
                player.is_moving = 1;
                g_new_scrollh -= player.speed;
                SMS_setBGScrollX(-g_new_scrollh);
                if (((g_new_scrollh % 8) == 0) || (((g_old_scrollh % 8) != 0) && ((g_old_scrollh % 8) < (g_new_scrollh % 8))))
                {
                    x = ((VIRTUALSCREEN_WIDTH / 8) + (g_old_scrollh / 8)) % (VIRTUALSCREEN_WIDTH / 8);
                    unsigned char metatile_x_offset_reset = updateMetaTileOffset(&g_metatile_x_offset, -1, g_metatile_x_offset_max);
                    if (metatile_x_offset_reset)
                        g_metatile_x--; // decrease metatile_x
                    loadMetaTileMapColumn(x, g_metatile_x, g_metatile_y, g_metatile_x_offset, g_metatile_y_offset);
                }
                g_old_scrollh = g_new_scrollh;
            }
            // ...END CHECK FOR HORIZONTAL MOVEMENT
            // START CHECK FOR VERTICAL MOVEMENT...
            // todo
            // ..END CHECK FOR VERTICAL MOVEMENT
        }
        else
        {
            player.is_moving = 0;
        }
    }
}