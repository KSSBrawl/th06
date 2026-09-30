#pragma once
#include "Chain.hpp"
#include "ChainPriorities.hpp"
#include "ReplayData.hpp"
#include "decomp.hpp"

namespace th06
{
#define REPLAY_MAGIC "T6RP"

// TODO: Move struct def into cpp file
struct ReplayManager
{
    static ZunResult RegisterChain(ZunBool isDemo, const char *replayFile);
    static ChainCallbackResult OnUpdate(ReplayManager *mgr);
    static ChainCallbackResult OnUpdateDemoHighPrio(ReplayManager *mgr);
    static ChainCallbackResult OnUpdateDemoLowPrio(ReplayManager *mgr);
    static ChainCallbackResult OnDraw(ReplayManager *mgr);
    static ZunResult AddedCallback(ReplayManager *mgr);
    static ZunResult AddedCallbackDemo(ReplayManager *mgr);
    static ZunResult DeletedCallback(ReplayManager *mgr);
    static void StopRecording();
    static void SaveReplay(const char *replay_path, const char *param_2);
    static ZunResult ValidateReplayData(ReplayData *data, i32 fileSize);

    ReplayManager()
    {
    }

    ZunBool IsDemo()
    {
        return this->isDemo;
    }

    i32 frameId;
    ReplayData *replayData;
    ZunBool isDemo;
    const char *replayFile;
    unreferenced_fields(0x34);
    u16 unk44;
    alignment_padding(0x2);
    ReplayDataInput *replayInputs;
    ReplayDataInput *replayInputStageBookmarks[7];
    ChainElem *calcChain;
    ChainElem *drawChain;
    ChainElem *calcChainDemoHighPrio;
};
ZUN_ASSERT_TYPE(ReplayManager, 0x74, 4);
} // namespace th06
