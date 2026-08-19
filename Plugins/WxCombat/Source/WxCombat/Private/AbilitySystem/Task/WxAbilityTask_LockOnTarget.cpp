// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_LockOnTarget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "Targeting/WxLockOnManagerComponent.h"
#include "Targeting/WxLockOnPointComponent.h"
#include "WxGameplayTags.h"

UWxAbilityTask_LockOnTarget* UWxAbilityTask_LockOnTarget::CreateTask(UGameplayAbility* OwningAbility, USceneComponent* InTarget, float InInterpSpeed, float InPitchOffset, float InMaxDistance, float InCharacterInterpSpeed, TSubclassOf<UUserWidget> InReticleWidgetClass, float InRetargetLookThreshold)
{
	UWxAbilityTask_LockOnTarget* Task = NewAbilityTask<UWxAbilityTask_LockOnTarget>(OwningAbility);
	Task->Target = InTarget;
	Task->InterpSpeed = InInterpSpeed;
	Task->CharacterInterpSpeed = InCharacterInterpSpeed;
	Task->PitchOffset = InPitchOffset;
	Task->MaxDistanceSquared = InMaxDistance * InMaxDistance;
	Task->ReticleWidgetClass = InReticleWidgetClass;
	Task->RetargetLookThreshold = InRetargetLookThreshold;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_LockOnTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	USceneComponent* TargetComponent = Target.Get();
	if (!TargetComponent)
	{
		// 대상 액터 또는 추적 중인 부위 컴포넌트가 파괴되면 약참조가 풀려 여기서 락온이 해제된다.
		OnTargetLost.Broadcast();
		return;
	}

	// 사망 등 태그 조건 상실도 거리·널 상실과 같이 폴링으로 감지한다.
	const UWxLockOnPointComponent* TargetPoint = Cast<UWxLockOnPointComponent>(TargetComponent);
	if (TargetPoint && !TargetPoint->CanBeLockedOn())
	{
		OnTargetLost.Broadcast();
		return;
	}

	APawn* AvatarPawn = Cast<APawn>(GetAvatarActor());
	if (!AvatarPawn)
	{
		return;
	}

	const FVector TargetLocation = TargetComponent->GetComponentLocation();

	const float DistanceSquared = FVector::DistSquared(AvatarPawn->GetActorLocation(), TargetLocation);
	if (DistanceSquared > MaxDistanceSquared)
	{
		OnTargetLost.Broadcast();
		return;
	}

	APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController());
	if (!PC)
	{
		return;
	}

	const FRotator LookAtRotation = (TargetLocation - AvatarPawn->GetActorLocation()).Rotation();

	// 회피 중에도 적을 화면에 두도록 카메라 추적은 유지한다.
	FRotator DesiredControlRotation = LookAtRotation;
	DesiredControlRotation.Pitch += PitchOffset;
	const FRotator NewControlRotation = FMath::RInterpTo(PC->GetControlRotation(), DesiredControlRotation, DeltaTime, InterpSpeed);
	PC->SetControlRotation(NewControlRotation);

	// 현재 방향에서 출발하는 보간이라 활성화 순간에 튀지 않는다.
	// 루트모션이 몸 기준이라 이 회전이 사이드 회피를 타겟 중심 호 궤적으로 만든다.
	const FRotator DesiredActorRotation(0.f, LookAtRotation.Yaw, 0.f);
	const FRotator NewActorRotation = FMath::RInterpTo(AvatarPawn->GetActorRotation(), DesiredActorRotation, DeltaTime, CharacterInterpSpeed);
	AvatarPawn->SetActorRotation(FRotator(0.f, NewActorRotation.Yaw, 0.f));

	UWxLockOnManagerComponent* Comp = LockOnManagerComponent.Get();
	if (!Comp)
	{
		return;
	}

	const FVector2D LookAxis = Comp->ConsumeLookInput();
	if (LookAxis.IsNearlyZero())
	{
		// 입력이 없는 프레임에는 누적을 초기화해, 띄엄띄엄 들어온 입력이 아니라 한 번의 큰 시선 이동만 묶는다.
		AccumulatedLook = FVector2D::ZeroVector;
		return;
	}

	AccumulatedLook += LookAxis;
	if (AccumulatedLook.Size() >= RetargetLookThreshold)
	{
		OnRetargetRequested.Broadcast(AccumulatedLook.GetSafeNormal());
		AccumulatedLook = FVector2D::ZeroVector;
	}
}

