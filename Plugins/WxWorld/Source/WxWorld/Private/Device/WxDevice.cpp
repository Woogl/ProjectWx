// Copyright Woogle. All Rights Reserved.

#include "Device/WxDevice.h"

#include "Device/WxDeviceStateTreeComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "WxGameplayTags.h"
#include "WxWorldModule.h"

#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#endif

AWxDevice::AWxDevice()
{
	// 상태는 서버 권위이고 클라는 복제 값만 보고 따라간다 — 꺼지면 클라 장치가 통째로 멈추므로 배치 측에 맡기지 않는다.
	bReplicates = true;

	StateTreeComponent = CreateDefaultSubobject<UWxDeviceStateTreeComponent>(TEXT("StateTree"));
}

void AWxDevice::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxDevice, InteractingCharacter);
}

#if WITH_EDITOR
// 런타임엔 안정적인 액터 식별자가 없다(ActorGuid 는 에디터 전용 데이터).
//
// 생성·등록 훅이 아니라 직렬화 직전에 심는 것이 핵심이다.
// 에디터의 액터 복제(Ctrl+W·Alt-드래그)와 붙여넣기는 T3D 텍스트 경로라 ImportObjectProperties 가 원본의 SaveId 를 그대로 덮어쓰는데, 저장 직전에 자기 ActorGuid 로 재확정하면 생성 경로가 무엇이든 상관없어진다.
void AWxDevice::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);

	// 쿠킹·EditorDomain 등 사용자 편집이 개입할 수 없는 저장은 건드리지 않는다. 값은 맵 저장 때 이미 확정되어 패키지에 실려 있다.
	if (ObjectSaveContext.IsProceduralSave())
	{
		return;
	}

	// Modify() 는 부르지 않는다. 이미 저장 중이라 트랜잭션에 남길 이유가 없고, 오히려 저장 직후 패키지가 dirty 로 남는다.
	SaveId = GetActorGuid();
}
#endif

bool AWxDevice::IsInteractionEnabled() const
{
	return bInteractionEnabled;
}

void AWxDevice::SetInteractionEnabled(bool bEnabled)
{
	SetInteractionBinding(bEnabled, FWxDeviceInteractionBinding());
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
	if (!bInteractionEnabled)
	{
		return;
	}

	// 당사자는 복제로 각 피어에 전해진다 — 이동·몽타주 태스크가 모든 머신에서 같은 대상을 본다.
	InteractingCharacter = Cast<ACharacter>(Interactor);

	// 이 상호작용이 상태를 바꾸는지 아닌지는 트리가 발행을 소화해 봐야 안다 — 컴포넌트가 그 결과를 보고 재진입을 가려낸다.
	StateTreeComponent->NotifyInteractionPending();

	BroadcastInteractionDelegate();
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

void AWxDevice::NotifyDeviceInteracted(AActor* Interactor)
{
	// 발동 장치가 서버 권위에서만 부르지만, 상태를 움직이는 것은 권위 트리뿐이므로 한 번 더 가른다.
	if (!HasAuthority())
	{
		return;
	}

	// 멈춘 트리엔 이벤트가 닿지 않는다.
	if (!StateTreeComponent->IsRunning())
	{
		return;
	}

	// 당사자는 복제로 각 피어에 전해진다 — 이동·몽타주 태스크가 모든 머신에서 같은 대상을 본다.
	InteractingCharacter = Cast<ACharacter>(Interactor);

	// 잠든 트리는 이 발송이 예약하는 다음 틱이 깨운다.
	// 자기 상호작용과 달리 재진입 판정은 걸지 않는다 — 이벤트엔 「지금 듣고 있는가」를 가릴 자리가 없어,
	// 반응하지 않는 상태에서 당길 때마다 클라만 현재 상태를 재진입해 연출을 헛재생하게 된다.
	StateTreeComponent->SendStateTreeEvent(WxGameplayTags::Event_Interact);
}

FGameplayTag AWxDevice::GetStateTag() const
{
	return StateTreeComponent->GetStateTag();
}

ACharacter* AWxDevice::GetInteractingCharacter() const
{
	return InteractingCharacter;
}

void AWxDevice::SetInteractionBinding(bool bEnabled, const FWxDeviceInteractionBinding& Binding)
{
	bInteractionEnabled = bEnabled;

	// 끌 때는 다음 켜짐이 자기 값을 다시 담도록 비운다 — 남겨 두면 꺼진 상태의 문구가 프롬프트로 새어 나간다.
	InteractionBinding = bEnabled ? Binding : FWxDeviceInteractionBinding();
}

void AWxDevice::BroadcastInteractionDelegate()
{
	InteractionBinding.Context.BroadcastDelegate(InteractionBinding.Dispatcher);
}
