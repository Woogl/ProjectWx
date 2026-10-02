// Copyright Woogle. All Rights Reserved.

#include "Input/WxInputBufferComponent.h"
#include "AbilitySystem/Abilities/WxAbilityBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "WxGameplayTags.h"

UWxInputBufferComponent::UWxInputBufferComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWxInputBufferComponent::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystemComponent = Cast<UWxAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->OnAbilityEnded.AddUObject(this, &UWxInputBufferComponent::HandleAbilityEnded);
	}
}

void UWxInputBufferComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearBufferedInputs();
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->OnAbilityEnded.RemoveAll(this);
	}
	AbilitySystemComponent = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UWxInputBufferComponent::RequestBufferedInputFlush()
{
	if (!AbilitySystemComponent || BufferedInputs.IsEmpty() || FlushTimerHandle.IsValid())
	{
		return;
	}
	// 타이머는 월드의 PostPhysics 이후 실행되며, 노티파이·종료 콜스택에 재진입하지 않는다.
	FlushTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::FlushBufferedInputs);
}

void UWxInputBufferComponent::InputActionTriggered(const UInputAction* Action)
{
	if (!Action || !AbilitySystemComponent)
	{
		return;
	}

	// 액션이 아닌 어빌리티(질주·락온)는 버퍼에 관여하지 않는다 — 거절 주체가 액션이 아니고, 락온 해제 입력을 기억하면 그 종료가 곧 재시도 지점이 되어 다시 켜진다.
	// 키 상태는 ASC가 이 호출에서 세우므로 그 전에 읽는다 — 이미 서 있으면 쥔 채 반복해서 들어온 홀드다.
	bool bAction = false;
	bool bHeld = false;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (!Ability || Ability->ActivationInputAction.Get() != Action)
		{
			continue;
		}

		bAction = bAction || Ability->GetAssetTags().HasTag(WxGameplayTags::Ability_Action);
		bHeld = bHeld || Spec.InputPressed;
	}

	if (AbilitySystemComponent->AbilityInputActionTriggered(Action))
	{
		// 액션이 성립했으면 쌓아 둔 입력은 전부 낡은 것이다 — 남겨 두면 같은 입력이 라이브와 재생으로 두 번 나간다.
		if (bAction)
		{
			ClearBufferedInputs();
		}
		return;
	}

	if (!bAction)
	{
		return;
	}

	const double Now = GetWorld()->GetRealTimeSeconds();
	for (int32 Index = 0; Index < BufferedInputs.Num(); ++Index)
	{
		if (BufferedInputs[Index].Action != Action)
		{
			continue;
		}

		// 쥔 동안은 자리를 지킨 채 나이만 갱신한다 — 매 프레임 뒤로 옮기면 그 뒤에 누른 탭을 밀어낸다. 나이는 사실상 뗀 뒤부터 센다.
		if (bHeld)
		{
			BufferedInputs[Index].TriggeredTime = Now;
			return;
		}

		BufferedInputs.RemoveAt(Index);
		break;
	}

	// 쥔 채 반복 진입인데 버퍼에 없으면 뒤에 누른 입력에 밀려난 것이다. 라이브 경로가 매 프레임 재시도하므로 다시 넣지 않는다.
	if (bHeld)
	{
		return;
	}

	while (BufferedInputs.Num() >= MaxBufferedInputs)
	{
		BufferedInputs.RemoveAt(0);
	}

	BufferedInputs.Add({Action, Now});
}

void UWxInputBufferComponent::FlushBufferedInputs()
{
	if (!AbilitySystemComponent)
	{
		FlushTimerHandle.Invalidate();
		return;
	}

	const double Now = GetWorld()->GetRealTimeSeconds();
	for (int32 Index = 0; Index < BufferedInputs.Num();)
	{
		if (Now - BufferedInputs[Index].TriggeredTime > BufferDuration)
		{
			BufferedInputs.RemoveAt(Index);
			continue;
		}

		// 실패한 항목은 다음 재시도 지점까지 남긴다. 콤보 창은 다음 타만 허용하므로, 거기서 버리면 같이 쌓인 회피가 후딜에 못 나간다.
		if (AbilitySystemComponent->TryActivateByInputAction(BufferedInputs[Index].Action))
		{
			ClearBufferedInputs();
			return;
		}

		++Index;
	}
	FlushTimerHandle.Invalidate();
}

void UWxInputBufferComponent::HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	// 종료 통지는 재발동(이전 인스턴스 종료 → 새 활성화)과 취소 경로 안에서 동기로 온다.
	// 새 인스턴스가 서기 전에 다른 입력이 끼어들지 않도록 공통 재생 시점에 맡긴다.
	RequestBufferedInputFlush();
}

void UWxInputBufferComponent::ClearBufferedInputs()
{
	BufferedInputs.Reset();
	GetWorld()->GetTimerManager().ClearTimer(FlushTimerHandle);
}
