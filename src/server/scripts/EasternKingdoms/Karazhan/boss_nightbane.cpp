/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/* ScriptData
SDName: Boss_Nightbane
SD%Complete: 80
SDComment: SDComment: Timers may incorrect
SDCategory: Karazhan
EndScriptData */

#include "Player.h"
#include "ScriptObject.h"
#include "ScriptedCreature.h"
#include "TaskScheduler.h"
#include "karazhan.h"

enum Spells
{
    // phase 1
    SPELL_BELLOWING_ROAR        = 39427,
    SPELL_CHARRED_EARTH         = 30129,
    SPELL_DISTRACTING_ASH       = 30130,
    SPELL_SMOLDERING_BREATH     = 30210,
    SPELL_TAIL_SWEEP            = 25653,
    // phase 2
    SPELL_RAIN_OF_BONES         = 37098,
    SPELL_SMOKING_BLAST         = 37057,
    SPELL_FIREBALL_BARRAGE      = 30282,
    SPELL_SEARING_CINDERS       = 30127,
    SPELL_SUMMON_SKELETON       = 30170
};

enum Says
{
    EMOTE_SUMMON                = 0, // Not used in script
    YELL_AGGRO                  = 1,
    YELL_FLY_PHASE              = 2,
    YELL_LAND_PHASE             = 3,
    EMOTE_BREATH                = 4
};

enum Groups
{
    GROUP_GROUND                = 0,
    GROUP_FLYING                = 1
};

float IntroWay[8][3] =
{
    {-11053.37f, -1794.48f, 149.00f},
    {-11141.07f, -1841.40f, 125.00f},
    {-11187.28f, -1890.23f, 125.00f},
    {-11189.20f, -1931.25f, 125.00f},
    {-11153.76f, -1948.93f, 125.00f},
    {-11128.73f, -1929.75f, 125.00f},
    {-11140.00f, -1915.00f, 122.00f},
    {-11163.00f, -1903.00f, 91.473f}
}; //TODO: move to table

struct boss_nightbane : public BossAI
{
    boss_nightbane(Creature* creature) : BossAI(creature, DATA_NIGHTBANE)
    {
        _intro = true;
        _skeletonCount = 5;
        scheduler.SetValidator([this]
        {
            return !me->HasUnitState(UNIT_STATE_CASTING);
        });
    }

    void Reset() override
    {
        BossAI::Reset();
        _skeletonscheduler.CancelAll();
        Phase = 1;
        MovePhase = 0;
        me->SetUnitFlag(UNIT_FLAG_NOT_SELECTABLE);

        me->SetSpeed(MOVE_RUN, 2.0f);
        me->SetDisableGravity(_intro);
        me->SetWalk(false);
        me->setActive(true);

        if (instance)
        {
            if (instance->GetData(DATA_NIGHTBANE) == DONE)
                me->DisappearAndDie();
            else
                instance->SetData(DATA_NIGHTBANE, NOT_STARTED);
        }

        HandleTerraceDoors(true);

        _flying = false;
        _movement = false;

        if (!_intro)
        {
            //when boss is reset and we're past the intro
            //cannot despawn, but have to move to a location where he normally is
            //me->SetHomePosition(IntroWay[7][0], IntroWay[7][1], IntroWay[7][2], 0);
            Position preSpawnPosis = me->GetHomePosition();
            me->NearTeleportTo(preSpawnPosis);
            instance->SetData(DATA_NIGHTBANE, NOT_STARTED);
            _intro = true;
            Phase = 1;
            MovePhase = 0;
        }

        ScheduleHealthCheckEvent({ 75, 50, 25 }, [&]{
            TakeOff();
        });
    }

    void HandleTerraceDoors(bool open)
    {
        if (instance)
        {
            instance->HandleGameObject(instance->GetGuidData(DATA_MASTERS_TERRACE_DOOR_1), open);
            instance->HandleGameObject(instance->GetGuidData(DATA_MASTERS_TERRACE_DOOR_2), open);
        }
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _JustEngagedWith();
        if (instance)
            instance->SetData(DATA_NIGHTBANE, IN_PROGRESS);

        HandleTerraceDoors(false);
        Talk(YELL_AGGRO);
        ScheduleGround();
    }

