// Copyright Woogle. All Rights Reserved.

#include "WxAIController.h"
#include "WxBlackboardKeys.h"
#include "WxAIPerceptionComponent.h"
#include "Character/WxCharacterBase.h"
#include "Character/Component/WxAIBehaviorComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Targeting/WxLockOnComponent.h"

AWxAIController::AWxAIController()
{
	WxAIPerceptionComponent = CreateDefaultSubobject<UWxAIPerceptionComponent>(TEXT("WxAIPerceptionComponent"));
	WxAIPerceptionComponent->OnTargetChanged.AddUObject(this, &ThisClass::HandleAITargetChanged);
}

void AWxAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (const IGenericTeamAgentInterface* PawnTeam = Cast<IGenericTeamAgentInterface>(InPawn))
	{
		SetGenericTeamId(PawnTeam->GetGenericTeamId());

		// 퍼셉션 리스너는 빙의 전에 등록되면서 그 자리에서 컨트롤러 팀을 캐시하는데, 엔진은 팀이 바뀌어도 퍼셉션에 통보하지 않는다.
		// 청각·촉각의 피아 판정이 그 캐시를 쓰므로, 여기서 갱신하지 않으면 무팀으로 남아 자기 발소리와 아군 소음까지 적대로 듣는다.
		WxAIPerceptionComponent->RequestStimuliListenerUpdate();
	}

	// Blackboard 컴포넌트는 RunBehaviorTree 안에서 생성되므로, BT 를 먼저 실행한 뒤에 컨텍스트 키를 세팅한다.
	if (AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(InPawn))
	{
		WxCharacter->OnDeath.AddDynamic(this, &AWxAIController::HandlePawnDeath);

		// 재사용된 폰도 새 빙의에서는 대상 없이 시작한다.
		WxCharacter->GetLockOnComponent()->SetLockOnTarget(nullptr);

		const UWxAIBehaviorComponent* AIBehaviorComponent = WxCharacter->FindComponentByClass<UWxAIBehaviorComponent>();
		if (UBehaviorTree* BT = AIBehaviorComponent ? AIBehaviorComponent->GetBehaviorTree() : nullptr)
		{
			RunBehaviorTree(BT);
		}
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		WxBlackboardKeys::SetSelfActor(BB, InPawn);
		WxBlackboardKeys::SetHomeLocation(BB, InPawn->GetActorLocation());

		// 소환자는 Deferred Spawn 시 Instigator로 지정되어 빙의보다 먼저 사용할 수 있다.
		// 그런 폰의 블랙보드에는 Master 키가 없으니 쓰지 않는다 — 쓰면 키를 못 찾았다는 경고만 남는다.
		if (APawn* Master = ResolveMinionMaster(InPawn))
		{
			WxBlackboardKeys::SetMaster(BB, Master);
		}
	}
}

void AWxAIController::OnUnPossess()
{
	// 엔진이 Super 에서 폰 참조를 끊으므로, 그보다 앞에서 이전 폰을 찾아 구독을 해제한다.
	if (AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(GetPawn()))
	{
		WxCharacter->OnDeath.RemoveDynamic(this, &AWxAIController::HandlePawnDeath);
		WxCharacter->GetLockOnComponent()->SetLockOnTarget(nullptr);
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		WxBlackboardKeys::SetSelfActor(BB, nullptr);

		// 키를 쓴 폰만 지운다.
		const APawn* PreviousPawn = GetPawn();
		if (ResolveMinionMaster(PreviousPawn))
		{
			WxBlackboardKeys::SetMaster(BB, nullptr);
		}
	}

	Super::OnUnPossess();
}

void AWxAIController::HandleAITargetChanged(AActor* NewTarget)
{
	AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(GetPawn());
	if (!WxCharacter)
	{
		return;
	}

	// 겨누는 대상은 서버 권위로 복제돼야 스냅 워프·타겟팅 필터·발사체가 전 머신에서 같은 답을 읽는다.
	// 조준 지점은 대상의 루트다 — 락온 지점은 플레이어 락온의 대상 계약이라 AI 가 겨누는 액터(플레이어 등)에는 없고, 지점 조건도 플레이어 락온 전용 게이트다.
	// 퍼셉션이 무는 대상은 복제되는 폰이라는 전제다. 루트가 런타임 생성 비복제 컴포넌트인 액터를 물면 원격에는 null 로 도착한다.
	WxCharacter->GetLockOnComponent()->SetLockOnTarget(NewTarget ? NewTarget->GetRootComponent() : nullptr);
}

APawn* AWxAIController::ResolveMinionMaster(const APawn* InPawn) const
{
	if (!InPawn)
	{
		return nullptr;
	}

	APawn* SpawnInstigator = InPawn->GetInstigator();
	return SpawnInstigator != InPawn ? SpawnInstigator : nullptr;
}

void AWxAIController::HandlePawnDeath(AWxCharacterBase* DeadCharacter)
{
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Pawn died"));
	}
}
