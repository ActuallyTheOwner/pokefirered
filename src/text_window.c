#include "global.h"
#include "gflib.h"
#include "text_window.h"
#include "text_window_graphics.h"
#include "help_message.h"
#include "new_menu_helpers.h"
#include "menu.h"

// Wasn't in pull request?
const u8 gTextWindowFrame1_Gfx[] = INCBIN_U8("graphics/text_window/1.4bpp");
static const u8 sTextWindowFrame2_Gfx[] = INCBIN_U8("graphics/text_window/2.4bpp");
static const u8 sTextWindowFrame3_Gfx[] = INCBIN_U8("graphics/text_window/3.4bpp");
static const u8 sTextWindowFrame4_Gfx[] = INCBIN_U8("graphics/text_window/4.4bpp");
static const u8 sTextWindowFrame5_Gfx[] = INCBIN_U8("graphics/text_window/5.4bpp");
static const u8 sTextWindowFrame6_Gfx[] = INCBIN_U8("graphics/text_window/6.4bpp");
static const u8 sTextWindowFrame7_Gfx[] = INCBIN_U8("graphics/text_window/7.4bpp");
static const u8 sTextWindowFrame8_Gfx[] = INCBIN_U8("graphics/text_window/8.4bpp");
static const u8 sTextWindowFrame9_Gfx[] = INCBIN_U8("graphics/text_window/9.4bpp");
static const u8 sTextWindowFrame10_Gfx[] = INCBIN_U8("graphics/text_window/10.4bpp");
static const u8 sTextWindowFrame11_Gfx[] = INCBIN_U8("graphics/text_window/11.4bpp");
static const u8 sTextWindowFrame12_Gfx[] = INCBIN_U8("graphics/text_window/12.4bpp");
static const u8 sTextWindowFrame13_Gfx[] = INCBIN_U8("graphics/text_window/13.4bpp");
static const u8 sTextWindowFrame14_Gfx[] = INCBIN_U8("graphics/text_window/14.4bpp");
static const u8 sTextWindowFrame15_Gfx[] = INCBIN_U8("graphics/text_window/15.4bpp");
static const u8 sTextWindowFrame16_Gfx[] = INCBIN_U8("graphics/text_window/16.4bpp");
static const u8 sTextWindowFrame17_Gfx[] = INCBIN_U8("graphics/text_window/17.4bpp");
static const u8 sTextWindowFrame18_Gfx[] = INCBIN_U8("graphics/text_window/18.4bpp");
static const u8 sTextWindowFrame19_Gfx[] = INCBIN_U8("graphics/text_window/19.4bpp");
static const u8 sTextWindowFrame20_Gfx[] = INCBIN_U8("graphics/text_window/20.4bpp");

const u16 gTextWindowFrame1_Pal[] = INCBIN_U16("graphics/text_window/1.gbapal");
static const u16 sTextWindowFrame2_Pal[] = INCBIN_U16("graphics/text_window/2.gbapal");
static const u16 sTextWindowFrame3_Pal[] = INCBIN_U16("graphics/text_window/3.gbapal");
static const u16 sTextWindowFrame4_Pal[] = INCBIN_U16("graphics/text_window/4.gbapal");
static const u16 sTextWindowFrame5_Pal[] = INCBIN_U16("graphics/text_window/5.gbapal");
static const u16 sTextWindowFrame6_Pal[] = INCBIN_U16("graphics/text_window/6.gbapal");
static const u16 sTextWindowFrame7_Pal[] = INCBIN_U16("graphics/text_window/7.gbapal");
static const u16 sTextWindowFrame8_Pal[] = INCBIN_U16("graphics/text_window/8.gbapal");
static const u16 sTextWindowFrame9_Pal[] = INCBIN_U16("graphics/text_window/9.gbapal");
static const u16 sTextWindowFrame10_Pal[] = INCBIN_U16("graphics/text_window/10.gbapal");
static const u16 sTextWindowFrame11_Pal[] = INCBIN_U16("graphics/text_window/11.gbapal");
static const u16 sTextWindowFrame12_Pal[] = INCBIN_U16("graphics/text_window/12.gbapal");
static const u16 sTextWindowFrame13_Pal[] = INCBIN_U16("graphics/text_window/13.gbapal");
static const u16 sTextWindowFrame14_Pal[] = INCBIN_U16("graphics/text_window/14.gbapal");
static const u16 sTextWindowFrame15_Pal[] = INCBIN_U16("graphics/text_window/15.gbapal");
static const u16 sTextWindowFrame16_Pal[] = INCBIN_U16("graphics/text_window/16.gbapal");
static const u16 sTextWindowFrame17_Pal[] = INCBIN_U16("graphics/text_window/17.gbapal");
static const u16 sTextWindowFrame18_Pal[] = INCBIN_U16("graphics/text_window/18.gbapal");
static const u16 sTextWindowFrame19_Pal[] = INCBIN_U16("graphics/text_window/19.gbapal");
static const u16 sTextWindowFrame20_Pal[] = INCBIN_U16("graphics/text_window/20.gbapal");

