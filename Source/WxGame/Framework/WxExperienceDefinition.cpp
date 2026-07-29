// Copyright Woogle. All Rights Reserved.

#include "Framework/WxExperienceDefinition.h"

#include "GameFeatureAction.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FPrimaryAssetType UWxExperienceDefinition::GetPrimaryAssetTypeStatic()
{
	return FPrimaryAssetType(StaticClass()->GetFName());
}

#if WITH_EDITOR
EDataValidationResult UWxExperienceDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	for (int32 Index = 0; Index < Actions.Num(); ++Index)
	{
		if (!Actions[Index])
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::FromString(FString::Printf(TEXT("Actions[%d] 항목이 비어 있습니다."), Index)));
		}
	}

	for (int32 Index = 0; Index < ActionSets.Num(); ++Index)
	{
		if (!ActionSets[Index])
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::FromString(FString::Printf(TEXT("ActionSets[%d] 항목이 비어 있습니다."), Index)));
		}
	}

	return Result;
}
#endif

#if WITH_EDITORONLY_DATA
void UWxExperienceDefinition::UpdateAssetBundleData()
{
	Super::UpdateAssetBundleData();

	// 액션의 소프트 참조(주입 컴포넌트 클래스 등)를 번들 데이터에 실어 비동기 프리로드와 쿠킹 의존성을 성립시킨다.
	for (UGameFeatureAction* Action : Actions)
	{
		if (Action)
		{
			Action->AddAdditionalAssetBundleData(AssetBundleData);
		}
	}
}
#endif
