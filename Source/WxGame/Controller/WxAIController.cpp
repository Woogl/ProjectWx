// Copyright Woogle. All Rights Reserved.

#include "WxAIController.h"
#include "WxBlackboardKeys.h"
#include "WxAIBehaviorComponent.h"
#include "Character/WxCharacterBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Minion/WxMinionSubsystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Sight.h"
#include "Targeting/WxLockOnComponent.h"

AWxAIController::AWxAIController()
{
	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SetPerceptionComponent(*Perception);

	// 감지 거리·각도는 폰의 UWxAIBehaviorComponent 가 정하므로, 여기서는 폰을 가리지 않는 피아 필터와 자극 수명만 잡는다.
	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	UAISenseConfig_Hearing* HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = false;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;

	UAISenseConfig_Damage* DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));

	// Sight 와 달리 이 둘은 성공 자극만 등록하는 일회성 센스라 해제 이벤트가 없다.
	// MaxAge 를 비워 두면 GetMaxAge 가 NeverHappenedAge 를 돌려줘 자극이 영영 "감지 중" 으로 남으므로, 유한한 수명을 준다.
	HearingConfig->SetMaxAge(5.0f);
	DamageConfig->SetMaxAge(5.0f);

	// 엔진은 여기서 넘긴 센스만 OnRegister 에서 퍼셉션 시스템 리스너로 올린다.
	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->ConfigureSense(*DamageConfig);

	Perception->SetDominantSense(UAISense_Sight::StaticClass());
}

void AWxAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UAIPerceptionComponent* Perception = GetPerceptionComponent())
	{
		// 폰을 놓은 동안 재워 둔 감각을 되살린다.
		if (!Perception->IsRegistered())
		{
			Perception->RegisterComponent();
		}

		// 감지 기록은 옛 폰이 모은 것이라 지운다 — 남겨 두면 새 폰이 보고 있는 액터가 이미 감지 상태여서 상태 변화 통지가 나오지 않는다.
		Perception->ForgetAll();

		if (const IGenericTeamAgentInterface* PawnTeam = Cast<IGenericTeamAgentInterface>(InPawn))
		{
			SetGenericTeamId(PawnTeam->GetGenericTeamId());

			// 퍼셉션 리스너는 빙의 전에 등록되면서 그 자리에서 컨트롤러 팀을 캐시하는데, 엔진은 팀이 바뀌어도 퍼셉션에 통보하지 않는다.
			// 청각·촉각의 피아 판정이 그 캐시를 쓰므로, 여기서 갱신하지 않으면 무팀으로 남아 자기 발소리와 아군 소음까지 적대로 듣는다.
			Perception->RequestStimuliListenerUpdate();
		}
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

		// 재사용된 컨트롤러가 이전 폰의 타겟을 물려받지 않도록 비운다.
		WxBlackboardKeys::SetTargetActor(BB, nullptr);

		BB->RegisterObserver(BB->GetKeyID(WxBlackboardKeys::TargetActor), this,
			FOnBlackboardChangeNotification::CreateUObject(this, &AWxAIController::HandleTargetActorChanged));

		// 소환자는 Deferred Spawn 시 Instigator로 지정되어 빙의보다 먼저 사용할 수 있다.
		// 소환물이 아닌 폰의 블랙보드에는 Master 키가 없으니 쓰지 않는다 — 쓰면 키를 못 찾았다는 경고만 남는다.
		if (APawn* Master = UWxMinionSubsystem::GetMaster(*InPawn))
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
		BB->UnregisterObserversFrom(this);

		WxBlackboardKeys::SetSelfActor(BB, nullptr);
		WxBlackboardKeys::SetTargetActor(BB, nullptr);

		const APawn* PreviousPawn = GetPawn();
		if (PreviousPawn && UWxMinionSubsystem::GetMaster(*PreviousPawn))
		{
			WxBlackboardKeys::SetMaster(BB, nullptr);
		}
	}

	// 몸이 없는 리스너는 마지막 시점에 머문 채 시야 쿼리를 계속 도므로, 폰을 놓은 동안에는 감각을 재운다.
	if (UAIPerceptionComponent* Perception = GetPerceptionComponent())
	{
		Perception->UnregisterComponent();
	}

	Super::OnUnPossess();
}

EBlackboardNotificationResult AWxAIController::HandleTargetActorChanged(const UBlackboardComponent& InBlackboard, FBlackboard::FKey KeyID)
{
	AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(GetPawn());
	if (!WxCharacter)
	{
		return EBlackboardNotificationResult::ContinueObserving;
	}

	// 겨누는 대상은 서버 권위로 복제돼야 스냅 워프·타겟팅 필터·발사체가 전 머신에서 같은 답을 읽는다.
	// 조준 지점은 대상의 루트다 — 락온 지점은 플레이어 락온 전용 계약이라 AI 가 겨누는 액터에는 없다.
	// 무는 대상은 복제되는 폰이라는 전제다. 루트가 런타임 생성 비복제 컴포넌트인 액터를 물면 원격에는 null 로 도착한다.
	AActor* NewTarget = WxBlackboardKeys::GetTargetActor(&InBlackboard);
	WxCharacter->GetLockOnComponent()->SetLockOnTarget(NewTarget ? NewTarget->GetRootComponent() : nullptr);

	return EBlackboardNotificationResult::ContinueObserving;
}

void AWxAIController::HandlePawnDeath(AWxCharacterBase* DeadCharacter)
{
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Pawn died"));
	}
}