static const struct TilesPal sWindowFrames[WINDOW_FRAMES_COUNT] =
{
    {gTextWindowFrame1_Gfx, gTextWindowFrame1_Pal},
    {sTextWindowFrame2_Gfx, sTextWindowFrame2_Pal},
    {sTextWindowFrame3_Gfx, sTextWindowFrame3_Pal},
    {sTextWindowFrame4_Gfx, sTextWindowFrame4_Pal},
    {sTextWindowFrame5_Gfx, sTextWindowFrame5_Pal},
    {sTextWindowFrame6_Gfx, sTextWindowFrame6_Pal},
    {sTextWindowFrame7_Gfx, sTextWindowFrame7_Pal},
    {sTextWindowFrame8_Gfx, sTextWindowFrame8_Pal},
    {sTextWindowFrame9_Gfx, sTextWindowFrame9_Pal},
    {sTextWindowFrame10_Gfx, sTextWindowFrame10_Pal},
    {sTextWindowFrame11_Gfx, sTextWindowFrame11_Pal},
    {sTextWindowFrame12_Gfx, sTextWindowFrame12_Pal},
    {sTextWindowFrame13_Gfx, sTextWindowFrame13_Pal},
    {sTextWindowFrame14_Gfx, sTextWindowFrame14_Pal},
    {sTextWindowFrame15_Gfx, sTextWindowFrame15_Pal},
    {sTextWindowFrame16_Gfx, sTextWindowFrame16_Pal},
    {sTextWindowFrame17_Gfx, sTextWindowFrame17_Pal},
    {sTextWindowFrame18_Gfx, sTextWindowFrame18_Pal},
    {sTextWindowFrame19_Gfx, sTextWindowFrame19_Pal},
    {sTextWindowFrame20_Gfx, sTextWindowFrame20_Pal}
};

// Equivalent to LoadStdWindowGfx, but takes a bg id directly
void LoadStdWindowGfxOnBg(u8 bgId, u16 destOffset, u8 palOffset)
{
    LoadBgTiles(bgId, gStdTextWindow_Gfx, 0x120, destOffset);
    LoadPalette(GetTextWindowPalette(3), palOffset, PLTT_SIZE_4BPP);
}

void LoadHelpMessageWindowGfx(u8 windowId, u16 destOffset, u8 palOffset)
{
    LoadBgTiles(GetWindowAttribute(windowId, WINDOW_BG), gHelpMessageWindow_Gfx, 0x280, destOffset);
    LoadPalette(GetTextWindowPalette(2), palOffset, PLTT_SIZE_4BPP);
}

void LoadMenuMessageWindowGfx(u8 windowId, u16 destOffset, u8 palOffset)
{
    LoadBgTiles(GetWindowAttribute(windowId, WINDOW_BG), gMenuMessageWindow_Gfx, 0x280, destOffset);
    LoadPalette(GetTextWindowPalette(0), palOffset, PLTT_SIZE_4BPP);
}

void LoadSignpostWindowGfx(u8 windowId, u16 destOffset, u8 palOffset)
{
    LoadBgTiles(GetWindowAttribute(windowId, WINDOW_BG), gSignpostWindow_Gfx, 0x260, destOffset);
    LoadPalette(GetTextWindowPalette(1), palOffset, PLTT_SIZE_4BPP);
}

void LoadStdWindowGfx(u8 windowId, u16 destOffset, u8 palOffset)
{
    LoadBgTiles(GetWindowAttribute(windowId, WINDOW_BG), gStdTextWindow_Gfx, 0x120, destOffset);
    LoadPalette(GetTextWindowPalette(3), palOffset, PLTT_SIZE_4BPP);
}

void LoadStdWindowTiles(u8 windowId, u16 destOffset)
{
    LoadBgTiles(GetWindowAttribute(windowId, WINDOW_BG), gStdTextWindow_Gfx, 0x120, destOffset);
}

void LoadQuestLogWindowTiles(u8 windowId, u16 destOffset)
{
    LoadBgTiles(GetWindowAttribute(windowId, WINDOW_BG), gQuestLogWindow_Gfx, 0x280, destOffset);
}

static void LoadUserWindowGfxByFrame(u8 windowId, u8 frameType, u16 destOffset, u8 palOffset)
{
    LoadBgTiles(GetWindowAttribute(windowId, WINDOW_BG), gUserFrames[frameType].tiles, 0x120, destOffset);
    LoadPalette(gUserFrames[frameType].palette, palOffset, PLTT_SIZE_4BPP);
}

