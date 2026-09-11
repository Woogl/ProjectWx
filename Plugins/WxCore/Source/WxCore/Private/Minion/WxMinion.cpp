// Copyright Woogle. All Rights Reserved.

#include "Minion/WxMinion.h"

int32 IWxMinion::GetMaxCountPerMaster_Implementation() const
{
	return 1;
}

bool IWxMinion::IsAggroIgnored_Implementation() const
{
	return false;
}
