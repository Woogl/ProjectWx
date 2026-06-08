// Copyright Woogle. All Rights Reserved.

#include "Items/WxItemFragment.h"

#include "Items/WxItemInstance.h"

void UWxItemFragment::OnInstanceCreated(UWxItemInstance* Instance) const
{
}

void UWxItemFragment_Charges::OnInstanceCreated(UWxItemInstance* Instance) const
{
	Super::OnInstanceCreated(Instance);

	if (Instance)
	{
		// 인스턴스를 충전량 만땅으로 시작시킨다.
		Instance->SetCurrentCharges(MaxCharges);
	}
}