void UWxAbilityTask_LockOnTarget::OnDestroy(bool bInOwnerFinished)
{
	if (UWxLockOnManagerComponent* Comp = LockOnManagerComponent.Get())
	{
		Comp->OnLockOnTargetChanged.RemoveDynamic(this, &UWxAbilityTask_LockOnTarget::HandleLockOnTargetChanged);
	}

	UnbindTarget();

	Super::OnDestroy(bInOwnerFinished);
}

void UWxAbilityTask_LockOnTarget::Activate()
{
	Super::Activate();

	// 락온 대상의 권위·복제 소스는 컴포넌트다.
	LockOnManagerComponent = UWxLockOnManagerComponent::FindComponent(GetAvatarActor());
	if (UWxLockOnManagerComponent* Comp = LockOnManagerComponent.Get())
	{
		Comp->OnLockOnTargetChanged.AddDynamic(this, &UWxAbilityTask_LockOnTarget::HandleLockOnTargetChanged);
		Target = Comp->GetLockOnTarget();
	}

	// 컴포넌트가 없으면 생성 시 주입된 초기 타겟으로 폴백한다.
	BindTarget();
}

void UWxAbilityTask_LockOnTarget::HandleLockOnTargetChanged(USceneComponent* NewTarget)
{
	if (NewTarget == Target.Get())
	{
		return;
	}

	UnbindTarget();
	Target = NewTarget;
	if (NewTarget)
	{
		BindTarget();
	}
	// NewTarget 이 null 이면 다음 TickTask 가 무효 Target 을 감지해 OnTargetLost 를 발생시킨다.
}

void UWxAbilityTask_LockOnTarget::BindTarget()
{
	USceneComponent* TargetComponent = Target.Get();
	if (!TargetComponent)
	{
		return;
	}

	// 파괴 이벤트는 소유 액터 단위라, 부위 컴포넌트만 파괴돼도 정확히 해제하려고 바인딩한 액터를 캐시한다.
	AActor* TargetActor = TargetComponent->GetOwner();
	BoundTargetActor = TargetActor;
	if (TargetActor)
	{
		TargetActor->OnDestroyed.AddDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);
	}

	// 피대상 표시는 네임플레이트 조건일 뿐인 개인 UI다.
	// 이 태스크는 로컬 플레이어의 락온에서만 생성되므로 레티클과 같은 수명으로 로컬 태그를 붙인다.
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		TargetASC->AddLooseGameplayTag(WxGameplayTags::State_LockedOn);
	}

	CreateReticleWidget();
}

void UWxAbilityTask_LockOnTarget::UnbindTarget()
{
	DestroyReticleWidget();

	// Target 약참조가 이미 풀렸어도 캐시한 소유 액터로 바인딩과 표시 태그를 해제한다.
	if (AActor* TargetActor = BoundTargetActor.Get())
	{
		TargetActor->OnDestroyed.RemoveDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			TargetASC->RemoveLooseGameplayTag(WxGameplayTags::State_LockedOn);
		}
	}
	BoundTargetActor = nullptr;
}

void UWxAbilityTask_LockOnTarget::HandleTargetDestroyed(AActor* DestroyedActor)
{
	OnTargetLost.Broadcast();
}

void UWxAbilityTask_LockOnTarget::CreateReticleWidget()
{
	USceneComponent* TargetComponent = Target.Get();
	if (!TargetComponent || !ReticleWidgetClass)
	{
		return;
	}

	// 추적 대상 컴포넌트에 직접 부착해 부위를 그대로 따라가게 한다(루트 컴포넌트면 액터 중심).
	ReticleWidgetComponent = NewObject<UWidgetComponent>(TargetComponent->GetOwner());
	ReticleWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	ReticleWidgetComponent->SetWidgetClass(ReticleWidgetClass);
	ReticleWidgetComponent->SetDrawAtDesiredSize(true);
	ReticleWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReticleWidgetComponent->RegisterComponent();
	ReticleWidgetComponent->AttachToComponent(TargetComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void UWxAbilityTask_LockOnTarget::DestroyReticleWidget()
{
	if (ReticleWidgetComponent)
	{
		ReticleWidgetComponent->DestroyComponent();
		ReticleWidgetComponent = nullptr;
	}
}
