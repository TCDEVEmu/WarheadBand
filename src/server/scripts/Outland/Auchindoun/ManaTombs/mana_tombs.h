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

#ifndef MANA_TOMBS_H_
#define MANA_TOMBS_H_

#include "CreatureAIImpl.h"

#define MTScriptName "instance_mana_tombs"
#define DataHeader "MT"

uint32 const EncounterCount = 4;

enum MTDataTypes
{
    // Encounter States/Boss GUIDs
    DATA_PANDEMONIUS            = 0,
    DATA_TAVAROK                = 1,
    DATA_NEXUSPRINCE_SHAFFAR    = 2,
    DATA_YOR                    = 3
};

template <class AI, class T>
inline AI* GetManaTombsAI(T* obj)
{
    return GetInstanceAI<AI>(obj, MTScriptName);
}

#define RegisterManaTombsCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetManaTombsAI)

#endif // MANA_TOMBS_H_
