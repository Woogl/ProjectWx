// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxRagdollComponent.generated.h"

/**
 * 래그돌 컴포넌트.
 *
 * EnableRagdoll() 호출 시 소유 캐릭터의 메시를 물리 시뮬레이션으로 전환.
 * 캡슐 콜리전 비활성화, 무브먼트 비활성화를 함께 처리.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class WXCOMBAT_API UWxRagdollComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxRagdollComponent();

	void EnableRagdoll();
	void DisableRagdoll();

	bool IsRagdollActive() const { return bRagdollActive; }

private:
	bool bRagdollActive = false;
};
