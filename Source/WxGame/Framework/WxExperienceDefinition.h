// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/WxRewardTableRow.h"
#include "WxExperienceDefinition.generated.h"

class FDataValidationContext;
class UGameFeatureAction;
class UWxExperienceActionSet;
class UWxPawnData;

/**
 * 게임 모드 하나의 게임플레이 구성을 정의하는 프라이머리 데이터 에셋 (Lyra Experience 이식).
 * GameMode(서버 전용)가 선택해 GameState 의 Experience 매니저에 넘기면, 매니저가 참조를 복제해 서버·클라가 각자
 * 에셋 번들 로드 → GameFeature 플러그인 활성 → 액션 실행의 로드 파이프라인을 주행한다.
 * 어느 사이드에 붙을지는 여기서 지정하지 않는다 — 복제 컴포넌트는 엔진이 authority 로 제한하고, 그 밖의 사이드 제한은 컴포넌트가 스스로 한다.
 *
 * 에셋은 네이티브 클래스 인스턴스로만 만든다 — BP 서브클래스 인스턴스는 PrimaryAssetType 이 달라져 스캔·URL 해석에서 빠진다.
 */
UCLASS()
class UWxExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** ?Experience= URL 옵션 해석 등에 쓰는 프라이머리 에셋 타입("WxExperienceDefinition"). */
	static FPrimaryAssetType GetPrimaryAssetTypeStatic();

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

	/** 이 Experience 의 플레이어 폰 구성. 미지정이면 GameMode 의 DefaultPawnClass 로 폴백한다(경고 로그). */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<const UWxPawnData> DefaultPawnData;

	/** 접속한 플레이어에게 1회 지급할 시작 아이템. 액션셋의 목록과 합산되며, 빈(아이템 미지정) 항목은 무시된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<FWxItemRewardEntry> DefaultInventoryItems;

	/** 로드 완료 시 활성화할 GameFeature 플러그인 이름 목록. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<FString> GameFeaturesToEnable;

	/** 이 Experience 전용 액션. 여러 Experience 가 공유하는 묶음은 ActionSets 로 합성한다. */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Wx")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	/** 합성할 공용 액션 묶음. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<TObjectPtr<UWxExperienceActionSet>> ActionSets;
};
