// Copyright Woogle. All Rights Reserved.

#include "WxAIController.h"
#include "WxBlackboardKeys.h"
#include "WxAIPerceptionComponent.h"
#include "Character/WxCharacterBase.h"
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

		if (UBehaviorTree* BT = WxCharacter->GetBehaviorTree())
		{
			RunBehaviorTree(BT);
		}
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		WxBlackboardKeys::SetSelfActor(BB, InPawn);
		WxBlackboardKeys::SetHomeLocation(BB, InPawn->GetActorLocation());

		// 주인은 소환자다 — MinionComponent 가 스폰 파라미터로 심어 둔 Instigator 가 그 값이다.
		// 엔진은 빈 Instigator 에 폰 자신을 넣으므로(APawn::PreInitializeComponents), 주인 없이 태어난 폰은 자기 자신이 들어온다.
		// 그런 폰의 블랙보드에는 Master 키가 없으니 쓰지 않는다 — 쓰면 키를 못 찾았다는 경고만 남는다.
		APawn* Summoner = InPawn->GetInstigator();
		if (Summoner != InPawn)
		{
			WxBlackboardKeys::SetMaster(BB, Summoner);
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

		// 키를 쓴 폰만 지운다. GetPawn() 은 Super 앞이라 아직 이전 폰이다.
		const APawn* PreviousPawn = GetPawn();
		if (PreviousPawn && PreviousPawn->GetInstigator() != PreviousPawn)
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

void AWxAIController::HandlePawnDeath(AWxCharacterBase* DeadCharacter)
{
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Pawn died"));
	}
}
