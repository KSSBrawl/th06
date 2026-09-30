#include "EclManager.hpp"
#include "AnmManager.hpp"
#include "EffectManager.hpp"
#include "Enemy.hpp"
#include "EnemyEclInstr.hpp"
#include "EnemyManager.hpp"
#include "GameManager.hpp"
#include "Global.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "Stage.hpp"

namespace th06
{
DIFFABLE_STATIC_ARRAY_ASSIGN(i32, 64, g_SpellcardScore) = {
    200000, 200000, 200000, 200000, 200000, 200000, 200000, 250000, 250000, 250000, 250000, 250000, 250000,
    250000, 300000, 300000, 300000, 300000, 300000, 300000, 300000, 300000, 300000, 300000, 300000, 300000,
    300000, 300000, 300000, 300000, 300000, 300000, 400000, 400000, 400000, 400000, 400000, 400000, 400000,
    400000, 500000, 500000, 500000, 500000, 500000, 500000, 600000, 600000, 600000, 600000, 600000, 700000,
    700000, 700000, 700000, 700000, 700000, 700000, 700000, 700000, 700000, 700000, 700000, 700000};
typedef void (*ExInsn)(Enemy *, EclRawInstr *);
DIFFABLE_STATIC_ARRAY_ASSIGN(ExInsn, 17, g_EclExInsn) = {EnemyEclInstr::ExInsCirnoRainbowBallJank,
                                                         EnemyEclInstr::ExInsShootAtRandomArea,
                                                         EnemyEclInstr::ExInsShootStarPattern,
                                                         EnemyEclInstr::ExInsPatchouliShottypeSetVars,
                                                         EnemyEclInstr::ExInsStage56Func4,
                                                         EnemyEclInstr::ExInsStage5Func5,
                                                         EnemyEclInstr::ExInsBatWingEffect,
                                                         EnemyEclInstr::ExInsStage6Func7,
                                                         EnemyEclInstr::ExInsStage6Func8,
                                                         EnemyEclInstr::ExInsStage6Func9,
                                                         EnemyEclInstr::ExInsHandleBatTransformation,
                                                         EnemyEclInstr::ExInsStage6Func11,
                                                         EnemyEclInstr::ExInsStage4Func12,
                                                         EnemyEclInstr::ExInsStageXFunc13,
                                                         EnemyEclInstr::ExInsStageXFunc14,
                                                         EnemyEclInstr::ExInsStageXFunc15,
                                                         EnemyEclInstr::ExInsFlandreFinalContextUpdate};

DIFFABLE_STATIC_SORTED(C5, ChainElem, g_EclManagerCalcChain); // unused
DIFFABLE_STATIC_SORTED(C4, EclManager, g_EclManager);
DIFFABLE_STATIC_SORTED(C1, i32, g_PlayerShot);
DIFFABLE_STATIC_SORTED(C2, f32, g_PlayerDistance);
DIFFABLE_STATIC_SORTED(C3, f32, g_PlayerAngle);

ZunResult EclManager::Load(const char *eclPath)
{
    i32 idx;

    this->eclFile = (EclRawHeader *)FileSystem::OpenPath(eclPath);
    if (this->eclFile == NULL)
    {
        g_GameErrorContext.Log(TH_ERR_ECLMANAGER_ENEMY_DATA_CORRUPT);
        return ZUN_ERROR;
    }
    this->eclFile->timelineOffsets[0] =
        (EclTimelineInstr *)((int)this->eclFile->timelineOffsets[0] + (int)this->eclFile);
    this->subTable = &this->eclFile->subOffsets[0];
    for (idx = 0; idx < this->eclFile->subCount; idx++)
    {
        this->subTable[idx] = (EclRawInstr *)((int)this->subTable[idx] + (int)this->eclFile);
    }
    this->timeline = this->eclFile->timelineOffsets[0];
    return ZUN_SUCCESS;
}

void EclManager::Unload()
{
    if (this->eclFile != NULL)
    {
        ZUN_FREE(this->eclFile);
    }
    this->eclFile = NULL;
}

ZunResult EclManager::CallEclSub(EnemyEclContext *ctx, i16 subId)
{
    ctx->currentInstr = this->subTable[subId];
    ctx->time = 0;
    ctx->subId = subId;
    return ZUN_SUCCESS;
}

#pragma var_order(genericFloat3, genericInt, genericFloat, args, instruction)
ZunResult EclManager::RunEcl(Enemy *enemy)
{
    EclRawInstr *instruction;
    EclRawInstrArgs *args;
    ZunVec3 genericFloat3;
    i32 genericInt;
    f32 genericFloat;

    for (;;)
    {
        instruction = enemy->currentContext.currentInstr;
        if (enemy->runInterrupt >= 0)
        {
            goto HANDLE_INTERRUPT;
        }

    YOLO:
        if (enemy->currentContext.time == instruction->time)
        {
            if (!(instruction->skipForDifficulty & (1 << g_GameManager.difficulty)))
            {
                goto NEXT_INSN;
            }

            args = &instruction->args;
            switch (instruction->opCode)
            {
            case ECL_OPCODE_UNIMP:
                return ZUN_ERROR;
            case ECL_OPCODE_JUMPDEC:
                genericInt = *EnemyEclInstr::GetVar(enemy, &args->jump.var, NULL);
                genericInt--;
                EnemyEclInstr::SetVar(enemy, args->jump.var, &genericInt);
                if (genericInt <= 0)
                    break;
            case ECL_OPCODE_JUMP:
            HANDLE_JUMP:
                enemy->currentContext.time.current = instruction->args.jump.time;
                instruction = (EclRawInstr *)((int)instruction + args->jump.offset);
                goto YOLO;
            case ECL_OPCODE_SETINT:
            case ECL_OPCODE_SETFLOAT:
                EnemyEclInstr::SetVar(enemy, instruction->args.alu.res, &args->alu.arg1.i32);
                break;
            case ECL_OPCODE_MATHNORMANGLE:
                genericFloat = *(f32 *)EnemyEclInstr::GetVar(enemy, &instruction->args.alu.res, NULL);
                genericFloat = utils::AddNormalizeAngle(genericFloat, 0.0f);
                EnemyEclInstr::SetVar(enemy, instruction->args.alu.res, &genericFloat);
                break;
            case ECL_OPCODE_SETINTRAND: {
                i32 range = *EnemyEclInstr::GetVar(enemy, &args->alu.arg1.id, NULL);
                genericInt = g_Rng.GetRandomU32InRange(range);
                EnemyEclInstr::SetVar(enemy, instruction->args.alu.res, &genericInt);
                break;
            }
#pragma var_order(range, minVal)
            case ECL_OPCODE_SETINTRANDMIN: {
                i32 range = *EnemyEclInstr::GetVar(enemy, &args->alu.arg1.id, NULL);
                i32 minVal = *EnemyEclInstr::GetVar(enemy, &args->alu.arg2.id, NULL);
                genericInt = g_Rng.GetRandomU32InRange(range);
                genericInt += minVal;
                EnemyEclInstr::SetVar(enemy, instruction->args.alu.res, &genericInt);
                break;
            }
            case ECL_OPCODE_SETFLOATRAND: {
                f32 range = *EnemyEclInstr::GetVarFloat(enemy, &args->alu.arg1.f32, NULL);
                genericFloat = g_Rng.GetRandomF32InRange(range);
                EnemyEclInstr::SetVar(enemy, instruction->args.alu.res, &genericFloat);
                break;
            }
#pragma var_order(range, minVal)
            case ECL_OPCODE_SETFLOATRANDMIN: {
                float range = *EnemyEclInstr::GetVarFloat(enemy, &args->alu.arg1.f32, NULL);
                float minVal = *EnemyEclInstr::GetVarFloat(enemy, &args->alu.arg2.f32, NULL);
                genericFloat = g_Rng.GetRandomF32InRange(range);
                genericFloat += minVal;
                EnemyEclInstr::SetVar(enemy, instruction->args.alu.res, &genericFloat);
                break;
            }
            case ECL_OPCODE_SETVARSELFX:
                EnemyEclInstr::SetVar(enemy, instruction->args.alu.res, &enemy->position.x);
                break;
            case ECL_OPCODE_SETVARSELFY:
                EnemyEclInstr::SetVar(enemy, instruction->args.alu.res, &enemy->position.y);
                break;
            case ECL_OPCODE_SETVARSELFZ:
                EnemyEclInstr::SetVar(enemy, instruction->args.alu.res, &enemy->position.z);
                break;
            case ECL_OPCODE_MATHINTADD:
            case ECL_OPCODE_MATHFLOATADD:
                EnemyEclInstr::MathAdd(enemy, instruction->args.alu.res, &args->alu.arg1.id, &args->alu.arg2.id);
                break;
            case ECL_OPCODE_MATHINC: {
                i32* var = EnemyEclInstr::GetVar(enemy, &instruction->args.alu.res, NULL);
                *var += 1;
                break;
            }
            case ECL_OPCODE_MATHDEC: {
                i32* var = EnemyEclInstr::GetVar(enemy, &instruction->args.alu.res, NULL);
                *var -= 1;
                break;
            }
            case ECL_OPCODE_MATHINTSUB:
            case ECL_OPCODE_MATHFLOATSUB:
                EnemyEclInstr::MathSub(enemy, instruction->args.alu.res, &args->alu.arg1.id, &args->alu.arg2.id);
                break;
            case ECL_OPCODE_MATHINTMUL:
            case ECL_OPCODE_MATHFLOATMUL:
                EnemyEclInstr::MathMul(enemy, instruction->args.alu.res, &args->alu.arg1.id, &args->alu.arg2.id);
                break;
            case ECL_OPCODE_MATHINTDIV:
            case ECL_OPCODE_MATHFLOATDIV:
                EnemyEclInstr::MathDiv(enemy, instruction->args.alu.res, &args->alu.arg1.id, &args->alu.arg2.id);
                break;
            case ECL_OPCODE_MATHINTMOD:
            case ECL_OPCODE_MATHFLOATMOD:
                EnemyEclInstr::MathMod(enemy, instruction->args.alu.res, &args->alu.arg1.id, &args->alu.arg2.id);
                break;
            case ECL_OPCODE_MATHATAN2:
                EnemyEclInstr::MathAtan2(enemy, instruction->args.alu.res, &args->alu.arg1.f32, &args->alu.arg2.f32,
                                         &args->alu.arg3.f32, &args->alu.arg4.f32);
                break;
#pragma var_order(rhs, lhs)
            case ECL_OPCODE_CMPINT: {
                i32 lhs = *EnemyEclInstr::GetVar(enemy, &instruction->args.cmp.lhs.id, NULL);
                i32 rhs = *EnemyEclInstr::GetVar(enemy, &instruction->args.cmp.rhs.id, NULL);
                enemy->currentContext.compareRegister = lhs == rhs ? 0 : lhs < rhs ? -1 : 1;
                break;
            }
#pragma var_order(lhs, rhs)
            case ECL_OPCODE_CMPFLOAT: {
                float lhs = *EnemyEclInstr::GetVarFloat(enemy, &instruction->args.cmp.lhs.f32, NULL);
                float rhs = *EnemyEclInstr::GetVarFloat(enemy, &instruction->args.cmp.rhs.f32, NULL);
                enemy->currentContext.compareRegister = lhs == rhs ? 0 : (lhs < rhs ? -1 : 1);
                break;
            }
            case ECL_OPCODE_JUMPLSS:
                if (enemy->currentContext.compareRegister < 0)
                    goto HANDLE_JUMP;
                break;
            case ECL_OPCODE_JUMPLEQ:
                if (enemy->currentContext.compareRegister <= 0)
                    goto HANDLE_JUMP;
                break;
            case ECL_OPCODE_JUMPEQU:
                if (enemy->currentContext.compareRegister == 0)
                    goto HANDLE_JUMP;
                break;
            case ECL_OPCODE_JUMPGRE:
                if (enemy->currentContext.compareRegister > 0)
                    goto HANDLE_JUMP;
                break;
            case ECL_OPCODE_JUMPGEQ:
                if (enemy->currentContext.compareRegister >= 0)
                    goto HANDLE_JUMP;
                break;
            case ECL_OPCODE_JUMPNEQ:
                if (enemy->currentContext.compareRegister != 0)
                    goto HANDLE_JUMP;
                break;
            case ECL_OPCODE_CALL:
            HANDLE_CALL:
                genericInt = instruction->args.call.eclSub;
                enemy->currentContext.currentInstr = (EclRawInstr *)((u8 *)instruction + instruction->offsetToNext);
                if (!enemy->flags.disableCallStack)
                {
                    enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
                }
                g_EclManager.CallEclSub(&enemy->currentContext, genericInt);
                if (!enemy->flags.disableCallStack &&
                    enemy->stackDepth < MAX_ECL_STACK_DEPTH)
                {
                    enemy->stackDepth++;
                }
                enemy->currentContext.var0 = instruction->args.call.var0;
                enemy->currentContext.float0 = instruction->args.call.float0;
                continue;
            case ECL_OPCODE_RET:
                if (enemy->flags.disableCallStack)
                {
                    utils::DebugPrint2("error : no Stack Ret\n");
                }
                enemy->stackDepth--;
                enemy->currentContext = enemy->savedContextStack[enemy->stackDepth];
                continue;
            case ECL_OPCODE_CALLLSS:
                genericInt = *EnemyEclInstr::GetVar(enemy, &args->call.cmpLhs, NULL);
                if (genericInt < args->call.cmpRhs)
                    goto HANDLE_CALL;
                break;
            case ECL_OPCODE_CALLLEQ:
                genericInt = *EnemyEclInstr::GetVar(enemy, &args->call.cmpLhs, NULL);
                if (genericInt <= args->call.cmpRhs)
                    goto HANDLE_CALL;
                break;
            case ECL_OPCODE_CALLEQU:
                genericInt = *EnemyEclInstr::GetVar(enemy, &args->call.cmpLhs, NULL);
                if (genericInt == args->call.cmpRhs)
                    goto HANDLE_CALL;
                break;
            case ECL_OPCODE_CALLGRE:
                genericInt = *EnemyEclInstr::GetVar(enemy, &args->call.cmpLhs, NULL);
                if (genericInt > args->call.cmpRhs)
                    goto HANDLE_CALL;
                break;
            case ECL_OPCODE_CALLGEQ:
                genericInt = *EnemyEclInstr::GetVar(enemy, &args->call.cmpLhs, NULL);
                if (genericInt >= args->call.cmpRhs)
                    goto HANDLE_CALL;
                break;
            case ECL_OPCODE_CALLNEQ:
                genericInt = *EnemyEclInstr::GetVar(enemy, &args->call.cmpLhs, NULL);
                if (genericInt != args->call.cmpRhs)
                    goto HANDLE_CALL;
                break;
            case ECL_OPCODE_ANMSETMAIN:
                g_AnmManager->SetAndExecuteScriptIdx(&enemy->primaryVm,
                                                     instruction->args.anmSetMain.scriptIdx + ANM_SCRIPT_ENEMY_START);
                break;
            case ECL_OPCODE_ANMSETSLOT:
                if (instruction->args.anmSetSlot.vmIdx >= ENEMY_ANM_SLOTS)
                {
                    utils::DebugPrint2("error : sub anim overflow\n");
                }
                g_AnmManager->SetAndExecuteScriptIdx(&enemy->vms[instruction->args.anmSetSlot.vmIdx],
                                                     args->anmSetSlot.scriptIdx + ANM_SCRIPT_ENEMY_START);
                break;
            case ECL_OPCODE_MOVEPOSITION:
                enemy->position = *instruction->args.move.pos.AsD3dXVec();
                enemy->position.x = *EnemyEclInstr::GetVarFloat(enemy, &enemy->position.x, NULL);
                enemy->position.y = *EnemyEclInstr::GetVarFloat(enemy, &enemy->position.y, NULL);
                enemy->position.z = *EnemyEclInstr::GetVarFloat(enemy, &enemy->position.z, NULL);
                enemy->ClampPos();
                break;
            case ECL_OPCODE_MOVEAXISVELOCITY:
                enemy->axisSpeed = *instruction->args.move.pos.AsD3dXVec();
                enemy->axisSpeed.x = *EnemyEclInstr::GetVarFloat(enemy, &enemy->axisSpeed.x, NULL);
                enemy->axisSpeed.y = *EnemyEclInstr::GetVarFloat(enemy, &enemy->axisSpeed.y, NULL);
                enemy->axisSpeed.z = *EnemyEclInstr::GetVarFloat(enemy, &enemy->axisSpeed.z, NULL);
                enemy->flags.movementMode = 0;
                break;
            case ECL_OPCODE_MOVEVELOCITY:
                genericFloat3 = instruction->args.move.pos;
                enemy->angle = *EnemyEclInstr::GetVarFloat(enemy, &genericFloat3.x, NULL);
                enemy->speed = *EnemyEclInstr::GetVarFloat(enemy, &genericFloat3.y, NULL);
                enemy->flags.movementMode = 1;
                break;
            case ECL_OPCODE_MOVEANGULARVELOCITY:
                genericFloat3 = instruction->args.move.pos;
                enemy->angularVelocity = *EnemyEclInstr::GetVarFloat(enemy, &genericFloat3.x, NULL);
                enemy->flags.movementMode = 1;
                break;
            case ECL_OPCODE_MOVEATPLAYER:
                genericFloat3 = instruction->args.move.pos;
                enemy->angle = g_Player.AngleToPlayer(&enemy->position) + genericFloat3.x;
                enemy->speed = *EnemyEclInstr::GetVarFloat(enemy, &genericFloat3.y, NULL);
                enemy->flags.movementMode = 1;
                break;
            case ECL_OPCODE_MOVESPEED:
                genericFloat3 = instruction->args.move.pos;
                enemy->speed = *EnemyEclInstr::GetVarFloat(enemy, &genericFloat3.x, NULL);
                enemy->flags.movementMode = 1;
                break;
            case ECL_OPCODE_MOVEACCELERATION:
                genericFloat3 = instruction->args.move.pos;
                enemy->acceleration = *EnemyEclInstr::GetVarFloat(enemy, &genericFloat3.x, NULL);
                enemy->flags.movementMode = 1;
                break;
            case ECL_OPCODE_BULLETFANAIMED:
            case ECL_OPCODE_BULLETFAN:
            case ECL_OPCODE_BULLETCIRCLEAIMED:
            case ECL_OPCODE_BULLETCIRCLE:
            case ECL_OPCODE_BULLETOFFSETCIRCLEAIMED:
            case ECL_OPCODE_BULLETOFFSETCIRCLE:
            case ECL_OPCODE_BULLETRANDOMANGLE:
            case ECL_OPCODE_BULLETRANDOMSPEED:
            case ECL_OPCODE_BULLETRANDOM:
#pragma var_order(args, shooter)
            {
                EclRawInstrBulletArgs *args = &instruction->args.bullet;
                EnemyBulletShooter *shooter = &enemy->bulletProps;
                shooter->sprite = args->sprite;
                shooter->aimMode = instruction->opCode - ECL_OPCODE_BULLETFANAIMED;
                shooter->count1 = *EnemyEclInstr::GetVar(enemy, &args->count1, NULL);
                shooter->count1 += enemy->BulletRankAmount1(g_GameManager.rank);
                if (shooter->count1 <= 0)
                {
                    shooter->count1 = 1;
                }

                shooter->count2 = *EnemyEclInstr::GetVar(enemy, &args->count2, NULL);
                shooter->count2 += enemy->BulletRankAmount2(g_GameManager.rank);
                if (shooter->count2 <= 0)
                {
                    shooter->count2 = 1;
                }
                shooter->position = enemy->position + enemy->shootOffset;
                shooter->angle1 = *EnemyEclInstr::GetVarFloat(enemy, &args->angle1, NULL);
                shooter->angle1 = utils::AddNormalizeAngle(shooter->angle1, 0.0f);
                shooter->speed1 = *EnemyEclInstr::GetVarFloat(enemy, &args->speed1, NULL);
                if (shooter->speed1 != 0.0f)
                {
                    shooter->speed1 += enemy->BulletRankSpeed(g_GameManager.rank);
                    if (shooter->speed1 < 0.3f)
                    {
                        shooter->speed1 = 0.3;
                    }
                }
                shooter->angle2 = *EnemyEclInstr::GetVarFloat(enemy, &args->angle2, NULL);
                shooter->speed2 = *EnemyEclInstr::GetVarFloat(enemy, &args->speed2, NULL);
                shooter->speed2 += enemy->BulletRankSpeed(g_GameManager.rank) / 2.0f;
                if (shooter->speed2 < 0.3f)
                {
                    shooter->speed2 = 0.3f;
                }
                shooter->unk_4a = 0;
                shooter->flags = args->flags;
                genericInt = args->color;
                // TODO: Strict aliasing rule be like.
                shooter->spriteOffset = *EnemyEclInstr::GetVar(enemy, (EclVarId *)&genericInt, NULL);
                if (!enemy->flags.shootingDisabled)
                {
                    g_BulletManager.SpawnBulletPattern(shooter);
                }
                break;
            }
            case ECL_OPCODE_BULLETEFFECTS:
                enemy->bulletProps.exInts[0] = *EnemyEclInstr::GetVar(enemy, &args->bulletEffects.ivar1, NULL);
                enemy->bulletProps.exInts[1] = *EnemyEclInstr::GetVar(enemy, &args->bulletEffects.ivar2, NULL);
                enemy->bulletProps.exInts[2] = *EnemyEclInstr::GetVar(enemy, &args->bulletEffects.ivar3, NULL);
                enemy->bulletProps.exInts[3] = *EnemyEclInstr::GetVar(enemy, &args->bulletEffects.ivar4, NULL);
                enemy->bulletProps.exFloats[0] = *EnemyEclInstr::GetVarFloat(enemy, &args->bulletEffects.fvar1, NULL);
                enemy->bulletProps.exFloats[1] = *EnemyEclInstr::GetVarFloat(enemy, &args->bulletEffects.fvar2, NULL);
                enemy->bulletProps.exFloats[2] = *EnemyEclInstr::GetVarFloat(enemy, &args->bulletEffects.fvar3, NULL);
                enemy->bulletProps.exFloats[3] = *EnemyEclInstr::GetVarFloat(enemy, &args->bulletEffects.fvar4, NULL);
                break;
            case ECL_OPCODE_ANMSETDEATH: {
                EclRawInstrAnmSetDeathArgs *args = &instruction->args.anmSetDeath;
                enemy->deathParticle1 = args->deathParticle1;
                enemy->deathParticle2 = args->deathParticle2;
                enemy->deathAnm3 = args->deathAnm3;
                break;
            }
            case ECL_OPCODE_SHOOTINTERVAL:
                enemy->shootInterval = instruction->args.setInt;
                enemy->shootInterval += enemy->ShootInterval(g_GameManager.rank);
                enemy->shootIntervalTimer = 0;
                break;
            case ECL_OPCODE_SHOOTINTERVALDELAYED:
                enemy->shootInterval = instruction->args.setInt;
                enemy->shootInterval += enemy->ShootInterval(g_GameManager.rank);
                if (enemy->shootInterval != 0)
                {
                    enemy->shootIntervalTimer = g_Rng.GetRandomU32InRange(enemy->shootInterval);
                }
                break;
            case ECL_OPCODE_SHOOTDISABLED:
                enemy->flags.shootingDisabled = true;
                break;
            case ECL_OPCODE_SHOOTENABLED:
                enemy->flags.shootingDisabled = false;
                break;
            case ECL_OPCODE_SHOOTNOW:
                enemy->bulletProps.position = enemy->position + enemy->shootOffset;
                g_BulletManager.SpawnBulletPattern(&enemy->bulletProps);
                break;
            case ECL_OPCODE_SHOOTOFFSET:
                enemy->shootOffset.x = *EnemyEclInstr::GetVarFloat(enemy, &args->move.pos.x, NULL);
                enemy->shootOffset.y = *EnemyEclInstr::GetVarFloat(enemy, &args->move.pos.y, NULL);
                enemy->shootOffset.z = *EnemyEclInstr::GetVarFloat(enemy, &args->move.pos.z, NULL);
                break;
            case ECL_OPCODE_LASERCREATE:
            case ECL_OPCODE_LASERCREATEAIMED:
#pragma var_order(shooter, args)
            {
                EclRawInstrLaserArgs *args = &instruction->args.laser;
                EnemyLaserShooter *shooter = &enemy->laserProps;
                shooter->position = enemy->position + enemy->shootOffset;
                shooter->sprite = args->sprite;
                shooter->spriteOffset = args->color;
                shooter->angle = *EnemyEclInstr::GetVarFloat(enemy, &args->angle, NULL);
                shooter->speed = *EnemyEclInstr::GetVarFloat(enemy, &args->speed, NULL);
                shooter->startOffset = *EnemyEclInstr::GetVarFloat(enemy, &args->startOffset, NULL);
                shooter->endOffset = *EnemyEclInstr::GetVarFloat(enemy, &args->endOffset, NULL);
                shooter->startLength = *EnemyEclInstr::GetVarFloat(enemy, &args->startLength, NULL);
                shooter->width = args->width;
                shooter->startTime = args->startTime;
                shooter->duration = args->duration;
                shooter->despawnDuration = args->despawnDuration;
                shooter->hitboxStartTime = args->hitboxStartTime;
                shooter->hitboxEndDelay = args->hitboxEndDelay;
                shooter->flags = args->flags;
                if (instruction->opCode == ECL_OPCODE_LASERCREATEAIMED)
                {
                    shooter->type = 0;
                }
                else
                {
                    shooter->type = 1;
                }
                enemy->lasers[enemy->laserStore] = g_BulletManager.SpawnLaserPattern(shooter);
                break;
            }
            case ECL_OPCODE_LASERINDEX:
                enemy->laserStore = *EnemyEclInstr::GetVar(enemy, &instruction->args.alu.res, NULL);
                break;
            case ECL_OPCODE_LASERROTATE:
                if (enemy->lasers[instruction->args.laserOp.laserIdx] != NULL)
                {
                    enemy->lasers[instruction->args.laserOp.laserIdx]->angle +=
                        *EnemyEclInstr::GetVarFloat(enemy, &instruction->args.laserOp.arg1.x, NULL);
                }
                break;
            case ECL_OPCODE_LASERROTATEFROMPLAYER:
                if (enemy->lasers[instruction->args.laserOp.laserIdx] != NULL)
                {
                    enemy->lasers[instruction->args.laserOp.laserIdx]->angle =
                        g_Player.AngleToPlayer(&enemy->lasers[instruction->args.laserOp.laserIdx]->pos) +
                        *EnemyEclInstr::GetVarFloat(enemy, &instruction->args.laserOp.arg1.x, NULL);
                }
                break;
            case ECL_OPCODE_LASEROFFSET:
                if (enemy->lasers[instruction->args.laserOp.laserIdx] != NULL)
                {
                    enemy->lasers[instruction->args.laserOp.laserIdx]->pos =
                        enemy->position + *instruction->args.laserOp.arg1.AsD3dXVec();
                }
                break;
            case ECL_OPCODE_LASERTEST:
                if (enemy->lasers[instruction->args.laserOp.laserIdx] != NULL &&
                    enemy->lasers[instruction->args.laserOp.laserIdx]->inUse)
                {
                    enemy->currentContext.compareRegister = 0;
                }
                else
                {
                    enemy->currentContext.compareRegister = 1;
                }
                break;
            case ECL_OPCODE_LASERCANCEL:
                if (enemy->lasers[instruction->args.laserOp.laserIdx] != NULL &&
                    enemy->lasers[instruction->args.laserOp.laserIdx]->inUse &&
                    enemy->lasers[instruction->args.laserOp.laserIdx]->state < 2)
                {
                    enemy->lasers[instruction->args.laserOp.laserIdx]->state = 2;
                    enemy->lasers[instruction->args.laserOp.laserIdx]->timer = 0;
                }
                break;
            case ECL_OPCODE_LASERCLEARALL: {
                for (i32 idx = 0; idx < MAX_LASERS_PER_ENEMY; idx++)
                {
                    enemy->lasers[idx] = NULL;
                }
                break;
            }
            case ECL_OPCODE_BOSSSET:
                if (instruction->args.setInt >= 0)
                {
                    g_EnemyManager.bosses[instruction->args.setInt] = enemy;
                    g_Gui.bossPresent = true;
                    g_Gui.SetBossHealthBar(1.0f);
                    enemy->flags.isBoss = true;
                    enemy->bossId = instruction->args.setInt;
                }
                else
                {
                    g_Gui.bossPresent = false;
                    g_EnemyManager.bosses[enemy->bossId] = NULL;
                    enemy->flags.isBoss = false;
                }
                break;
            case ECL_OPCODE_SPELLCARDEFFECT: {
                EclRawInstrSpellcardEffectArgs* args = &instruction->args.spellcardEffect;
                enemy->effectArray[enemy->effectIdx] = g_EffectManager.SpawnParticles(
                    PARTICLE_EFFECT_UNK_13, &enemy->position, 1, (ZunColor)g_EffectsColor[args->effectColorId]);
                enemy->effectArray[enemy->effectIdx]->pos2 = *args->pos.AsD3dXVec();
                enemy->effectDistance = args->effectDistance;
                enemy->effectIdx++;
                break;
            }
            case ECL_OPCODE_MOVEDIRTIMEDECELERATE:
                EnemyEclInstr::MoveDirTime(enemy, instruction);
                enemy->flags.movementEaseType = 1;
                break;
            case ECL_OPCODE_MOVEDIRTIMEDECELERATEFAST:
                EnemyEclInstr::MoveDirTime(enemy, instruction);
                enemy->flags.movementEaseType = 2;
                break;
            case ECL_OPCODE_MOVEDIRTIMEACCELERATE:
                EnemyEclInstr::MoveDirTime(enemy, instruction);
                enemy->flags.movementEaseType = 3;
                break;
            case ECL_OPCODE_MOVEDIRTIMEACCELERATEFAST:
                EnemyEclInstr::MoveDirTime(enemy, instruction);
                enemy->flags.movementEaseType = 4;
                break;
            case ECL_OPCODE_MOVEPOSITIONTIMELINEAR:
                EnemyEclInstr::MovePosTime(enemy, instruction);
                enemy->flags.movementEaseType = 0;
                break;
            case ECL_OPCODE_MOVEPOSITIONTIMEDECELERATE:
                EnemyEclInstr::MovePosTime(enemy, instruction);
                enemy->flags.movementEaseType = 1;
                break;
            case ECL_OPCODE_MOVEPOSITIONTIMEDECELERATEFAST:
                EnemyEclInstr::MovePosTime(enemy, instruction);
                enemy->flags.movementEaseType = 2;
                break;
            case ECL_OPCODE_MOVEPOSITIONTIMEACCELERATE:
                EnemyEclInstr::MovePosTime(enemy, instruction);
                enemy->flags.movementEaseType = 3;
                break;
            case ECL_OPCODE_MOVEPOSITIONTIMEACCELERATEFAST:
                EnemyEclInstr::MovePosTime(enemy, instruction);
                enemy->flags.movementEaseType = 4;
                break;
            case ECL_OPCODE_MOVETIMEDECELERATE:
                EnemyEclInstr::MoveTime(enemy, instruction);
                enemy->flags.movementEaseType = 1;
                break;
            case ECL_OPCODE_MOVETIMEDECELERATEFAST:
                EnemyEclInstr::MoveTime(enemy, instruction);
                enemy->flags.movementEaseType = 2;
                break;
            case ECL_OPCODE_MOVETIMEACCELERATE:
                EnemyEclInstr::MoveTime(enemy, instruction);
                enemy->flags.movementEaseType = 3;
                break;
            case ECL_OPCODE_MOVETIMEACCELERATEFAST:
                EnemyEclInstr::MoveTime(enemy, instruction);
                enemy->flags.movementEaseType = 4;
                break;
            case ECL_OPCODE_MOVEBOUNDSSET:
                enemy->lowerMoveLimit.x = instruction->args.moveBoundSet.lowerMoveLimit.x;
                enemy->lowerMoveLimit.y = instruction->args.moveBoundSet.lowerMoveLimit.y;
                enemy->upperMoveLimit.x = instruction->args.moveBoundSet.upperMoveLimit.x;
                enemy->upperMoveLimit.y = instruction->args.moveBoundSet.upperMoveLimit.y;
                enemy->flags.shouldClampPos = true;
                break;
            case ECL_OPCODE_MOVEBOUNDSDISABLE:
                enemy->flags.shouldClampPos = false;
                break;
            case ECL_OPCODE_MOVERAND:
                genericFloat3 = instruction->args.move.pos;
                enemy->angle = g_Rng.GetRandomF32InRange(genericFloat3.y - genericFloat3.x) + genericFloat3.x;
                break;
            case ECL_OPCODE_MOVERANDINBOUND:
                genericFloat3 = instruction->args.move.pos;
                enemy->angle = g_Rng.GetRandomF32InRange(genericFloat3.y - genericFloat3.x) + genericFloat3.x;
                if (enemy->position.x < enemy->lowerMoveLimit.x + 96.0f)
                {
                    if (enemy->angle > ZUN_HALF_PI)
                    {
                        enemy->angle = ZUN_PI - enemy->angle;
                    }
                    else if (enemy->angle < -ZUN_HALF_PI)
                    {
                        enemy->angle = -ZUN_PI - enemy->angle;
                    }
                }
                if (enemy->position.x > enemy->upperMoveLimit.x - 96.0f)
                {
                    if (enemy->angle < ZUN_HALF_PI && enemy->angle >= 0.0f)
                    {
                        enemy->angle = ZUN_PI - enemy->angle;
                    }
                    else if (enemy->angle > -ZUN_HALF_PI && enemy->angle <= 0.0f)
                    {
                        enemy->angle = -ZUN_PI - enemy->angle;
                    }
                }
                if (enemy->position.y < enemy->lowerMoveLimit.y + 48.0f && enemy->angle < 0.0f)
                {
                    enemy->angle = -enemy->angle;
                }
                if (enemy->position.y > enemy->upperMoveLimit.y - 48.0f && enemy->angle > 0.0f)
                {
                    enemy->angle = -enemy->angle;
                }
                break;
            case ECL_OPCODE_ANMSETPOSES:
                enemy->anmPoseDefault = instruction->args.anmSetPoses.anmPoseDefault;
                enemy->anmPoseNeutralFromLeft = instruction->args.anmSetPoses.anmPoseNeutralFromLeft;
                enemy->anmPoseNeutralFromRight = instruction->args.anmSetPoses.anmPoseNeutralFromRight;
                enemy->anmPoseLeft = instruction->args.anmSetPoses.anmPoseLeft;
                enemy->anmPoseRight = instruction->args.anmSetPoses.anmPoseRight;
                enemy->anmPoseCurrent = EnemyPose_Default;
                break;
            case ECL_OPCODE_ENEMYSETHITBOX:
                enemy->hitboxDimensions.x = instruction->args.move.pos.x;
                enemy->hitboxDimensions.y = instruction->args.move.pos.y;
                enemy->hitboxDimensions.z = instruction->args.move.pos.z;
                break;
            case ECL_OPCODE_ENEMYFLAGCOLLISION:
                enemy->flags.isCollidable = instruction->args.setInt;
                break;
            case ECL_OPCODE_ENEMYFLAGCANTAKEDAMAGE:
                enemy->flags.isDamageable = instruction->args.setInt;
                break;
            case ECL_OPCODE_EFFECTSOUND:
                g_SoundPlayer.PlaySoundByIdx((SoundIdx)instruction->args.setInt);
                break;
            case ECL_OPCODE_ENEMYFLAGDEATH:
                enemy->flags.deathMode = instruction->args.setInt;
                break;
            case ECL_OPCODE_DEATHCALLBACKSUB:
                enemy->deathCallbackSub = instruction->args.setInt;
                break;
            case ECL_OPCODE_ENEMYINTERRUPTSET:
                enemy->interrupts[args->setInterrupt.interruptId] = args->setInterrupt.interruptSub;
                break;
            case ECL_OPCODE_ENEMYINTERRUPT:
                enemy->runInterrupt = instruction->args.setInt;
            HANDLE_INTERRUPT:
                enemy->currentContext.currentInstr = (EclRawInstr *)((u8 *)instruction + instruction->offsetToNext);
                if (!enemy->flags.disableCallStack)
                {
                    enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
                }
                g_EclManager.CallEclSub(&enemy->currentContext, enemy->interrupts[enemy->runInterrupt]);
                if (enemy->stackDepth < MAX_ECL_STACK_DEPTH)
                {
                    enemy->stackDepth++;
                }
                enemy->runInterrupt = -1;
                continue;
            case ECL_OPCODE_ENEMYLIFESET:
                enemy->life = enemy->maxLife = instruction->args.setInt;
                break;
#pragma var_order(catk, length, csum)
            case ECL_OPCODE_SPELLCARDSTART: {
                g_Gui.ShowSpellcard(instruction->args.spellcardStart.spellcardSprite,
                                    instruction->args.spellcardStart.spellcardName);
                g_EnemyManager.spellcardInfo.isCapturing = true;
                g_EnemyManager.spellcardInfo.isActive = 1;
                g_EnemyManager.spellcardInfo.idx = instruction->args.spellcardStart.spellcardId;
                g_EnemyManager.spellcardInfo.captureScore = g_SpellcardScore[g_EnemyManager.spellcardInfo.idx];
                g_BulletManager.TurnAllBulletsIntoPoints();
                g_Stage.spellcardState = RUNNING;
                g_Stage.ticksSinceSpellcardStarted = 0;
                enemy->bulletRankSpeedLow = -0.5f;
                enemy->bulletRankSpeedHigh = 0.5f;
                enemy->bulletRankAmount1Low = 0;
                enemy->bulletRankAmount1High = 0;
                enemy->bulletRankAmount2Low = 0;
                enemy->bulletRankAmount2High = 0;
                Catk* catk = &g_GameManager.catk[g_EnemyManager.spellcardInfo.idx];
                i32 csum = 0;
                i32 length;
                if (!g_GameManager.isInReplay)
                {
                    strcpy(catk->name, instruction->args.spellcardStart.spellcardName);
                    length = strlen(catk->name);
                    while (length > 0)
                    {
                        csum += catk->name[--length];
                    }
                    if (catk->nameCsum != (u8)csum)
                    {
                        catk->numSuccess = 0;
                        catk->numAttempts = 0;
                        catk->nameCsum = csum;
                    }
                    catk->captureScore = g_EnemyManager.spellcardInfo.captureScore;
                    if (catk->numAttempts < 9999)
                    {
                        catk->numAttempts++;
                    }
                }
                break;
            }
            case ECL_OPCODE_SPELLCARDEND:
                if (g_EnemyManager.spellcardInfo.isActive)
                {
                    g_Gui.EndEnemySpellcard();
                    if (g_EnemyManager.spellcardInfo.isActive == 1)
                    {
                        i32 scoreIncrease = g_BulletManager.DespawnBullets(12800, true);
#pragma var_order(catk, idx, score)
                        if (g_EnemyManager.spellcardInfo.isCapturing)
                        {
                            i32 idx;
                            Catk* catk = &g_GameManager.catk[g_EnemyManager.spellcardInfo.idx];
                            i32 score = g_EnemyManager.spellcardInfo.captureScore >= 500000
                                           ? 500000 / 10
                                           : g_EnemyManager.spellcardInfo.captureScore / 10;
                            scoreIncrease =
                                g_EnemyManager.spellcardInfo.captureScore +
                                g_EnemyManager.spellcardInfo.captureScore * g_Gui.SpellcardSecondsRemaining() / 10;
                            g_Gui.ShowSpellcardBonus(scoreIncrease);
                            g_GameManager.score += scoreIncrease;
                            if (!g_GameManager.isInReplay)
                            {
                                catk->numSuccess++;
                                for (idx = 4; idx > 0; idx--)
                                {
                                    catk->characterShotType[idx] = catk->characterShotType[idx - 1];
                                }
                                catk->characterShotType[0] = g_GameManager.CharacterShotType();
                            }
                            g_GameManager.spellcardsCaptured++;
                        }
                    }
                    g_EnemyManager.spellcardInfo.isActive = 0;
                }
                g_Stage.spellcardState = NOT_RUNNING;
                break;
            case ECL_OPCODE_BOSSTIMERSET:
                enemy->bossTimer = instruction->args.setInt;
                break;
            case ECL_OPCODE_LIFECALLBACKTHRESHOLD:
                enemy->lifeCallbackThreshold = instruction->args.setInt;
                break;
            case ECL_OPCODE_LIFECALLBACKSUB:
                enemy->lifeCallbackSub = instruction->args.setInt;
                break;
            case ECL_OPCODE_TIMERCALLBACKTHRESHOLD:
                enemy->timerCallbackThreshold = instruction->args.setInt;
                enemy->bossTimer = 0;
                break;
            case ECL_OPCODE_TIMERCALLBACKSUB:
                enemy->timerCallbackSub = instruction->args.setInt;
                break;
            case ECL_OPCODE_ENEMYFLAGINTERACTABLE:
                enemy->flags.isInteractable = instruction->args.setInt;
                break;
            case ECL_OPCODE_EFFECTPARTICLE:
                g_EffectManager.SpawnParticles(instruction->args.effectParticle.effectId, &enemy->position,
                                               instruction->args.effectParticle.numParticles,
                                               instruction->args.effectParticle.particleColor);
                break;
            case ECL_OPCODE_DROPITEMS: {
                for (i32 idx = 0; idx < instruction->args.setInt; idx++)
                {
                    D3DXVECTOR3 pos = enemy->position;

                    pos[0] += g_Rng.GetRandomF32InRange(144.0f) - 72.0f;
                    pos[1] += g_Rng.GetRandomF32InRange(144.0f) - 72.0f;
                    if (g_GameManager.currentPower < MAX_POWER)
                    {
                        g_ItemManager.SpawnItem(&pos, idx == 0 ? ITEM_POWER_BIG : ITEM_POWER_SMALL, 0);
                    }
                    else
                    {
                        g_ItemManager.SpawnItem(&pos, ITEM_POINT, 0);
                    }
                }
                break;
            }
            case ECL_OPCODE_ANMFLAGROTATION:
                enemy->flags.rotateAnm = instruction->args.setInt;
                break;
            case ECL_OPCODE_EXINSCALL:
                g_EclExInsn[instruction->args.setInt](enemy, instruction);
                break;
            case ECL_OPCODE_EXINSREPEAT:
                if (instruction->args.setInt >= 0)
                {
                    enemy->currentContext.funcSetFunc = g_EclExInsn[instruction->args.setInt];
                }
                else
                {
                    enemy->currentContext.funcSetFunc = NULL;
                }
                break;
            case ECL_OPCODE_TIMESET:
                enemy->currentContext.time += *EnemyEclInstr::GetVar(enemy, &instruction->args.timeSet.timeToSet, NULL);
                break;
            case ECL_OPCODE_DROPITEMID:
                g_ItemManager.SpawnItem(&enemy->position, instruction->args.dropItem.itemId, 0);
                break;
            case ECL_OPCODE_STDUNPAUSE:
                g_Stage.unpauseFlag = 1;
                break;
            case ECL_OPCODE_BOSSSETLIFECOUNT:
                g_Gui.eclSetLives = instruction->args.GetBossLifeCount();
                g_GameManager.counat += 1800;
                break;
            case ECL_OPCODE_ENEMYCREATE: {
                EclRawInstrEnemyCreateArgs args = instruction->args.enemyCreate;
                args.pos.x = *EnemyEclInstr::GetVarFloat(enemy, &args.pos.x, NULL);
                args.pos.y = *EnemyEclInstr::GetVarFloat(enemy, &args.pos.y, NULL);
                args.pos.z = *EnemyEclInstr::GetVarFloat(enemy, &args.pos.z, NULL);
                g_EnemyManager.SpawnEnemy(args.subId, args.pos.AsD3dXVec(), args.life, args.itemDrop, args.score);
                break;
            }
#pragma var_order(currentEnemy, idx)
            case ECL_OPCODE_ENEMYKILLALL: {
                Enemy *currentEnemy;
                i32 idx;
                for (currentEnemy = &g_EnemyManager.enemies[0], idx = 0; idx < MAX_ENEMY_COUNT; idx++, currentEnemy++)
                {
                    if (!currentEnemy->flags.isSlotOccupied)
                    {
                        continue;
                    }
                    if (currentEnemy->flags.isBoss)
                    {
                        continue;
                    }

                    currentEnemy->life = 0;
                    if (!currentEnemy->flags.isInteractable && currentEnemy->deathCallbackSub >= 0)
                    {
                        g_EclManager.CallEclSub(&currentEnemy->currentContext, currentEnemy->deathCallbackSub);
                        currentEnemy->deathCallbackSub = -1;
                    }
                }
                break;
            }
            case ECL_OPCODE_ANMINTERRUPTMAIN:
                enemy->primaryVm.pendingInterrupt = instruction->args.setInt;
                break;
            case ECL_OPCODE_ANMINTERRUPTSLOT:
                enemy->vms[args->anmInterruptSlot.vmId].pendingInterrupt = args->anmInterruptSlot.interruptId;
                break;
            case ECL_OPCODE_BULLETCANCEL:
                g_BulletManager.TurnAllBulletsIntoPoints();
                break;
            case ECL_OPCODE_BULLETSOUND:
                if (instruction->args.bulletSound.bulletSfx >= 0)
                {
                    enemy->bulletProps.sfx = instruction->args.bulletSound.bulletSfx;
                    enemy->bulletProps.flags |= 0x200;
                }
                else
                {
                    enemy->bulletProps.flags &= ~0x200;
                }
                break;
            case ECL_OPCODE_ENEMYFLAGDISABLECALLSTACK:
                enemy->flags.disableCallStack = instruction->args.setInt;
                break;
            case ECL_OPCODE_BULLETRANKINFLUENCE:
                enemy->bulletRankSpeedLow = args->bulletRankInfluence.bulletRankSpeedLow;
                enemy->bulletRankSpeedHigh = args->bulletRankInfluence.bulletRankSpeedHigh;
                enemy->bulletRankAmount1Low = args->bulletRankInfluence.bulletRankAmount1Low;
                enemy->bulletRankAmount1High = args->bulletRankInfluence.bulletRankAmount1High;
                enemy->bulletRankAmount2Low = args->bulletRankInfluence.bulletRankAmount2Low;
                enemy->bulletRankAmount2High = args->bulletRankInfluence.bulletRankAmount2High;
                break;
            case ECL_OPCODE_ENEMYFLAGINVISIBLE:
                enemy->flags.isInvisible = instruction->args.setInt;
                break;
            case ECL_OPCODE_BOSSTIMERCLEAR:
                enemy->timerCallbackSub = enemy->deathCallbackSub;
                enemy->bossTimer = 0;
                break;
            case ECL_OPCODE_SPELLCARDFLAGTIMEOUT:
                enemy->flags.isTimeoutSpell = instruction->args.setInt;
                break;
            }
        NEXT_INSN:
            instruction = (EclRawInstr *)((u8 *)instruction + instruction->offsetToNext);
            goto YOLO;
        }
        else
        {
            switch (enemy->flags.movementMode)
            {
            case 1:
                enemy->angle = utils::AddNormalizeAngle(enemy->angle, g_Supervisor.effectiveFramerateMultiplier *
                                                                          enemy->angularVelocity);
                enemy->speed = g_Supervisor.effectiveFramerateMultiplier * enemy->acceleration + enemy->speed;
                sincosmul(&enemy->axisSpeed, enemy->angle, enemy->speed);
                enemy->axisSpeed.z = 0.0f;
                break;
            case 2: {
                enemy->moveInterpTimer--;
                f32 interpVal = enemy->moveInterpTimer.AsFramesFloat() / enemy->moveInterpStartTime;
                if (interpVal >= 1.0f)
                {
                    interpVal = 1.0f;
                }
                switch (enemy->flags.movementEaseType)
                {
                case 0:
                    interpVal = 1.0f - interpVal;
                    break;
                case 1:
                    interpVal = 1.0f - interpVal * interpVal;
                    break;
                case 2:
                    interpVal = 1.0f - interpVal * interpVal * interpVal * interpVal;
                    break;
                case 3:
                    interpVal = 1.0f - interpVal;
                    interpVal *= interpVal;
                    break;
                case 4:
                    interpVal = 1.0f - interpVal;
                    interpVal = interpVal * interpVal * interpVal * interpVal;
                }
                enemy->axisSpeed = interpVal * enemy->moveInterp + enemy->moveInterpStartPos - enemy->position;
                enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
                if (enemy->moveInterpTimer <= 0)
                {
                    enemy->flags.movementMode = 0;
                    enemy->position = enemy->moveInterpStartPos + enemy->moveInterp;
                    enemy->axisSpeed = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
                }
                break;
            }
            }
            if (enemy->life > 0)
            {
                if (enemy->shootInterval > 0)
                {
                    enemy->shootIntervalTimer++;
                    if (enemy->shootIntervalTimer >= enemy->shootInterval)
                    {
                        enemy->bulletProps.position = enemy->position + enemy->shootOffset;
                        g_BulletManager.SpawnBulletPattern(&enemy->bulletProps);
                        enemy->shootIntervalTimer = 0;
                    }
                }
                if (enemy->anmPoseLeft >= 0) // Check if poses are enabled
                {
                    EnemyPose newPose = EnemyPose_Neutral;
                    if (enemy->axisSpeed.x < 0.0f)
                    {
                        newPose = EnemyPose_Left;
                    }
                    else if (enemy->axisSpeed.x > 0.0f)
                    {
                        newPose = EnemyPose_Right;
                    }
                    if (enemy->anmPoseCurrent != newPose)
                    {
                        switch (newPose)
                        {
                        case EnemyPose_Neutral:
                            if (enemy->anmPoseCurrent == EnemyPose_Default)
                            {
                                g_AnmManager->SetAndExecuteScriptIdx(&enemy->primaryVm,
                                                                     enemy->anmPoseDefault + ANM_OFFSET_ENEMY);
                            }
                            else if (enemy->anmPoseCurrent == EnemyPose_Left)
                            {
                                g_AnmManager->SetAndExecuteScriptIdx(&enemy->primaryVm,
                                                                     enemy->anmPoseNeutralFromLeft + ANM_OFFSET_ENEMY);
                            }
                            else // EnemyPose_Right
                            {
                                g_AnmManager->SetAndExecuteScriptIdx(&enemy->primaryVm,
                                                                     enemy->anmPoseNeutralFromRight + ANM_OFFSET_ENEMY);
                            }
                            break;
                        case EnemyPose_Left:
                            g_AnmManager->SetAndExecuteScriptIdx(&enemy->primaryVm,
                                                                 enemy->anmPoseLeft + ANM_OFFSET_ENEMY);
                            break;
                        case EnemyPose_Right:
                            g_AnmManager->SetAndExecuteScriptIdx(&enemy->primaryVm,
                                                                 enemy->anmPoseRight + ANM_OFFSET_ENEMY);
                            break;
                        }
                        enemy->anmPoseCurrent = newPose;
                    }
                }
                if (enemy->currentContext.funcSetFunc != NULL)
                {
                    enemy->currentContext.funcSetFunc(enemy, NULL);
                }
            }
            enemy->currentContext.currentInstr = instruction;
            enemy->currentContext.time++;
            return ZUN_SUCCESS;
        }
    }
}
namespace EnemyEclInstr
{

#pragma var_order(alu, angle)
void MoveDirTime(Enemy *enemy, EclRawInstr *instr)
{
    EclRawInstrAluArgs *alu;
    f32 angle;

    alu = &instr->args.alu;
    angle = *GetVarFloat(enemy, &alu->arg1.f32, NULL);

    enemy->moveInterp.x = cosf(angle) * alu->arg2.f32 * alu->res / 2.0f;
    enemy->moveInterp.y = sinf(angle) * alu->arg2.f32 * alu->res / 2.0f;
    enemy->moveInterp.z = 0.0f;

    enemy->moveInterpStartPos = enemy->position;
    enemy->moveInterpStartTime = alu->res;

    enemy->moveInterpTimer = enemy->moveInterpStartTime;

    enemy->flags.movementMode = 2;
}

void MovePosTime(Enemy *enemy, EclRawInstr *instr)
{
    D3DXVECTOR3 newPos;
    EclRawInstrAluArgs *alu = &instr->args.alu;

    newPos.x = *GetVarFloat(enemy, &alu->arg1.f32, NULL);
    newPos.y = *GetVarFloat(enemy, &alu->arg2.f32, NULL);
    newPos.z = *GetVarFloat(enemy, &alu->arg3.f32, NULL);

    enemy->moveInterp = newPos - enemy->position;
    enemy->moveInterpStartPos = enemy->position;
    enemy->moveInterpStartTime = alu->res;

    enemy->moveInterpTimer = enemy->moveInterpStartTime;

    enemy->flags.movementMode = 2;
    enemy->axisSpeed = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
}

void MoveTime(Enemy *enemy, EclRawInstr *instr)
{
    EclRawInstrAluArgs *alu;
    f32 angle;

    alu = &instr->args.alu;
    angle = *GetVarFloat(enemy, &enemy->angle, NULL);

    enemy->moveInterp.x = cosf(angle) * enemy->speed * alu->res / 2.0f;
    enemy->moveInterp.y = sinf(angle) * enemy->speed * alu->res / 2.0f;
    enemy->moveInterp.z = 0.0f;

    enemy->moveInterpStartPos = enemy->position;
    enemy->moveInterpStartTime = alu->res;

    enemy->moveInterpTimer = enemy->moveInterpStartTime;

    enemy->flags.movementMode = 2;
}

i32 *GetVar(Enemy *enemy, EclVarId *eclVarId, EclValueType *valueType)
{
    if (valueType != NULL)
        *valueType = ECL_VALUE_TYPE_UNDEFINED;

    switch (*eclVarId)
    {
    case ECL_VAR_I32_0:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_INT;
        return &enemy->currentContext.var0;

    case ECL_VAR_I32_1:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_INT;
        return &enemy->currentContext.var1;

    case ECL_VAR_I32_2:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_INT;
        return &enemy->currentContext.var2;

    case ECL_VAR_I32_3:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_INT;
        return &enemy->currentContext.var3;

    case ECL_VAR_F32_0:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_FLOAT;
        return (i32 *)&enemy->currentContext.float0;

    case ECL_VAR_F32_1:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_FLOAT;
        return (i32 *)&enemy->currentContext.float1;

    case ECL_VAR_F32_2:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_FLOAT;
        return (i32 *)&enemy->currentContext.float2;

    case ECL_VAR_F32_3:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_FLOAT;
        return (i32 *)&enemy->currentContext.float3;

    case ECL_VAR_I32_4:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_INT;
        return &enemy->currentContext.var4;

    case ECL_VAR_I32_5:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_INT;
        return &enemy->currentContext.var5;

    case ECL_VAR_I32_6:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_INT;
        return &enemy->currentContext.var6;

    case ECL_VAR_I32_7:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_INT;
        return &enemy->currentContext.var7;

    case ECL_VAR_DIFFICULTY:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_READONLY;
        return (i32 *)&g_GameManager.difficulty;

    case ECL_VAR_RANK:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_READONLY;
        return &g_GameManager.rank;

    case ECL_VAR_ENEMY_POS_X:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_FLOAT;
        return (i32 *)&enemy->position;

    case ECL_VAR_ENEMY_POS_Y:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_FLOAT;
        return (i32 *)&enemy->position.y;

    case ECL_VAR_ENEMY_POS_Z:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_FLOAT;
        return (i32 *)&enemy->position.z;

    case ECL_VAR_PLAYER_POS_X:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_READONLY;
        return (i32 *)&g_Player.positionCenter;

    case ECL_VAR_PLAYER_POS_Y:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_READONLY;
        return (i32 *)&g_Player.positionCenter.y;

    case ECL_VAR_PLAYER_POS_Z:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_READONLY;
        return (i32 *)&g_Player.positionCenter.z;

    case ECL_VAR_PLAYER_ANGLE:
        g_PlayerAngle = g_Player.AngleToPlayer(&enemy->position);
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_READONLY;
        return (i32 *)&g_PlayerAngle;

    case ECL_VAR_ENEMY_TIMER:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_INT;
        return &enemy->bossTimer.current;

    case ECL_VAR_PLAYER_DISTANCE:
        g_PlayerDistance = D3DXVec3Length(&(g_Player.positionCenter - enemy->position));
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_READONLY;
        return (i32 *)&g_PlayerDistance;

    case ECL_VAR_ENEMY_LIFE:
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_INT;
        return &enemy->life;

    case ECL_VAR_PLAYER_SHOT:
        g_PlayerShot = g_GameManager.CharacterShotType();
        if (valueType != NULL)
            *valueType = ECL_VALUE_TYPE_INT;
        return &g_PlayerShot;
    }
    return (i32 *)eclVarId;
}

f32 *GetVarFloat(Enemy *enemy, f32 *eclVarId, EclValueType *valueType)
{
    i32 varId = *eclVarId;
    i32 *res = GetVar(enemy, (EclVarId *)&varId, valueType);
    if (res == &varId)
    {
        return eclVarId;
    }
    else
    {
        return (f32 *)res;
    }
}

#pragma var_order(lhsPtr, rhsPtr, lhsType)
void SetVar(Enemy *enemy, EclVarId lhs, void *rhs)
{
    i32 *lhsPtr;
    EclValueType lhsType;
    i32 *rhsPtr;

    rhsPtr = GetVar(enemy, (EclVarId *)rhs, NULL);
    lhsPtr = GetVar(enemy, &lhs, &lhsType);
    if (lhsType == ECL_VALUE_TYPE_INT)
    {
        *lhsPtr = *rhsPtr;
    }
    else if (lhsType == ECL_VALUE_TYPE_FLOAT)
    {
        *(f32 *)lhsPtr = *(f32 *)rhsPtr;
    }
}

#pragma var_order(outPtr, rhsPtr, lhsPtr, outType)
void MathAdd(Enemy *enemy, EclVarId outVarId, EclVarId *lhsVarId, EclVarId *rhsVarId)
{
    EclValueType outType;
    i32 *outPtr;
    i32 *lhsPtr;
    i32 *rhsPtr;

    // Get output variable.
    outPtr = GetVar(enemy, &outVarId, &outType);
    if (outType == ECL_VALUE_TYPE_INT)
    {
        lhsPtr = GetVar(enemy, lhsVarId, NULL);
        rhsPtr = GetVar(enemy, rhsVarId, NULL);
        *outPtr = *lhsPtr + *rhsPtr;
    }
    else if (outType == ECL_VALUE_TYPE_FLOAT)
    {
        lhsPtr = (i32 *)GetVarFloat(enemy, (f32 *)lhsVarId, NULL);
        rhsPtr = (i32 *)GetVarFloat(enemy, (f32 *)rhsVarId, NULL);
        *(f32 *)outPtr = *(f32 *)lhsPtr + *(f32 *)rhsPtr;
    }
}

#pragma var_order(outPtr, rhsPtr, lhsPtr, outType)
void MathSub(Enemy *enemy, EclVarId outVarId, EclVarId *lhsVarId, EclVarId *rhsVarId)
{
    EclValueType outType;
    i32 *outPtr;
    i32 *lhsPtr;
    i32 *rhsPtr;

    outPtr = GetVar(enemy, &outVarId, &outType);
    if (outType == ECL_VALUE_TYPE_INT)
    {
        lhsPtr = GetVar(enemy, lhsVarId, NULL);
        rhsPtr = GetVar(enemy, rhsVarId, NULL);
        *outPtr = *lhsPtr - *rhsPtr;
    }
    else if (outType == ECL_VALUE_TYPE_FLOAT)
    {
        lhsPtr = (i32 *)GetVarFloat(enemy, (f32 *)lhsVarId, NULL);
        rhsPtr = (i32 *)GetVarFloat(enemy, (f32 *)rhsVarId, NULL);
        *(f32 *)outPtr = *(f32 *)lhsPtr - *(f32 *)rhsPtr;
    }
}

#pragma var_order(outPtr, rhsPtr, lhsPtr, outType)
void MathMul(Enemy *enemy, EclVarId outVarId, EclVarId *lhsVarId, EclVarId *rhsVarId)
{
    EclValueType outType;
    i32 *outPtr;
    i32 *lhsPtr;
    i32 *rhsPtr;

    lhsPtr = GetVar(enemy, lhsVarId, NULL);
    rhsPtr = GetVar(enemy, rhsVarId, NULL);
    outPtr = GetVar(enemy, &outVarId, &outType);
    if (outType == ECL_VALUE_TYPE_INT)
    {
        lhsPtr = GetVar(enemy, lhsVarId, NULL);
        rhsPtr = GetVar(enemy, rhsVarId, NULL);
        *outPtr = *lhsPtr * *rhsPtr;
    }
    else if (outType == ECL_VALUE_TYPE_FLOAT)
    {
        lhsPtr = (i32 *)GetVarFloat(enemy, (f32 *)lhsVarId, NULL);
        rhsPtr = (i32 *)GetVarFloat(enemy, (f32 *)rhsVarId, NULL);
        *(f32 *)outPtr = *(f32 *)lhsPtr * *(f32 *)rhsPtr;
    }
}

#pragma var_order(outPtr, rhsPtr, lhsPtr, outType)
void MathDiv(Enemy *enemy, EclVarId outVarId, EclVarId *lhsVarId, EclVarId *rhsVarId)
{
    EclValueType outType;
    i32 *outPtr;
    i32 *lhsPtr;
    i32 *rhsPtr;

    outPtr = GetVar(enemy, &outVarId, &outType);
    if (outType == ECL_VALUE_TYPE_INT)
    {
        lhsPtr = GetVar(enemy, lhsVarId, NULL);
        rhsPtr = GetVar(enemy, rhsVarId, NULL);
        *outPtr = *lhsPtr / *rhsPtr;
    }
    else if (outType == ECL_VALUE_TYPE_FLOAT)
    {
        lhsPtr = (i32 *)GetVarFloat(enemy, (f32 *)lhsVarId, NULL);
        rhsPtr = (i32 *)GetVarFloat(enemy, (f32 *)rhsVarId, NULL);
        *(f32 *)outPtr = *(f32 *)lhsPtr / *(f32 *)rhsPtr;
    }
}

#pragma var_order(outPtr, rhsPtr, lhsPtr, outType)
void MathMod(Enemy *enemy, EclVarId outVarId, EclVarId *lhsVarId, EclVarId *rhsVarId)
{
    EclValueType outType;
    i32 *outPtr;
    i32 *lhsPtr;
    i32 *rhsPtr;

    outPtr = GetVar(enemy, &outVarId, &outType);
    if (outType == ECL_VALUE_TYPE_INT)
    {
        lhsPtr = GetVar(enemy, lhsVarId, NULL);
        rhsPtr = GetVar(enemy, rhsVarId, NULL);
        *outPtr = *lhsPtr % *rhsPtr;
    }
    else if (outType == ECL_VALUE_TYPE_FLOAT)
    {
        lhsPtr = (i32 *)GetVarFloat(enemy, (f32 *)lhsVarId, NULL);
        rhsPtr = (i32 *)GetVarFloat(enemy, (f32 *)rhsVarId, NULL);
        *(f32 *)outPtr = fmodf(*(f32 *)lhsPtr, *(f32 *)rhsPtr);
    }
}

#pragma var_order(y2Ptr, outPtr, x1Ptr, y1Ptr, outType, x2Ptr)
void MathAtan2(Enemy *enemy, EclVarId outVarId, f32 *x1, f32 *y1, f32 *y2, f32 *x2)
{
    EclValueType outType;
    f32 *outPtr;
    f32 *y1Ptr, *x1Ptr, *x2Ptr, *y2Ptr;

    outPtr = (f32 *)GetVar(enemy, &outVarId, &outType);
    if (outType == ECL_VALUE_TYPE_FLOAT)
    {
        y1Ptr = GetVarFloat(enemy, x1, NULL);
        x1Ptr = GetVarFloat(enemy, y1, NULL);
        y2Ptr = GetVarFloat(enemy, y2, NULL);
        x2Ptr = GetVarFloat(enemy, x2, NULL);
        *outPtr = atan2f(*x2Ptr - *x1Ptr, *y2Ptr - *y1Ptr);
    }
}
} // namespace EnemyEclInstr
} // namespace th06
