
#include "playerbot/playerbot.h"
#include "MoveToTravelTargetAction.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/ServerFacade.h"
#include "playerbot/LootObjectStack.h"
#include "MotionGenerators/PathFinder.h"
#include "playerbot/TravelMgr.h"
#include "playerbot/strategy/values/FreeMoveValues.h"
#include <iomanip>

using namespace ai;

namespace
{
    const char* TravelStatusName(TravelStatus status)
    {
        switch (status)
        {
            case TravelStatus::TRAVEL_STATUS_NONE: return "NONE";
            case TravelStatus::TRAVEL_STATUS_PREPARE: return "PREPARE";
            case TravelStatus::TRAVEL_STATUS_READY: return "READY";
            case TravelStatus::TRAVEL_STATUS_TRAVEL: return "TRAVEL";
            case TravelStatus::TRAVEL_STATUS_WORK: return "WORK";
            case TravelStatus::TRAVEL_STATUS_COOLDOWN: return "COOLDOWN";
            case TravelStatus::TRAVEL_STATUS_EXPIRED: return "EXPIRED";
        }

        return "UNKNOWN";
    }
}

bool MoveToTravelTargetAction::Execute(Event& event)
{
    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");

    if (target->GetStatus() == TravelStatus::TRAVEL_STATUS_READY)
    {
        ai->TellDebug(ai->GetMaster(), "The target is ready to travel start now.", "debug travel");
        if (sPlayerbotAIConfig.debugBotAI)
            sLog.outString("PBDBG travel move bot=%s guid=%u transition=READY->TRAVEL",
                bot->GetName(), bot->GetGUIDLow());
        target->SetStatus(TravelStatus::TRAVEL_STATUS_TRAVEL);
    }

    target->CheckStatus();

    if (target->GetStatus() != TravelStatus::TRAVEL_STATUS_TRAVEL)
    {
        if (sPlayerbotAIConfig.debugBotAI)
            sLog.outString("PBDBG travel move bot=%s guid=%u result=skip reason=status status=%s",
                bot->GetName(), bot->GetGUIDLow(), TravelStatusName(target->GetStatus()));
        return true;
    }

    WorldPosition botLocation(bot);
    WorldPosition location = *target->GetPosition();
    
    Group* group = bot->GetGroup();
    if (ai->IsGroupLeader() && !urand(0, 1) && !bot->IsInCombat())
    {        
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->getSource();
            if (member == bot)
                continue;

            if (!member->IsAlive())
                continue;

            if (!member->IsMoving())
                continue;

            if (member->GetPlayerbotAI() &&
                !(member->GetPlayerbotAI()->HasStrategy("follow", BotState::BOT_STATE_NON_COMBAT) || member->GetPlayerbotAI()->HasStrategy("wander", BotState::BOT_STATE_NON_COMBAT)))
                continue;

            WorldPosition memberPos(member);
            WorldPosition targetPos = *target->GetPosition();

            float memberDistance = std::min(botLocation.distance(memberPos), location.distance(memberPos));

            if (memberDistance < 50.0f)
                continue;
            if (memberDistance > sPlayerbotAIConfig.reactDistance * 20)
                continue;

           // float memberAngle = botLocation.getAngleBetween(targetPos, memberPos);

           // if (botLocation.getMapId() == targetPos.getMapId() && botLocation.getMapId() == memberPos.getMapId() && memberAngle < M_PI_F / 2) //We are heading that direction anyway.
           //     continue;

            if (!urand(0, 5))
            {
                std::ostringstream out;
                if ((ai->GetMaster() && !bot->GetGroup()->IsMember(ai->GetMaster()->GetObjectGuid())) || !ai->HasActivePlayerMaster())
                    out << "Waiting a bit for ";
                else
                    out << "Please hurry up ";

                out << member->GetName();

                if (bot->GetPlayerbotAI() && !ai->HasActivePlayerMaster())
                {
                    out << " who is " << round(memberDistance) << "y away";
                    if (!memberPos.getAreaName().empty())
                        out << " in " << memberPos.getAreaName();
                }

                ai->TellPlayerNoFacing(GetMaster(), out, PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, false);
            }

            // Introduce a random delay between 80% and 120% of maxWaitForMove to make waiting more natural
            uint32 randomDelay = sPlayerbotAIConfig.maxWaitForMove * (urand(80, 120) / 100.0f);
            target->SetExpireIn(target->GetTimeLeft() + randomDelay);

            SetDuration(randomDelay);

            // Occasionally face the member and perform an emote
            if (urand(0, 3) == 0) { // 25% chance to emote
                bot->SetFacingToObject(member);
                uint32 emoteChoice = urand(0, 2);
                switch (emoteChoice) {
                    case 0:
                        bot->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
                        break;
                    case 1:
                        bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                        break;
                    case 2:
                        bot->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
                        break;
                }
            }

            return true;
        }
    }

    float x = location.getX();
    float y = location.getY();
    float z = location.getZ();
    float mapId = location.getMapId();

    if (botLocation.getMapId() == location.getMapId() && botLocation.sqDistance2d(location) < 10000.0f)
    {
        float maxDistance = target->GetDestination()->GetRadiusMin();

        float angle = 2 * M_PI * urand(0, 100) / 100.0;
        float mod = urand(50, 100) / 100.0;

        x += cos(angle) * maxDistance * mod;
        y += sin(angle) * maxDistance * mod;

        if (ai->HasStrategy("debug move", BotState::BOT_STATE_NON_COMBAT))
        {
            std::ostringstream out;
            out << "Moving to ";
            out << target->GetDestination()->GetTitle();
            if (!(*target->GetPosition() == WorldPosition()))
            {
                out << " at " << uint32(target->GetPosition()->distance(bot)) << "y";
            }
            if (target->GetStatus() != TravelStatus::TRAVEL_STATUS_EXPIRED)
                out << " for " << (target->GetTimeLeft() / 1000) << "s";
            if (target->GetRetryCount(true))
                out << " (move retry: " << target->GetRetryCount(true) << ")";
            else if (target->GetRetryCount(false))
                out << " (retry: " << target->GetRetryCount(false) << ")";
            ai->TellPlayerNoFacing(GetMaster(), out);
        }
    }

    bool canMove = MoveTo(mapId, x, y, z, false, false);

    if (sPlayerbotAIConfig.debugBotAI)
    {
        WorldPosition* pos = target->GetPosition();
        sLog.outString("PBDBG travel move bot=%s guid=%u result=%s status=%s dest=%s targetMap=%u x=%.2f y=%.2f z=%.2f dist=%.1f moving=%u retries=%u forced=%u",
            bot->GetName(), bot->GetGUIDLow(), canMove ? "move-ok" : "move-fail", TravelStatusName(target->GetStatus()),
            target->GetDestination() ? target->GetDestination()->GetShortName().c_str() : "none",
            pos ? pos->getMapId() : 0, x, y, z, pos ? pos->distance(bot) : 0.0f,
#ifndef MANGOSBOT_ZERO
            bot->IsMovingIgnoreFlying(),
#else
            bot->IsMoving(),
#endif
            target->GetRetryCount(true), target->IsForced());
    }

    if (!canMove)
    {
        target->IncRetry(true);

        if (target->IsMaxRetry(true))
        {
            ai->TellDebug(ai->GetMaster(), "The target is cooling down because we failed to move to it a few times in a row.", "debug travel");
            target->SetStatus(TravelStatus::TRAVEL_STATUS_COOLDOWN);      
            target->SetForced(false);
        }
    }
    else
        target->DecRetry(true);

    if (ai->HasStrategy("debug move", BotState::BOT_STATE_NON_COMBAT))
    {
        WorldPosition* pos = target->GetPosition();
        GuidPosition* guidP = dynamic_cast<GuidPosition*>(pos);

        std::string name = (guidP && guidP->GetWorldObject(bot->GetInstanceId())) ? chat->formatWorldobject(guidP->GetWorldObject(bot->GetInstanceId())) : "travel target";

        if (mapId == bot->GetMapId())
        {
            ai->Poi(x, y, name);
        }
        else
        {
            LastMovement& lastMove = *context->GetValue<LastMovement&>("last movement");
            if (!lastMove.lastPath.empty() && lastMove.lastPath.getBack().distance(location) < 20.0f)
            {
                for (auto& p : lastMove.lastPath.getPointPath())
                {
                    if (p.getMapId() == bot->GetMapId())
                        ai->Poi(p.getX(), p.getY(), name);
                }
            }
        }
    }
     
    return canMove;
}

