// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffectComponent_Table.h"
#include "AbilitySystem/Effect/WxEffectTableRow.h"
#include "GameplayEffect.h"

UWxEffectComponent_Table::UWxEffectComponent_Table()
{
#if WITH_EDITORONLY_DATA
	EditorFriendlyName = TEXT("Wx Effect Data");
#endif
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
