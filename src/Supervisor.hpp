#pragma once
#include <d3d8.h>
#include <d3dx8math.h>
#include <dinput.h>

#include "Chain.hpp"
#include "Global.hpp"
#include "MidiOutput.hpp"
#include "ZunBool.hpp"
#include "ZunResult.hpp"
#include "decomp.hpp"
#include "pbg3/Pbg3Archive.hpp"

namespace th06
{
#define GAME_VERSION 0x102

struct GameConfigOpts
{
    u32 useSwTextureBlending : 1;
    u32 dontUseVertexBuf : 1;
    u32 force16bitColorMode : 1;
    u32 clearBackBufferOnRefresh : 1;
    u32 displayMinimumGraphics : 1;
    u32 suppressUseOfGoroudShading : 1;
    u32 disableDepthTest : 1;
    u32 force60Fps : 1;
    u32 disableColorCompositing : 1;
    u32 referenceRasterizerMode : 1;
    u32 disableFog : 1;
    u32 dontUseDirectInput : 1;
    alignment_bitfields(u32, 20);
};
ZUN_ASSERT_TYPE(GameConfigOpts, 0x4, 4);

enum MusicMode
{
    OFF = 0,
    WAV = 1,
    MIDI = 2
};

struct GameConfiguration
{
    ControllerMapping controllerMapping;
    // Always 0x102 for 1.02
    i32 version;
    u8 lifeCount;
    u8 bombCount;
    u8 colorMode16bit;
    u8 musicMode;
    u8 playSounds;
    u8 defaultDifficulty;
    u8 windowed;
    // 0 = fullspeed, 1 = 1/2 speed, 2 = 1/4 speed.
    u8 frameskipConfig;
    i16 padXAxis;
    i16 padYAxis;
    unreferenced_fields(0x10);
    // GameConfigOpts bitfield.
    GameConfigOpts opts;
};
ZUN_ASSERT_TYPE(GameConfiguration, 0x38, 4);

#define IN_PBG3_INDEX 0
#define MD_PBG3_INDEX 1
#define ST_PBG3_INDEX 2
#define TL_PBG3_INDEX 3
#define CM_PBG3_INDEX 4
#define ED_PBG3_INDEX 5

typedef char Pbg3ArchiveName[32];

enum SupervisorState
{
    SUPERVISOR_STATE_INIT,
    SUPERVISOR_STATE_MAINMENU,
    SUPERVISOR_STATE_GAMEMANAGER,
    SUPERVISOR_STATE_NEXT_STAGE,
    SUPERVISOR_STATE_EXITSUCCESS,
    SUPERVISOR_STATE_EXITERROR,
    SUPERVISOR_STATE_RESULTSCREEN,
    SUPERVISOR_STATE_RESULTSCREEN_FROMGAME,
    SUPERVISOR_STATE_MAINMENU_REPLAY,
    SUPERVISOR_STATE_MUSICROOM,
    SUPERVISOR_STATE_ENDING,
};

struct Supervisor
{
    Supervisor()
    {
        memset(this, 0, sizeof(Supervisor));
    }

    ZunBool ReadMidiFile(u32 midiFileIdx, const char *path);
    ZunBool PlayMidiFile(i32 midiFileIdx);
    ZunResult PlayAudio(const char *path);
    ZunResult StopAudio();
    ZunResult SetupMidiPlayback(const char *path);
    ZunResult FadeOutMusic(f32 fadeOutSeconds);

    BOOL LoadPbg3(i32 pbg3FileIdx, const char *filename);
    void ReleasePbg3(i32 pbg3FileIdx);

    ZunResult LoadConfig(const char *path);

    void TickTimer(i32 *frames, f32 *subframes);

    f32 FramerateMultiplier()
    {
        return this->effectiveFramerateMultiplier;
    }

    ZunBool IsHardwareBlendingDisabled()
    {
        return this->cfg.opts.useSwTextureBlending;
    }

    ZunBool IsVertexBufferDisabled()
    {
        return this->cfg.opts.dontUseVertexBuf;
    }

    ZunBool Is16bitColorMode()
    {
        return this->cfg.opts.force16bitColorMode;
    }

