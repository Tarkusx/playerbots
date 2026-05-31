
#include "playerbot/playerbot.h"
#include "DuelTargetValue.h"

using namespace ai;

Unit* DuelTargetValue::Calculate()
{
    return (bot->m_duel && bot->m_duel->opponent) ? ObjectAccessor::FindPlayer(bot->m_duel->opponent) : nullptr;
}
