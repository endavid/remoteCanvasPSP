// primitive graphics adapted to RGB and other special cases

void pgInit32();
void pgScreenFlipV();
void pgWaitVn(u32 count);
// font size = 8
void pgPrint(u32 x, u32 y, u32 color, const char *str);
// font size = 16
void pgPrint2(u32 x, u32 y, u32 color, const char *str);
// font size = 32
void pgPrint4(u32 x, u32 y, u32 color, const char *str);
void pgFillvram(u32 color);
void pgPutChar(u32 x, u32 y, u32 color, u32 bgcolor, unsigned char ch, char drawfg, char drawbg, char mag);
u32* pgGetVramAddr(u32 x,u32 y);

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272

#define DRAW_LINESIZE   512

// with pixel locations
void pgPrint1AtPixel(u32 x, u32 y, u32 color, const char *str);
void pgPrint2AtPixel(u32 x, u32 y, u32 color, const char *str);
void pgPrint4AtPixel(u32 x, u32 y, u32 color, const char *str);

//bool DecodeJpgAndDisplay(const u8* buffer, int cb);

// useful RGB colors (ABGR format)

#define COLOR_WHITE  0xFFFFFFFF
#define COLOR_GREY12 0xFF222222
#define COLOR_GREY25 0xFF444444
#define COLOR_GREY50 0xFF888888
#define COLOR_GREY75 0xFFCCCCCC
#define COLOR_RED    0xFF0000FF
#define COLOR_GREEN  0xFF00FF00
#define COLOR_BLUE   0xFFFF0000

#define RGB(r, g, b)    (0xFF000000 | ((b)<<16) | ((g)<<8) | (r))