bool MoveToTravelTargetAction::isUseful()
{
    if (!ai->AllowActivity(TRAVEL_ACTIVITY))
    {
        if (sPlayerbotAIConfig.debugBotAI)
            sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=0 reason=activity",
                bot->GetName(), bot->GetGUIDLow());
        return false;
    }

    if (!AI_VALUE(bool, "travel target traveling") && AI_VALUE(TravelTarget*, "travel target")->GetStatus() != TravelStatus::TRAVEL_STATUS_READY)
    {
        if (sPlayerbotAIConfig.debugBotAI)
        {
            TravelTarget* travelTarget = AI_VALUE(TravelTarget*, "travel target");
            sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=0 reason=not-ready status=%s traveling=%u",
                bot->GetName(), bot->GetGUIDLow(), TravelStatusName(travelTarget->GetStatus()), AI_VALUE(bool, "travel target traveling"));
        }
        return false;
    }

    if (bot->IsTaxiFlying())
    {
        if (sPlayerbotAIConfig.debugBotAI)
            sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=0 reason=taxi",
                bot->GetName(), bot->GetGUIDLow());
        return false;
    }

    if (MEM_AI_VALUE(WorldPosition, "current position")->LastChangeDelay() < 10)
#ifndef MANGOSBOT_ZERO
        if (bot->IsMovingIgnoreFlying())
        {
            if (sPlayerbotAIConfig.debugBotAI)
                sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=0 reason=already-moving delay=%u",
                    bot->GetName(), bot->GetGUIDLow(), MEM_AI_VALUE(WorldPosition, "current position")->LastChangeDelay());
            return false;
        }
