// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"

#include "WxGameFeatureAction_AddComponents.generated.h"

class FDataValidationContext;
class UGameFrameworkComponent;
class UGameInstance;
struct FComponentRequestHandle;
struct FWorldContext;

/** 주입 1건: 붙일 프레임워크 컴포넌트 클래스. 사이드 플래그는 두지 않는다. */
USTRUCT()
struct FWxGameFeatureComponentEntry
{
	GENERATED_BODY()

	/**
	 * 대상 액터는 이 클래스가 상속한 ModularGameplay 베이스(Pawn·Controller·PlayerState·GameState 컴포넌트)에서 도출한다.
	 * 대상은 ModularGameplay receiver 로 opt-in 돼 있어야 한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	TSoftClassPtr<UGameFrameworkComponent> ComponentClass;
};

/**
 * 스톡 AddComponents 를 대체하는, 사이드 플래그 없는 컴포넌트 주입 액션 (스톡은 final 이라 신설).
 * 대상 액터도 엔트리가 지정하지 않는다 — 컴포넌트 클래스가 상속한 프레임워크 컴포넌트 베이스가 곧 대상 선언이므로 거기서 도출한다.
 * 넷모드와 무관하게 전 엔트리를 컴포넌트 매니저에 요청만 한다 — 복제 컴포넌트는 매니저가 authority 액터에서만 생성하고,
 * 비복제 컴포넌트의 사이드 제한은 컴포넌트 자신이 한다(권위 전용은 HasAuthority 가드, 로컬 표시 전용은 IsLocalController 가드).
 * 요청이 넷모드에 의존하지 않으므로 스톡의 월드 넷모드 변화 추적은 두지 않는다.
 */
UCLASS(meta = (DisplayName = "Add Components"))
class UWxGameFeatureAction_AddComponents : public UGameFeatureAction
{
	GENERATED_BODY()

public:
	//~ Begin UGameFeatureAction
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
#if WITH_EDITORONLY_DATA
	virtual void AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData) override;
#endif
	//~ End UGameFeatureAction

#if WITH_EDITOR
	//~ Begin UObject
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	//~ End UObject
#endif

	UPROPERTY(EditAnywhere, meta = (TitleProperty = "{ComponentClass}"))
	TArray<FWxGameFeatureComponentEntry> ComponentList;

private:
	/** 활성 컨텍스트 1건이 잡아 둔 델리게이트와 요청 핸들. 핸들이 해제되면 매니저가 만든 컴포넌트를 회수한다. */
	struct FContextHandles
	{
		FDelegateHandle GameInstanceStartHandle;
		TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequestHandles;
	};

	/** 월드의 컴포넌트 매니저에 전 엔트리를 요청하고 핸들을 보관한다. */
	void AddToWorld(const FWorldContext& WorldContext, FContextHandles& Handles);

	/** 활성 이후 시작되는 게임 인스턴스 콜백: 컨텍스트가 적용되는 월드면 거기에도 요청한다. */
	void HandleGameInstanceStart(UGameInstance* GameInstance, FGameFeatureStateChangeContext ChangeContext);

	TMap<FGameFeatureStateChangeContext, FContextHandles> ContextHandles;
};
