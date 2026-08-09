// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameplayEffectTypes.h"
#include "WxLockOnPointComponent.generated.h"

/**
 * 락온이 걸릴 위치를 표시하는 마커 컴포넌트.
 * 락온 대상은 이 컴포넌트 자체이고 카메라·캐릭터는 그 월드 위치를 조준하므로, 한 액터에 여러 개를 붙여 부위별 락온을 구성한다.
 * 이 컴포넌트가 없는 액터는 락온 대상이 될 수 없다.
 *
 * C++ 디폴트 서브오브젝트나 BP 컴포넌트 트리로 추가해야 네트워크 주소가 안정적이라 복제된 대상 레퍼런스가 원격에서 해소된다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent, DisplayName = "Wx Lock-On Point"))
class WXCOMBAT_API UWxLockOnPointComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UWxLockOnPointComponent();

	/**
	 * 소유 액터 ASC의 보유 태그를 LockOnRequirements로 판정한다.
	 * ASC가 없으면 빈 태그로 평가하므로 요구 태그가 없을 때만 통과한다.
	 */
	bool CanBeLockedOn() const;

	/**
	 * 락온 가능한 첫 지점을 반환한다.
	 * 초기 락온·타겟 상실 재탐색처럼 액터당 지점 하나면 충분한 경로에서 쓴다.
	 */
	static USceneComponent* ResolveLockOnTarget(const AActor* Actor);

	/** 같은 액터의 여러 부위를 후보로 비교해야 하는 경로(시선 재탐색)에서 쓴다 */
	static void GatherLockOnPoints(const AActor* Actor, TArray<USceneComponent*>& OutPoints);

protected:
	/** 이 지점에 락온하기 위한 대상 태그 조건 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTagRequirements LockOnRequirements;
};
