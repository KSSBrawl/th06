#include "ZunTimer.hpp"
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
#include "i18n.hpp"
#include "inttypes.hpp"

#include <stdio.h>
#include <string.h>

namespace th06
{

void ZunTimer::Initialize()
{
    this->current = 0;
    this->previous = -1;
    this->subFrame = 0.0f;
}

void ZunTimer::Increment(i32 value)
{
    if (g_Supervisor.framerateMultiplier > 0.99f)
    {
        this->current += value;

        return;
    }

    if (value < 0)
    {
        Decrement(-value);

        return;
    }

    this->previous = this->current;
    this->subFrame += g_Supervisor.effectiveFramerateMultiplier * (float)value;

    while (this->subFrame >= 1.0f)
    {
        this->current += 1;
        this->subFrame -= 1.0f;
    }
}

void ZunTimer::Decrement(i32 value)
{
    if (g_Supervisor.framerateMultiplier > 0.99f)
    {
        this->current -= value;

        return;
    }

    if (value < 0)
    {
        Increment(-value);

        return;
    }

    this->previous = this->current;
    this->subFrame -= g_Supervisor.effectiveFramerateMultiplier * (float)value;

    while (this->subFrame < 0.0f)
    {
        this->current -= 1;
        this->subFrame += 1.0f;
    }
}

void Supervisor::TickTimer(i32 *frames, f32 *subframes)
{
    if (this->framerateMultiplier <= 0.99f)
    {
        *subframes += this->effectiveFramerateMultiplier;
        if (*subframes >= 1.0f)
        {
            *frames += 1;
            *subframes -= 1.0f;
        }
    }
    else
    {
        *frames += 1;
    }
}
} // namespace th06
