// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateSourceComponent.h"

TArray<TWeakObjectPtr<UWxNameplateSourceComponent>> UWxNameplateSourceComponent::RegisteredComponents;

UWxNameplateSourceComponent::UWxNameplateSourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

const TArray<TWeakObjectPtr<UWxNameplateSourceComponent>>& UWxNameplateSourceComponent::GetRegisteredComponents()
{
	return RegisteredComponents;
}

void UWxNameplateSourceComponent::BeginPlay()
{
	Super::BeginPlay();

	// NameplateManager와 대상 중 누가 먼저 생길지 모르므로, 대상은 목록에 올려 두기만 하고 NameplateManager가 매번 목록을 읽는다.
	RegisteredComponents.Add(this);
}

void UWxNameplateSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RegisteredComponents.RemoveSwap(this);

	Super::EndPlay(EndPlayReason);
}
