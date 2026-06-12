// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_LockOnTarget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Targeting/WxLockOnComponent.h"
#include "WxGameplayTags.h"

UWxAbilityTask_LockOnTarget* UWxAbilityTask_LockOnTarget::CreateTask(UGameplayAbility* OwningAbility, AActor* InTarget, float InInterpSpeed, float InPitchOffset, float InMaxDistance, float InCharacterInterpSpeed, TSubclassOf<UUserWidget> InReticleWidgetClass, UInputAction* InLookAction, float InRetargetLookThreshold)
{
	UWxAbilityTask_LockOnTarget* Task = NewAbilityTask<UWxAbilityTask_LockOnTarget>(OwningAbility);
	Task->Target = InTarget;
	Task->InterpSpeed = InInterpSpeed;
	Task->CharacterInterpSpeed = InCharacterInterpSpeed;
	Task->PitchOffset = InPitchOffset;
	Task->MaxDistanceSquared = InMaxDistance * InMaxDistance;
	Task->ReticleWidgetClass = InReticleWidgetClass;
	Task->LookAction = InLookAction;
	Task->RetargetLookThreshold = InRetargetLookThreshold;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_LockOnTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	AActor* TargetActor = Target.Get();
	if (!TargetActor)
	{
		OnTargetLost.Broadcast();
		return;
	}

	APawn* AvatarPawn = Cast<APawn>(GetAvatarActor());
	if (!AvatarPawn)
	{
		return;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();

	// 거리 초과 시 락온 해제
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

	// 카메라(컨트롤러)를 타겟 방향으로 보간. PitchOffset만큼 살짝 내려다본다. 회피 중에도 적을 화면에 두도록 카메라 추적은 유지한다.
	FRotator DesiredControlRotation = LookAtRotation;
	DesiredControlRotation.Pitch += PitchOffset;
	const FRotator NewControlRotation = FMath::RInterpTo(PC->GetControlRotation(), DesiredControlRotation, DeltaTime, InterpSpeed);
	PC->SetControlRotation(NewControlRotation);

	// 캐릭터 몸체를 타겟 방향으로 yaw만 부드럽게 보간. 현재 방향에서 출발하므로 활성화 시 튀지 않는다.
	// 회피 중에도 추적을 유지한다. 루트모션은 몸 기준이므로 추적 회전이 사이드 회피를 타겟 중심 호 궤적으로 만든다.
	const FRotator DesiredActorRotation(0.f, LookAtRotation.Yaw, 0.f);
	const FRotator NewActorRotation = FMath::RInterpTo(AvatarPawn->GetActorRotation(), DesiredActorRotation, DeltaTime, CharacterInterpSpeed);
	AvatarPawn->SetActorRotation(FRotator(0.f, NewActorRotation.Yaw, 0.f));

	// IA_Look 입력을 폴링해 누적하다 임계값을 넘으면 그 방향으로 재탐색을 요청한다.
	// 캐릭터의 Look 콜백은 락온 중 시점 회전을 무시하지만, Enhanced Input은 매 프레임 액션을 평가해 두므로 현재 값을 직접 읽을 수 있다.
	if (!LookAction)
	{
		return;
	}

	FVector2D LookAxis = FVector2D::ZeroVector;
	if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			if (UEnhancedPlayerInput* PlayerInput = Subsystem->GetPlayerInput())
			{
				LookAxis = PlayerInput->GetActionValue(LookAction).Get<FVector2D>();
			}
		}
	}

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
	if (UWxLockOnComponent* Comp = LockOnComponent.Get())
	{
		Comp->OnLockOnTargetChanged.RemoveDynamic(this, &UWxAbilityTask_LockOnTarget::HandleLockOnTargetChanged);
	}

	UnbindTarget();

	Super::OnDestroy(bInOwnerFinished);
}

void UWxAbilityTask_LockOnTarget::Activate()
{
	Super::Activate();

	// 락온 대상은 컴포넌트가 권위·복제 소스다. 변경을 구독하고 현재 값을 초기 대상으로 채택한다(이후 재탐색/복제 정합은 델리게이트가 처리).
	LockOnComponent = UWxLockOnComponent::FindComponent(GetAvatarActor());
	if (UWxLockOnComponent* Comp = LockOnComponent.Get())
	{
		Comp->OnLockOnTargetChanged.AddDynamic(this, &UWxAbilityTask_LockOnTarget::HandleLockOnTargetChanged);
		Target = Comp->GetLockOnTarget();
	}

	// 컴포넌트가 없으면 생성 시 주입된 초기 타겟(Target)으로 폴백한다. BindTarget 은 내부에서 null 을 체크한다.
	BindTarget();
}

void UWxAbilityTask_LockOnTarget::HandleLockOnTargetChanged(AActor* NewTarget)
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
	// NewTarget 이 null 이면 다음 TickTask 가 무효 Target 을 감지해 OnTargetLost 를 발생시킨다(기존 로직 재사용).
}

void UWxAbilityTask_LockOnTarget::BindTarget()
{
	AActor* TargetActor = Target.Get();
	if (!TargetActor)
	{
		return;
	}

	TargetActor->OnDestroyed.AddDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		TargetASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UWxAbilityTask_LockOnTarget::HandleTargetDeathTagChanged);
	}

	CreateReticleWidget();
}

void UWxAbilityTask_LockOnTarget::UnbindTarget()
{
	DestroyReticleWidget();

	if (AActor* TargetActor = Target.Get())
	{
		TargetActor->OnDestroyed.RemoveDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			TargetASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
				.RemoveAll(this);
		}
	}
}

void UWxAbilityTask_LockOnTarget::HandleTargetDestroyed(AActor* DestroyedActor)
{
	OnTargetLost.Broadcast();
}

void UWxAbilityTask_LockOnTarget::HandleTargetDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		OnTargetLost.Broadcast();
	}
}

void UWxAbilityTask_LockOnTarget::CreateReticleWidget()
{
	AActor* TargetActor = Target.Get();
	if (!TargetActor || !ReticleWidgetClass)
	{
		return;
	}

	ReticleWidgetComponent = NewObject<UWidgetComponent>(TargetActor);
	ReticleWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	ReticleWidgetComponent->SetWidgetClass(ReticleWidgetClass);
	ReticleWidgetComponent->SetDrawAtDesiredSize(true);
	ReticleWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReticleWidgetComponent->RegisterComponent();
	ReticleWidgetComponent->AttachToComponent(TargetActor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void UWxAbilityTask_LockOnTarget::DestroyReticleWidget()
{
	if (ReticleWidgetComponent)
	{
		ReticleWidgetComponent->DestroyComponent();
		ReticleWidgetComponent = nullptr;
	}
}
