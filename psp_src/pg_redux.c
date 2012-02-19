// 32bit RGB and other features
    // adapted from primitive graphics from Hello World PSP (by nem)
    // butchered by PspPet

#include "std.h"
#include "util.h"
#include "pg_redux.h"

#include "font_ega.c_" // Classic EGA font - mixed case

#define    TRUE_PIXELSIZE    3                //32bit RGB
#define    LINESIZE    512 // 512 pixels per line, 480 shown
#define    FRAMESIZE    0x88000

//480*272 = 60*38
#define CMAX_X 60
#define CMAX_Y 38
#define CMAX2_X 30
#define CMAX2_Y 19
#define CMAX4_X 15
#define CMAX4_Y 9

// draw buffer for video memory (accessed as 32 bit RGB values)
static u32 *pg_vram=(u32*)0x04000000;
static u32 *pg_vramDraw=(u32*)0x44000000; // no cache

// last shown
static u32 *pg_vram2=(u32*)(0x04000000 + FRAMESIZE);
static u32 *pg_vramDraw2=(u32*)(0x44000000 + FRAMESIZE); // no cache

void pgInit32()
{
    // full screen, 32bit RGB, double buffered
    sceDisplaySetMode(0, SCREEN_WIDTH, SCREEN_HEIGHT);
    pgFillvram(0);
    pgScreenFlipV(); // show first
}

void pgWaitVn(u32 count)
{
    while (count--)
        sceDisplayWaitVblankStart();
}

u32* pgGetVramAddr(u32 x, u32 y)
{
    return pg_vramDraw + (y * LINESIZE) + x;
}

void pgPrint(u32 x, u32 y, u32 color, const char *str)
{
    while (*str!=0 && x<CMAX_X && y<CMAX_Y) {
        pgPutChar(x*8, y*8, color, 0, *str, 1, 0, 1);
        str++;
        x++;
        if (x>=CMAX_X) {
            x=0;
            y++;
        }
    }
}

void pgPrint2(u32 x, u32 y, u32 color, const char *str)
{
    while (*str!=0 && x<CMAX2_X && y<CMAX2_Y) {
        pgPutChar(x*16, y*16, color, 0, *str, 1, 0, 2);
        str++;
        x++;
        if (x>=CMAX2_X) {
            x=0;
            y++;
        }
    }
}


void pgPrint4(u32 x, u32 y, u32 color, const char *str)
{
    while (*str!=0 && x<CMAX4_X && y<CMAX4_Y) {
        pgPutChar(x*32, y*32, color, 0, *str, 1, 0, 4);
        str++;
        x++;
        if (x>=CMAX4_X) {
            x=0;
            y++;
        }
    }
}

// pixel positioning (no wrap)
void pgPrint1AtPixel(u32 xPixel, u32 yPixel, u32 color, const char *str)
{
    while (*str!='\0')
    {
        pgPutChar(xPixel, yPixel, color, 0, *str, 1, 0, 1);
        str++;
        xPixel += 8*1;
    }
}
void pgPrint2AtPixel(u32 xPixel, u32 yPixel, u32 color, const char *str)
{
    while (*str!='\0')
    {
        pgPutChar(xPixel, yPixel, color, 0, *str, 1, 0, 2);
        str++;
        xPixel += 8*2;
    }
}
void pgPrint4AtPixel(u32 xPixel, u32 yPixel, u32 color, const char *str)
{
    while (*str!='\0')
    {
        pgPutChar(xPixel, yPixel, color, 0, *str, 1, 0, 4);
        str++;
        xPixel += 8*4;
    }
}


void pgFillvram(u32 color)
{
    u32* vptr0=(u32*)pgGetVramAddr(0, 0);
    int i;
    for (i=0; i<SCREEN_HEIGHT*LINESIZE; i++) {
        *vptr0++ = color;
    }
}


void pgPutChar(u32 x, u32 y, u32 color, u32 bgcolor, u8 ch, char drawfg, char drawbg, char mag)
{
    u32 *vptr0;        //pointer to vram
    u32 *vptr;        //pointer to vram
    const u8 *cfont;        //pointer to font
    u32 cx, cy;
    u32 b;
    char mx, my;

    // if (ch>255) return;
    cfont=font+ch*8;
    vptr0=pgGetVramAddr(x, y);
    for (cy=0; cy<8; cy++) {
        for (my=0; my<mag; my++) {
            vptr=vptr0;
            b=0x80;
            for (cx=0; cx<8; cx++) {
                for (mx=0; mx<mag; mx++) {
                    if ((*cfont&b)!=0) {
                        if (drawfg) *vptr=color;
                    } else {
                        if (drawbg) *vptr=bgcolor;
                    }
                    vptr++;
                }
                b=b>>1;
            }
            vptr0 += LINESIZE;
        }
        cfont++;
    }
}

#define swap_pg_(a, b) { u32* t = a; a = b; b = t; }
void pgScreenFlipV()
{
    sceDisplayWaitVblankStart();
    sceDisplaySetFrameBuf((void*)pg_vram, LINESIZE, TRUE_PIXELSIZE, 1);
    swap_pg_(pg_vram, pg_vram2);
    swap_pg_(pg_vramDraw, pg_vramDraw2);
}