    ZunBool IsMinimumGraphicsMode()
    {
        return this->cfg.opts.displayMinimumGraphics;
    }

    ZunBool IsShadingDisabled()
    {
        return this->cfg.opts.suppressUseOfGoroudShading;
    }

    ZunBool IsDepthTestDisabled()
    {
        return this->cfg.opts.disableDepthTest;
    }

    ZunBool IsForced60Fps()
    {
        return this->cfg.opts.force60Fps;
    }

    ZunBool IsColorCompositingDisabled()
    {
        return this->cfg.opts.disableColorCompositing;
    }

    ZunBool IsReferenceRasterizerMode()
    {
        return this->cfg.opts.referenceRasterizerMode;
    }

    ZunBool IsFogDisabled()
    {
        return this->cfg.opts.disableFog;
    }

    ZunBool IsDInputDisabled()
    {
        return this->cfg.opts.dontUseDirectInput;
    }

    ZunBool ShouldForceBackbufferClear()
    {
        return this->cfg.opts.clearBackBufferOnRefresh | this->IsMinimumGraphicsMode();
    }

    u32 IsSoftwareTexturing()
    {
        return this->IsColorCompositingDisabled() | this->IsHardwareBlendingDisabled();
    }

    ZunBool ShouldRunAt60Fps()
    {
        return this->IsForced60Fps() && this->vsyncEnabled;
    }

    ZunBool IsWindowed()
    {
        return this->cfg.windowed;
    }

    ZunBool IsNotLoadingNextStage()
    {
        return this->curState != SUPERVISOR_STATE_NEXT_STAGE;
    }

    HINSTANCE hInstance;
    LPDIRECT3D8 d3dIface;
    LPDIRECT3DDEVICE8 d3dDevice;
    LPDIRECTINPUT8 dinputIface;
    LPDIRECTINPUTDEVICE8 keyboard;
    LPDIRECTINPUTDEVICE8 controller;
    DIDEVCAPS controllerCaps;
    HWND hwndGameWindow;
    D3DXMATRIX viewMatrix;
    D3DXMATRIX projectionMatrix;
    D3DVIEWPORT8 viewport;
    D3DPRESENT_PARAMETERS presentParameters;
    GameConfiguration cfg;
#if !TRIALBUILD
    // NOTE: This is not even close to a default config
    GameConfiguration defaultConfig;
#endif
    i32 calcCount;
    i32 wantedState;
    i32 curState;
    i32 prevState;

    unreferenced_fields(0x4);
    i32 forceRedrawFrames;
    ZunBool isInEnding;

    ZunBool vsyncEnabled;
    i32 lastFrameTime;
    f32 effectiveFramerateMultiplier;
    f32 framerateMultiplier;

    MidiOutput *midiOutput;

    f32 unk1b4;
    f32 unk1b8;

    Pbg3Archive *pbg3Archives[16];
    Pbg3ArchiveName pbg3ArchiveNames[16];

    u8 hasD3dHardwareVertexProcessing;
    u8 lockableBackbuffer;
    u8 colorMode16Bits;
    alignment_padding(0x1);

    u32 startupTimeBeforeMenuMusic;
    D3DCAPS8 d3dCaps;
};
ZunResult Supervisor_RegisterChain();
ChainCallbackResult Supervisor_OnUpdate(Supervisor *s);
ChainCallbackResult Supervisor_OnDraw(Supervisor *s);
ZunResult Supervisor_AddedCallback(Supervisor *s);
ZunResult Supervisor_DeletedCallback(Supervisor *s);
void Supervisor_DrawFpsCounter();

BOOL CALLBACK Supervisor_ControllerCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);

BOOL CALLBACK Supervisor_EnumGameControllersCb(LPCDIDEVICEINSTANCE pdidInstance, LPVOID pContext);

#if !TRIALBUILD
ZUN_ASSERT_SIZE(Supervisor, 0x4d8);
#else
ZUN_ASSERT_SIZE(Supervisor, 0x4a0);
#endif

DIFFABLE_EXTERN(Supervisor, g_Supervisor);

} // namespace th06
