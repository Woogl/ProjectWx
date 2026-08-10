// Copyright Woogle. All Rights Reserved.


#include "Widget/WxActionWidget.h"

#include "CommonInputBaseTypes.h"
#include "CommonInputTypeEnum.h"
#include "CommonUITypes.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#endif

FSlateBrush UWxActionWidget::GetIcon() const
{
#if WITH_EDITOR
	// 디자인 타임엔 엔진이 DesignTimeKey만 읽으므로, 여기서 액션으로부터 직접 아이콘을 해석한다.
	if (IsDesignTime())
	{
		ECommonInputType InputType;
		FName GamepadName;
		FCommonInputBase::GetCurrentPlatformDefaults(InputType, GamepadName);

		// EI 경로: 키는 IA가 아니라 IMC에 있고 디자인타임엔 서브시스템이 없어 IMC 애셋을 직접 훑는다.
		if (const UInputAction* EnhancedAction = GetEnhancedInputAction())
		{
			const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

			TArray<FAssetData> MappingContextAssets;
			AssetRegistry.GetAssetsByClass(UInputMappingContext::StaticClass()->GetClassPathName(), MappingContextAssets);

			for (const FAssetData& MappingContextAsset : MappingContextAssets)
			{
				const UInputMappingContext* MappingContext = Cast<UInputMappingContext>(MappingContextAsset.GetAsset());
				if (!MappingContext)
				{
					continue;
				}

				for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
				{
					if (Mapping.Action != EnhancedAction || !CommonUI::IsKeyValidForInputType(Mapping.Key, InputType))
					{
						continue;
					}

					FSlateBrush Brush;
					if (UCommonInputPlatformSettings::Get()->TryGetInputBrush(Brush, Mapping.Key, InputType, GamepadName))
					{
						return Brush;
					}
				}
			}
		}
		// DataTable 경로.
		else if (const FCommonInputActionDataBase* InputActionData = GetInputActionData())
		{
			const FKey Key = InputActionData->GetInputTypeInfo(InputType, GamepadName).GetKey();

			FSlateBrush Brush;
			if (UCommonInputPlatformSettings::Get()->TryGetInputBrush(Brush, Key, InputType, GamepadName))
			{
				return Brush;
			}
		}
	}
#endif

	return Super::GetIcon();
}
