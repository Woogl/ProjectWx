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
		Instance->SetCurrentCharges(MaxCharges);
	}
}

UWxItemFragment_Grade::UWxItemFragment_Grade()
{
	Color = GetDefaultColorForGrade(Grade);
}

FLinearColor UWxItemFragment_Grade::GetDefaultColorForGrade(EWxItemGrade Grade)
{
	switch (Grade)
	{
	case EWxItemGrade::Common:    return FLinearColor(0.7f, 0.7f, 0.7f);
	case EWxItemGrade::Rare:      return FLinearColor(0.2f, 0.45f, 1.0f);
	case EWxItemGrade::Epic:      return FLinearColor(0.6f, 0.25f, 0.9f);
	case EWxItemGrade::Legendary: return FLinearColor(1.0f, 0.55f, 0.1f);
	default:                      return FLinearColor::White;
	}
}

#if WITH_EDITOR
void UWxItemFragment_Grade::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UWxItemFragment_Grade, Grade))
	{
		Color = GetDefaultColorForGrade(Grade);
	}
}
#endif
