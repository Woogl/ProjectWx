// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxLeverDevice.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Gimmick/WxGimmickStateTreeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AWxLeverDevice::AWxLeverDevice()
{
	// 당김 연출 동안에만 틱이 돈다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 복제 프로퍼티는 없고 당김 멀티캐스트만 나간다 — 그때까지 채널을 열 이유가 없어 잠들어 시작한다.
	bReplicates = true;
	NetDormancy = DORM_Initial;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
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

bool AWxLeverDevice::IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const
{
	// 콜리전을 함께 봐야 서버 검증(활성 → 사거리 순)이 잠긴 레버를 사거리 ensure 앞에서 거른다.
	// 당김 중 게이트도 여기에 흡수한다 — 어빌리티의 서버 활성 검증이 연출 중 재조작을 자연 차단한다.
	return Mesh == BodyMesh && BodyMesh->IsQueryCollisionEnabled() && !IsPulling();
}

void AWxLeverDevice::SetInteractionEnabled(bool bEnabled)
{
	// 잠금은 별도 플래그가 아니라 영역 메시의 쿼리 콜리전이다. 실체 프롭이라 물리 차단은 유지한다.
	BodyMesh->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::PhysicsOnly);
}

void AWxLeverDevice::OnInteracted(AActor* Interactor, const UActorComponent* Source)
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

	// 구독 기믹이 이 장치의 역할을 역조회해 자기 트리에 발행한다. 구독자가 없어도(독립 레버) 연출은 그대로 성립한다.
	for (const TWeakObjectPtr<UWxGimmickStateTreeComponent>& Subscriber : Subscribers)
	{
		if (UWxGimmickStateTreeComponent* Gimmick = Subscriber.Get())
		{
			Gimmick->NotifyDeviceInteracted(this, Interactor);
		}
	}
}

FText AWxLeverDevice::GetInteractionPrompt(const UActorComponent* Source) const
{
	return PushedPrompt.IsEmpty() ? Prompt : PushedPrompt;
}

void AWxLeverDevice::SetDeviceActive(bool bActive, const FText& InPrompt)
{
	SetInteractionEnabled(bActive);

	// 켜면서 밀어 준 문구만 남긴다 — 비워 켜면 저작 기본 문구로 폴백하고, 끄면 다음 켜짐을 위해 지운다.
	PushedPrompt = bActive ? InPrompt : FText::GetEmpty();
}

void AWxLeverDevice::RegisterSubscriber(UWxGimmickStateTreeComponent* Gimmick)
{
	Subscribers.AddUnique(Gimmick);
}

void AWxLeverDevice::UnregisterSubscriber(UWxGimmickStateTreeComponent* Gimmick)
{
	Subscribers.Remove(Gimmick);
}

void AWxLeverDevice::BeginPlay()
{
	Super::BeginPlay();

	HandleRestRotation = HandleMesh->GetRelativeRotation();
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
