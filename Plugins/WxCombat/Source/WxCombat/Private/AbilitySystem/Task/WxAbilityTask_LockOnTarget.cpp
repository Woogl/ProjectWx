// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_LockOnTarget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "WxGameplayTags.h"

UWxAbilityTask_LockOnTarget* UWxAbilityTask_LockOnTarget::CreateTask(UGameplayAbility* OwningAbility, AActor* InTarget, float InInterpSpeed, float InPitchOffset, float InMaxDistance, float InCharacterInterpSpeed, TSubclassOf<UUserWidget> InReticleWidgetClass)
{
	UWxAbilityTask_LockOnTarget* Task = NewAbilityTask<UWxAbilityTask_LockOnTarget>(OwningAbility);
	Task->Target = InTarget;
	Task->InterpSpeed = InInterpSpeed;
	Task->CharacterInterpSpeed = InCharacterInterpSpeed;
	Task->PitchOffset = InPitchOffset;
	Task->MaxDistanceSquared = InMaxDistance * InMaxDistance;
	Task->ReticleWidgetClass = InReticleWidgetClass;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_LockOnTarget::Activate()
{
	Super::Activate();

	if (AActor* TargetActor = Target.Get())
	{
		TargetActor->OnDestroyed.AddDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			TargetASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UWxAbilityTask_LockOnTarget::HandleTargetDeathTagChanged);
		}

		CreateReticleWidget();
	}
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

	// 회피가 몸체 회전을 점유하는 동안에는(Ability.Dodge 발행) 몸체 회전을 양보한다.
	// 양쪽이 매 틱 SetActorRotation 으로 다른 목표를 잡아당기면 회피 방향이 엉키므로, 충돌을 피한다. 회피 종료(태그 해제) 후 다시 타겟을 향해 보간한다.
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AvatarPawn);
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Dodge))
	{
		return;
	}

	// 캐릭터 몸체를 타겟 방향으로 yaw만 부드럽게 보간. 현재 방향에서 출발하므로 활성화 시 튀지 않는다.
	const FRotator DesiredActorRotation(0.f, LookAtRotation.Yaw, 0.f);
	const FRotator NewActorRotation = FMath::RInterpTo(AvatarPawn->GetActorRotation(), DesiredActorRotation, DeltaTime, CharacterInterpSpeed);
	AvatarPawn->SetActorRotation(FRotator(0.f, NewActorRotation.Yaw, 0.f));
}

void UWxAbilityTask_LockOnTarget::OnDestroy(bool bInOwnerFinished)
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

	Super::OnDestroy(bInOwnerFinished);
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
