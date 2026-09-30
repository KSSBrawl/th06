#include "Supervisor.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "ChainPriorities.hpp"
#include "Ending.hpp"
#include "GameManager.hpp"
#include "GameWindow.hpp"
#include "Global.hpp"
#include "MainMenu.hpp"
#include "MusicRoom.hpp"
#include "ReplayManager.hpp"
#include "ResultScreen.hpp"
#include "SoundPlayer.hpp"
#include "TextHelper.hpp"
#include "ZunTimer.hpp"
#include "i18n.hpp"
#include "inttypes.hpp"

#include <stdio.h>
#include <string.h>

namespace th06
{
// Using an explicit section name here because the static local guard happens to work...
#pragma section(".data$M1Supervisor", read, write)
#pragma bss_seg(".data$M1Supervisor")

__declspec(allocate(".data$M1Supervisor")) DIFFABLE_STATIC(Supervisor, g_Supervisor);

ChainCallbackResult Supervisor::OnUpdate(Supervisor *s)
{
    if (g_SoundPlayer.backgroundMusic != NULL)
    {
        g_SoundPlayer.backgroundMusic->UpdateFadeOut();
    }
    g_LastFrameInput = g_CurFrameInput;
    g_CurFrameInput = Controller::GetInput();
    g_IsEigthFrameOfHeldInput = false;
    if (g_LastFrameInput == g_CurFrameInput)
    {
        if (g_NumOfFramesInputsWereHeld >= 30)
        {
            if (g_NumOfFramesInputsWereHeld % 8 == 0)
            {
                g_IsEigthFrameOfHeldInput = true;
            }
            if (g_NumOfFramesInputsWereHeld >= 38)
            {
                g_NumOfFramesInputsWereHeld = 30;
            }
        }
        g_NumOfFramesInputsWereHeld++;
    }
    else
    {
        g_NumOfFramesInputsWereHeld = 0;
    }

    if (s->wantedState != s->curState)
    {
        s->wantedState2 = s->wantedState;
        switch (s->wantedState)
        {
        case SUPERVISOR_STATE_INIT:
        REINIT_MAINMENU:
            s->curState = SUPERVISOR_STATE_MAINMENU;
            g_Supervisor.d3dDevice->ResourceManagerDiscardBytes(0);
            if (MainMenu_RegisterChain(FALSE) != ZUN_SUCCESS)
            {
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            }
            break;
        case SUPERVISOR_STATE_MAINMENU:
            switch (s->curState)
            {
            case SUPERVISOR_STATE_EXITSUCCESS:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case SUPERVISOR_STATE_GAMEMANAGER:
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case SUPERVISOR_STATE_EXITERROR:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR;
            case SUPERVISOR_STATE_RESULTSCREEN:
                if (ResultScreen_RegisterChain(false) != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case SUPERVISOR_STATE_MUSICROOM:
                if (MusicRoom_RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case SUPERVISOR_STATE_ENDING:
                GameManager::CutChain();
                if (Ending_RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            }
            break;

        case SUPERVISOR_STATE_RESULTSCREEN:
            switch (s->curState)
            {
            case SUPERVISOR_STATE_EXITSUCCESS:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case SUPERVISOR_STATE_MAINMENU:
                s->curState = SUPERVISOR_STATE_INIT;
                goto REINIT_MAINMENU;
            }
            break;
        case SUPERVISOR_STATE_GAMEMANAGER:
            switch (s->curState)
            {
            case SUPERVISOR_STATE_EXITSUCCESS:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;

            case SUPERVISOR_STATE_MAINMENU:
            RETURN_TO_MENU_FROM_GAME:
                GameManager::CutChain();
                s->curState = SUPERVISOR_STATE_INIT;
                ReplayManager::SaveReplay(NULL, NULL);
                goto REINIT_MAINMENU;

            case SUPERVISOR_STATE_RESULTSCREEN_FROMGAME:
                GameManager::CutChain();
                if (ResultScreen_RegisterChain(true) != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case SUPERVISOR_STATE_NEXT_STAGE:
                GameManager::CutChain();
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                if (s->curState == SUPERVISOR_STATE_MAINMENU)
                {
                    goto RETURN_TO_MENU_FROM_GAME;
                }
                s->curState = SUPERVISOR_STATE_GAMEMANAGER;
                break;
            case SUPERVISOR_STATE_MAINMENU_REPLAY:
                GameManager::CutChain();
                s->curState = SUPERVISOR_STATE_INIT;
                ReplayManager::SaveReplay(NULL, NULL);
                s->curState = SUPERVISOR_STATE_MAINMENU;
                g_Supervisor.d3dDevice->ResourceManagerDiscardBytes(0);
                if (MainMenu_RegisterChain(true) != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;

            case 10:
                GameManager::CutChain();
                if (Ending_RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            }
            break;
        case SUPERVISOR_STATE_RESULTSCREEN_FROMGAME:
            switch (s->curState)
            {
            case SUPERVISOR_STATE_EXITSUCCESS:
                ReplayManager::SaveReplay(NULL, NULL);
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case SUPERVISOR_STATE_MAINMENU:
                s->curState = SUPERVISOR_STATE_INIT;
                ReplayManager::SaveReplay(NULL, NULL);
                goto REINIT_MAINMENU;
            }
            break;
        case SUPERVISOR_STATE_MUSICROOM:
            switch (s->curState)
            {
            case SUPERVISOR_STATE_EXITSUCCESS:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;

            case SUPERVISOR_STATE_MAINMENU:
                s->curState = SUPERVISOR_STATE_INIT;
                goto REINIT_MAINMENU;
            }
            break;
        case SUPERVISOR_STATE_ENDING:
            switch (s->curState)
            {
            case SUPERVISOR_STATE_EXITSUCCESS:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case SUPERVISOR_STATE_MAINMENU:
                s->curState = SUPERVISOR_STATE_INIT;
                goto REINIT_MAINMENU;
            case SUPERVISOR_STATE_RESULTSCREEN_FROMGAME:
                if (ResultScreen_RegisterChain(true) != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
            }
            break;
        }
        g_CurFrameInput = g_LastFrameInput = g_IsEigthFrameOfHeldInput = 0;
    }

    s->wantedState = s->curState;
    s->calcCount++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult Supervisor::OnDraw(Supervisor *s)
{
    g_AnmManager->SetCurrentVertexShader(AnmVertexShader_NotSet);
    g_AnmManager->SetCurrentSprite(NULL);
    g_AnmManager->SetCurrentTexture(NULL);
    g_AnmManager->SetCurrentColorOp(AnmColorOp_NotSet);
    g_AnmManager->SetCurrentBlendMode(AnmBlendMode_NotSet);
    g_AnmManager->SetCurrentZWriteDisable(AnmZWriteState_NotSet);

    Supervisor::DrawFpsCounter();
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(diprange, pvRefBackup)
BOOL CALLBACK Supervisor::ControllerCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
    LPVOID pvRefBackup;
    DIPROPRANGE diprange;
    pvRefBackup = pvRef;

    if (lpddoi->dwType & DIDFT_AXIS)
    {
        diprange.diph.dwSize = sizeof(diprange);
        diprange.diph.dwHeaderSize = sizeof(diprange.diph);
        diprange.diph.dwHow = DIPH_BYID;
        diprange.diph.dwObj = lpddoi->dwType;
        diprange.lMin = -1000;
        diprange.lMax = 1000;

        if (FAILED(g_Supervisor.controller->SetProperty(DIPROP_RANGE, &diprange.diph)))
        {
            return FALSE;
        }
    }
    return TRUE;
}

#pragma var_order(chain, supervisor)
ZunResult Supervisor::RegisterChain()
{
    ChainElem *chain;
    Supervisor *supervisor = &g_Supervisor;

    supervisor->wantedState = 0;
    supervisor->curState = -1;
    supervisor->calcCount = 0;

    chain = g_Chain.CreateElem((ChainCallback)Supervisor::OnUpdate);
    chain->arg = supervisor;
    chain->addedCallback = (ChainAddedCallback)Supervisor::AddedCallback;
    chain->deletedCallback = (ChainDeletedCallback)Supervisor::DeletedCallback;
    if (g_Chain.AddToCalcChain(chain, TH_CHAIN_PRIO_CALC_SUPERVISOR) != 0)
    {
        return ZUN_ERROR;
    }

    chain = g_Chain.CreateElem((ChainCallback)Supervisor::OnDraw);
    chain->arg = supervisor;
    g_Chain.AddToDrawChain(chain, TH_CHAIN_PRIO_DRAW_SUPERVISOR);

    return ZUN_SUCCESS;
}

static ZunResult SetupDInput(Supervisor *supervisor)
{
    HINSTANCE hInst;

    hInst = (HINSTANCE)GetWindowLong(supervisor->hwndGameWindow, GWL_HINSTANCE);
    if (supervisor->IsDInputDisabled())
    {
        return ZUN_ERROR;
    }

    if (FAILED(DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID *)&supervisor->dinputIface,
                                  NULL)))
    {
        supervisor->dinputIface = NULL;
        g_GameErrorContext.Log(TH_ERR_DIRECTINPUT_NOT_AVAILABLE);
        return ZUN_ERROR;
    }

    if (FAILED(supervisor->dinputIface->CreateDevice(GUID_SysKeyboard, &supervisor->keyboard, NULL)))
    {
        SAFE_RELEASE(supervisor->dinputIface);
        g_GameErrorContext.Log(TH_ERR_DIRECTINPUT_NOT_AVAILABLE);
        return ZUN_ERROR;
    }

    if (FAILED(supervisor->keyboard->SetDataFormat(&c_dfDIKeyboard)))
    {
        SAFE_RELEASE(supervisor->keyboard);

        SAFE_RELEASE(supervisor->dinputIface);

        g_GameErrorContext.Log(TH_ERR_DIRECTINPUT_SETDATAFORMAT_NOT_AVAILABLE);
        return ZUN_ERROR;
    }

    if (FAILED(supervisor->keyboard->SetCooperativeLevel(supervisor->hwndGameWindow,
                                                         DISCL_NONEXCLUSIVE | DISCL_FOREGROUND | DISCL_NOWINKEY)))
    {
        SAFE_RELEASE(supervisor->keyboard);

        SAFE_RELEASE(supervisor->dinputIface);

        g_GameErrorContext.Log(TH_ERR_DIRECTINPUT_SETCOOPERATIVELEVEL_NOT_AVAILABLE);
        return ZUN_ERROR;
    }

    supervisor->keyboard->Acquire();
    g_GameErrorContext.Log(TH_ERR_DIRECTINPUT_INITIALIZED);

    supervisor->dinputIface->EnumDevices(DI8DEVCLASS_GAMECTRL, Supervisor::EnumGameControllersCb, NULL,
                                         DIEDFL_ATTACHEDONLY);
    if (supervisor->controller)
    {
        supervisor->controller->SetDataFormat(&c_dfDIJoystick2);
        supervisor->controller->SetCooperativeLevel(supervisor->hwndGameWindow, DISCL_EXCLUSIVE | DISCL_FOREGROUND);

        g_Supervisor.controllerCaps.dwSize = sizeof(g_Supervisor.controllerCaps);

        supervisor->controller->GetCapabilities(&g_Supervisor.controllerCaps);
        supervisor->controller->EnumObjects(Supervisor::ControllerCallback, NULL, DIDFT_ALL);

        g_GameErrorContext.Log(TH_ERR_PAD_FOUND);
    }
    return ZUN_SUCCESS;
}

ZunResult Supervisor::AddedCallback(Supervisor *s)
{
    i32 i;

    for (i = 0; i < ARRAY_SIZE_SIGNED(s->pbg3Archives); i++)
    {
        s->pbg3Archives[i] = NULL;
    }

    g_Pbg3Archives = s->pbg3Archives;
    if (s->LoadPbg3(IN_PBG3_INDEX, TH_IN_DAT_FILE))
    {
        return ZUN_ERROR;
    }
    g_AnmManager->LoadSurface(0, "data/title/th06logo.jpg");
    g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);

    g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);

    g_AnmManager->ReleaseSurface(0);

    s->startupTimeBeforeMenuMusic = timeGetTime();
    SetupDInput(s);

    s->midiOutput = ZUN_NEW(MidiOutput);

    g_Rng.Initialize(timeGetTime());

    g_SoundPlayer.InitSoundBuffers();
    if (g_AnmManager->LoadAnm(ANM_FILE_TEXT, "data/text.anm", ANM_OFFSET_TEXT) != 0)
    {
        return ZUN_ERROR;
    }

    if (AsciiManager::RegisterChain() != 0)
    {
        g_GameErrorContext.Log(TH_ERR_ASCIIMANAGER_INIT_FAILED);
        return ZUN_ERROR;
    }

    s->unk198 = 0;
    g_AnmManager->SetupVertexBuffer();
    TextHelper::CreateTextBuffer();
    s->ReleasePbg3(IN_PBG3_INDEX);
    if (g_Supervisor.LoadPbg3(MD_PBG3_INDEX, TH_MD_DAT_FILE) != 0)
        return ZUN_ERROR;

    return ZUN_SUCCESS;
}

BOOL CALLBACK Supervisor::EnumGameControllersCb(LPCDIDEVICEINSTANCE pdidInstance, LPVOID pContext)
{
    HRESULT result;

    if (!g_Supervisor.controller)
    {
        result = g_Supervisor.dinputIface->CreateDevice(pdidInstance->guidInstance, &g_Supervisor.controller, NULL);
        if (FAILED(result))
        {
            return TRUE;
        }
    }
    return FALSE;
}

ZunResult Supervisor::DeletedCallback(Supervisor *s)
{
    i32 pbg3Idx;

    g_AnmManager->ReleaseVertexBuffer();
    for (pbg3Idx = 0; pbg3Idx < ARRAY_SIZE_SIGNED(s->pbg3Archives); pbg3Idx++)
    {
        s->ReleasePbg3(pbg3Idx);
    }
    g_AnmManager->ReleaseAnm(0);
    AsciiManager::CutChain();
    g_SoundPlayer.StopBGM();
    if (s->midiOutput != NULL)
    {
        s->midiOutput->StopPlayback();
        ZUN_DELETE(s->midiOutput);
    }
    ReplayManager::SaveReplay(NULL, NULL);
    TextHelper::ReleaseTextBuffer();
    if (s->keyboard != NULL)
    {
        s->keyboard->Unacquire();
    }
    SAFE_RELEASE(s->keyboard);
    if (s->controller != NULL)
    {
        s->controller->Unacquire();
    }
    SAFE_RELEASE(s->controller);
    SAFE_RELEASE(s->dinputIface);
    return ZUN_SUCCESS;
}

#pragma var_order(curTime, framerate, fps, elapsed)
void Supervisor::DrawFpsCounter()
{
    DWORD curTime;
    float framerate;
    float elapsed;
    float fps;

    __declspec(allocate(".data$M1Supervisor")) static DWORD g_LastTime = timeGetTime();
    __declspec(allocate(".data$M1Supervisor")) static u32 g_NumFramesSinceLastTime = 0;
    __declspec(allocate(".data$M1Supervisor")) static char g_FpsCounterBuffer[256];

    curTime = timeGetTime();
    g_NumFramesSinceLastTime = g_NumFramesSinceLastTime + 1 + (u32)g_Supervisor.cfg.frameskipConfig;
    if (500 <= curTime - g_LastTime)
    {
        elapsed = (curTime - g_LastTime) / 1000.0f;
        fps = g_NumFramesSinceLastTime / elapsed;
        g_LastTime = curTime;
        g_NumFramesSinceLastTime = 0;
        sprintf(g_FpsCounterBuffer, "%.02ffps", fps);
        if (g_GameManager.isInMenu)
        {
            framerate = 60.0f / g_Supervisor.framerateMultiplier;
            g_Supervisor.unk1b8 = g_Supervisor.unk1b8 + framerate;

            if (framerate * 0.89999998f < fps)
                g_Supervisor.unk1b4 = g_Supervisor.unk1b4 + framerate;
            else if (framerate * 0.69999999f < fps)
                g_Supervisor.unk1b4 = framerate * 0.8f + g_Supervisor.unk1b4;
            else if (framerate * 0.5f < fps)
                g_Supervisor.unk1b4 = framerate * 0.6f + g_Supervisor.unk1b4;
            else
                g_Supervisor.unk1b4 = framerate * 0.5f + g_Supervisor.unk1b4;
        }
    }
    if (!g_Supervisor.isInEnding)
    {
        g_AsciiManager.AddString(&D3DXVECTOR3(512.0f, 464.0f, 0.0f), g_FpsCounterBuffer);
    }
}
} // namespace th06
