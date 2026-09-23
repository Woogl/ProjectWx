// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxNameplateSourceComponent.generated.h"

/**
 * Nameplate를 붙일 수 있는 대상임을 알린다. 붙일지와 위치, 위젯 생성은 로컬 플레이어의 UWxNameplateManagerComponent가 정한다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXUI_API UWxNameplateSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxNameplateSourceComponent();

	/** NameplateManager가 훑는 목록이다. PIE에서는 여러 월드의 것이 섞이므로 쓰는 쪽이 월드를 거른다. */
	static const TArray<TWeakObjectPtr<UWxNameplateSourceComponent>>& GetRegisteredComponents();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	static TArray<TWeakObjectPtr<UWxNameplateSourceComponent>> RegisteredComponents;
};
