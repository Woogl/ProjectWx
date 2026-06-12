// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_LockOnTarget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Targeting/WxLockOnManagerComponent.h"
#include "WxGameplayTags.h"

UWxAbilityTask_LockOnTarget* UWxAbilityTask_LockOnTarget::CreateTask(UGameplayAbility* OwningAbility, USceneComponent* InTarget, float InInterpSpeed, float InPitchOffset, float InMaxDistance, float InCharacterInterpSpeed, TSubclassOf<UUserWidget> InReticleWidgetClass, UInputAction* InLookAction, float InRetargetLookThreshold)
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

	USceneComponent* TargetComponent = Target.Get();
	if (!TargetComponent)
	{
		// 대상 액터 또는 추적 중인 부위 컴포넌트가 파괴되면 약참조가 풀려 여기서 락온이 해제된다.
		OnTargetLost.Broadcast();
		return;
	}

	APawn* AvatarPawn = Cast<APawn>(GetAvatarActor());
	if (!AvatarPawn)
	{
		return;
	}

	const FVector TargetLocation = TargetComponent->GetComponentLocation();

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

	// 락온 대상은 컴포넌트가 권위·복제 소스다. 변경을 구독하고 현재 값을 초기 대상으로 채택한다(이후 재탐색/복제 정합은 델리게이트가 처리).
	LockOnManagerComponent = UWxLockOnManagerComponent::FindComponent(GetAvatarActor());
	if (UWxLockOnManagerComponent* Comp = LockOnManagerComponent.Get())
	{
		Comp->OnLockOnTargetChanged.AddDynamic(this, &UWxAbilityTask_LockOnTarget::HandleLockOnTargetChanged);
		Target = Comp->GetLockOnTarget();
	}

	// 컴포넌트가 없으면 생성 시 주입된 초기 타겟(Target)으로 폴백한다. BindTarget 은 내부에서 null 을 체크한다.
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
	// NewTarget 이 null 이면 다음 TickTask 가 무효 Target 을 감지해 OnTargetLost 를 발생시킨다(기존 로직 재사용).
}

void UWxAbilityTask_LockOnTarget::BindTarget()
{
	USceneComponent* TargetComponent = Target.Get();
	if (!TargetComponent)
	{
		return;
	}

	// 파괴/사망 이벤트는 소유 액터 단위다. 부위 컴포넌트만 파괴되고 액터는 살아있는 경우에도
	// 정확히 해제할 수 있도록 바인딩한 소유 액터를 캐시한다.
	AActor* TargetActor = TargetComponent->GetOwner();
	BoundTargetActor = TargetActor;
	if (TargetActor)
	{
		TargetActor->OnDestroyed.AddDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			TargetASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UWxAbilityTask_LockOnTarget::HandleTargetDeathTagChanged);
		}
	}

	CreateReticleWidget();
}

void UWxAbilityTask_LockOnTarget::UnbindTarget()
{
	DestroyReticleWidget();

	// Target(컴포넌트)이 이미 파괴되어 약참조가 풀렸어도 캐시한 소유 액터로 바인딩을 해제한다.
	if (AActor* TargetActor = BoundTargetActor.Get())
	{
		TargetActor->OnDestroyed.RemoveDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			TargetASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
				.RemoveAll(this);
		}
	}
	BoundTargetActor = nullptr;
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
	USceneComponent* TargetComponent = Target.Get();
	if (!TargetComponent || !ReticleWidgetClass)
	{
		return;
	}

	// 레티클은 추적 대상 컴포넌트에 직접 부착해 부위를 그대로 따라가게 한다(루트 컴포넌트면 액터 중심).
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
