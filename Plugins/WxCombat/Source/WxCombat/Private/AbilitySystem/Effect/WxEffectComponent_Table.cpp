// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffectComponent_Table.h"
#include "AbilitySystem/Effect/WxEffectTableRow.h"
#include "GameplayEffect.h"

UWxEffectComponent_Table::UWxEffectComponent_Table()
{
}

FText UWxEffectComponent_Table::GetTitle() const
{
	const FWxEffectTableRow* Row = GetRow();
	return Row ? Row->Title : FText::GetEmpty();
}

FText UWxEffectComponent_Table::GetDescription() const
{
	const FWxEffectTableRow* Row = GetRow();
	return Row ? Row->Description : FText::GetEmpty();
}

TSoftObjectPtr<UObject> UWxEffectComponent_Table::GetIcon() const
{
	const FWxEffectTableRow* Row = GetRow();
	return Row ? Row->Icon : nullptr;
}

const FWxEffectTableRow* UWxEffectComponent_Table::GetRow() const
{
	if (EffectDataRow.IsNull())
	{
		return nullptr;
	}

	return EffectDataRow.GetRow<FWxEffectTableRow>(TEXT("WxEffectComponent_Data::GetRow"));
}

const FWxEffectTableRow* UWxEffectComponent_Table::FindRow(const UGameplayEffect* Def)
{
	const UWxEffectComponent_Table* DataComponent = Def ? Def->FindComponent<UWxEffectComponent_Table>() : nullptr;
	return DataComponent ? DataComponent->GetRow() : nullptr;
}

float UWxMMC_EffectMagnitude::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FWxEffectTableRow* Row = UWxEffectComponent_Table::FindRow(Spec.Def);
	return Row ? Row->Magnitude : 0.f;
}

float UWxMMC_EffectDuration::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FWxEffectTableRow* Row = UWxEffectComponent_Table::FindRow(Spec.Def);
	return Row ? Row->Duration : 0.f;
}
