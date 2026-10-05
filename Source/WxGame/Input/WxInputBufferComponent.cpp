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
}

void UWxInputBufferComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearBufferedInputs();
	AbilitySystemComponent = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UWxInputBufferComponent::InputActionTriggered(const UInputAction* Action)
{
	if (!Action || !AbilitySystemComponent)
	{
		return;
	}

	// 액션이 아닌 어빌리티(질주·락온)는 버퍼에 관여하지 않는다 — 거절 주체가 액션이 아니고, 락온 해제 입력을 기억하면 다시 켜진다.
	// 키 상태는 ASC가 이 호출에서 세우므로 그 전에 읽는다 — 이미 서 있으면 쥔 채 반복해서 들어온 홀드다.
	bool bAction = false;
	bool bHeld = false;
	FGameplayAbilitySpecHandle IntendedHandle;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (!Ability || Ability->ActivationInputAction.Get() != Action)
		{
			continue;
		}

		bAction = bAction || Ability->GetAssetTags().HasTag(WxGameplayTags::Ability_Action);
		bHeld = bHeld || Spec.InputPressed;

		// 발동 시도와 같은 부여 순서로, 소유자 태그 조건을 만족하는 첫 어빌리티가 이 입력이 노린 것이다.
		if (!IntendedHandle.IsValid() && Ability->DoesOwnerSatisfyActivationTags(*AbilitySystemComponent))
		{
			IntendedHandle = Spec.Handle;
		}
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

	// 쿨다운·비용으로 막힌 입력을 기억하면, 충전이나 자원이 돌아오는 순간 플레이어가 이미 그만둔 입력이 나간다.
	if (const FGameplayAbilitySpec* Intended = AbilitySystemComponent->FindAbilitySpecFromHandle(IntendedHandle))
	{
		const UGameplayAbility* Ability = Intended->GetPrimaryInstance() ? Intended->GetPrimaryInstance() : Intended->Ability.Get();
		const FGameplayAbilityActorInfo* ActorInfo = AbilitySystemComponent->AbilityActorInfo.Get();
		if (!Ability->CheckCooldown(IntendedHandle, ActorInfo) || !Ability->CheckCost(IntendedHandle, ActorInfo))
		{
			return;
		}
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

	// 타이머는 월드의 PostPhysics 이후 실행되므로, 루트모션 이동이나 노티파이·종료 콜스택 도중에 발동하지 않는다.
	if (!FlushTimerHandle.IsValid())
	{
		FlushTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::FlushBufferedInputs);
	}
}

void UWxInputBufferComponent::FlushBufferedInputs()
{
	// 실행 중인 단발 예약을 놓아야 다음 틱을 예약할 수 있다.
	FlushTimerHandle.Invalidate();
	if (!AbilitySystemComponent)
	{
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

		if (AbilitySystemComponent->TryActivateByInputAction(BufferedInputs[Index].Action))
		{
			ClearBufferedInputs();
			return;
		}

		++Index;
	}

	// 남은 입력은 발동되거나 만료될 때까지 매 틱 다시 시도한다.
	if (!BufferedInputs.IsEmpty())
	{
		FlushTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::FlushBufferedInputs);
	}
}

void UWxInputBufferComponent::ClearBufferedInputs()
{
	BufferedInputs.Reset();
	GetWorld()->GetTimerManager().ClearTimer(FlushTimerHandle);
}