    void ScheduleGround() {
        scheduler.Schedule(30s, GROUP_GROUND, [this](TaskContext context)
        {
            DoCastAOE(SPELL_BELLOWING_ROAR);
            context.Repeat(30s, 40s);
        }).Schedule(15s, GROUP_GROUND, [this](TaskContext context)
        {
            DoCastRandomTarget(SPELL_CHARRED_EARTH, 0, 100.0f, true);
            context.Repeat(20s);
        }).Schedule(10s, GROUP_GROUND, [this](TaskContext context)
        {
            DoCastVictim(SPELL_SMOLDERING_BREATH);
            context.Repeat(20s);
        }).Schedule(12s, GROUP_GROUND, [this](TaskContext context)
        {
            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100, true))
            {
                if (!me->HasInArc(M_PI, target))
                {
                    DoCast(target, SPELL_TAIL_SWEEP);
                }
            }
            context.Repeat(15s);
        }).Schedule(14s, GROUP_GROUND, [this](TaskContext context)
        {
            DoCastRandomTarget(SPELL_SEARING_CINDERS);
            context.Repeat(10s);
        });
    }

    void ScheduleFly() {
        _skeletonSpawnCounter = 0;

        scheduler.Schedule(2s, GROUP_FLYING, [this](TaskContext)
        {
            DoCastVictim(SPELL_RAIN_OF_BONES);
            _skeletonscheduler.Schedule(50ms, [this](TaskContext context)
            {
                //spawns skeletons every second until skeletonCount is reached
                if(_skeletonSpawnCounter < _skeletonCount)
                {
                    DoCastVictim(SPELL_SUMMON_SKELETON, true);
                    _skeletonSpawnCounter++;
                    context.Repeat(2s);
                }
            });
        }).Schedule(20s, GROUP_FLYING, [this](TaskContext context)
        {
            DoCastRandomTarget(SPELL_DISTRACTING_ASH);
            context.Repeat(2s); //timer wrong?
        }).Schedule(25s, GROUP_FLYING, [this](TaskContext context)
        {
            //5 seconds added due to double trigger?
            //trigger for timer in original + in rain of bones
            //timers need some investigation
            DoCastVictim(SPELL_SMOKING_BLAST);
            context.Repeat(1500ms); //timer wrong?
        }).Schedule(13s, GROUP_FLYING, [this](TaskContext context)
        {
            DoCastOnFarAwayPlayers(SPELL_FIREBALL_BARRAGE, false, 80.0f);
            context.Repeat(20s);
        });
    }

    void AttackStart(Unit* who) override
    {
        if (!_intro && !_flying)
            ScriptedAI::AttackStart(who);
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();
        HandleTerraceDoors(true);
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (!_intro && !_flying)
            ScriptedAI::MoveInLineOfSight(who);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE)
            return;

        if (_intro)
        {
            if (id >= 8)
            {
                _intro = false;
                //me->SetHomePosition(IntroWay[7][0], IntroWay[7][1], IntroWay[7][2], 0);
                //doesn't need home position because we have to "despawn" boss on reset
                me->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
                me->SetInCombatWithZone();
                return;
            }

            MovePhase = id + 1;
            return;
        }

        if (_flying)
        {
            if (id == 0)
            {
                Talk(EMOTE_BREATH);
                _flying = false;
                Phase = 2;
                return;
            }

            if (id < 8)
                MovePhase = id + 1;
            else
            {
                Phase = 1;
                _flying = false;
                _movement = true;
                return;
            }
        }
    }

    void JustSummoned(Creature* summon) override
    {
        summon->AI()->AttackStart(me->GetVictim());
        summons.Summon(summon);
    }

    void DoCastOnFarAwayPlayers(uint32 spellid, bool triggered, float tresholddistance)
    {
        //resembles DoCastToAllHostilePlayers a bit/lot
        ThreatContainer::StorageType targets = me->GetThreatMgr().GetThreatList();
        for (ThreatContainer::StorageType::const_iterator itr = targets.begin(); itr != targets.end(); ++itr)
        {
            if (Unit* unit = ObjectAccessor::GetUnit(*me, (*itr)->getUnitGuid()))
            {
                if (unit->IsPlayer() && !unit->IsWithinDist(me, tresholddistance, false))
                {
                    me->CastSpell(unit, spellid, triggered);
                }
            }
        }
    }

    void TakeOff()
    {
        Talk(YELL_FLY_PHASE);
        scheduler.CancelGroup(GROUP_GROUND);

        me->InterruptSpell(CURRENT_GENERIC_SPELL);
        me->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
        me->SetDisableGravity(true);
        me->GetMotionMaster()->Clear(false);
        me->GetMotionMaster()->MovePoint(0, IntroWay[2][0], IntroWay[2][1], IntroWay[2][2]);

        _flying = true;

        ScheduleFly();

        //handle landing again
        scheduler.Schedule(45s, 60s, [this](TaskContext)
        {
            Talk(YELL_LAND_PHASE);

            me->GetMotionMaster()->Clear(false);
            me->GetMotionMaster()->MovePoint(3, IntroWay[3][0], IntroWay[3][1], IntroWay[3][2]);

            _flying = true;
            scheduler.CancelGroup(GROUP_FLYING);
            scheduler.Schedule(2s, [this](TaskContext)
            {
                ScheduleGround();
            });
        });
    }

    void UpdateAI(uint32 diff) override
    {
        if (_intro)
        {
            if (MovePhase)
            {
                if (MovePhase >= 7)
                {
                    me->SetDisableGravity(false);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
                    me->GetMotionMaster()->MovePoint(8, IntroWay[7][0], IntroWay[7][1], IntroWay[7][2]);
                }
                else
                {
                    me->GetMotionMaster()->MovePoint(MovePhase, IntroWay[MovePhase][0], IntroWay[MovePhase][1], IntroWay[MovePhase][2]);
                }
                MovePhase = 0;
            }
            return;
        }

        if (_flying && MovePhase)
        {
            if (MovePhase >= 7)
            {
                me->SetDisableGravity(false);
                me->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
                me->GetMotionMaster()->MovePoint(8, IntroWay[7][0], IntroWay[7][1], IntroWay[7][2]);
            }
            else
                me->GetMotionMaster()->MovePoint(MovePhase, IntroWay[MovePhase][0], IntroWay[MovePhase][1], IntroWay[MovePhase][2]);

            MovePhase = 0;
        }

        if (!UpdateVictim())
            return;

        if (_flying)
            return;

        scheduler.Update(diff);
        _skeletonscheduler.Update(diff);

        //  Phase 1 "GROUND FIGHT"
        if (Phase == 1)
        {
            if (_movement)
            {
                DoStartMovement(me->GetVictim());
                _movement = false;
            }

            DoMeleeAttackIfReady();
        }
    }

private:
    uint32 Phase;

    TaskScheduler _skeletonscheduler;

    bool _intro;
    bool _flying;
    bool _movement;

    uint32 MovePhase;
    uint8 _skeletonCount;
    uint8 _skeletonSpawnCounter;
};

class go_blackened_urn : public GameObjectScript
{
public:
    go_blackened_urn() : GameObjectScript("go_blackened_urn") { }

    //if we summon an entity instead of using a sort of invisible entity, we could unsummon boss on reset
    //right now that doesn't work because of how the urn works
    bool OnGossipHello(Player* /*player*/, GameObject* go) override
    {
        if (InstanceScript* instance = go->GetInstanceScript())
        {
            if (instance->GetData(DATA_NIGHTBANE) != DONE && !go->FindNearestCreature(NPC_NIGHTBANE, 40.0f))
            {
                if (Creature* cr = instance->GetCreature(DATA_NIGHTBANE))
                {
                    cr->GetMotionMaster()->MovePoint(0, IntroWay[0][0], IntroWay[0][1], IntroWay[0][2]);
                }
            }
        }

        return false;
    }
};

void AddSC_boss_nightbane()
{
    RegisterKarazhanCreatureAI(boss_nightbane);
    new go_blackened_urn();
}
