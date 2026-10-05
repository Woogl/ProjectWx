// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WxAIController.generated.h"

class AWxCharacterBase;
class UStateTreeAIComponent;
struct FGameplayTag;

/**
 * AI 가 모는 폰 전부의 컨트롤러다 — 적이든 소환수든 팀을 가리지 않으며, 적대 여부는 빙의한 폰의 팀이 정한다.
 * 감각의 모양(피아 필터·자극 수명·지배 감각)은 여기서 잡고, 감지 거리·각도는 폰의 UWxAIBehaviorComponent 가 빙의 시점에 밀어 넣는다.
 * 폰의 UWxAIBehaviorComponent 가 지정한 State Tree 를 돌린다. 감지한 것 중 누구를 적으로 삼을지는 그 트리의 평가자가 정하고, 정한 대상은 여기에 맡긴다.
 * 트리의 정지·잠금은 이 컨트롤러만 한다 — 엔진의 일시정지는 단일 플래그라 여러 곳이 걸면 서로의 정지를 풀어 버린다.
 */
UCLASS()
class WXGAME_API AWxAIController : public AAIController
{
	GENERATED_BODY()

public:
	AWxAIController();

	//~ Begin IGenericTeamAgentInterface
	/** 시야 센스가 퍼셉션 오너인 컨트롤러에게 묻는 피아 판정을 폰의 규칙에 맡긴다. 청각은 이 함수를 거치지 않고 팀 ID 솔버로 판정한다. */
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~ End IGenericTeamAgentInterface

	AActor* GetTargetActor() const;

	/**
	 * 겨누는 대상은 서버 권위로 복제돼야 스냅 워프·타겟팅 필터·발사체가 전 머신에서 같은 답을 읽으므로, 폰의 락온 컴포넌트에 함께 싣는다.
	 * 무는 대상은 복제되는 폰이라는 전제다. 루트가 런타임 생성 비복제 컴포넌트인 액터를 물면 원격에는 null 로 도착한다.
	 */
	void SetTargetActor(AActor* NewTargetActor);

	/** 빙의한 순간 폰이 서 있던 자리. 리시의 기준점이다. */
	FVector GetHomeLocation() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	/** 시체는 어빌리티가 Ability.Death 에 막히고 이동은 래그돌이 막으므로, 트리를 계속 돌리면 실패할 브랜치만 매 틱 다시 고른다. */
	UFUNCTION()
	void HandlePawnDeath(AWxCharacterBase* DeadCharacter);

	/** 그로기는 트리를 멈추지 않고 Reaction 우선순위로 잠가, 풀리면 멈춘 자리에서 잇는다. 엔진 AI 태스크가 Logic 잠금을 풀어도 그로기 중에는 재개되지 않는다. */
	void HandleGroggyTagChanged(const FGameplayTag Tag, int32 NewCount);

	UPROPERTY(VisibleAnywhere, Category = "Wx|AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	/** State Tree 가 컨텍스트 바인딩으로 읽는다. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Wx|AI")
	TObjectPtr<AActor> TargetActor;

	FVector HomeLocation = FVector::ZeroVector;
};
