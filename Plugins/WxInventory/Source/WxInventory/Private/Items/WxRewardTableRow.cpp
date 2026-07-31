// Copyright Woogle. All Rights Reserved.

#include "Items/WxRewardTableRow.h"

bool FWxItemRewardEntry::IsValid() const
{
	return !Item.IsNull() && Quantity > 0;
}

void FWxRewardTableRow::GetValidRewards(TArray<FWxItemRewardEntry>& OutRewards) const
{
	OutRewards.Reset();
	if (Reward1.IsValid()) { OutRewards.Add(Reward1); }
	if (Reward2.IsValid()) { OutRewards.Add(Reward2); }
	if (Reward3.IsValid()) { OutRewards.Add(Reward3); }
	if (Reward4.IsValid()) { OutRewards.Add(Reward4); }
	if (Reward5.IsValid()) { OutRewards.Add(Reward5); }
}
