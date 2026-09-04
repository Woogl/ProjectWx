// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WxAIController.generated.h"

class AWxCharacterBase;
class UWxAIPerceptionComponent;

/**
 * AI 가 모는 폰 전부의 컨트롤러다 — 적이든 소환수든 팀을 가리지 않으며, 적대 여부는 빙의한 폰의 팀이 정한다.
 * Perception/감지 → Blackboard 동기화 책임은 UWxAIPerceptionComponent 에 위임한다.
 */
UCLASS()
class WXGAME_API AWxAIController : public AAIController
{
	GENERATED_BODY()

public:
	AWxAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	/** 시체는 어빌리티가 Ability.Death 에 막히고 이동은 래그돌이 막으므로, 트리를 계속 돌리면 실패할 브랜치만 매 틱 다시 고른다. */
	UFUNCTION()
	void HandlePawnDeath(AWxCharacterBase* DeadCharacter);

	void HandleAITargetChanged(AActor* NewTarget);
	APawn* ResolveMinionMaster(const APawn* InPawn) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|AI")
	TObjectPtr<UWxAIPerceptionComponent> WxAIPerceptionComponent;
};
