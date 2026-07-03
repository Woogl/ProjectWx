// Copyright Woogle. All Rights Reserved.

#include "WxBGMSourceComponent.h"
#include "System/WxMusicSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/World.h"

UWxBGMSourceComponent::UWxBGMSourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWxBGMSourceComponent::BeginPlay()
{
	Super::BeginPlay();

	// BGM 은 로컬 전용. 데디케이티드 서버에서는 아무 것도 하지 않는다.
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	// ActivationTag(구독 대상) 와 MusicTag(기여할 값) 가 모두 유효할 때만 동작한다.
	// 미설정(예: MusicTag 를 비운 트래시 몹)이면 아무 것도 등록하지 않아 선택을 오염시키지 않는다.
	if (!ActivationTag.IsValid() || !MusicTag.IsValid())
	{
		return;
	}

	// 소유자 ASC 를 부착 관계에서 자동 해석하고 ActivationTag 이벤트를 구독한다.
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}

	OwnerASC = ASC;
	ASC->RegisterGameplayTagEvent(ActivationTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UWxBGMSourceComponent::HandleActivationTagChanged);

	// 이미 태그가 켜져 있을 수 있으므로 초기 상태를 반영한다.
	UpdateRegistration(ASC->HasMatchingGameplayTag(ActivationTag));
}

void UWxBGMSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = OwnerASC.Get())
	{
		ASC->RegisterGameplayTagEvent(ActivationTag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
	}
	OwnerASC = nullptr;

	UpdateRegistration(false);

	Super::EndPlay(EndPlayReason);
}

void UWxBGMSourceComponent::HandleActivationTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	UpdateRegistration(NewCount > 0);
}

void UWxBGMSourceComponent::UpdateRegistration(bool bActive)
{
	if (bActive == bRegistered)
	{
		return;
	}

	const UWorld* World = GetWorld();
	UWxMusicSubsystem* Music = World ? World->GetSubsystem<UWxMusicSubsystem>() : nullptr;
	if (!Music)
	{
		return;
	}

	if (bActive)
	{
		Music->RegisterBGMSource(this, MusicTag);
	}
	else
	{
		Music->UnregisterBGMSource(this);
	}
	bRegistered = bActive;
}
