// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/TargetData/WxAbilityTargetData_Interaction.h"
#include "Interaction/WxInteractionComponent.h"

UScriptStruct* FWxAbilityTargetData_Interaction::GetScriptStruct() const
{
	return FWxAbilityTargetData_Interaction::StaticStruct();
}

bool FWxAbilityTargetData_Interaction::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	UObject* Obj = Component;
	Map->SerializeObject(Ar, UWxInteractionComponent::StaticClass(), Obj);
	Component = Cast<UWxInteractionComponent>(Obj);
	bOutSuccess = true;
	return true;
}
