// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "WxAIController.generated.h"

class AWxCharacterBase;
class UBlackboardComponent;
class UWxAIPerceptionComponent;

/**
 * AI 가 모는 폰 전부의 컨트롤러다 — 적이든 소환수든 팀을 가리지 않으며, 적대 여부는 빙의한 폰의 팀이 정한다.
 * 감지는 UWxAIPerceptionComponent 가, 그중 누구를 적으로 삼을지는 BT 서비스가 정한다.
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

	/** BT 서비스가 고른 타겟을 락온 대상으로 옮긴다. WxCombat 을 아는 쪽이 컨트롤러뿐이라 이 통로는 여기 남는다. */
	EBlackboardNotificationResult HandleTargetActorChanged(const UBlackboardComponent& InBlackboard, FBlackboard::FKey KeyID);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|AI")
	TObjectPtr<UWxAIPerceptionComponent> WxAIPerceptionComponent;
};
