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
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "violet_hold.h"

enum eSpells
{
    SPELL_RAY_OF_SUFFERING_N                = 54442,
    SPELL_RAY_OF_SUFFERING_H                = 59524,
    //SPELL_RAY_OF_SUFFERING_TRIGGERED      = 54417,

    SPELL_RAY_OF_PAIN_N                     = 54438,
    SPELL_RAY_OF_PAIN_H                     = 59523,
    //SPELL_RAY_OF_PAIN_TRIGGERED_N         = 54416,
    //SPELL_RAY_OF_PAIN_TRIGGERED_H         = 59525,

    SPELL_CORROSIVE_SALIVA                  = 54527,
    SPELL_OPTIC_LINK                        = 54396,
};

#define SPELL_RAY_OF_SUFFERING              DUNGEON_MODE(SPELL_RAY_OF_SUFFERING_N, SPELL_RAY_OF_SUFFERING_H)
#define SPELL_RAY_OF_PAIN                   DUNGEON_MODE(SPELL_RAY_OF_PAIN_N, SPELL_RAY_OF_PAIN_H)

enum eEvents
{
    EVENT_SPELL_CORROSIVE_SALIVA = 1,
    EVENT_SPELL_OPTIC_LINK,
};

class boss_moragg : public CreatureScript
{
public:
    boss_moragg() : CreatureScript("boss_moragg") { }

    CreatureAI* GetAI(Creature* pCreature) const override
    {
        return GetVioletHoldAI<boss_moraggAI>(pCreature);
    }

    struct boss_moraggAI : public ScriptedAI
    {
        boss_moraggAI(Creature* c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
        }

        InstanceScript* pInstance;
        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            DoZoneInCombat();
            me->CastSpell(me, SPELL_RAY_OF_SUFFERING, true);
            me->CastSpell(me, SPELL_RAY_OF_PAIN, true);
            events.Reset();
            events.RescheduleEvent(EVENT_SPELL_CORROSIVE_SALIVA, 4s, 6s);
            events.RescheduleEvent(EVENT_SPELL_OPTIC_LINK, 10s, 11s);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch(events.ExecuteEvent())
            {
                case 0:
                    break;
                case EVENT_SPELL_CORROSIVE_SALIVA:
                    me->CastSpell(me->GetVictim(), SPELL_CORROSIVE_SALIVA, false);
                    events.Repeat(8s, 10s);
                    break;
                case EVENT_SPELL_OPTIC_LINK:
                    if (Unit* target = SelectTarget(SelectTargetMethod::MinDistance, 0, 40.0f, true))
                    {
                        me->CastSpell(target, SPELL_OPTIC_LINK, false);
                        events.Repeat(18s, 21s);
                    }
                    else
                        events.Repeat(5s);
                    break;
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (pInstance)
                pInstance->SetData(DATA_BOSS_DIED, 0);
        }

        void MoveInLineOfSight(Unit* /*who*/) override {}

        void EnterEvadeMode(EvadeReason why) override
        {
            ScriptedAI::EnterEvadeMode(why);
            events.Reset();
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
            if (pInstance)
                pInstance->SetData(DATA_FAILED, 1);
        }
    };
};

class spell_optic_link : public SpellScriptLoader
{
public:
    spell_optic_link() : SpellScriptLoader("spell_optic_link") { }

    class spell_optic_linkAuraScript : public AuraScript
    {
        PrepareAuraScript(spell_optic_linkAuraScript)

        void HandleEffectPeriodic(AuraEffect const* aurEff)
        {
            if (Unit* target = GetTarget())
                if (Unit* caster = GetCaster())
                    if (GetAura() && GetAura()->GetEffect(0))
                        GetAura()->GetEffect(0)->SetAmount(aurEff->GetSpellInfo()->Effects[EFFECT_0].BasePoints + (((int32)target->GetExactDist(caster)) * 25) + (aurEff->GetTickNumber() * 100));
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_optic_linkAuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_optic_linkAuraScript();
    }
};

void AddSC_boss_moragg()
{
    new boss_moragg();
    new spell_optic_link();
}
