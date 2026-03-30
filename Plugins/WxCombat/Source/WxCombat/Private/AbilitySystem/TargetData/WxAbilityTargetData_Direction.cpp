// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/TargetData/WxAbilityTargetData_Direction.h"

UScriptStruct* FWxAbilityTargetData_Direction::GetScriptStruct() const
{
	return FWxAbilityTargetData_Direction::StaticStruct();
}

bool FWxAbilityTargetData_Direction::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Direction;
	bOutSuccess = true;
	return true;
}
