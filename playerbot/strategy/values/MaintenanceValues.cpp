
#include "MaintenanceValues.h"
#include "Mails/Mail.h"
#include "playerbot/strategy/values/GuildValues.h"

using namespace ai;

bool ShouldAHSellValue::Calculate() 
{
    if (ShouldSellValue::Calculate()) //We need space so we want to try to AH items anyway.
        return true;

    std::list<Item*> items = AI_VALUE2(std::list<Item*>, "inventory items", "inventory");

    for (auto& item : items)
    {
        if (!item->GetUInt32Value(ITEM_FIELD_DURABILITY)) //Does the item need to be repaired?
            continue;

        uint32 maxDurability = item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY);

        ItemPrototype const* ditemProto = item->GetProto();

        DurabilityCostsEntry const* dcost = sDurabilityCostsStore.LookupEntry(ditemProto->ItemLevel);
        if (!dcost)
            continue;

        uint32 dQualitymodEntryId = (ditemProto->Quality + 1) * 2;
        DurabilityQualityEntry const* dQualitymodEntry = sDurabilityQualityStore.LookupEntry(dQualitymodEntryId);
        if (!dQualitymodEntry)
            continue;

        uint32 dmultiplier = dcost->multiplier[ItemSubClassToDurabilityMultiplierId(ditemProto->Class, ditemProto->SubClass)];
        uint32 costs = uint32(maxDurability * dmultiplier * double(dQualitymodEntry->quality_mod));

        if (bot->GetMoney() && (costs * 100) / bot->GetMoney() <= 1) //Would repairing this item use more than 1% of our current gold?
            continue;

        if (!WorldPosition(bot).HasAreaFlag(AREA_FLAG_CAPITAL)) //We are not in a city so not easy to repair now.

        if (bot->GetMoney() && (costs * 100) / bot->GetMoney() <= 10) //Would repairing this item use more than 10% of our current gold?
                continue;

        ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", ItemQualifier(item).GetQualifier());

        if (usage != ItemUsage::ITEM_USAGE_AH && usage != ItemUsage::ITEM_USAGE_BROKEN_AH) //Do we want to AH this item?
            continue;

        return true; //We have an item that can be damaged (and gives significant repair cost) that we want to auction. We should auction it now!
    }

    return false;
}


bool CanGetMailValue::Calculate() {
    // Turtle mail is not exposed through PlayerMails; disable proactive mailbox maintenance for now.
    return false;
}

bool ShouldGetMailValue::Calculate() {
    // Turtle mail is not exposed through PlayerMails; disable proactive mailbox maintenance for now.
    return false;
}