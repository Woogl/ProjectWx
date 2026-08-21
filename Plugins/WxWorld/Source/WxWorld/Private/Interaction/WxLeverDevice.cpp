// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxLeverDevice.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Gimmick/WxGimmickStateTreeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "WxWorldModule.h"

AWxLeverDevice::AWxLeverDevice()
{
	// 당김 연출 동안에만 틱이 돈다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 복제 프로퍼티는 없고 당김 멀티캐스트만 나간다 — 그때까지 채널을 열 이유가 없어 잠들어 시작한다.
	bReplicates = true;
	NetDormancy = DORM_Initial;

	// 상시 활성으로 시작한다 — 상태별로 잠글 일이 있으면 기믹 트리의 '상호작용 켜기'(Target 갈래)가 계약으로 내린다.
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BodyMesh->SetGenerateOverlapEvents(false);
	SetRootComponent(BodyMesh);

	HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
	HandleMesh->SetMobility(EComponentMobility::Movable);
	HandleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandleMesh->SetGenerateOverlapEvents(false);
	HandleMesh->SetupAttachment(BodyMesh);
}

void AWxLeverDevice::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UWorld* World = GetWorld();
	if (!World || PullStartTime < 0.0)
	{
		SetActorTickEnabled(false);
		return;
	}

	// 전반은 당기고 후반은 되돌아오는 핑퐁 — 연출이 끝나면 손잡이는 원위치다.
	const float Progress = FMath::Clamp(float((World->GetTimeSeconds() - PullStartTime) / PullDuration), 0.f, 1.f);
	const float Alpha = 1.f - FMath::Abs(2.f * Progress - 1.f);
	HandleMesh->SetRelativeRotation(HandleRestRotation + HandlePulledRotation * Alpha);

	if (Progress >= 1.f)
	{
		HandleMesh->SetRelativeRotation(HandleRestRotation);
		SetActorTickEnabled(false);
	}
}

bool AWxLeverDevice::IsInteractionEnabled() const
{
	// 잠금 상태를 따로 들지 않고 몸체 메시의 쿼리 콜리전이 곧 그 값이다.
	// 당김 중 게이트도 여기에 흡수한다 — 어빌리티의 서버 활성 검증이 연출 중 재조작을 자연 차단한다.
	if (!BodyMesh->IsQueryCollisionEnabled() || IsPulling())
	{
		return false;
	}

	if (GimmickStateRequirements.IsEmpty())
	{
		return true;
	}

	// 지목한 기믹이 하나라도 요건을 벗어난 상태면 잠긴다. 읽는 값이 복제된 권위 상태라 서버의 활성 검증과 클라의 프롬프트가 같은 답을 낸다.
	for (const AActor* Target : Gimmicks)
	{
		const UWxGimmickStateTreeComponent* Gimmick = IsValid(Target) ? Target->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr;
		if (Gimmick && !GimmickStateRequirements.RequirementsMet(Gimmick->GetStateTag().GetSingleTagContainer()))
		{
			return false;
		}
	}

	return true;
}

void AWxLeverDevice::SetInteractionEnabled(bool bEnabled)
{
	// 잠금은 별도 플래그가 아니라 영역 메시의 쿼리 콜리전이다. 실체 프롭이라 물리 차단은 유지한다.
	BodyMesh->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::PhysicsOnly);
}

void AWxLeverDevice::OnInteracted(AActor* Interactor)
{
	// 상호작용 어빌리티가 서버 권위에서만 부르지만, 멀티캐스트·통지 모두 권위 전용이므로 한 번 더 가른다.
	if (!HasAuthority())
	{
		return;
	}

	if (IsPulling())
	{
		return;
	}

	// 잠들어 있던 채널을 깨워야 멀티캐스트가 나간다.
	FlushNetDormancy();
	Multicast_PlayPull();

	// 지목한 기믹이 없어도(독립 레버) 연출은 그대로 성립한다.
	for (AActor* Target : Gimmicks)
	{
		if (UWxGimmickStateTreeComponent* Gimmick = IsValid(Target) ? Target->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr)
		{
			Gimmick->NotifyDeviceInteracted(Interactor);
		}
	}
}

FText AWxLeverDevice::GetInteractionPrompt() const
{
	return Prompt;
}

void AWxLeverDevice::BeginPlay()
{
	Super::BeginPlay();

	HandleRestRotation = HandleMesh->GetRelativeRotation();

	// 기믹 컴포넌트가 없는 액터를 지목하면 당겨도 아무 일이 없어 배선 실수를 알아채기 어렵다.
	for (const AActor* Target : Gimmicks)
	{
		if (Target && !Target->FindComponentByClass<UWxGimmickStateTreeComponent>())
		{
			UE_LOG(LogWxWorld, Error, TEXT("Lever(%s): 지목한 %s 에 기믹 컴포넌트가 없어 눌림이 전달되지 않는다."), *GetName(), *Target->GetName());
		}
	}
}

void AWxLeverDevice::Multicast_PlayPull_Implementation()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	PullStartTime = World->GetTimeSeconds();
	SetActorTickEnabled(true);

	if (PullSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PullSound, HandleMesh->GetComponentLocation());
	}
}

bool AWxLeverDevice::IsPulling() const
{
	const UWorld* World = GetWorld();
	return World && PullStartTime >= 0.0 && World->GetTimeSeconds() - PullStartTime < PullDuration;
}
