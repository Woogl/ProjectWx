// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WxExperienceDefinition.generated.h"

class APawn;
class FDataValidationContext;
class UGameFeatureAction;
class UWxExperienceActionSet;

/**
 * 게임 모드 하나의 게임플레이 구성을 정의하는 프라이머리 데이터 에셋 (Lyra Experience 이식).
 * GameMode(서버 전용)가 선택해 GameState 의 Experience 매니저에 넘기면, 매니저가 참조를 복제해 서버·클라가 각자 로드 파이프라인을 주행한다.
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

	/**
	 * FrontEnd에서 전달한 선택 없이 직접 실행할 때의 기본 폰이다.
	 * 비우면 플레이어 폰 없이 도는 Experience(프론트엔드)다 — GameMode 의 SpectatorClass 로 빙의한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<APawn> DefaultPawnClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<FString> GameFeaturesToEnable;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Wx")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<TObjectPtr<UWxExperienceActionSet>> ActionSets;
};
