// Copyright Woogle. All Rights Reserved.

#include "WxBTDecorator_MasterAbility.h"
#include "WxBTService_ObserveMasterAbility.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTDecorator_MasterAbility::UWxBTDecorator_MasterAbility()
{
	NodeName = TEXT("Master Ability");
	FlowAbortMode = EBTFlowAbortMode::LowerPriority;
	bAllowAbortNone = false;
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
}

bool UWxBTDecorator_MasterAbility::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UWxBTService_ObserveMasterAbility* Observer = UWxBTService_ObserveMasterAbility::Find(OwnerComp, this);
	return Observer && MasterAbilityTag.IsValid() && Observer->GetPendingAbility() == MasterAbilityTag;
}

void UWxBTDecorator_MasterAbility::OnNodeActivation(FBehaviorTreeSearchData& SearchData)
{
	Super::OnNodeActivation(SearchData);
	if (UWxBTService_ObserveMasterAbility* Observer = UWxBTService_ObserveMasterAbility::Find(SearchData.OwnerComp, this))
	{
		Observer->ConsumeAbility();
	}
}

FString UWxBTDecorator_MasterAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("Master가 %s 발동 시"), *MasterAbilityTag.ToString());
}
