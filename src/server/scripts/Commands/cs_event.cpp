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
 Name: event_commandscript
 %Complete: 100
 Comment: All event related commands
 Category: commandscripts
 EndScriptData */

#include "Chat.h"
#include "GameEventMgr.h"
#include "GameTime.h"
#include "Player.h"
#include "ScriptObject.h"
#include "Timer.h"

using namespace Warhead::ChatCommands;

using EventEntry = Variant<Hyperlink<gameevent>, uint16>;

class event_commandscript : public CommandScript
{
public:
    event_commandscript() : CommandScript("event_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable eventCommandTable =
        {
            { "activelist", HandleEventActiveListCommand, SEC_GAMEMASTER, Console::Yes },
            { "start",      HandleEventStartCommand,      SEC_GAMEMASTER, Console::Yes },
            { "stop",       HandleEventStopCommand,       SEC_GAMEMASTER, Console::Yes },
            { "info",       HandleEventInfoCommand,       SEC_GAMEMASTER, Console::Yes }
        };
        static ChatCommandTable commandTable =
        {
            { "event", eventCommandTable }
        };
        return commandTable;
    }

    static bool HandleEventActiveListCommand(ChatHandler* handler)
    {
        uint32 counter = 0;

        GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();
        GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr->GetActiveEventList();

        std::string active = handler->GetWarheadString(LANG_ACTIVE);

        for (uint16 eventId : activeEvents)
        {
            GameEventData const& eventData = events[eventId];

            if (handler->GetSession())
            {
                handler->PSendSysMessage(LANG_EVENT_ENTRY_LIST_CHAT, eventId, eventId, eventData.description, active);
            }
            else
            {
                handler->PSendSysMessage(LANG_EVENT_ENTRY_LIST_CONSOLE, eventId, eventData.description, active);
            }

            ++counter;
        }

        if (counter == 0)
        {
            handler->SendSysMessage(LANG_NOEVENTFOUND);
        }

        handler->SetSentErrorMessage(true);

        return true;
    }

    static bool HandleEventInfoCommand(ChatHandler* handler, EventEntry const eventId)
    {
        GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();

        if (std::size_t(eventId) >= events.size())
        {
            handler->SendSysMessage(LANG_EVENT_NOT_EXIST);
            handler->SetSentErrorMessage(true);
            return false;
        }

        GameEventData const& eventData = events[eventId];
        if (!eventData.isValid())
        {
            handler->SendSysMessage(LANG_EVENT_NOT_EXIST);
            handler->SetSentErrorMessage(true);
            return false;
        }

        GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr->GetActiveEventList();
        bool active = activeEvents.find(eventId) != activeEvents.end();
        std::string activeStr = active ? handler->GetWarheadString(LANG_ACTIVE) : "";

        std::string startTimeStr = Warhead::Time::TimeToTimestampStr(Seconds(eventData.start));
        std::string endTimeStr = Warhead::Time::TimeToTimestampStr(Seconds(eventData.end));

        uint32 delay = sGameEventMgr->NextCheck(eventId);
        time_t nextTime = GameTime::GetGameTime().count() + delay;
        std::string nextStr = nextTime >= eventData.start && nextTime < eventData.end ? Warhead::Time::TimeToTimestampStr(Seconds(nextTime)) : "-";

        std::string occurenceStr = Warhead::Time::ToTimeString(Minutes(eventData.occurence));
        std::string lengthStr = Warhead::Time::ToTimeString(Minutes(eventData.length));

        handler->PSendSysMessage(LANG_EVENT_INFO, uint16(eventId), eventData.description, activeStr,
            startTimeStr, endTimeStr, occurenceStr, lengthStr,
            nextStr);

        return true;
    }

    static bool HandleEventStartCommand(ChatHandler* handler, EventEntry const eventId)
    {
        GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();

        if (*eventId < 1 || *eventId >= events.size())
        {
            handler->SendSysMessage(LANG_EVENT_NOT_EXIST);
            handler->SetSentErrorMessage(true);
            return false;
        }

        GameEventData const& eventData = events[eventId];
        if (!eventData.isValid())
        {
            handler->SendSysMessage(LANG_EVENT_NOT_EXIST);
            handler->SetSentErrorMessage(true);
            return false;
        }

        GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr->GetActiveEventList();
        if (activeEvents.find(eventId) != activeEvents.end())
        {
            handler->PSendSysMessage(LANG_EVENT_ALREADY_ACTIVE, uint16(eventId), eventData.description.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage(LANG_EVENT_STARTED, uint16(eventId), eventData.description.c_str());
        sGameEventMgr->StartEvent(eventId, true);
        return true;
    }

    static bool HandleEventStopCommand(ChatHandler* handler, EventEntry const eventId)
    {
        GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();

        if (*eventId < 1 || *eventId >= events.size())
        {
            handler->SendSysMessage(LANG_EVENT_NOT_EXIST);
            handler->SetSentErrorMessage(true);
            return false;
        }

        GameEventData const& eventData = events[eventId];
        if (!eventData.isValid())
        {
            handler->SendSysMessage(LANG_EVENT_NOT_EXIST);
            handler->SetSentErrorMessage(true);
            return false;
        }

        GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr->GetActiveEventList();

        if (activeEvents.find(eventId) == activeEvents.end())
        {
            handler->PSendSysMessage(LANG_EVENT_NOT_ACTIVE, uint16(eventId), eventData.description.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage(LANG_EVENT_STOPPED, uint16(eventId), eventData.description.c_str());
        sGameEventMgr->StopEvent(eventId, true);
        return true;
    }
};

void AddSC_event_commandscript()
{
    new event_commandscript();
}