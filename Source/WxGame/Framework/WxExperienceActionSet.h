// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/WxRewardTableRow.h"
#include "WxExperienceActionSet.generated.h"

class FDataValidationContext;
class UGameFeatureAction;

/**
 * 여러 Experience 가 공유하는 액션·GameFeature 묶음.
 * Experience 정의의 ActionSets 로 합성되며, 로드 파이프라인에서 본체 액션과 같은 방식으로 실행된다.
 * 에셋은 네이티브 클래스 인스턴스로만 만든다 — Experience 정의와 같은 스캔 정책.
 */
UCLASS()
class UWxExperienceActionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	//~ Begin UObject
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	//~ End UObject
#endif

#if WITH_EDITORONLY_DATA
	//~ Begin UPrimaryDataAsset
	virtual void UpdateAssetBundleData() override;
	//~ End UPrimaryDataAsset
#endif

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Wx")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<FString> GameFeaturesToEnable;

	/** Experience 가 참조한 전체 묶음의 목록이 합산된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<FWxItemRewardEntry> DefaultInventoryItems;
};
