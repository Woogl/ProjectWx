// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxLockOnComponent.generated.h"

class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnLockOnTargetChanged, USceneComponent*, NewTarget);

/**
 * 락온 대상을 관리하는 복제 컴포넌트.
 * 캐릭터에 부착하여 현재 락온 중인 컴포넌트(부위)를 저장/조회한다.
 * 대상을 액터가 아닌 SceneComponent 단위로 들고 있어, 부위별 락온(특정 본/소켓 컴포넌트 추적)으로 확장할 수 있다.
 * 대상 액터 전체를 가리키려면 루트 컴포넌트를 넘기면 되며(루트 위치 = 액터 위치), 소유 액터는 GetOwner()로 얻는다.
 *
 * 락온 대상은 서버 권위로 전 머신에 복제된다(발사체 방향/몽타주 스냅 등 서버·시뮬프록시 소비처가 일관된 값을 읽어야 하기 때문).
 * 단, 컴포넌트 레퍼런스 복제는 대상 컴포넌트가 네트워크 주소를 가질 때만 원격에서 해소된다.
 * 캐릭터 루트(캡슐)·메시 같은 디폴트 서브오브젝트는 안전하지만, 동적 생성한 비복제 컴포넌트는 원격에서 null로 도착할 수 있다.
 * 소유 클라이언트는 응답성을 위해 SetLockOnTarget 시 로컬에 즉시 반영(예측)한 뒤 서버에 권위 설정을 요청하고, 복제값이 도착하면 정합한다.
 * 대상 변경 시 OnLockOnTargetChanged 를 브로드캐스트하므로, 락온 태스크가 이를 구독해 클라/서버 양쪽에서 추적 대상을 동기화한다.
 */
UCLASS()
class WXCOMBAT_API UWxLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxLockOnComponent();

	/** 액터에서 UWxLockOnComponent를 찾아 반환. 없으면 nullptr */
	static UWxLockOnComponent* FindComponent(const AActor* Actor);

	/**
	 * 락온 대상 설정/해제. nullptr로 해제.
	 * 권위 측에서 호출하면 즉시 권위 반영(복제). 소유 클라에서 호출하면 로컬 즉시 반영 후 서버에 권위 설정을 요청한다.
	 */
	void SetLockOnTarget(USceneComponent* InTarget);

	/** 현재 락온 대상 컴포넌트 반환. 없으면 nullptr. 소유 액터는 반환값의 GetOwner()로 얻는다. */
	USceneComponent* GetLockOnTarget() const;

	/** 락온 대상 변경 시(로컬 예측·권위 적용·복제 도착 모두) 브로드캐스트된다. */
	UPROPERTY()
	FWxOnLockOnTargetChanged OnLockOnTargetChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** 소유 클라의 락온 선택을 서버에 권위 설정한다. */
	UFUNCTION(Server, Reliable)
	void ServerSetLockOnTarget(USceneComponent* InTarget);

	UFUNCTION()
	void OnRep_LockOnTarget();

	/** 대상 값을 적용하고(변경 시) 델리게이트를 브로드캐스트한다. 권위/예측 양쪽의 공통 경로. */
	void ApplyLockOnTarget(USceneComponent* InTarget);

	UPROPERTY(ReplicatedUsing = OnRep_LockOnTarget)
	TObjectPtr<USceneComponent> LockOnTarget;
};
