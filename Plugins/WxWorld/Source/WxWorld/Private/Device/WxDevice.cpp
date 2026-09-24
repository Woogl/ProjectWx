// Copyright Woogle. All Rights Reserved.

#include "Device/WxDevice.h"

#include "Device/WxDeviceStateTreeComponent.h"
#include "Device/WxStateTreeTask_WaitForTrigger.h"
#include "GameFramework/Character.h"
#include "WxWorldModule.h"

AWxDevice::AWxDevice()
{
	// 상태는 서버 권위이고 클라는 복제 값만 보고 따라간다.
	bReplicates = true;

	StateTreeComponent = CreateDefaultSubobject<UWxDeviceStateTreeComponent>(TEXT("StateTree"));
}

void AWxDevice::GetInteractionOptions(const AActor* Interactor, TArray<FWxInteractionOption>& OutOptions) const
{
	if (!WaitingTask || !WaitingTask->bPlayerInteraction)
	{
		return;
	}

	const int32 FirstIndex = OutOptions.Num();

	if (!WaitingTask->bOnlyWhenLinkedAccepts)
	{
		// 직접 눌리는 장치는 자기 대기 노드의 수락 규칙이 곧 선택지다 — 눌러도 받지 않을 조합이면 프롬프트부터 뜨지 않는다.
		WaitingTask->GetAcceptedOptions(*this, nullptr, OutOptions);
	}
	else
	{
		// 잠금도 선택지도 받는 쪽이 정한다 — 연결 장치가 아무도 작동을 기다리지 않으면 목록이 비어 이 장치가 잠긴다.
		for (const AWxDevice* LinkedDevice : LinkedDevices)
		{
			if (IsValid(LinkedDevice))
			{
				LinkedDevice->GetAcceptedOptions(this, OutOptions);
			}
			// 클라에 아직 로드되지 않은 연결 장치(다른 월드 파티션 셀)는 상태를 알 수 없으니 열어 둔다 — 받을지는 서버가 다시 검증한다.
			else if (!HasAuthority() && !OutOptions.ContainsByPredicate([](const FWxInteractionOption& Option) { return Option.Prompt.IsEmpty() && Option.Value == INDEX_NONE; }))
			{
				OutOptions.Add(FWxInteractionOption());
			}
		}
	}

	for (int32 Index = FirstIndex; Index < OutOptions.Num(); ++Index)
	{
		if (OutOptions[Index].Prompt.IsEmpty())
		{
			OutOptions[Index].Prompt = WaitingTask->Prompt;
		}
	}
}

void AWxDevice::OnInteracted(AActor* Interactor, int32 OptionValue)
{
	// 선택지 값은 상호작용 어빌리티가 이미 지금의 선택지와 대조했다. 여기서는 자기 대기 노드가 받는지만 아래에서 본다.
	NotifyDeviceInteracted(Interactor, nullptr, OptionValue);
}

void AWxDevice::NotifyDeviceInteracted(AActor* Interactor, const AWxDevice* Sender, int32 Value)
{
	// 보내는 쪽이 서버 권위에서만 부르지만, 상태를 움직이는 것은 권위 트리뿐이므로 한 번 더 가른다.
	if (!HasAuthority())
	{
		return;
	}

	// 값 없는 선택지(INDEX_NONE)는 「그냥 받는다」는 뜻이라 어떤 값이 실려 와도 받는다.
	// 잠든 트리는 이 완료가 예약하는 다음 틱이 깨운다.
	TArray<FWxInteractionOption> Accepted;
	GetAcceptedOptions(Sender, Accepted);
	if (!Accepted.ContainsByPredicate([Value](const FWxInteractionOption& Option) { return Option.Value == Value || Option.Value == INDEX_NONE; })
		|| !WaitingContext.FinishTask(EStateTreeFinishTaskType::Succeeded))
	{
		UE_LOG(LogWxWorld, Verbose, TEXT("Device(%s): 작동 신호를 받지 않음(보낸 장치 %s, 값 %d) — 지금 상태에 '작동 대기' 가 없거나, 그 수락 규칙이 받지 않는 값이다."), *GetName(), *GetNameSafe(Sender), Value);

		return;
	}

	// 실제 상태 진입이 관측되면 컴포넌트가 당사자와 값을 상태 스냅샷에 함께 담는다.
	InteractingCharacter = Cast<ACharacter>(Interactor);
	SelectedOptionValue = Value;

	// 받아들인 즉시 등록을 걷는다 — 트리가 틱하기 전 같은 프레임에 온 다음 신호가 당사자·값을 덮어쓰지 못한다(첫 신호가 이긴다).
	WaitingTask = nullptr;
	WaitingContext = FStateTreeWeakExecutionContext();
}

ACharacter* AWxDevice::GetInteractingCharacter() const
{
	return InteractingCharacter;
}

int32 AWxDevice::GetSelectedOptionValue() const
{
	return SelectedOptionValue;
}

void AWxDevice::GetAcceptedOptions(const AWxDevice* Sender, TArray<FWxInteractionOption>& OutOptions) const
{
	if (WaitingTask)
	{
		WaitingTask->GetAcceptedOptions(*this, Sender, OutOptions);
	}
}

void AWxDevice::BeginWaitForTrigger(const FWxStateTreeTask_WaitForTrigger& Task, const FStateTreeWeakExecutionContext& Context)
{
	if (WaitingTask && WaitingTask != &Task)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Device(%s): 활성 경로에 '작동 대기' 가 둘 이상이다 — 나중에 진입한 것만 작동을 받는다. 부모·자식 상태에 겹쳐 두지 않는다."), *GetName());
	}

	WaitingTask = &Task;
	WaitingContext = Context;
}

void AWxDevice::EndWaitForTrigger(const FWxStateTreeTask_WaitForTrigger& Task)
{
	// 자기 등록만 걷는다 — 겹쳐 둔 다른 노드의 등록이나, 신호를 받아들이며 이미 걷힌 자리를 건드리지 않는다.
	if (WaitingTask == &Task)
	{
		WaitingTask = nullptr;
		WaitingContext = FStateTreeWeakExecutionContext();
	}
}

void AWxDevice::PostActorCreated()
{
	Super::PostActorCreated();

	// 맵에 배치된 장치의 자식 액터 장치(엘리베이터 호출 버튼)는 맵 로드 뒤에 다시 스폰돼도 서버·클라가 같은 이름을 쓴다.
	// 경로로 주소 지정되게 해 두지 않으면 클라가 이 장치를 가리키는 RPC(ServerInteract)를 직렬화하다 엔진 assert 로 죽는다.
	const AActor* ParentActor = GetParentActor();
	if (ParentActor && ParentActor->IsNameStableForNetworking())
	{
		SetNetAddressable();
		UE_LOG(LogWxWorld, Verbose, TEXT("Device(%s): 자식 액터 장치를 경로 주소 지정으로 표시(%s)."), *GetName(), GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server"));
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
