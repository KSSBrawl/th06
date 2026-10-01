#pragma once
#include <d3dx8math.h>

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "StageMenu.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "ZunTimer.hpp"
#include "decomp.hpp"

namespace th06
{
#define TEXT_RIGHT_ARROW 0x7f

#define ASCII_SCORE_POPUPS_COUNT 512
#define ASCII_PLAYER_POPUPS_COUNT 3
#define ASCII_TOTAL_POPUPS_COUNT (ASCII_SCORE_POPUPS_COUNT + ASCII_PLAYER_POPUPS_COUNT)

#define ASCII_SCORE_POPUPS_START 0
#define ASCII_PLAYER_POPUPS_START ASCII_SCORE_POPUPS_COUNT

#define ASCII_STRING_COUNT 256

struct AsciiManagerString
{
    char text[64];
    D3DXVECTOR3 position;
    D3DCOLOR color;
    D3DXVECTOR2 scale;
    // If true, we are drawing the currently selected element of the MainMenu
    // class.
    ZunBool isSelected;
    // If true, we are drawing an element of the Gui class.
    ZunBool isGui;
};
ZUN_ASSERT_TYPE(AsciiManagerString, 0x60, 4);

struct AsciiManagerPopup
{
    char digits[8];
    D3DXVECTOR3 position;
    D3DCOLOR color;
    ZunTimer timer;
    u8 inUse;
    u8 characterCount;
    alignment_padding(0x2);
};
ZUN_ASSERT_TYPE(AsciiManagerPopup, 0x28, 4);

// The AsciiManager is responsible for drawing various textual elements on the
// screen:
//
// - The FPS counter
// - The in-game menus
// - Various text elements such as the "Stage clear" prompt.
struct AsciiManager
{
    static ZunResult RegisterChain();
    static void CutChain();

    static ChainCallbackResult OnUpdate(AsciiManager *s);
    static ChainCallbackResult OnDrawMenus(AsciiManager *s);
    static ChainCallbackResult OnDrawPopups(AsciiManager *s);
    static ZunResult AddedCallback(AsciiManager *s);
    static ZunResult DeletedCallback(AsciiManager *s);

    // TODO: Make this inline somehow
    void InitializeVms();

    void DrawStrings();
    void DrawPopupsWithHwVertexProcessing();
    void DrawPopupsWithoutHwVertexProcessing();

    void AddString(D3DXVECTOR3 *position, const char *text);
    void AddFormatText(D3DXVECTOR3 *position, const char *fmt, ...);
    void CreatePopup1(D3DXVECTOR3 *position, i32 value, D3DCOLOR color);
    void CreatePopup2(D3DXVECTOR3 *position, i32 value, D3DCOLOR color);

    void SetColor(ZunColor color)
    {
        this->color = color;
    }

    AnmVm vm0;
    AnmVm vm1;
    AsciiManagerString strings[ASCII_STRING_COUNT];
    i32 numStrings;
    D3DCOLOR color;
    D3DXVECTOR2 scale;
    // If true, we are drawing an element of the Gui class.
    ZunBool isGui;
    // If true, we are drawing the currently selected element of the MainMenu
    // class.
    ZunBool isSelected;
    i32 nextPopupIndex1;
    i32 nextPopupIndex2;
    unreferenced_fields(0x4);
    // Menu that shows up when the player presses the menu button while in-game.
    StageMenu gameMenu;
    // Menu that shows up when the player dies after losing their last life.
    StageMenu retryMenu;
    AsciiManagerPopup popups[ASCII_TOTAL_POPUPS_COUNT];
};
ZUN_ASSERT_TYPE(AsciiManager, 0xc1ac, 4);

DIFFABLE_EXTERN(AsciiManager, g_AsciiManager);
} // namespace th06
