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

#include "GameConfig.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "Player.h"
#include "WorldSession.h"

void WorldSession::HandleGrantLevel(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: CMSG_GRANT_LEVEL");

    ObjectGuid guid;
    recvData >> guid.ReadAsPacked();

    Player* target = ObjectAccessor::GetPlayer(*_player, guid);

    // check cheating
    uint8 levels = _player->GetGrantableLevels();
    uint8 error = 0;
    if (!target)
        error = ERR_REFER_A_FRIEND_NO_TARGET;
    else if (levels == 0)
        error = ERR_REFER_A_FRIEND_INSUFFICIENT_GRANTABLE_LEVELS;
    else if (GetRecruiterId() != target->GetSession()->GetAccountId())
        error = ERR_REFER_A_FRIEND_NOT_REFERRED_BY;
    else if (target->GetTeamId() != _player->GetTeamId())
        error = ERR_REFER_A_FRIEND_DIFFERENT_FACTION;
    else if (target->GetLevel() >= _player->GetLevel())
        error = ERR_REFER_A_FRIEND_TARGET_TOO_HIGH;
    else if (target->GetLevel() >= CONF_GET_INT("RecruitAFriend.MaxLevel"))
        error = ERR_REFER_A_FRIEND_GRANT_LEVEL_MAX_I;
    else if (target->GetGroup() != _player->GetGroup())
        error = ERR_REFER_A_FRIEND_NOT_IN_GROUP;

    if (error)
    {
        WorldPacket data(SMSG_REFER_A_FRIEND_FAILURE, 24);
        data << uint32(error);
        if (error == ERR_REFER_A_FRIEND_NOT_IN_GROUP)
            data << target->GetName();

        SendPacket(&data);
        return;
    }

    WorldPacket data2(SMSG_PROPOSE_LEVEL_GRANT, 8);
    data2 << _player->GetPackGUID();
    target->GetSession()->SendPacket(&data2);
}

void WorldSession::HandleAcceptGrantLevel(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: CMSG_ACCEPT_LEVEL_GRANT");

    ObjectGuid guid;
    recvData >> guid.ReadAsPacked();

    Player* other = ObjectAccessor::GetPlayer(*_player, guid);
    if (!other)
        return;

    if (GetAccountId() != other->GetSession()->GetRecruiterId())
        return;

    if (other->GetGrantableLevels())
        other->SetGrantableLevels(other->GetGrantableLevels() - 1);
    else
        return;

    _player->GiveLevel(_player->GetLevel() + 1);
}