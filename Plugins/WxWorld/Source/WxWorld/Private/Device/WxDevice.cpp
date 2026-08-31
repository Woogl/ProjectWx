// Copyright Woogle. All Rights Reserved.

#include "Device/WxDevice.h"

#include "Device/WxDeviceStateTreeComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "WxWorldModule.h"

AWxDevice::AWxDevice()
{
	// 상태는 서버 권위이고 클라는 복제 값만 보고 따라간다.
	bReplicates = true;

	StateTreeComponent = CreateDefaultSubobject<UWxDeviceStateTreeComponent>(TEXT("StateTree"));
}

void AWxDevice::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxDevice, InteractingCharacter);
}

bool AWxDevice::CanInteract(const AActor* Interactor) const
{
	return bInteractionEnabled;
}

void AWxDevice::OnInteracted(AActor* Interactor)
{
	// 상호작용 어빌리티가 서버 권위에서만 부르지만, 상태를 움직이는 것은 권위 트리뿐이므로 한 번 더 가른다.
	if (!HasAuthority())
	{
		return;
	}

	// 멈춘 트리엔 발행이 닿지 않는다. 소화될 일 없는 재진입 판정을 예약해 두면 다음 시작 때 헛재진입으로 새어 나간다.
	if (!StateTreeComponent->IsRunning())
	{
		return;
	}

	// 꺼져 있으면 애초에 스캔 후보에서 빠지지만, 여기서도 같은 기준으로 걸러 상태를 건드리지 않는다.
	if (!CanInteract(Interactor))
	{
		return;
	}

	// 당사자는 복제로 각 피어에 전해진다 — 몽타주·GE 태스크가 모든 머신에서 같은 대상을 본다.
	InteractingCharacter = Cast<ACharacter>(Interactor);

	// 닿지 않은 발행은 트리를 움직일 수 없다.
	// 그런데도 재진입 판정을 예약하면 다음 권위 틱이 「상태가 안 바뀐 재진입」으로 오판해 클라 전원이 연출을 헛재생한다.
	if (!BroadcastInteractionDelegate())
	{
		UE_LOG(LogWxWorld, Verbose, TEXT("Device(%s): 상호작용이 트리에 닿지 않았다 — 이 상태에 발행 자리가 없거나, 자리를 연 상태를 이미 떠났다."), *GetName());

		return;
	}

	// 이 상호작용이 상태를 바꾸는지 아닌지는 트리가 발행을 소화해 봐야 안다 — 컴포넌트가 그 결과를 보고 재진입을 가려낸다.
	StateTreeComponent->NotifyInteractionPending();
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

	// 당사자는 복제로 각 피어에 전해진다 — 몽타주·GE 태스크가 모든 머신에서 같은 대상을 본다.
	InteractingCharacter = Cast<ACharacter>(Interactor);

	// 잠든 트리는 이 발송이 예약하는 다음 틱이 깨운다.
	// 자기 상호작용과 달리 재진입 판정은 걸지 않는다 — 이벤트엔 「지금 듣고 있는가」를 가릴 자리가 없어, 반응하지 않는 상태에서 당길 때마다 클라만 현재 상태를 재진입해 연출을 헛재생하게 된다.
	StateTreeComponent->SendStateTreeEvent(EventTag, Payload);
}

ACharacter* AWxDevice::GetInteractingCharacter() const
{
	return InteractingCharacter;
}

void AWxDevice::SetInteractionBinding(bool bEnabled, const FWxDeviceInteractionBinding& Binding)
{
	bInteractionEnabled = bEnabled;

	// 끌 때 비우지 않는다 — 켜는 노드마다 자기 것을 새로 담으므로 지울 이유가 없고, 꺼진 동안은 CanInteract 가 먼저 막는다.
	if (bEnabled)
	{
		InteractionBinding = Binding;
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
