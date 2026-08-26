// Copyright Woogle. All Rights Reserved.

#include "Device/WxDevice.h"

#include "Device/WxDeviceStateTreeComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "WxWorldModule.h"

#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#endif

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

#if WITH_EDITOR
// 에디터의 액터 복제(Ctrl+W·Alt-드래그)와 붙여넣기는 T3D 텍스트 경로라 생성 훅으로는 원본의 SaveId 가 따라오는 것을 막지 못한다(AWxSpawner::PreSave 의 상세 참조).
// 그래서 직렬화 직전에 이 액터의 ActorGuid 로 재확정한다.
void AWxDevice::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);

	// 쿠킹·EditorDomain 등 사용자 편집이 개입할 수 없는 저장은 건드리지 않는다. 값은 맵 저장 때 이미 확정되어 패키지에 실려 있다.
	if (ObjectSaveContext.IsProceduralSave())
	{
		return;
	}

	SaveId = GetActorGuid();
}
#endif

bool AWxDevice::CanInteract(const AActor* Interactor) const
{
	return bInteractionEnabled;
}

void AWxDevice::SetInteractionEnabled(bool bEnabled)
{
	// 바인딩은 건드리지 않는다 — 자기 트리가 프롬프트·발행자를 담아 둔 장치를 남이 껐다 켜도 원래대로 눌려야 한다.
	bInteractionEnabled = bEnabled;
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
	// ST 가 이 상태에 세팅한 프롬프트가 전부다. 태스크에서 문구를 지정하지 않았으면 표시할 것이 없으므로 공백을 답한다.
	return InteractionBinding.Prompt;
}

FGuid AWxDevice::GetSaveId() const
{
	return SaveId;
}

void AWxDevice::OnSaveRestored()
{
	// 컴포넌트의 StateTag 는 이 호출 전에 이미 역직렬화로 복원되어 있다(WxSave 가 액터+컴포넌트를 먼저 복원하고 이 훅을 부른다).
	StateTreeComponent->NotifySaveRestored();
}

void AWxDevice::NotifyDeviceInteracted(AActor* Interactor, FGameplayTag EventTag, FConstStructView Payload)
{
	// 보내는 쪽이 서버 권위에서만 부르지만, 상태를 움직이는 것은 권위 트리뿐이므로 한 번 더 가른다.
	if (!HasAuthority())
	{
		return;
	}

	// 멈춘 트리엔 이벤트가 닿지 않는다.
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

	// 끌 때 비우지 않는다 — 남이 껐다 켜도 이 상태의 문구·발행자로 돌아와야 한다.
	// 꺼진 장치는 스캔 후보에서 빠지므로 담아 둔 문구가 새어 나갈 일도 없다.
	if (bEnabled)
	{
		InteractionBinding = Binding;
	}
}

void AWxDevice::BeginPlay()
{
	Super::BeginPlay();

	/** ChildActor나 Attach되어 포함된 Device가 Owner 장치를 찾는다. */
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
