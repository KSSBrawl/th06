#pragma once

#include <D3D8.h>

#include "AnmVm.hpp"
#include "Chain.hpp"
#include "ReplayData.hpp"
#include "ZunBool.hpp"
#include "ZunResult.hpp"
#include "inttypes.hpp"

#define REPLAYS_PER_PAGE 15
#define NORMAL_REPLAY_COUNT REPLAYS_PER_PAGE
#define USER_REPLAY_PAGES 3
#define USER_REPLAY_COUNT (USER_REPLAY_PAGES * REPLAYS_PER_PAGE)
#define TOTAL_REPLAY_COUNT (NORMAL_REPLAY_COUNT + USER_REPLAY_COUNT)

namespace th06
{
ZunResult MainMenu_RegisterChain(ZunBool isDemo);
} // namespace th06
