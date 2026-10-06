#pragma once
#include "Chain.hpp"
#include "ChainPriorities.hpp"
#include "ReplayData.hpp"
#include "decomp.hpp"

namespace th06
{
#define REPLAY_MAGIC "T6RP"

ZunResult ReplayManager_RegisterChain(ZunBool isDemo, const char *replayFile);

void StopRecordingReplay();
void SaveReplay(const char *replayPath, const char *replayName);
ZunResult ValidateReplayData(ReplayData *data, i32 fileSize);

} // namespace th06
