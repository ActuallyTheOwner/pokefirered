#include "global.h"
#include "gflib.h"
#include "task.h"
#include "scanline_effect.h"
#include "text_window.h"
#include "menu.h"
#include "new_menu_helpers.h"
#include "mystery_gift_menu.h"
#include "title_screen.h"
#include "list_menu.h"
#include "link_rfu.h"
#include "mystery_gift.h"
#include "save.h"
#include "link.h"
#include "event_data.h"
#include "mystery_gift_server.h"
#include "mystery_gift_client.h"
#include "wonder_news.h"
#include "help_system.h"
#include "strings.h"
#include "decompress.h"
#include "constants/cable_club.h"
#include "constants/songs.h"
#include "constants/union_room.h"

EWRAM_DATA u8 sDownArrowCounterAndYCoordIdx[8] = {};
EWRAM_DATA bool8 gGiftIsFromEReader = FALSE;

static void CreateMysteryGiftTask(void);
static void Task_MysteryGift(u8 taskId);
extern void CreateEReaderTask(void);

static const u16 sTextboxBorder_Pal[] = INCBIN_U16("graphics/interface/mystery_gift_textbox_border.gbapal");
static const u32 sTextboxBorder_Gfx[] = INCBIN_U32("graphics/interface/mystery_gift_textbox_border.4bpp.lz");

struct MysteryGiftTaskData
{
    u16 var; // Multipurpose
    u16 unused1;
    u16 unused2;
    u16 unused3;
    u8 state;
    u8 textState;
    u8 unused4;
    u8 unused5;
    bool8 isWonderNews;
    bool8 sourceIsFriend;
    u8 msgId;
    u8 * clientMsg;
};

static const struct BgTemplate sBGTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 15,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0x000
    }, {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 14,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0x000
    }, {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 13,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0x000
    }, {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 12,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0x000
    }
};

