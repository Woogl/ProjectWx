// Copyright Woogle. All Rights Reserved.

#include "WxAIBehaviorComponent.h"
#include "WxAIModule.h"
#include "WxGameplayTags.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Damage.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#endif

UWxAIBehaviorComponent::UWxAIBehaviorComponent()
{
#if WITH_EDITOR
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
#endif
}

void UWxAIBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	// PIE에서도 저작용 드로우를 위한 Tick은 필요하지 않다.
	SetComponentTickEnabled(false);
#endif

	APawn* Pawn = GetOwner<APawn>();
	if (!Pawn)
	{
		UE_LOG(LogWxAI, Warning, TEXT("%s: WxAIBehaviorComponent 는 Pawn 에만 부착할 수 있다."), *GetNameSafe(GetOwner()));
		return;
	}

	// 배치된 폰은 이 컴포넌트의 BeginPlay 전에 빙의되므로 델리게이트만으로는 첫 컨트롤러를 놓친다.
	Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UWxAIBehaviorComponent::HandleControllerChanged);
	ApplySenseSettings(Pawn->GetController());

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		// 가드 브레이크 히트는 Event.Hit.GuardBreak 자식으로 나가므로 정확 매칭 구독은 놓친다.
		ASC->AddGameplayEventTagContainerDelegate(FGameplayTagContainer(WxGameplayTags::Event_Hit),
			FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UWxAIBehaviorComponent::HandlePawnHit));
	}
}

#if WITH_EDITOR
void UWxAIBehaviorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	const APawn* Pawn = GetOwner<APawn>();
	if (!World || World->WorldType != EWorldType::Editor || !Pawn)
	{
		return;
	}

	// ChildActor 미리보기는 Pawn 대신 이를 배치한 부모 액터가 선택된다.
	const AActor* SelectedActor = Pawn;
	while (SelectedActor && !SelectedActor->IsSelected())
	{
		SelectedActor = SelectedActor->GetParentActor();
	}
	if (!SelectedActor)
	{
		return;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	Pawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const float Radius = FMath::Max(0.f, SightRadius);
	const float HalfAngle = FMath::Clamp(SightAngle, 0.f, 180.f);
	const FColor Color = FColor::Green;
	const FVector Forward = EyeRotation.Vector();
	const FVector Right = FRotationMatrix(EyeRotation).GetUnitAxis(EAxis::Y);
	const float StartRadians = FMath::DegreesToRadians(-HalfAngle);
	FVector PreviousPoint = EyeLocation + Radius * (Forward * FMath::Cos(StartRadians) + Right * FMath::Sin(StartRadians));
	DrawDebugLine(World, EyeLocation, PreviousPoint, Color);

	// 거리 제한과 편측 각도를 수평 단면으로 표시한다. 180도는 원 전체가 된다.
	constexpr int32 SegmentCount = 64;
	for (int32 Segment = 1; Segment <= SegmentCount; ++Segment)
	{
		const float AngleRadians = FMath::DegreesToRadians(FMath::Lerp(-HalfAngle, HalfAngle, static_cast<float>(Segment) / SegmentCount));
		const FVector Point = EyeLocation + Radius * (Forward * FMath::Cos(AngleRadians) + Right * FMath::Sin(AngleRadians));
		DrawDebugLine(World, PreviousPoint, Point, Color);
		PreviousPoint = Point;
	}
	DrawDebugLine(World, EyeLocation, PreviousPoint, Color);
}
#endif

UBehaviorTree* UWxAIBehaviorComponent::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

float UWxAIBehaviorComponent::GetSightRadius() const
{
	return SightRadius;
}

float UWxAIBehaviorComponent::GetSightAngle() const
{
	return SightAngle;
}

float UWxAIBehaviorComponent::GetHearingRadius() const
{
	return HearingRadius;
}

void UWxAIBehaviorComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	ApplySenseSettings(NewController);
}

void UWxAIBehaviorComponent::ApplySenseSettings(AController* Controller) const
{
	AAIController* AIController = Cast<AAIController>(Controller);
	UAIPerceptionComponent* Perception = AIController ? AIController->GetPerceptionComponent() : nullptr;
	if (!Perception)
	{
		return;
	}

	if (UAISenseConfig_Sight* SightConfig = Perception->GetSenseConfig<UAISenseConfig_Sight>())
	{
		SightConfig->SightRadius = SightRadius;
		// 반경 경계에서 붙었다 떨어졌다 하는 것은 리시 복귀가 다루므로 시야 상실 반경에 히스테리시스를 두지 않는다.
		SightConfig->LoseSightRadius = SightRadius;
		SightConfig->PeripheralVisionAngleDegrees = SightAngle;

		// 엔진은 config 를 고친 것을 스스로 알아채지 못한다. 같은 config 를 다시 넘기는 것이 재구성 통로다.
		// 아직 등록 전이면 설정만 갱신되고, 등록 시 그 값으로 리스너가 만들어진다.
		Perception->ConfigureSense(*SightConfig);
	}

	if (UAISenseConfig_Hearing* HearingConfig = Perception->GetSenseConfig<UAISenseConfig_Hearing>())
	{
		HearingConfig->HearingRange = HearingRadius;
		Perception->ConfigureSense(*HearingConfig);
	}
}

void UWxAIBehaviorComponent::HandlePawnHit(FGameplayTag MatchingTag, const FGameplayEventData* Payload)
{
	// 패리 반동은 대미지 없이 Event.Hit.Parry 로 같은 구독에 걸리므로 자극에서 뺀다.
	if (!Payload || Payload->EventMagnitude <= 0.f)
	{
		return;
	}

	APawn* Pawn = GetOwner<APawn>();
	AActor* DamageInstigator = Payload->ContextHandle.GetInstigator();
	if (!Pawn || !DamageInstigator)
	{
		return;
	}

	// Sight·Hearing 과 달리 Damage 센스에는 DetectionByAffiliation 이 없어 엔진이 가해자를 가려 주지 않는다.
	// 여기서 막지 않으면 아군 오사 한 번에 서로를 타겟으로 확정한다.
	if (FGenericTeamId::GetAttitude(Pawn, DamageInstigator) != ETeamAttitude::Hostile)
	{
		return;
	}

	const FHitResult* HitResult = Payload->ContextHandle.GetHitResult();
	const FVector HitLocation = HitResult ? FVector(HitResult->ImpactPoint) : Pawn->GetActorLocation();
	UAISense_Damage::ReportDamageEvent(this, Pawn, DamageInstigator, Payload->EventMagnitude, DamageInstigator->GetActorLocation(), HitLocation);
}
