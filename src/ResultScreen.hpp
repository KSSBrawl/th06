#pragma once
#include "AnmVm.hpp"
#include "Global.hpp"
#include "ReplayData.hpp"
#include "ZunResult.hpp"
#include "decomp.hpp"

namespace th06
{
#define DEFAULT_MAGIC "DMYS"
#define TH6K_MAGIC 'K6HT'
#define HSCR_MAGIC 'RCSH'
#define CLRD_MAGIC 'DRLC'
#define PSCR_MAGIC 'RCSP'
#define CATK_MAGIC 'KTAC'

#define HSCR_NUM_DIFFICULTIES 5
#define HSCR_NUM_SCORES_SLOTS 10

#define TH6K_VERSION 16

#define RESULT_KEYBOARD_COLUMNS 16
#define RESULT_KEYBOARD_ROWS 6
#define RESULT_KEYBOARD_CHARACTERS RESULT_KEYBOARD_COLUMNS *RESULT_KEYBOARD_ROWS
#define RESULT_KEYBOARD_SPACE 94
#define RESULT_KEYBOARD_END 95

#define SCORE_DAT_FILE_BUFFER_SIZE 0xa0000

enum ResultScreenState
{
    RESULT_SCREEN_STATE_INIT = 0,
    RESULT_SCREEN_STATE_CHOOSING_DIFFICULTY,
    RESULT_SCREEN_STATE_EXITING,
    RESULT_SCREEN_STATE_BEST_SCORES_EASY,
    RESULT_SCREEN_STATE_BEST_SCORES_NORMAL,
    RESULT_SCREEN_STATE_BEST_SCORES_HARD,
    RESULT_SCREEN_STATE_BEST_SCORES_LUNATIC,
    RESULT_SCREEN_STATE_BEST_SCORES_EXTRA,
    RESULT_SCREEN_STATE_SPELLCARDS,
    RESULT_SCREEN_STATE_WRITING_HIGHSCORE_NAME,
    RESULT_SCREEN_STATE_SAVE_REPLAY_QUESTION,
    RESULT_SCREEN_STATE_CANT_SAVE_REPLAY,
    RESULT_SCREEN_STATE_CHOOSING_REPLAY_FILE,
    RESULT_SCREEN_STATE_WRITING_REPLAY_NAME,
    RESULT_SCREEN_STATE_OVERWRITE_REPLAY_FILE,
    RESULT_SCREEN_STATE_STATS_SCREEN,
    RESULT_SCREEN_STATE_STATS_TO_SAVE_TRANSITION,
    RESULT_SCREEN_STATE_EXIT,
};

enum ResultScreenMainMenuCursor
{
    RESULT_SCREEN_CURSOR_EASY,
    RESULT_SCREEN_CURSOR_NORMAL,
    RESULT_SCREEN_CURSOR_HARD,
    RESULT_SCREEN_CURSOR_LUNATIC,
    RESULT_SCREEN_CURSOR_EXTRA,
    RESULT_SCREEN_CURSOR_SPELLCARDS,
    RESULT_SCREEN_CURSOR_EXIT
};

struct Th6k
{
    u32 magic;
    u16 th6kLen;
    u16 unkLen;
    u8 version;
    u8 flag_9;
    alignment_padding(0x2);
};
ZUN_ASSERT_TYPE(Th6k, 0xc, 4);

struct Catk
{
    Th6k base;
    i32 captureScore;
    u16 idx;
    u8 nameCsum;
    u8 characterShotType[SHOTTYPE_COUNT + 1];
    char name[34]; // probably 36 since 34 as the ECL spell buffer length is likely a bug
    unreferenced_fields(0x2);
    u16 numAttempts;
    u16 numSuccess;
};
ZUN_ASSERT_TYPE(Catk, 0x40, 4);

struct Clrd
{
    Th6k base;
    u8 difficultyClearedWithRetries[5];
    u8 difficultyClearedWithoutRetries[5];
    u8 characterShotType;
    alignment_padding(0x1);
};
ZUN_ASSERT_TYPE(Clrd, 0x18, 4);

struct Pscr
{
    Th6k base;
    i32 score;
    u8 character;
    u8 difficulty;
    u8 stage;
    alignment_padding(0x1);
};
ZUN_ASSERT_TYPE(Pscr, 0x14, 4);

struct Hscr
{
    Th6k base;
    u32 score;
    u8 character;
    u8 difficulty;
    u8 stage;
    char name[9];
};
ZUN_ASSERT_TYPE(Hscr, 0x1c, 4);

struct ScoreListNode
{
    ScoreListNode()
    {
        this->prev = NULL;
        this->next = NULL;
        this->data = NULL;
    }

    ScoreListNode *prev;
    ScoreListNode *next;
    Hscr *data;
};
ZUN_ASSERT_TYPE(ScoreListNode, 0xc, 4);

struct ScoreDat
{
    u8 xorseed[2];
    u16 csum;
    u16 unk_8;
    u8 unk_9;
    alignment_padding(0x1);
    u32 dataOffset;
    ScoreListNode *scores;
    u32 fileLen;
};
ZUN_ASSERT_TYPE(ScoreDat, 0x14, 4);

ScoreDat *OpenScore(const char *path);
void ReleaseScoreDat(ScoreDat *s);

u32 GetHighScore(ScoreDat *s, ScoreListNode *node, u32 character, u32 difficulty);

ZunResult ParseCatk(ScoreDat *s, Catk *catk);
ZunResult ParseClrd(ScoreDat *s, Clrd *out);
ZunResult ParsePscr(ScoreDat *s, Pscr *out);

ZunResult ResultScreen_RegisterChain(ZunBool unk);
} // namespace th06