static const struct WindowTemplate sMainWindows[] = {
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x0013
    }, {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 15,
        .width = 28,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x004f
    }, {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 15,
        .width = 30,
        .height = 5,
        .paletteNum = 13,
        .baseBlock = 0x004f
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sWindowTemplate_YesNoMsg_Wide = {
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 15,
    .width = 28,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 0x00e5
};

static const struct WindowTemplate sWindowTemplate_YesNoMsg = {
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 15,
    .width = 20,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 0x00e5
};

static const struct WindowTemplate sWindowTemplate_GiftSelect = {
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 15,
    .width = 19,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 0x00e5
};

static const struct WindowTemplate sWindowTemplate_ThreeOptions = {
    .bg = 0,
    .tilemapLeft = 8,
    .tilemapTop = 5,
    .width = 14,
    .height = 5,
    .paletteNum = 14,
    .baseBlock = 0x0155
};

static const struct WindowTemplate sWindowTemplate_YesNoBox = {
    .bg = 0,
    .tilemapLeft = 23,
    .tilemapTop = 15,
    .width = 6,
    .height = 4,
    .paletteNum = 14,
    .baseBlock = 0x0155
};

static const struct WindowTemplate sWindowTemplate_GiftSelect_3Options = {
    .bg = 0,
    .tilemapLeft = 22,
    .tilemapTop = 12,
    .width = 7,
    .height = 7,
    .paletteNum = 14,
    .baseBlock = 0x0155
};

static const struct WindowTemplate sWindowTemplate_GiftSelect_2Options = {
    .bg = 0,
    .tilemapLeft = 22,
    .tilemapTop = 14,
    .width = 7,
    .height = 5,
    .paletteNum = 14,
    .baseBlock = 0x0155
};

static const struct WindowTemplate sWindowTemplate_GiftSelect_1Option = {
    .bg = 0,
    .tilemapLeft = 22,
    .tilemapTop = 15,
    .width = 7,
    .height = 4,
    .paletteNum = 14,
    .baseBlock = 0x0155
};

static const struct ListMenuItem sListMenuItems_CardsOrNews[] = {
    { gText_WonderCards,  0 },
    { gText_WonderNews,   1 },
    { gText_Exit3,        LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_WirelessOrFriend[] = {
    { gText_WirelessCommunication,  0 },
    { gText_Friend2,                1 },
    { gFameCheckerText_Cancel,      LIST_CANCEL }
};

static const struct ListMenuTemplate sListMenuTemplate_ThreeOptions = {
    .items = NULL,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 3,
    .maxShowed = 3,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 0,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuItem sListMenuItems_ReceiveSendToss[] = {
    { gText_Receive,  0 },
    { gText_Send,     1 },
    { gText_Toss,     2 },
    { gFameCheckerText_Cancel, LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_ReceiveToss[] = {
    { gText_Receive,  0 },
    { gText_Toss,     2 },
    { gFameCheckerText_Cancel, LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_ReceiveSend[] = {
    { gText_Receive,  0 },
    { gText_Send,     1 },
    { gFameCheckerText_Cancel, LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_Receive[] = {
    { gText_Receive,  0 },
    { gFameCheckerText_Cancel, LIST_CANCEL }
};

static const struct ListMenuTemplate sListMenu_ReceiveSendToss = {
    .items = sListMenuItems_ReceiveSendToss,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 4,
    .maxShowed = 4,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 2,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuTemplate sListMenu_ReceiveToss = {
    .items = sListMenuItems_ReceiveToss,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 3,
    .maxShowed = 3,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 0,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuTemplate sListMenu_ReceiveSend = {
    .items = sListMenuItems_ReceiveSend,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 3,
    .maxShowed = 3,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 0,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuTemplate sListMenu_Receive = {
    .items = sListMenuItems_Receive,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 2,
    .maxShowed = 2,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 0,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 2,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

ALIGNED(4) static const u8 sTextColors_TopMenu[3]      = { 0, 1, 2 };
ALIGNED(4) static const u8 sTextColors_TopMenu_Copy[3] = { 0, 1, 2 };
ALIGNED(4) static const u8 sMG_Ereader_TextColor_2[3]  = { 1, 2, 3 };

static const u8 sText_Test[] = _("テスト");
static const u8 sText_EonTicket[] = _("むげんのチケット");

static void VBlankCB_MysteryGiftEReader(void)
{
    ProcessSpriteCopyRequests();
    LoadOam();
    TransferPlttBuffer();
}

void CB2_MysteryGiftEReader(void)
{
    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
}

bool32 HandleMysteryGiftOrEReaderSetup(s32 isEReader)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        ResetPaletteFade();
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetTasks();
        ScanlineEffect_Stop();
        ResetBgsAndClearDma3BusyFlags(1);

        InitBgsFromTemplates(0, sBGTemplates, ARRAY_COUNT(sBGTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 0, BG_COORD_SET);

        SetBgTilemapBuffer(3, Alloc(BG_SCREEN_SIZE));
        SetBgTilemapBuffer(2, Alloc(BG_SCREEN_SIZE));
        SetBgTilemapBuffer(1, Alloc(BG_SCREEN_SIZE));
        SetBgTilemapBuffer(0, Alloc(BG_SCREEN_SIZE));

        LoadUserWindowGfx2(0, 10, BG_PLTT_ID(14));
        LoadStdWindowGfxOnBg(0,  1, BG_PLTT_ID(15));
        DecompressAndLoadBgGfxUsingHeap(3, sTextboxBorder_Gfx, 0x100, 0, 0);
        InitWindows(sMainWindows);
        DeactivateAllTextPrinters();
        ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        gMain.state++;
        break;
    case 1:
        LoadPalette(sTextboxBorder_Pal, BG_PLTT_ID(0), sizeof(sTextboxBorder_Pal));
        LoadPalette(GetTextWindowPalette(2), BG_PLTT_ID(13), PLTT_SIZE_4BPP);
        FillBgTilemapBufferRect(0, 0x000, 0, 0, 32, 32, 17);
        FillBgTilemapBufferRect(1, 0x000, 0, 0, 32, 32, 17);
        FillBgTilemapBufferRect(2, 0x000, 0, 0, 32, 32, 17);
        MG_DrawCheckerboardPattern();
        PrintMysteryGiftOrEReaderTopMenu(isEReader, FALSE);
        gMain.state++;
        break;
    case 2:
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);
        gMain.state++;
        break;
    case 3:
        ShowBg(0);
        ShowBg(3);
        PlayBGM(MUS_MYSTERY_GIFT);
        SetVBlankCallback(VBlankCB_MysteryGiftEReader);
        EnableInterrupts(INTR_FLAG_VBLANK | INTR_FLAG_VCOUNT | INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
        return TRUE;
    }

    return FALSE;
}

void CB2_InitMysteryGift(void)
{
    if (HandleMysteryGiftOrEReaderSetup(FALSE))
    {
        SetMainCallback2(CB2_MysteryGiftEReader);
        gGiftIsFromEReader = FALSE;
        CreateMysteryGiftTask();
    }
}

void CB2_InitEReader(void)
{
    if (HandleMysteryGiftOrEReaderSetup(TRUE))
    {
        SetMainCallback2(CB2_MysteryGiftEReader);
        gGiftIsFromEReader = TRUE;
        CreateEReaderTask();
    }
}

void MainCB_FreeAllBuffersAndReturnToInitTitleScreen(void)
{
    gGiftIsFromEReader = FALSE;
    FreeAllWindowBuffers();
    Free(GetBgTilemapBuffer(0));
    Free(GetBgTilemapBuffer(1));
    Free(GetBgTilemapBuffer(2));
    Free(GetBgTilemapBuffer(3));
    SetMainCallback2(CB2_InitTitleScreen);
}

void PrintMysteryGiftOrEReaderTopMenu(bool8 isEReader, bool32 useCancel)
{
    const u8 * options;
    s32 width;
    FillWindowPixelBuffer(0, 0x00);
    if (!isEReader)
    {
        options = useCancel == TRUE ? gText_PickOKExit : gText_PickOKCancel;
        AddTextPrinterParameterized4(0, FONT_NORMAL, 2, 2, 0, 0, sTextColors_TopMenu, 0, gText_MysteryGift2);
        width = 222 - GetStringWidth(FONT_SMALL, options, 0);
        AddTextPrinterParameterized4(0, FONT_SMALL, width, 2, 0, 0, sTextColors_TopMenu, 0, options);
    }
    else
    {
        AddTextPrinterParameterized4(0, FONT_NORMAL, 2, 2, 0, 0, sTextColors_TopMenu, 0, gJPText_MysteryGift);
        AddTextPrinterParameterized4(0, FONT_SMALL, 120, 2, 0, 0, sTextColors_TopMenu, 0, gJPText_DecideStop);
    }
    CopyWindowToVram(0, COPYWIN_GFX);
    PutWindowTilemap(0);
}

void MG_DrawTextBorder(u8 windowId)
{
    DrawTextBorderOuter(windowId, 0x01, 15);
}

void MG_DrawCheckerboardPattern(void)
{
    s32 i = 0, j;

    FillBgTilemapBufferRect(3, 0x003, 0, 0, 32, 2, 17);

    for (i = 0; i < 18; i++)
    {
        for (j = 0; j < 32; j++)
        {
            if ((i & 1) != (j & 1))
                FillBgTilemapBufferRect(3, 1, j, i + 2, 1, 1, 17);
            else
                FillBgTilemapBufferRect(3, 2, j, i + 2, 1, 1, 17);
        }
    }
}

void ClearScreenInBg0(bool32 ignoreTopTwoRows)
{
    switch (ignoreTopTwoRows)
    {
    case 0:
        FillBgTilemapBufferRect(0, 0, 0, 0, 32, 32, 17);
        break;
    case 1:
        FillBgTilemapBufferRect(0, 0, 0, 2, 32, 30, 17);
        break;
    }
    CopyBgTilemapBufferToVram(0);
}

void AddTextPrinterToWindow1(const u8 *str)
{
    StringExpandPlaceholders(gStringVar4, str);
    FillWindowPixelBuffer(1, 0x11);
    AddTextPrinterParameterized4(1, FONT_NORMAL, 0, 2, 0, 2, sMG_Ereader_TextColor_2, 0, gStringVar4);
    DrawTextBorderOuter(1, 0x001, 15);
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_FULL);
}

static void ClearTextWindow(void)
{
    rbox_fill_rectangle(1);
    ClearWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_MAP);
}

#define DOWN_ARROW_X 208
#define DOWN_ARROW_Y 20

bool32 PrintMysteryGiftMenuMessage(u8 *textState, const u8 *str)
{
    switch (*textState)
    {
    case 0:
        AddTextPrinterToWindow1(str);
        goto inc;
    case 1:
        DrawDownArrow(1, DOWN_ARROW_X, DOWN_ARROW_Y, 1, FALSE, &sDownArrowCounterAndYCoordIdx[0], &sDownArrowCounterAndYCoordIdx[1]);
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            inc:
            (*textState)++;
        }
        break;
    case 2:
        DrawDownArrow(1, DOWN_ARROW_X, DOWN_ARROW_Y, 1, TRUE, &sDownArrowCounterAndYCoordIdx[0], &sDownArrowCounterAndYCoordIdx[1]);
        *textState = 0;
        ClearTextWindow();
        return TRUE;
    case 0xFF:
        *textState = 2;
        break;
    }
    return FALSE;
}

static void HideDownArrow(void)
{
    DrawDownArrow(1, DOWN_ARROW_X, DOWN_ARROW_Y, 1, FALSE, &sDownArrowCounterAndYCoordIdx[0], &sDownArrowCounterAndYCoordIdx[1]);
}

static void ShowDownArrow(void)
{
    DrawDownArrow(1, DOWN_ARROW_X, DOWN_ARROW_Y, 1, TRUE, &sDownArrowCounterAndYCoordIdx[0], &sDownArrowCounterAndYCoordIdx[1]);
}

static bool32 PrintStringAndWait2Seconds(u8 * counter, const u8 * str)
{
    if (*counter == 0)
        AddTextPrinterToWindow1(str);

    if (++(*counter) > 120)
    {
        *counter = 0;
        ClearTextWindow();
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static u32 MysteryGift_HandleThreeOptionMenu(u8 * unused0, u16 * unused1, u8 whichMenu)
{
    struct ListMenuTemplate listMenuTemplate = sListMenuTemplate_ThreeOptions;
    struct WindowTemplate windowTemplate = sWindowTemplate_ThreeOptions;
    u32 width;
    s32 finalWidth;
    s32 response;
    u32 i;

    if (whichMenu == 0)
        listMenuTemplate.items = sListMenuItems_CardsOrNews;
    else
        listMenuTemplate.items = sListMenuItems_WirelessOrFriend;

    width = 0;
    for (i = 0; i < listMenuTemplate.totalItems; i++)
    {
        u32 curWidth = GetStringWidth(FONT_NORMAL, listMenuTemplate.items[i].label, listMenuTemplate.lettersSpacing);
        if (curWidth > width)
            width = curWidth;
    }
    finalWidth = (((width + 9) / 8) + 2) & ~1;
    windowTemplate.width = finalWidth;
    windowTemplate.tilemapLeft = (30 - finalWidth) / 2;
    response = DoMysteryGiftListMenu(&windowTemplate, &listMenuTemplate, 1, 0x00A, BG_PLTT_ID(14));
    if (response != LIST_NOTHING_CHOSEN)
    {
        ClearWindowTilemap(2);
        CopyWindowToVram(2, COPYWIN_MAP);
    }
    return response;
}

s8 DoMysteryGiftYesNo(u8 * textState, u16 * windowId, bool8 yesNoBoxPlacement, const u8 * str)
{
    struct WindowTemplate windowTemplate;
    s8 input;

    switch (*textState)
    {
    case 0:
        // Print question message
        StringExpandPlaceholders(gStringVar4, str);
        if (yesNoBoxPlacement == 0)
            *windowId = AddWindow(&sWindowTemplate_YesNoMsg_Wide);
        else
            *windowId = AddWindow(&sWindowTemplate_YesNoMsg);
        FillWindowPixelBuffer(*windowId, 0x11);
        AddTextPrinterParameterized4(*windowId, FONT_NORMAL, 0, 2, 0, 2, sMG_Ereader_TextColor_2, 0, gStringVar4);
        DrawTextBorderOuter(*windowId, 0x001, 15);
        CopyWindowToVram(*windowId, COPYWIN_GFX);
        PutWindowTilemap(*windowId);
        (*textState)++;
        break;
    case 1:
        // Create Yes/No
        windowTemplate = sWindowTemplate_YesNoBox;
        if (yesNoBoxPlacement == 0)
            windowTemplate.tilemapTop = 9;
        else
            windowTemplate.tilemapTop = 15;
        CreateYesNoMenu(&windowTemplate, FONT_NORMAL, 0, 2, 10, 14, 0);
        (*textState)++;
        break;
    case 2:
        // Handle Yes/No input
        input = Menu_ProcessInputNoWrapClearOnChoose();
        if (input == MENU_B_PRESSED || input == 0 || input == 1)
        {
            *textState = 0;
            rbox_fill_rectangle(*windowId);
            ClearWindowTilemap(*windowId);
            CopyWindowToVram(*windowId, COPYWIN_MAP);
            RemoveWindow(*windowId);
            return input;
        }
        break;
    case 0xFF:
        *textState = 0;
        rbox_fill_rectangle(*windowId);
        ClearWindowTilemap(*windowId);
        CopyWindowToVram(*windowId, COPYWIN_MAP);
        RemoveWindow(*windowId);
        return MENU_B_PRESSED;
    }

    return MENU_NOTHING_CHOSEN;
}

// Handle the "Receive/Send/Toss" menu that appears when selecting Wonder Card/News
static s32 HandleMysteryGiftListMenu(u8 * textState, u16 * windowId, bool32 cannotToss, bool32 cannotSend)
{
    struct WindowTemplate windowTemplate;
    s32 input;

    switch (*textState)
    {
    case 0:
        // Print menu message
        if (!cannotToss)
            StringExpandPlaceholders(gStringVar4, gText_WhatToDoWithCards);
        else
            StringExpandPlaceholders(gStringVar4, gText_WhatToDoWithNews);
        *windowId = AddWindow(&sWindowTemplate_GiftSelect);
        FillWindowPixelBuffer(*windowId, 0x11);
        AddTextPrinterParameterized4(*windowId, FONT_NORMAL, 0, 2, 0, 2, sMG_Ereader_TextColor_2, 0, gStringVar4);
        DrawTextBorderOuter(*windowId, 0x001, 15);
        CopyWindowToVram(*windowId, COPYWIN_GFX);
        PutWindowTilemap(*windowId);
        (*textState)++;
        break;
    case 1:
        windowTemplate = sWindowTemplate_YesNoBox;
        if (cannotSend)
        {
            if (!cannotToss)
                input = DoMysteryGiftListMenu(&sWindowTemplate_GiftSelect_2Options, &sListMenu_ReceiveToss, 1, 0x00A, BG_PLTT_ID(14));
            else
                input = DoMysteryGiftListMenu(&sWindowTemplate_GiftSelect_1Option, &sListMenu_Receive, 1, 0x00A, BG_PLTT_ID(14));
        }
        else
        {
            if (!cannotToss)
                input = DoMysteryGiftListMenu(&sWindowTemplate_GiftSelect_3Options, &sListMenu_ReceiveSendToss, 1, 0x00A, BG_PLTT_ID(14));
            else
                input = DoMysteryGiftListMenu(&sWindowTemplate_GiftSelect_2Options, &sListMenu_ReceiveSend, 1, 0x00A, BG_PLTT_ID(14));
        }
        if (input != LIST_NOTHING_CHOSEN)
        {
            *textState = 0;
            rbox_fill_rectangle(*windowId);
            ClearWindowTilemap(*windowId);
            CopyWindowToVram(*windowId, COPYWIN_MAP);
            RemoveWindow(*windowId);
            return input;
        }
        break;
    case 0xFF:
        *textState = 0;
        rbox_fill_rectangle(*windowId);
        ClearWindowTilemap(*windowId);
        CopyWindowToVram(*windowId, COPYWIN_MAP);
        RemoveWindow(*windowId);
        return LIST_CANCEL;
    }

    return LIST_NOTHING_CHOSEN;
}

static bool32 ValidateCardOrNews(bool32 isWonderNews)
{
    if (!isWonderNews)
        return ValidateSavedWonderCard();
    else
        return ValidateSavedWonderNews();
}

static bool32 HandleLoadWonderCardOrNews(u8 * state, bool32 isWonderNews)
{
    switch (*state)
    {
    case 0:
        if (!isWonderNews)
            WonderCard_Init(GetSavedWonderCard(), GetSavedWonderCardMetadata());
        else
            WonderNews_Init(GetSavedWonderNews());
        (*state)++;
        break;
    case 1:
        if (!isWonderNews)
        {
            if (!WonderCard_Enter())
                return FALSE;
        }
        else
        {
            if (!WonderNews_Enter())
                return FALSE;
        }
        *state = 0;
        return TRUE;
    }

    return FALSE;
}

static bool32 ClearSavedNewsOrCard(bool32 isWonderNews)
{
    if (!isWonderNews)
        ClearSavedWonderCardAndRelated();
    else
        ClearSavedWonderNewsAndRelated();
    return TRUE;
}

static bool32 ExitWonderCardOrNews(bool32 isWonderNews, bool32 useCancel)
{
    if (!isWonderNews)
    {
        if (WonderCard_Exit(useCancel))
        {
            WonderCard_Destroy();
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        if (WonderNews_Exit(useCancel))
        {
            WonderNews_Destroy();
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

static s32 AskDiscardGift(u8 * textState, u16 * windowId, bool32 isWonderNews)
{
    if (!isWonderNews)
        return DoMysteryGiftYesNo(textState, windowId, TRUE, gText_IfThrowAwayCardEventWontHappen);
    else
        return DoMysteryGiftYesNo(textState, windowId, TRUE, gText_OkayToDiscardNews);
}

static bool32 PrintThrownAway(u8 * textState, bool32 isWonderNews)
{
    if (!isWonderNews)
        return PrintMysteryGiftMenuMessage(textState, gText_WonderCardThrownAway);
    else
        return PrintMysteryGiftMenuMessage(textState, gText_WonderNewsThrownAway);
}

static bool32 SaveOnMysteryGiftMenu(u8 * state)
{
    switch (*state)
    {
    case 0:
        AddTextPrinterToWindow1(gText_DataWillBeSaved);
        (*state)++;
        break;
    case 1:
        TrySavingData(SAVE_NORMAL);
        (*state)++;
        break;
    case 2:
        AddTextPrinterToWindow1(gText_SaveCompletedPressA);
        (*state)++;
        break;
    case 3:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
            (*state)++;
        break;
    case 4:
        *state = 0;
        ClearTextWindow();
        return TRUE;
    }

    return FALSE;
}

static const u8 * GetClientResultMessage(bool32 * successMsg, bool8 isWonderNews, bool8 sourceIsFriend, u32 msgId)
{
    const u8 * msg = NULL;
    *successMsg = FALSE;

    switch (msgId)
    {
    case CLI_MSG_NOTHING_SENT:
        *successMsg = FALSE;
        msg = gText_NothingSentOver;
        break;
    case CLI_MSG_RECORD_UPLOADED:
        *successMsg = FALSE;
        msg = gText_RecordUploadedViaWireless;
        break;
    case CLI_MSG_CARD_RECEIVED:
        *successMsg = TRUE;
        msg = !sourceIsFriend ? gText_WonderCardReceived : gText_WonderCardReceivedFrom;
        break;
    case CLI_MSG_NEWS_RECEIVED:
        *successMsg = TRUE;
        msg = !sourceIsFriend ? gText_WonderNewsReceived : gText_WonderNewsReceivedFrom;
        break;
    case CLI_MSG_STAMP_RECEIVED:
        *successMsg = TRUE;
        msg = gText_NewStampReceived;
        break;
    case CLI_MSG_HAD_CARD:
        *successMsg = FALSE;
        msg = gText_AlreadyHadCard;
        break;
    case CLI_MSG_HAD_STAMP:
        *successMsg = FALSE;
        msg = gText_AlreadyHadStamp;
        break;
    case CLI_MSG_HAD_NEWS:
        *successMsg = FALSE;
        msg = gText_AlreadyHadNews;
        break;
    case CLI_MSG_NO_ROOM_STAMPS:
        *successMsg = FALSE;
        msg = gText_NoMoreRoomForStamps;
        break;
    case CLI_MSG_COMM_CANCELED:
        *successMsg = FALSE;
        msg = gText_CommunicationCanceled;
        break;
    case CLI_MSG_CANT_ACCEPT:
        *successMsg = FALSE;
        msg = !isWonderNews ? gText_CantAcceptCardFromTrainer : gText_CantAcceptNewsFromTrainer;
        break;
    case CLI_MSG_COMM_ERROR:
        *successMsg = FALSE;
        msg = gText_CommunicationError;
        break;
    case CLI_MSG_TRAINER_RECEIVED:
        *successMsg = TRUE;
        msg = gText_NewTrainerReceived;
        break;
    case CLI_MSG_BUFFER_SUCCESS:
        *successMsg = TRUE;
        // msg is NULL, use buffer
        break;
    case CLI_MSG_BUFFER_FAILURE:
        *successMsg = FALSE;
        // msg is NULL, use buffer
        break;
    }

    return msg;
}

static bool32 PrintSuccessMessage(u8 * state, const u8 * msg, u16 * timer)
{
    switch (*state)
    {
    case 0:
        if (msg != NULL)
            AddTextPrinterToWindow1(msg);
        PlayFanfare(MUS_OBTAIN_ITEM);
        *timer = 0;
        (*state)++;
        break;
    case 1:
        if (++(*timer) > 240)
            (*state)++;
        break;
    case 2:
        if (IsFanfareTaskInactive())
        {
            *state = 0;
            ClearTextWindow();
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static const u8 * GetServerResultMessage(bool32 * wonderSuccess, u8 unused, u32 msgId)
{
    const u8 * result = gText_CommunicationError;
    *wonderSuccess = FALSE;
    switch (msgId)
    {
    case SVR_MSG_NOTHING_SENT:
        result = gText_NothingSentOver;
        break;
    case SVR_MSG_RECORD_UPLOADED:
        result = gText_RecordUploadedViaWireless;
        break;
    case SVR_MSG_CARD_SENT:
        result = gText_WonderCardSentTo;
        *wonderSuccess = TRUE;
        break;
    case SVR_MSG_NEWS_SENT:
        result = gText_WonderNewsSentTo;
        *wonderSuccess = TRUE;
        break;
    case SVR_MSG_STAMP_SENT:
        result = gText_StampSentTo;
        break;
    case SVR_MSG_HAS_CARD:
        result = gText_OtherTrainerHasCard;
        break;
    case SVR_MSG_HAS_STAMP:
        result = gText_OtherTrainerHasStamp;
        break;
    case SVR_MSG_HAS_NEWS:
        result = gText_OtherTrainerHasNews;
        break;
    case SVR_MSG_NO_ROOM_STAMPS:
        result = gText_NoMoreRoomForStamps;
        break;
    case SVR_MSG_CLIENT_CANCELED:
        result = gText_OtherTrainerCanceled;
        break;
    case SVR_MSG_CANT_SEND_GIFT_1:
        result = gText_CantSendGiftToTrainer;
        break;
    case SVR_MSG_COMM_ERROR:
        result = gText_CommunicationError;
        break;
    case SVR_MSG_GIFT_SENT_1:
        result = gText_GiftSentTo;
        break;
    case SVR_MSG_GIFT_SENT_2:
        result = gText_GiftSentTo;
        break;
    case SVR_MSG_CANT_SEND_GIFT_2:
        result = gText_CantSendGiftToTrainer;
        break;
    }
    return result;
}

static bool32 PrintServerResultMessage(u8 * state, u16 * timer, bool8 sourceIsFriend, u32 msgId)
{
    bool32 wonderSuccess;
    const u8 * str = GetServerResultMessage(&wonderSuccess, sourceIsFriend, msgId);
    if (wonderSuccess)
        return PrintSuccessMessage(state, str, timer);
    else
        return PrintMysteryGiftMenuMessage(state, str);
}

// States for Task_MysteryGift.
// CLIENT states are for when the player is receiving a gift, and use mystery_gift_client.c link functions.
// SERVER states are for when the player is sending a gift, and use mystery_gift_server.c link functions.
// Other states handle the general Mystery Gift menu usage.
enum {
    MG_STATE_TO_MAIN_MENU,
    MG_STATE_MAIN_MENU,
    MG_STATE_DONT_HAVE_ANY,
    MG_STATE_SOURCE_PROMPT,
    MG_STATE_SOURCE_PROMPT_INPUT,
    MG_STATE_CLIENT_LINK_START,
    MG_STATE_CLIENT_LINK_WAIT,
    MG_STATE_CLIENT_COMMUNICATING,
    MG_STATE_CLIENT_LINK,
    MG_STATE_CLIENT_YES_NO,
    MG_STATE_CLIENT_MESSAGE,
    MG_STATE_CLIENT_ASK_TOSS,
    MG_STATE_CLIENT_ASK_TOSS_UNRECEIVED,
    MG_STATE_CLIENT_LINK_END,
    MG_STATE_CLIENT_COMM_COMPLETED,
    MG_STATE_CLIENT_RESULT_MSG,
    MG_STATE_CLIENT_ERROR,
    MG_STATE_SAVE_LOAD_GIFT,
    MG_STATE_LOAD_GIFT,
    MG_STATE_UNUSED,
    MG_STATE_HANDLE_GIFT_INPUT,
    MG_STATE_HANDLE_GIFT_SELECT,
    MG_STATE_ASK_TOSS,
    MG_STATE_ASK_TOSS_UNRECEIVED,
    MG_STATE_TOSS,
    MG_STATE_TOSS_SAVE,
    MG_STATE_TOSSED,
    MG_STATE_GIFT_INPUT_EXIT,
    MG_STATE_RECEIVE,
    MG_STATE_SEND,
    MG_STATE_SERVER_LINK_WAIT,
    MG_STATE_SERVER_LINK_START,
    MG_STATE_SERVER_LINK,
    MG_STATE_SERVER_LINK_END,
    MG_STATE_SERVER_LINK_END_WAIT,
    MG_STATE_SERVER_RESULT_MSG,
    MG_STATE_SERVER_ERROR,
    MG_STATE_EXIT,
};

static void CreateMysteryGiftTask(void)
{
    u8 taskId = CreateTask(Task_MysteryGift, 0);
    struct MysteryGiftTaskData * data = (void *)gTasks[taskId].data;
    data->state = MG_STATE_TO_MAIN_MENU;
    data->textState = 0;
    data->isWonderNews = FALSE;
    data->sourceIsFriend = FALSE;
    data->var = 0;
    data->msgId = 0;
    data->clientMsg = AllocZeroed(CLIENT_MAX_MSG_SIZE);
}

static void Task_MysteryGift(u8 taskId)
{
    struct MysteryGiftTaskData * data = (void *)gTasks[taskId].data;
    bool32 successMsg, input;
    const u8 * msg;

    switch (data->state)
    {
    case MG_STATE_TO_MAIN_MENU:
        data->state = MG_STATE_MAIN_MENU;
        break;
    case MG_STATE_MAIN_MENU:
        // Main Mystery Gift menu, player can select Wonder Cards or News (or exit)
        switch (MysteryGift_HandleThreeOptionMenu(&data->textState, &data->var, FALSE))
        {
        case 0: // "Wonder Cards"
            data->isWonderNews = FALSE;
            if (ValidateSavedWonderCard() == TRUE)
                data->state = MG_STATE_LOAD_GIFT;
            else
                data->state = MG_STATE_DONT_HAVE_ANY;
            break;
        case 1: // "Wonder News"
            data->isWonderNews = TRUE;
            if (ValidateSavedWonderNews() == TRUE)
                data->state = MG_STATE_LOAD_GIFT;
            else
                data->state = MG_STATE_DONT_HAVE_ANY;
            break;
        case LIST_CANCEL:
            data->state = MG_STATE_EXIT;
            break;
        }
        break;
    case MG_STATE_DONT_HAVE_ANY:
    {
        // Player doesn't have any Wonder Card/News
        // Start prompt to ask where to read one from
        if (!data->isWonderNews)
        {
            if (PrintMysteryGiftMenuMessage(&data->textState, gText_DontHaveCardNewOneInput))
            {
                data->state = MG_STATE_SOURCE_PROMPT;
                PrintMysteryGiftOrEReaderTopMenu(FALSE, TRUE);
            }
        }
        else
        {
            if (PrintMysteryGiftMenuMessage(&data->textState, gText_DontHaveNewsNewOneInput))
            {
                data->state = MG_STATE_SOURCE_PROMPT;
                PrintMysteryGiftOrEReaderTopMenu(FALSE, TRUE);
            }
        }
        break;
    }
    case MG_STATE_SOURCE_PROMPT:
        if (!data->isWonderNews)
            AddTextPrinterToWindow1(gText_WhereShouldCardBeAccessed);
        else
            AddTextPrinterToWindow1(gText_WhereShouldNewsBeAccessed);
        data->state = MG_STATE_SOURCE_PROMPT_INPUT;
        break;
    case MG_STATE_SOURCE_PROMPT_INPUT:
        // Choose where to access the Wonder Card/News from
        switch (MysteryGift_HandleThreeOptionMenu(&data->textState, &data->var, TRUE))
        {
        case 0: // "Wireless Communication"
            ClearTextWindow();
            data->state = MG_STATE_CLIENT_LINK_START;
            data->sourceIsFriend = FALSE;
            break;
        case 1: // "Friend"
            ClearTextWindow();
            data->state = MG_STATE_CLIENT_LINK_START;
            data->sourceIsFriend = TRUE;
            break;
        case LIST_CANCEL:
            ClearTextWindow();
            if (ValidateCardOrNews(data->isWonderNews))
            {
                data->state = MG_STATE_LOAD_GIFT;
            }
            else
            {
                data->state = MG_STATE_TO_MAIN_MENU;
                PrintMysteryGiftOrEReaderTopMenu(FALSE, FALSE);
            }
            break;
        }
        break;
    case MG_STATE_CLIENT_LINK_START:
        *gStringVar1 = EOS;
        *gStringVar2 = EOS;
        *gStringVar3 = EOS;

        switch (data->isWonderNews)
        {
        case FALSE:
            if (data->sourceIsFriend == TRUE)
                CreateTask_LinkMysteryGiftWithFriend(ACTIVITY_WONDER_CARD);
            else if (data->sourceIsFriend == FALSE)
                CreateTask_LinkMysteryGiftOverWireless(ACTIVITY_WONDER_CARD);
            break;
        case TRUE:
            if (data->sourceIsFriend == TRUE)
                CreateTask_LinkMysteryGiftWithFriend(ACTIVITY_WONDER_NEWS);
            else if (data->sourceIsFriend == FALSE)
                CreateTask_LinkMysteryGiftOverWireless(ACTIVITY_WONDER_NEWS);
            break;
        }
        data->state = MG_STATE_CLIENT_LINK_WAIT;
        break;
    case MG_STATE_CLIENT_LINK_WAIT:
        if (gReceivedRemoteLinkPlayers)
        {
            ClearScreenInBg0(TRUE);
            data->state = MG_STATE_CLIENT_COMMUNICATING;
            MysteryGiftClient_Create();
        }
        else if (gSpecialVar_Result == LINKUP_FAILED)
        {
            // Link failed, return to link start menu
            ClearScreenInBg0(TRUE);
            data->state = MG_STATE_SOURCE_PROMPT;
        }
        break;
    case MG_STATE_CLIENT_COMMUNICATING:
        AddTextPrinterToWindow1(gText_Communicating);
        data->state = MG_STATE_CLIENT_LINK;
        break;
    case MG_STATE_CLIENT_LINK:
        switch (MysteryGiftClient_Run(&data->var))
        {
        case CLI_RET_END:
            Rfu_SetCloseLinkCallback();
            data->msgId = data->var;
            data->state = MG_STATE_CLIENT_LINK_END;
            break;
        case CLI_RET_COPY_MSG:
            memcpy(data->clientMsg, MysteryGiftClient_GetMsg(), CLIENT_MAX_MSG_SIZE);
            MysteryGiftClient_AdvanceState();
            break;
        case CLI_RET_PRINT_MSG:
            data->state = MG_STATE_CLIENT_MESSAGE;
            break;
        case CLI_RET_YES_NO:
            data->state = MG_STATE_CLIENT_YES_NO;
            break;
        case CLI_RET_ASK_TOSS:
            data->state = MG_STATE_CLIENT_ASK_TOSS;
            StringCopy(gStringVar1, gLinkPlayers[0].name);
            break;
        }
        break;
    case MG_STATE_CLIENT_YES_NO:
        input = DoMysteryGiftYesNo(&data->textState, &data->var, FALSE, MysteryGiftClient_GetMsg());
        switch (input)
        {
        case 0: // Yes
            MysteryGiftClient_SetParam(FALSE);
            MysteryGiftClient_AdvanceState();
            data->state = MG_STATE_CLIENT_COMMUNICATING;
            break;
        case 1: // No
        case MENU_B_PRESSED:
            MysteryGiftClient_SetParam(TRUE);
            MysteryGiftClient_AdvanceState();
            data->state = MG_STATE_CLIENT_COMMUNICATING;
            break;
        }
        break;
    case MG_STATE_CLIENT_MESSAGE:
        if (PrintMysteryGiftMenuMessage(&data->textState, MysteryGiftClient_GetMsg()))
        {
            MysteryGiftClient_AdvanceState();
            data->state = MG_STATE_CLIENT_COMMUNICATING;
        }
        break;
    case MG_STATE_CLIENT_ASK_TOSS:
        // Player is receiving a new Wonder Card/News but needs to toss an existing one to make room.
        // Ask for confirmation.
        input = DoMysteryGiftYesNo(&data->textState, &data->var, FALSE, gText_ThrowAwayWonderCard);
        switch (input)
        {
        case 0: // Yes
            if (IsSavedWonderCardGiftNotReceived() == TRUE)
            {
                data->state = MG_STATE_CLIENT_ASK_TOSS_UNRECEIVED;
            }
            else
            {
                MysteryGiftClient_SetParam(FALSE);
                MysteryGiftClient_AdvanceState();
                data->state = MG_STATE_CLIENT_COMMUNICATING;
            }
            break;
        case 1: // No
        case MENU_B_PRESSED:
            MysteryGiftClient_SetParam(TRUE);
            MysteryGiftClient_AdvanceState();
            data->state = MG_STATE_CLIENT_COMMUNICATING;
            break;
        }
        break;
    case MG_STATE_CLIENT_ASK_TOSS_UNRECEIVED:
        // Player has selected to toss a Wonder Card that they haven't received the gift for.
        // Ask for confirmation again.
        input = DoMysteryGiftYesNo(&data->textState, &data->var, FALSE, gText_HaventReceivedCardsGift);
        switch (input)
        {
        case 0: // Yes
            MysteryGiftClient_SetParam(FALSE);
            MysteryGiftClient_AdvanceState();
            data->state = MG_STATE_CLIENT_COMMUNICATING;
            break;
        case 1: // No
        case MENU_B_PRESSED:
            MysteryGiftClient_SetParam(TRUE);
            MysteryGiftClient_AdvanceState();
            data->state = MG_STATE_CLIENT_COMMUNICATING;
            break;
        }
        break;
    case MG_STATE_CLIENT_LINK_END:
        if (IsLinkRfuTaskFinished())
        {
            DestroyWirelessStatusIndicatorSprite();
            data->state = MG_STATE_CLIENT_COMM_COMPLETED;
        }
        break;
    case MG_STATE_CLIENT_COMM_COMPLETED:
        if (PrintStringAndWait2Seconds(&data->textState, gText_CommunicationCompleted))
        {
            if (data->sourceIsFriend == TRUE)
                StringCopy(gStringVar1, gLinkPlayers[0].name);
            data->state = MG_STATE_CLIENT_RESULT_MSG;
        }
        break;
    case MG_STATE_CLIENT_RESULT_MSG:
        msg = GetClientResultMessage(&successMsg, data->isWonderNews, data->sourceIsFriend, data->msgId);
        if (msg == NULL)
            msg = data->clientMsg;
        if (successMsg)
            input = PrintSuccessMessage(&data->textState, msg, &data->var);
        else
            input = PrintMysteryGiftMenuMessage(&data->textState, msg);

        // input var re-used, here it is TRUE if the message is finished
        if (input)
        {
            if (data->msgId == CLI_MSG_NEWS_RECEIVED)
            {
                if (data->sourceIsFriend == TRUE)
                    WonderNews_SetReward(WONDER_NEWS_RECV_FRIEND);
                else
                    WonderNews_SetReward(WONDER_NEWS_RECV_WIRELESS);
            }
            if (!successMsg)
            {
                // Did not receive card/news, return to main menu
                data->state = MG_STATE_TO_MAIN_MENU;
                PrintMysteryGiftOrEReaderTopMenu(FALSE, FALSE);
            }
            else
            {
                data->state = MG_STATE_SAVE_LOAD_GIFT;
            }
        }
        break;
    case MG_STATE_SAVE_LOAD_GIFT:
        if (SaveOnMysteryGiftMenu(&data->textState))
        {
            data->state = MG_STATE_TO_MAIN_MENU;
            PrintMysteryGiftOrEReaderTopMenu(FALSE, FALSE);
        }
        break;
    case MG_STATE_LOAD_GIFT:
        if (HandleLoadWonderCardOrNews(&data->textState, data->isWonderNews))
            data->state = MG_STATE_HANDLE_GIFT_INPUT;
        break;
    case MG_STATE_HANDLE_GIFT_INPUT:
        if (!data->isWonderNews)
        {
            // Handle Wonder Card input
            if (JOY_NEW(A_BUTTON))
                data->state = MG_STATE_HANDLE_GIFT_SELECT;
            if (JOY_NEW(B_BUTTON))
                data->state = MG_STATE_GIFT_INPUT_EXIT;
        }
        else
        {
            switch (WonderNews_GetInput(gMain.newKeys))
            {
            case NEWS_INPUT_A:
                WonderNews_RemoveScrollIndicatorArrowPair();
                data->state = MG_STATE_HANDLE_GIFT_SELECT;
                break;
            case NEWS_INPUT_B:
                data->state = MG_STATE_GIFT_INPUT_EXIT;
                break;
            }
        }
        break;
    case MG_STATE_HANDLE_GIFT_SELECT:
    {
        // A Wonder Card/News has been selected, handle its menu
        u32 result;
        if (!data->isWonderNews)
        {
            if (IsSendingSavedWonderCardAllowed())
                result = HandleMysteryGiftListMenu(&data->textState, &data->var, data->isWonderNews, FALSE);
            else
                result = HandleMysteryGiftListMenu(&data->textState, &data->var, data->isWonderNews, TRUE);
        }
        else
        {
            if (IsSendingSavedWonderNewsAllowed())
                result = HandleMysteryGiftListMenu(&data->textState, &data->var, data->isWonderNews, FALSE);
            else
                result = HandleMysteryGiftListMenu(&data->textState, &data->var, data->isWonderNews, TRUE);
        }
        switch (result)
        {
        case 0: // Receive
            data->state = MG_STATE_RECEIVE;
            break;
        case 1: // Send
            data->state = MG_STATE_SEND;
            break;
        case 2: // Toss
            data->state = MG_STATE_ASK_TOSS;
            break;
        case LIST_CANCEL:
            if (data->isWonderNews == TRUE)
                WonderNews_AddScrollIndicatorArrowPair();
            data->state = MG_STATE_HANDLE_GIFT_INPUT;
            break;
        }
        break;
    }
    case MG_STATE_ASK_TOSS:
        // Player is attempting to discard a saved Wonder Card/News
        switch (AskDiscardGift(&data->textState, &data->var, data->isWonderNews))
        {
        case 0: // Yes
            if (!data->isWonderNews && IsSavedWonderCardGiftNotReceived() == TRUE)
                data->state = MG_STATE_ASK_TOSS_UNRECEIVED;
            else
                data->state = MG_STATE_TOSS;
            break;
        case 1: // No
        case MENU_B_PRESSED:
            data->state = MG_STATE_HANDLE_GIFT_SELECT;
            break;
        }
        break;
    case MG_STATE_ASK_TOSS_UNRECEIVED:
        // Player has selected to toss a Wonder Card that they haven't received the gift for.
        // Ask for confirmation again.
        switch ((u32)DoMysteryGiftYesNo(&data->textState, &data->var, TRUE, gText_HaventReceivedGiftOkayToDiscard))
        {
        case 0: // Yes
            data->state = MG_STATE_TOSS;
            break;
        case 1: // No
        case MENU_B_PRESSED:
            data->state = MG_STATE_HANDLE_GIFT_SELECT;
            break;
        }
        break;
    case MG_STATE_TOSS:
        if (ExitWonderCardOrNews(data->isWonderNews, TRUE))
        {
            ClearSavedNewsOrCard(data->isWonderNews);
            data->state = MG_STATE_TOSS_SAVE;
        }
        break;
    case MG_STATE_TOSS_SAVE:
        if (SaveOnMysteryGiftMenu(&data->textState))
            data->state = MG_STATE_TOSSED;
        break;
    case MG_STATE_TOSSED:
        if (PrintThrownAway(&data->textState, data->isWonderNews))
        {
            data->state = MG_STATE_TO_MAIN_MENU;
            PrintMysteryGiftOrEReaderTopMenu(FALSE, FALSE);
        }
        break;
    case MG_STATE_GIFT_INPUT_EXIT:
        if (ExitWonderCardOrNews(data->isWonderNews, FALSE))
            data->state = MG_STATE_TO_MAIN_MENU;
        break;
    case MG_STATE_RECEIVE:
        if (ExitWonderCardOrNews(data->isWonderNews, TRUE))
            data->state = MG_STATE_SOURCE_PROMPT;
        break;
    case MG_STATE_SEND:
        if (ExitWonderCardOrNews(data->isWonderNews, TRUE))
        {
            switch (data->isWonderNews)
            {
            case FALSE:
                CreateTask_SendMysteryGift(ACTIVITY_WONDER_CARD);
                break;
            case TRUE:
                CreateTask_SendMysteryGift(ACTIVITY_WONDER_NEWS);
                break;
            }
            data->sourceIsFriend = TRUE;
            data->state = MG_STATE_SERVER_LINK_WAIT;
        }
        break;
    case MG_STATE_SERVER_LINK_WAIT:
        if (gReceivedRemoteLinkPlayers)
        {
            ClearScreenInBg0(TRUE);
            data->state = MG_STATE_SERVER_LINK_START;
        }
        else if (gSpecialVar_Result == LINKUP_FAILED)
        {
            ClearScreenInBg0(TRUE);
            data->state = MG_STATE_LOAD_GIFT;
        }
        break;
    case MG_STATE_SERVER_LINK_START:
        *gStringVar1 = EOS;
        *gStringVar2 = EOS;
        *gStringVar3 = EOS;
        if (!data->isWonderNews)
        {
            AddTextPrinterToWindow1(gText_SendingWonderCard);
            MysterGiftServer_CreateForCard();
        }
        else
        {
            AddTextPrinterToWindow1(gText_SendingWonderNews);
            MysterGiftServer_CreateForNews();
        }
        data->state = MG_STATE_SERVER_LINK;
        break;
    case MG_STATE_SERVER_LINK:
        if (MysterGiftServer_Run(&data->var) == SVR_RET_END)
        {
            data->msgId = data->var;
            data->state = MG_STATE_SERVER_LINK_END;
        }
        break;
    case MG_STATE_SERVER_LINK_END:
        Rfu_SetCloseLinkCallback();
        StringCopy(gStringVar1, gLinkPlayers[1].name);
        data->state = MG_STATE_SERVER_LINK_END_WAIT;
        break;
    case MG_STATE_SERVER_LINK_END_WAIT:
        if (IsLinkRfuTaskFinished())
        {
            DestroyWirelessStatusIndicatorSprite();
            data->state = MG_STATE_SERVER_RESULT_MSG;
        }
        break;
    case MG_STATE_SERVER_RESULT_MSG:
        if (PrintServerResultMessage(&data->textState, &data->var, data->sourceIsFriend, data->msgId))
        {
            if (data->sourceIsFriend == TRUE && data->msgId == SVR_MSG_NEWS_SENT)
            {
                WonderNews_SetReward(WONDER_NEWS_SENT);
                data->state = MG_STATE_SAVE_LOAD_GIFT;
            }
            else
            {
                data->state = MG_STATE_TO_MAIN_MENU;
                PrintMysteryGiftOrEReaderTopMenu(FALSE, FALSE);
            }
        }
        break;
    case MG_STATE_CLIENT_ERROR:
    case MG_STATE_SERVER_ERROR:
        if (PrintMysteryGiftMenuMessage(&data->textState, gText_CommunicationError))
        {
            data->state = MG_STATE_TO_MAIN_MENU;
            PrintMysteryGiftOrEReaderTopMenu(FALSE, FALSE);
        }
        break;
    case MG_STATE_EXIT:
        CloseLink();
        HelpSystem_Enable();
        Free(data->clientMsg);
        DestroyTask(taskId);
        SetMainCallback2(MainCB_FreeAllBuffersAndReturnToInitTitleScreen);
        break;
    }
}

u16 GetMysteryGiftBaseBlock(void)
{
    return 0x19B;
}
