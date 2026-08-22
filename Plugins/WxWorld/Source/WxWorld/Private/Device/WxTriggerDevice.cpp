// Copyright Woogle. All Rights Reserved.

#include "Device/WxTriggerDevice.h"

#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Device/WxDevice.h"
#include "Engine/World.h"
#include "WxWorldModule.h"

AWxTriggerDevice::AWxTriggerDevice()
{
	// 복제 프로퍼티는 없고 발동 멀티캐스트만 나간다 — 그때까지 채널을 열 이유가 없어 잠들어 시작한다.
	bReplicates = true;
	NetDormancy = DORM_Initial;

	// 상시 활성으로 시작한다 — 상태별로 잠글 일이 있으면 대상 트리의 '상호작용 켜기'(Target 갈래)가 계약으로 내린다.
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BodyMesh->SetGenerateOverlapEvents(false);
	SetRootComponent(BodyMesh);
	
	Prompt = FText::FromString(TEXT("Interact"));
}

bool AWxTriggerDevice::CanInteract() const
{
	// 잠금 상태를 따로 들지 않고 몸체 메시의 쿼리 콜리전이 곧 그 값이다.
	// 쿨다운 게이트도 여기에 흡수한다 — 어빌리티의 서버 활성 검증이 연출 중 재조작을 자연 차단한다.
	if (!BodyMesh->IsQueryCollisionEnabled() || IsOnCooldown())
	{
		return false;
	}

	if (StateTagRequirements.IsEmpty())
	{
		return true;
	}

	// 지목한 장치가 하나라도 요건을 벗어난 상태면 잠긴다. 읽는 값이 복제된 권위 상태라 서버의 활성 검증과 클라의 프롬프트가 같은 답을 낸다.
	for (const AActor* Target : LinkedDevices)
	{
		const AWxDevice* Device = IsValid(Target) ? Cast<AWxDevice>(Target) : nullptr;
		if (Device && !StateTagRequirements.RequirementsMet(Device->GetStateTag().GetSingleTagContainer()))
		{
			return false;
		}
	}

	return true;
}

void AWxTriggerDevice::SetInteractionEnabled(bool bEnabled)
{
	// 잠금은 별도 플래그가 아니라 영역 메시의 쿼리 콜리전이다. 실체 프롭이라 물리 차단은 유지한다.
	BodyMesh->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::PhysicsOnly);
}

void AWxTriggerDevice::OnInteracted(AActor* Interactor)
{
	// 상호작용 어빌리티가 서버 권위에서만 부르지만, 멀티캐스트·통지 모두 권위 전용이므로 한 번 더 가른다.
	if (!HasAuthority())
	{
		return;
	}

	if (IsOnCooldown())
	{
		return;
	}

	// 잠들어 있던 채널을 깨워야 멀티캐스트가 나간다.
	FlushNetDormancy();
	Multicast_Trigger();

	// 지목한 장치가 없어도(독립 발동 장치) 연출은 그대로 성립한다.
	for (AActor* Target : LinkedDevices)
	{
		if (AWxDevice* Device = IsValid(Target) ? Cast<AWxDevice>(Target) : nullptr)
		{
			Device->NotifyDeviceInteracted(Interactor);
		}
	}
}

FText AWxTriggerDevice::GetInteractionPrompt() const
{
	return Prompt;
}

void AWxTriggerDevice::BeginPlay()
{
	Super::BeginPlay();
	
	// 내장된 TriggerDevice는 자동 연결된다.
	// 예시: 문+레버처럼 한 몸으로 저작되는 쌍
	if (AActor* AttachParentActor = GetAttachParentActor())
	{
		if (AWxDevice* OwnerDevice = Cast<AWxDevice>(AttachParentActor))
		{
			LinkedDevices.AddUnique(OwnerDevice);
		}
	}
}

void AWxTriggerDevice::Multicast_Trigger_Implementation()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 각 피어가 자기 시계로 같은 게이트를 들도록 여기서 찍는다 — 클라의 프롬프트 표시와 서버의 활성 검증이 같은 답을 낸다.
	LastTriggerTime = World->GetTimeSeconds();

	OnTriggered();
}

bool AWxTriggerDevice::IsOnCooldown() const
{
	const UWorld* World = GetWorld();
	return World && LastTriggerTime >= 0.0 && World->GetTimeSeconds() - LastTriggerTime < RetriggerCooldown;
}
