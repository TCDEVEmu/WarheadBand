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

#include "ScriptObject.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "culling_of_stratholme.h"

enum Spells
{
    SPELL_SHADOW_BOLT_N                         = 57725,
    SPELL_SHADOW_BOLT_H                         = 58827,
    SPELL_STEAL_FLESH_CHANNEL                   = 52708,
    SPELL_STEAL_FLESH_TARGET                    = 52711,
    SPELL_STEAL_FLESH_CASTER                    = 52712,
    SPELL_SUMMON_GHOULS                         = 52451,
    SPELL_EXPLODE_GHOUL_N                       = 52480,
    SPELL_EXPLODE_GHOUL_H                       = 58825,
    SPELL_CURSE_OF_TWISTED_FAITH                = 58845,
};

enum Events
{
    EVENT_SPELL_SHADOW_BOLT                     = 1,
    EVENT_SPELL_STEAL_FLESH                     = 2,
    EVENT_SPELL_SUMMON_GHOULS                   = 3,
    EVENT_EXPLODE_GHOUL                         = 4,
    EVENT_SPELL_CURSE                           = 5,
};

enum Yells
{
    SAY_AGGRO                                   = 0,
    SAY_SPAWN                                   = 1,
    SAY_SLAY                                    = 2,
    SAY_DEATH                                   = 3,
    SAY_EXPLODE_GHOUL                           = 4,
    SAY_STEAL_FLESH                             = 5,
    SAY_SUMMON_GHOULS                           = 6
};

class boss_salramm : public CreatureScript
{
public:
    boss_salramm() : CreatureScript("boss_salramm") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetCullingOfStratholmeAI<boss_salrammAI>(creature);
    }

    struct boss_salrammAI : public ScriptedAI
    {
        boss_salrammAI(Creature* c) : ScriptedAI(c), summons(me)
        {
            Talk(SAY_SPAWN);
        }

        EventMap events;
        SummonList summons;
        void Reset() override
        {
            events.Reset();
            summons.DespawnAll();
        }

        void JustSummoned(Creature* cr) override { summons.Summon(cr); }

        void JustEngagedWith(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_SPELL_SHADOW_BOLT, 7000);
            events.ScheduleEvent(EVENT_SPELL_STEAL_FLESH, 11000);
            events.ScheduleEvent(EVENT_SPELL_SUMMON_GHOULS, 16000);
            events.ScheduleEvent(EVENT_EXPLODE_GHOUL, 22000);
            if (IsHeroic())
                events.ScheduleEvent(EVENT_SPELL_CURSE, 25000);
        }

        void JustDied(Unit* /*killer*/) override
        {
            summons.DespawnAll();
            Talk(SAY_DEATH);
        }

        void KilledUnit(Unit*  /*victim*/) override
        {
            if (!urand(0, 1))
                return;

            Talk(SAY_SLAY);
        }

        void ExplodeGhoul()
        {
            for (SummonList::const_iterator itr = summons.begin(); itr != summons.end(); ++itr)
                if (Creature* cr = ObjectAccessor::GetCreature(*me, (*itr)))
                    if (cr->IsAlive())
                    {
                        me->CastSpell(cr, DUNGEON_MODE(SPELL_EXPLODE_GHOUL_N, SPELL_EXPLODE_GHOUL_H), false);
                        return;
                    }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_SPELL_SHADOW_BOLT:
                    me->CastSpell(me->GetVictim(), DUNGEON_MODE(SPELL_SHADOW_BOLT_N, SPELL_SHADOW_BOLT_H), false);
                    events.RepeatEvent(10000);
                    break;
                case EVENT_SPELL_STEAL_FLESH:
                    if (!urand(0, 2))
                        Talk(SAY_STEAL_FLESH);
                    me->CastSpell(me->GetVictim(), SPELL_STEAL_FLESH_CHANNEL, false);
                    events.RepeatEvent(12000);
                    break;
                case EVENT_SPELL_SUMMON_GHOULS:
                    if (!urand(0, 2))
                        Talk(SAY_SUMMON_GHOULS);
                    me->CastSpell(me, SPELL_SUMMON_GHOULS, false);
                    events.RepeatEvent(10000);
                    break;
                case EVENT_EXPLODE_GHOUL:
                    if (!urand(0, 2))
                        Talk(SAY_EXPLODE_GHOUL);
                    ExplodeGhoul();
                    events.RepeatEvent(15000);
                    break;
                case EVENT_SPELL_CURSE:
                    me->CastSpell(me->GetVictim(), SPELL_CURSE_OF_TWISTED_FAITH, false);
                    events.RepeatEvent(30000);
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };
};

class spell_boss_salramm_steal_flesh : public SpellScriptLoader
{
public:
    spell_boss_salramm_steal_flesh() : SpellScriptLoader("spell_boss_salramm_steal_flesh") { }

    class spell_boss_salramm_steal_flesh_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_boss_salramm_steal_flesh_AuraScript);

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetUnitOwner();
            if (caster)
            {
                caster->CastSpell(caster, SPELL_STEAL_FLESH_CASTER, true);
                caster->CastSpell(target, SPELL_STEAL_FLESH_TARGET, true);
            }
        }

        void Register() override
        {
            AfterEffectRemove += AuraEffectRemoveFn(spell_boss_salramm_steal_flesh_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_boss_salramm_steal_flesh_AuraScript();
    }
};

void AddSC_boss_salramm()
{
    new boss_salramm();
    new spell_boss_salramm_steal_flesh();
}
