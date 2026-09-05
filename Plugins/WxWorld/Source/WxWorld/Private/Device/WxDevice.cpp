// Copyright Woogle. All Rights Reserved.

#include "Device/WxDevice.h"

#include "Device/WxDeviceStateTreeComponent.h"
#include "GameFramework/Character.h"

#include "WxWorldModule.h"

AWxDevice::AWxDevice()
{
	// 상태는 서버 권위이고 클라는 복제 값만 보고 따라간다.
	bReplicates = true;

	StateTreeComponent = CreateDefaultSubobject<UWxDeviceStateTreeComponent>(TEXT("StateTree"));
}

bool AWxDevice::CanInteract(const AActor* Interactor) const
{
	return bInteractionEnabled && StateTreeComponent->IsRunning();
}

void AWxDevice::OnInteracted(AActor* Interactor)
{
	// 상호작용 어빌리티가 서버 권위에서만 부르지만, 상태를 움직이는 것은 권위 트리뿐이므로 한 번 더 가른다.
	if (!HasAuthority())
	{
		return;
	}

	// 자동 완료된 트리도 발행을 받지 않는다.
	if (!StateTreeComponent->IsRunning())
	{
		return;
	}

	// 꺼져 있으면 애초에 스캔 후보에서 빠지지만, 여기서도 같은 기준으로 걸러 상태를 건드리지 않는다.
	if (!CanInteract(Interactor))
	{
		return;
	}

	// 실제 상태 진입이 관측되면 컴포넌트가 당사자를 상태 스냅샷에 함께 담는다.
	InteractingCharacter = Cast<ACharacter>(Interactor);

	// 발행 결과는 진단에만 사용한다. 재진입은 컴포넌트가 실제 전이에서 관측한다.
	if (!BroadcastInteractionDelegate())
	{
		UE_LOG(LogWxWorld, Verbose, TEXT("Device(%s): 상호작용이 트리에 닿지 않았다 — 이 상태에 발행 자리가 없거나, 자리를 연 상태를 이미 떠났다."), *GetName());

		return;
	}
}

FText AWxDevice::GetInteractionPrompt() const
{
	return InteractionBinding.Prompt;
}

void AWxDevice::NotifyDeviceInteracted(AActor* Interactor, FGameplayTag EventTag, FConstStructView Payload)
{
	// 보내는 쪽이 서버 권위에서만 부르지만, 상태를 움직이는 것은 권위 트리뿐이므로 한 번 더 가른다.
	if (!HasAuthority())
	{
		return;
	}

	if (!StateTreeComponent->IsRunning())
	{
		return;
	}

	// 실제 상태 진입이 관측되면 컴포넌트가 당사자를 상태 스냅샷에 함께 담는다.
	InteractingCharacter = Cast<ACharacter>(Interactor);

	// 잠든 트리는 이 발송이 예약하는 다음 틱이 깨운다.
	StateTreeComponent->SendStateTreeEvent(EventTag, Payload);
}

ACharacter* AWxDevice::GetInteractingCharacter() const
{
	return InteractingCharacter;
}

void AWxDevice::SetInteractionBinding(bool bEnabled, const FWxDeviceInteractionBinding& Binding)
{
	bInteractionEnabled = bEnabled;

	InteractionBinding = bEnabled ? Binding : FWxDeviceInteractionBinding();
}

uint32 AWxDevice::PushInteractionBinding(bool bEnabled, const FWxDeviceInteractionBinding& Binding)
{
	if (++NextInteractionToken == 0)
	{
		++NextInteractionToken;
	}
	ScopedInteractions.Add({NextInteractionToken, bEnabled, Binding});
	SetInteractionBinding(bEnabled, Binding);
	return NextInteractionToken;
}

void AWxDevice::PopInteractionBinding(uint32 Token)
{
	for (int32 Index = 0; Index < ScopedInteractions.Num(); ++Index)
	{
		if (ScopedInteractions[Index].Token == Token)
		{
			ScopedInteractions.RemoveAt(Index);
			if (ScopedInteractions.IsEmpty())
			{
				SetInteractionBinding(false, FWxDeviceInteractionBinding());
			}
			else
			{
				const FWxScopedInteraction& Current = ScopedInteractions.Last();
				SetInteractionBinding(Current.bEnabled, Current.Binding);
			}
			return;
		}
	}
}

void AWxDevice::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* AttachParentActor = GetAttachParentActor())
	{
		if (AWxDevice* ParentDevice = Cast<AWxDevice>(AttachParentActor))
		{
			LinkedDevices.AddUnique(ParentDevice);
		}
	}
}

bool AWxDevice::BroadcastInteractionDelegate()
{
	// 엔진은 빈 발행자를 「듣는 이가 없으니 오류 아님」으로 보아 참을 답하므로, 그 참이 「닿았다」로 새기 전에 먼저 가른다.
	// 발행자는 그것을 지목하는 전이가 있을 때만 컴파일이 ID 를 부여하니, 이 판정이 곧 「이 상태가 상호작용을 듣는가」다.
	if (!InteractionBinding.Dispatcher.IsValid())
	{
		return false;
	}

	// 자리를 연 상태를 이미 떠났으면 약참조 컨텍스트가 그 상태를 못 찾아 발행이 조용히 버려지고, 엔진이 거짓을 답한다.
	return InteractionBinding.Context.BroadcastDelegate(InteractionBinding.Dispatcher);
}
