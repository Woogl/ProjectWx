// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "WxGameState.generated.h"

class UWxExperienceDefinition;
struct FComponentRequestHandle;

/**
 * 프로젝트 공용 GameState.
 *
 * ModularGameplay 컴포넌트 receiver 이자 Experience 적용 지점이다.
 * GameMode(서버 전용)가 고른 Experience 참조를 복제해, 서버는 직접 호출·클라는 OnRep 으로 각자 자기 사이드의
 * 프레임워크 컴포넌트를 컴포넌트 매니저에 등록한다. 어떤 컴포넌트가 붙는지는 여전히 에셋만 안다.
 */
UCLASS()
class WXGAME_API AWxGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	//~ Begin AActor
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AActor

	/**
	 * 서버 전용. Experience 를 설정하고 서버 사이드 적용을 즉시 수행한다.
	 * 이미 설정돼 있으면 무시한다 — 엔진 Reset() 이 InitGameState 를 재호출하는 경로에 대한 멱등 처리.
	 */
	void SetCurrentExperience(const UWxExperienceDefinition* Experience);

private:
	UFUNCTION()
	void OnRep_CurrentExperience();

	/** 넷모드로 사이드를 판정해 자기 사이드 엔트리를 컴포넌트 매니저에 등록한다. 서버·클라 공통 경로. */
	void ApplyExperience();

	/** 이 판의 프레임워크 구성. 서버가 InitGameState 에서 설정하고 클라는 복제로 받는다. */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentExperience)
	TObjectPtr<const UWxExperienceDefinition> CurrentExperience;

	/** 주입 요청 핸들. 살아있는 동안 컴포넌트가 유지되므로 맵 수명(= GameState 수명) 동안 보유한다. */
	TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequestHandles;
};
