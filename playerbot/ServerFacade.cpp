
#include "playerbot/playerbot.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/ServerFacade.h"

#include "Database/DatabaseEnv.h"
#include "PlayerbotAI.h"

#include "Movement/TargetedMovementGenerator.h"

ServerFacade::ServerFacade() {}
ServerFacade::~ServerFacade() {}

float ServerFacade::GetDistance2d(Unit *unit, WorldObject* wo)
{
    if (!unit || !wo)
        return false;

    return round(unit->GetDistance2dToCenter(wo) * 10.0f) / 10.0f;
}

float ServerFacade::GetDistance2d(Unit *unit, float x, float y)
{
    return round(unit->GetDistance2dToCenter(x, y) * 10.0f) / 10.0f;
}

bool ServerFacade::IsDistanceLessThan(float dist1, float dist2)
{
    return dist1 - dist2 < sPlayerbotAIConfig.targetPosRecalcDistance;
}

bool ServerFacade::IsDistanceGreaterThan(float dist1, float dist2)
{
    return dist1 - dist2 > sPlayerbotAIConfig.targetPosRecalcDistance;
}

bool ServerFacade::IsDistanceGreaterOrEqualThan(float dist1, float dist2)
{
    return !IsDistanceLessThan(dist1, dist2);
}

bool ServerFacade::IsDistanceLessOrEqualThan(float dist1, float dist2)
{
    return !IsDistanceGreaterThan(dist1, dist2);
}

void ServerFacade::SetFacingTo(Unit* unit, float angle, bool force)
{
    MotionMaster &mm = *unit->GetMotionMaster();
    if (!force && !unit->IsStopped()) unit->SetFacingTo(angle);
    else
    {
        unit->SetOrientation(angle);
        unit->SendHeartBeat();
    }
    //unit->m_movementInfo.RemoveMovementFlag(MovementFlags(MOVEFLAG_SPLINE_ENABLED | MOVEFLAG_FORWARD));
}

bool ServerFacade::IsFriendlyTo(Unit* bot, Unit* to)
{
#ifdef MANGOS
    return bot->IsFriendlyTo(to);
#endif
#ifdef CMANGOS
    return bot->IsFriend(to);
#endif
}

bool ServerFacade::IsHostileTo(Unit* bot, Unit* to)
{
#ifdef MANGOS
    return bot->IsHostileTo(to);
#endif
#ifdef CMANGOS
    return bot->IsEnemy(to);
#endif
}

bool ServerFacade::IsFriendlyTo(WorldObject* bot, Unit* to)
{
#ifdef MANGOS
    return bot->IsFriendlyTo(to);
#endif
#ifdef CMANGOS
    return bot->IsFriend(to);
#endif
}

bool ServerFacade::IsHostileTo(WorldObject* bot, Unit* to)
{
#ifdef MANGOS
    return bot->IsHostileTo(to);
#endif
#ifdef CMANGOS
    return bot->IsEnemy(to);
#endif
}


bool ServerFacade::IsSpellReady(Player* bot, uint32 spell, uint32 itemId)
{
#ifdef MANGOS
    return !bot->HasSpellCooldown(spell);
#endif
#ifdef CMANGOS
    if (itemId)
    {
        const ItemPrototype* proto = sObjectMgr.GetItemPrototype(itemId);
        return bot->IsSpellReady(spell, proto);
    }
    else
        return bot->IsSpellReady(spell);
#endif
}



bool ServerFacade::IsUnderwater(Unit *unit)
{
    return unit->IsUnderwater();
}

FactionTemplateEntry const* ServerFacade::GetFactionTemplateEntry(Unit *unit)
{
    return unit->GetFactionTemplateEntry();
}

Unit* ServerFacade::GetChaseTarget(Unit* target)
{
    return static_cast<ChaseMovementGenerator<Player> const*>(target->GetMotionMaster()->GetCurrent())->GetTarget();
}

float ServerFacade::GetChaseAngle(Unit* target)
{
    Unit* chaseTarget = GetChaseTarget(target);
    if (!chaseTarget)
        return 0.0f;

    float angle = target->GetAngle(chaseTarget) - chaseTarget->GetOrientation();
    while (angle < 0.0f)
        angle += 2.0f * M_PI_F;
    while (angle >= 2.0f * M_PI_F)
        angle -= 2.0f * M_PI_F;
    return angle;
}

float ServerFacade::GetChaseOffset(Unit* target)
{
    Unit* chaseTarget = GetChaseTarget(target);
    return chaseTarget ? target->GetDistance2dToCenter(chaseTarget) : 0.0f;
}

bool ServerFacade::isMoving(Unit *unit)
{
#ifdef MANGOS
    return unit->m_movementInfo.HasMovementFlag(movementFlagsMask);
#endif
#ifdef CMANGOS
#ifdef MANGOSBOT_ONE
    return !unit->IsStopped() || unit->IsFalling() || unit->IsJumping();
#else
    return !unit->IsStopped() || unit->IsFalling();
#endif
#endif
}
