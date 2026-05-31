
#include "playerbot/playerbot.h"
#include "KarazhanDungeonTriggers.h"
#include "GenericTriggers.h"
#include "Maps/GridNotifiers.h"
#include "Maps/GridNotifiersImpl.h"
#include "Maps/CellImpl.h"
#include "Maps/GridSearchers.h"

using namespace ai;

bool NetherspiteBeamsCheatNeedRefreshTrigger::IsActive()
{
    //Checking that is portal phase
    std::list<Creature*> creatures;
    GetCreatureListWithEntryInGrid(creatures, bot, 17369, 100);

    if (creatures.empty())
        return false;

    //Checking that is Netherspite target
    return AI_VALUE2(bool, "has aggro", "current target");
}