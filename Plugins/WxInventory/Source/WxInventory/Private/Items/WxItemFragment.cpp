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

UWxItemFragment_Grade::UWxItemFragment_Grade()
{
	// 기본 등급(Common)의 기본 색으로 시드.
	// 등급 변경 시엔 PostEditChangeProperty 가 재시드한다.
	Color = GetDefaultColorForGrade(Grade);
}

FLinearColor UWxItemFragment_Grade::GetDefaultColorForGrade(EWxItemGrade Grade)
{
	// 등급별 기본 색 팔레트는 프로그래머만 여기서 정의한다(에디터 비노출).
	// 기획자는 Fragment 의 Color 로 오버라이드.
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

	// Grade 를 바꾸면 Color 를 해당 등급 기본값으로 재시드한다(이후 수동 변경은 유지된다).
	const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UWxItemFragment_Grade, Grade))
	{
		Color = GetDefaultColorForGrade(Grade);
	}
}
#endif
