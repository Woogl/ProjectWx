// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameplayEffectTypes.h"
#include "WxLockOnPointComponent.generated.h"

/**
 * 락온 지점 컴포넌트.
 * 액터에서 "락온이 걸릴 위치"를 표시하는 마커다. 락온 대상은 이 컴포넌트 자체가 되며,
 * 카메라·캐릭터가 바라보는 지점은 이 컴포넌트의 월드 위치다(부위 위치에 그대로 조준·추적).
 * 한 액터에 붙여 부위별 락온을 구성한다. 이 컴포넌트가 없는 액터는 락온 대상이 될 수 없다.
 *
 * 배치: C++ 디폴트 서브오브젝트 또는 BP 컴포넌트 트리(SCS)로 추가한다.
 * 두 경우 모두 네트워크 주소가 안정적이라 UWxLockOnManagerComponent가 복제하는 대상 레퍼런스가 원격에서 정상 해소된다.
 * 런타임에 NewObject로 동적 생성한 지점은 복제 등록을 하지 않는 한 원격(서버 권위 소비처/시뮬프록시)에서 null로 도착하니 주의한다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent, DisplayName = "Wx Lock-On Point"))
class WXCOMBAT_API UWxLockOnPointComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UWxLockOnPointComponent();

	/**
	 * 이 지점이 지금 락온 대상이 될 수 있는지 평가한다.
	 * 소유 액터 ASC의 보유 태그를 LockOnRequirements 로 판정한다(Must Have / Must Not Have / Query).
	 * ASC가 없으면 빈 태그로 평가한다(요구 태그가 없을 때만 가능). 구체 태그는 Requirements 가 정하며 본 함수는 모른다.
	 */
	bool CanBeLockedOn() const;

	/**
	 * 액터를 락온할 때 대상이 될 대표 SceneComponent를 반환한다.
	 * 락온 가능한 첫 지점을, 없으면 nullptr을 반환한다(지점이 없거나 모두 불가하면 락온 대상이 될 수 없다).
	 * 초기 락온·타겟 상실 재탐색처럼 액터당 지점 하나면 충분한 경로에서 쓴다.
	 */
	static USceneComponent* ResolveLockOnTarget(const AActor* Actor);

	/**
	 * 액터의 락온 가능한 지점을 모두 수집한다(부위별 락온 후보).
	 * 마우스 재탐색처럼 같은 액터의 여러 부위를 후보로 비교해야 하는 경로에서 쓴다.
	 */
	static void GatherLockOnPoints(const AActor* Actor, TArray<USceneComponent*>& OutPoints);

protected:
	/** 이 지점에 락온하기 위한 대상 태그 조건. Must Have(HasAll)·Must Not Have(HasAny면 불가)·Query. 기본 Must Not Have = State.Dead. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTagRequirements LockOnRequirements;
};