void LoadUserWindowGfx(u8 windowId, u16 destOffset, u8 palOffset)
{
    LoadUserWindowGfxByFrame(windowId, gSaveBlock2Ptr->optionsWindowFrameType, destOffset, palOffset);
}

void DrawTextBorderOuter(u8 windowId, u16 tileNum, u8 palNum)
{
    u8 bgLayer = GetWindowAttribute(windowId, WINDOW_BG);
    u16 tilemapLeft = GetWindowAttribute(windowId, WINDOW_TILEMAP_LEFT);
    u16 tilemapTop = GetWindowAttribute(windowId, WINDOW_TILEMAP_TOP);
    u16 width = GetWindowAttribute(windowId, WINDOW_WIDTH);
    u16 height = GetWindowAttribute(windowId, WINDOW_HEIGHT);

    FillBgTilemapBufferRect(bgLayer, tileNum + 0, tilemapLeft - 1,      tilemapTop - 1,         1,      1,      palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 1, tilemapLeft,          tilemapTop - 1,         width,  1,      palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 2, tilemapLeft + width,  tilemapTop - 1,         1,      1,      palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 3, tilemapLeft - 1,      tilemapTop,             1,      height, palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 5, tilemapLeft + width,  tilemapTop,             1,      height, palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 6, tilemapLeft - 1,      tilemapTop + height,    1,      1,      palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 7, tilemapLeft,          tilemapTop + height,    width,  1,      palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 8, tilemapLeft + width,  tilemapTop + height,    1,      1,      palNum);
}

void DrawTextBorderInner(u8 windowId, u16 tileNum, u8 palNum)
{
    u8 bgLayer = GetWindowAttribute(windowId, WINDOW_BG);
    u16 tilemapLeft = GetWindowAttribute(windowId, WINDOW_TILEMAP_LEFT);
    u16 tilemapTop = GetWindowAttribute(windowId, WINDOW_TILEMAP_TOP);
    u16 width = GetWindowAttribute(windowId, WINDOW_WIDTH);
    u16 height = GetWindowAttribute(windowId, WINDOW_HEIGHT);

    FillBgTilemapBufferRect(bgLayer, tileNum + 0, tilemapLeft,              tilemapTop,                 1,          1,          palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 1, tilemapLeft + 1,          tilemapTop,                 width - 2,  1,          palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 2, tilemapLeft + width - 1,  tilemapTop,                 1,          1,          palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 3, tilemapLeft,              tilemapTop + 1,             1,          height - 2, palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 5, tilemapLeft + width - 1,  tilemapTop + 1,             1,          height - 2, palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 6, tilemapLeft,              tilemapTop + height - 1,    1,          1,          palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 7, tilemapLeft + 1,          tilemapTop + height - 1,    width -     2,  1,      palNum);
    FillBgTilemapBufferRect(bgLayer, tileNum + 8, tilemapLeft + width - 1,  tilemapTop + height - 1,    1,          1,          palNum);
}

void rbox_fill_rectangle(u8 windowId)
{
    u8 bgLayer = GetWindowAttribute(windowId, WINDOW_BG);
    u16 tilemapLeft = GetWindowAttribute(windowId, WINDOW_TILEMAP_LEFT);
    u16 tilemapTop = GetWindowAttribute(windowId, WINDOW_TILEMAP_TOP);
    u16 width = GetWindowAttribute(windowId, WINDOW_WIDTH);
    u16 height = GetWindowAttribute(windowId, WINDOW_HEIGHT);

    FillBgTilemapBufferRect(bgLayer, 0, tilemapLeft - 1, tilemapTop - 1, width + 2, height + 2, 17);
}

// ??? This was missing from my hack but in https://github.com/Sotomura/pokefirered/tree/pokefirered-rtc
// copied these two from emerald to fill gaps
void LoadWindowGfx(u8 windowId, u8 frameId, u16 destOffset, u8 palOffset)
{
    LoadBgTiles(GetWindowAttribute(windowId, WINDOW_BG), sWindowFrames[frameId].tiles, 0x120, destOffset);
    LoadPalette(sWindowFrames[frameId].pal, palOffset, PLTT_SIZE_4BPP);
}

void LoadUserWindowBorderGfx(u8 windowId, u16 destOffset, u8 palOffset)
{
    LoadWindowGfx(windowId, gSaveBlock2Ptr->optionsWindowFrameType, destOffset, palOffset);
}

const u16 *GetTextWindowPalette(u8 id)
{
    switch (id)
    {
    case 0:
        id = 0;
        break;
    case 1:
        id = 0x10;
        break;
    case 2:
        id = 0x20;
        break;
    case 3:
        id = 0x30;
        break;
    case 4:
    default:
        id = 0x40;
        break;
    }

    return (const u16 *)(gTextWindowPalettes) + id;
}