#else
        if (bot->IsMoving())
        {
            if (sPlayerbotAIConfig.debugBotAI)
                sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=0 reason=already-moving delay=%u",
                    bot->GetName(), bot->GetGUIDLow(), MEM_AI_VALUE(WorldPosition, "current position")->LastChangeDelay());
            return false;
        }
#endif

    if (!AI_VALUE(bool, "can move around"))
    {
        if (sPlayerbotAIConfig.debugBotAI)
            sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=0 reason=can-move",
                bot->GetName(), bot->GetGUIDLow());
        return false;
    }

    TravelTarget* travelTarget = AI_VALUE(TravelTarget*, "travel target");

    if (ai->HasStrategy("follow", BotState::BOT_STATE_NON_COMBAT) || ai->HasStrategy("wander", BotState::BOT_STATE_NON_COMBAT))
    {
        auto conditions = travelTarget->GetConditions();
        for (auto& cond : conditions)
        {
            if (cond == "should travel named::guild order")
            {
                if (sPlayerbotAIConfig.debugBotAI)
                    sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=0 reason=guild-order-follow",
                        bot->GetName(), bot->GetGUIDLow());
                return false;
            }
        }
    }

    if (bot->GetGroup() && !bot->GetGroup()->IsLeader(bot->GetObjectGuid()))
        if (ai->HasStrategy("follow", BotState::BOT_STATE_NON_COMBAT) ||
            ai->HasStrategy("stay", BotState::BOT_STATE_NON_COMBAT) ||
            ai->HasStrategy("guard", BotState::BOT_STATE_NON_COMBAT))
            if (!travelTarget->IsForced())
            {
                if (sPlayerbotAIConfig.debugBotAI)
                    sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=0 reason=group-follower",
                        bot->GetName(), bot->GetGUIDLow());
                return false;
            }

    WorldPosition travelPos(*travelTarget->GetPosition());

    if (travelPos.isDungeon() && bot->GetGroup() && bot->GetGroup()->IsLeader(bot->GetObjectGuid()) && sTravelMgr.MapTransDistance(bot, travelPos, true) < sPlayerbotAIConfig.sightDistance && !AI_VALUE2(bool, "group and", "near leader"))
    {
        if (sPlayerbotAIConfig.debugBotAI)
            sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=0 reason=dungeon-group-near",
                bot->GetName(), bot->GetGUIDLow());
        return false;
    }
     
    if (AI_VALUE(bool, "has available loot"))
    {
        LootObject lootObject = AI_VALUE(LootObjectStack*, "available loot")->GetLoot(sPlayerbotAIConfig.lootDistance);
        if (lootObject.IsLootPossible(bot))
        {
            if (sPlayerbotAIConfig.debugBotAI)
                sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=0 reason=loot",
                    bot->GetName(), bot->GetGUIDLow());
            return false;
        }
    }

    if (!travelTarget->IsForced())
        if (!CanFreeMoveValue::CanFreeMoveTo(ai, *travelTarget->GetPosition()))
        {
            if (sPlayerbotAIConfig.debugBotAI)
                sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=0 reason=free-move status=%s dest=%s",
                    bot->GetName(), bot->GetGUIDLow(), TravelStatusName(travelTarget->GetStatus()),
                    travelTarget->GetDestination() ? travelTarget->GetDestination()->GetShortName().c_str() : "none");
            return false;
        }

    if (sPlayerbotAIConfig.debugBotAI)
    {
        WorldPosition* pos = travelTarget->GetPosition();
        sLog.outString("PBDBG travel move useful bot=%s guid=%u useful=1 status=%s dest=%s map=%u dist=%.1f forced=%u",
            bot->GetName(), bot->GetGUIDLow(), TravelStatusName(travelTarget->GetStatus()),
            travelTarget->GetDestination() ? travelTarget->GetDestination()->GetShortName().c_str() : "none",
            pos ? pos->getMapId() : 0, pos ? pos->distance(bot) : 0.0f, travelTarget->IsForced());
    }

    return true;
}
