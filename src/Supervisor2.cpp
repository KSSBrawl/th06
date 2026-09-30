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
#include "Supervisor.hpp"
#include "TextHelper.hpp"
#include "ZunTimer.hpp"
#include "i18n.hpp"
#include "inttypes.hpp"

#include <stdio.h>
#include <string.h>

namespace th06
{
void Supervisor::ReleasePbg3(i32 pbg3FileIdx)
{
    if (this->pbg3Archives[pbg3FileIdx] == NULL)
    {
        return;
    }

    // Double free! Release is called internally by the Pbg3Archive destructor,
    // and as such should not be called directly. By calling it directly here,
    // it ends up being called twice, which will cause the resources owned by
    // Pbg3Archive to be freed multiple times, which can result in crashes.
    //
    // For some reason, this double-free doesn't cause crashes in the original
    // game. However, this can cause problems in dllbuilds of the game. Maybe
    // some accuracy improvements in the PBG3 handling will remove this
    // difference.
    this->pbg3Archives[pbg3FileIdx]->Release();
    ZUN_DELETE(this->pbg3Archives[pbg3FileIdx]);
}

i32 Supervisor::LoadPbg3(i32 pbg3FileIdx, const char *filename)
{
    if (this->pbg3Archives[pbg3FileIdx] == NULL || strcmp(filename, this->pbg3ArchiveNames[pbg3FileIdx]) != 0)
    {
        this->ReleasePbg3(pbg3FileIdx);
        this->pbg3Archives[pbg3FileIdx] = ZUN_NEW(Pbg3Archive);
        utils::DebugPrint("%s open ...\n", filename);
        if (this->pbg3Archives[pbg3FileIdx]->Load(filename))
        {
            strcpy(this->pbg3ArchiveNames[pbg3FileIdx], filename);

            char verPath[128];
            sprintf(verPath, "ver%.4x.dat", GAME_VERSION);
            i32 res = this->pbg3Archives[pbg3FileIdx]->FindEntry(verPath);
            if (res < 0)
            {
                g_GameErrorContext.Fatal(TH_ERR_WRONG_DATA_VERSION);
                return 1;
            }
        }
        else
        {
            // Let's really make sure this is null by nulling twice. I assume
            // there's some kind of inline function here, like it's actually
            // calling this->pbg3Archives.delete(pbg3FileIdx), followed by a
            // manual nulling?
            ZUN_DELETE(this->pbg3Archives[pbg3FileIdx]);
            this->pbg3Archives[pbg3FileIdx] = NULL;
        }
    }
    return 0;
}

ZunResult Supervisor::LoadConfig(const char *path)
{
    memset(&g_Supervisor.cfg, 0, sizeof(GameConfiguration));
    g_Supervisor.cfg.opts.useSwTextureBlending = true;
    GameConfiguration *data = (GameConfiguration *)FileSystem::OpenPath(path, EXTERNAL_FILE);
    if (data == NULL)
    {
        g_Supervisor.cfg.lifeCount = 2;
        g_Supervisor.cfg.bombCount = 3;
        g_Supervisor.cfg.colorMode16bit = 0xff;
        g_Supervisor.cfg.version = GAME_VERSION;
        g_Supervisor.cfg.padXAxis = 600;
        g_Supervisor.cfg.padYAxis = 600;
        FILE *wavFile = fopen("bgm/th06_01.wav", "rb");
        if (wavFile != NULL)
        {
            g_Supervisor.cfg.musicMode = WAV;
            fclose(wavFile);
        }
        else
        {
            g_Supervisor.cfg.musicMode = MIDI;
            utils::DebugPrint(TH_ERR_NO_WAVE_FILE);
        }
        g_Supervisor.cfg.playSounds = true;
        g_Supervisor.cfg.defaultDifficulty = NORMAL;
        g_Supervisor.cfg.windowed = false;
        g_Supervisor.cfg.frameskipConfig = 0;
        g_Supervisor.cfg.controllerMapping = g_ControllerMapping;
        g_GameErrorContext.Log(TH_ERR_CONFIG_NOT_FOUND);
    }
    else
    {
        g_Supervisor.cfg = *data;
        if (g_Supervisor.cfg.lifeCount >= 5 || g_Supervisor.cfg.bombCount >= 4 ||
            g_Supervisor.cfg.colorMode16bit >= 2 || g_Supervisor.cfg.musicMode >= 3 ||
            g_Supervisor.cfg.defaultDifficulty >= 5 || g_Supervisor.cfg.playSounds >= 2 ||
            g_Supervisor.cfg.windowed >= 2 || g_Supervisor.cfg.frameskipConfig >= 3 ||
            g_Supervisor.cfg.version != GAME_VERSION || g_LastFileSize != sizeof(GameConfiguration))
        {
            g_Supervisor.cfg.lifeCount = 2;
            g_Supervisor.cfg.bombCount = 3;
            g_Supervisor.cfg.colorMode16bit = 0xff;
            g_Supervisor.cfg.version = GAME_VERSION;
            g_Supervisor.cfg.padXAxis = 600;
            g_Supervisor.cfg.padYAxis = 600;
            FILE* wavFile = fopen("bgm/th06_01.wav", "rb");
            if (wavFile != NULL)
            {
                g_Supervisor.cfg.musicMode = WAV;
                fclose(wavFile);
            }
            else
            {
                g_Supervisor.cfg.musicMode = MIDI;
                utils::DebugPrint(TH_ERR_NO_WAVE_FILE);
            }
            g_Supervisor.cfg.playSounds = true;
            g_Supervisor.cfg.defaultDifficulty = NORMAL;
            g_Supervisor.cfg.windowed = false;
            g_Supervisor.cfg.frameskipConfig = 0;
            g_Supervisor.cfg.controllerMapping = g_ControllerMapping;
            memset(&g_Supervisor.cfg.opts, 0, sizeof(GameConfigOpts));
            g_Supervisor.cfg.opts.useSwTextureBlending = true;
            g_GameErrorContext.Log(TH_ERR_CONFIG_CORRUPTED);
        }
        g_ControllerMapping = g_Supervisor.cfg.controllerMapping;
        ZUN_FREE(data);
    }
    if (this->IsVertexBufferDisabled())
    {
        g_GameErrorContext.Log(TH_ERR_NO_VERTEX_BUFFER);
    }
    if (this->IsFogDisabled())
    {
        g_GameErrorContext.Log(TH_ERR_NO_FOG);
    }
    if (this->Is16bitColorMode())
    {
        g_GameErrorContext.Log(TH_ERR_USE_16BIT_TEXTURES);
    }
    if (this->ShouldForceBackbufferClear())
    {
        g_GameErrorContext.Log(TH_ERR_FORCE_BACKBUFFER_CLEAR);
    }
    if (this->IsMinimumGraphicsMode())
    {
        g_GameErrorContext.Log(TH_ERR_DONT_RENDER_ITEMS);
    }
    if (this->IsShadingDisabled())
    {
        g_GameErrorContext.Log(TH_ERR_NO_GOURAUD_SHADING);
    }
    if (this->IsDepthTestDisabled())
    {
        g_GameErrorContext.Log(TH_ERR_NO_DEPTH_TESTING);
    }
    if (this->IsForced60Fps())
    {
        g_GameErrorContext.Log(TH_ERR_FORCE_60FPS_MODE);
        this->vsyncEnabled = false;
    }
    if (this->IsColorCompositingDisabled())
    {
        g_GameErrorContext.Log(TH_ERR_NO_TEXTURE_COLOR_COMPOSITING);
    }
    if (this->IsColorCompositingDisabled())
    {
        g_GameErrorContext.Log(TH_ERR_LAUNCH_WINDOWED);
    }
    if (this->IsReferenceRasterizerMode())
    {
        g_GameErrorContext.Log(TH_ERR_FORCE_REFERENCE_RASTERIZER);
    }
    if (this->IsDInputDisabled())
    {
        g_GameErrorContext.Log(TH_ERR_DO_NOT_USE_DIRECTINPUT);
    }
    if (FileSystem::WriteDataToFile(path, &g_Supervisor.cfg, sizeof(GameConfiguration)) != 0)
    {
        g_GameErrorContext.Fatal(TH_ERR_FILE_CANNOT_BE_EXPORTED, path);
        g_GameErrorContext.Fatal(TH_ERR_FOLDER_HAS_WRITE_PROTECT_OR_DISK_FULL);
        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

ZunBool Supervisor::ReadMidiFile(u32 midiFileIdx, const char *path)
{
    // Return conventions seem opposite of normal? But they're never used anyway
    if (g_Supervisor.cfg.musicMode == MIDI)
    {
        if (g_Supervisor.midiOutput != NULL)
        {
            g_Supervisor.midiOutput->ReadFileData(midiFileIdx, path);
        }

        return FALSE;
    }

    return TRUE;
}

i32 Supervisor::PlayMidiFile(i32 midiFileIdx)
{
    MidiOutput *globalMidiController;

    if (g_Supervisor.cfg.musicMode == MIDI)
    {
        if (g_Supervisor.midiOutput != NULL)
        {
            globalMidiController = g_Supervisor.midiOutput;
            globalMidiController->StopPlayback();
            globalMidiController->ParseFile(midiFileIdx);
            globalMidiController->Play();
        }

        return FALSE;
    }

    return TRUE;
}

ZunResult Supervisor::SetupMidiPlayback(const char *path)
{
    if (g_Supervisor.cfg.musicMode == MIDI)
    {
    }
    else if (g_Supervisor.cfg.musicMode == WAV)
    {
    }
    else
    {
        return ZUN_ERROR;
    }
    return ZUN_SUCCESS;
}

ZunResult Supervisor::PlayAudio(const char *path)
{
    char wavName[256];
    char wavPos[256];
    char *pathExtension;

    if (g_Supervisor.cfg.musicMode == MIDI)
    {
        if (g_Supervisor.midiOutput != NULL)
        {
            MidiOutput *midiOutput = g_Supervisor.midiOutput;
            midiOutput->StopPlayback();
            midiOutput->LoadFile(path);
            midiOutput->Play();
        }
    }
    else if (g_Supervisor.cfg.musicMode == WAV)
    {
        strcpy(wavName, path);
        strcpy(wavPos, path);
        pathExtension = strrchr(wavName, L'.');
        pathExtension[1] = 'w';
        pathExtension[2] = 'a';
        pathExtension[3] = 'v';
        pathExtension = strrchr(wavPos, L'.');
        pathExtension[1] = 'p';
        pathExtension[2] = 'o';
        pathExtension[3] = 's';
        g_SoundPlayer.LoadWav(wavName);
        if (g_SoundPlayer.LoadPos(wavPos) < ZUN_SUCCESS)
        {
            g_SoundPlayer.PlayBGM(FALSE);
        }
        else
        {
            g_SoundPlayer.PlayBGM(TRUE);
        }
    }
    else
    {
        return ZUN_ERROR;
    }
    return ZUN_SUCCESS;
}

ZunResult Supervisor::StopAudio()
{
    if (g_Supervisor.cfg.musicMode == MIDI)
    {
        if (g_Supervisor.midiOutput != NULL)
        {
            g_Supervisor.midiOutput->StopPlayback();
        }
    }
    else
    {
        if (g_Supervisor.cfg.musicMode == WAV)
        {
            g_SoundPlayer.StopBGM();
        }
        else
        {
            return ZUN_ERROR;
        }
    }

    return ZUN_SUCCESS;
}

ZunResult Supervisor::FadeOutMusic(f32 fadeOutSeconds)
{
    if (g_Supervisor.cfg.musicMode == MIDI)
    {
        if (g_Supervisor.midiOutput != NULL)
        {
            g_Supervisor.midiOutput->SetFadeOut(1000.0f * fadeOutSeconds);
        }
    }
    else
    {
        if (g_Supervisor.cfg.musicMode == WAV)
        {
            if (this->effectiveFramerateMultiplier == 0.0f)
            {
                g_SoundPlayer.FadeOut(fadeOutSeconds);
            }
            else
            {
                if (this->effectiveFramerateMultiplier > 1.0f)
                {
                    g_SoundPlayer.FadeOut(fadeOutSeconds);
                }
                else
                {
                    g_SoundPlayer.FadeOut(fadeOutSeconds / this->effectiveFramerateMultiplier);
                }
            }
        }
        else
        {
            return ZUN_ERROR;
        }
    }

    return ZUN_SUCCESS;
}
} // namespace th06