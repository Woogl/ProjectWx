// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxLockOnManagerComponent.generated.h"

class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnLockOnTargetChanged, USceneComponent*, NewTarget);

/**
 * 대상을 액터가 아닌 SceneComponent 단위로 들고 있어 부위별 락온으로 확장할 수 있고, 액터 전체를 가리키려면 루트 컴포넌트를 넘기면 된다.
 *
 * 발사체 방향·몽타주 스냅 등 서버와 시뮬프록시 소비처가 일관된 값을 읽어야 하므로 서버 권위로 전 머신에 복제한다.
 * 컴포넌트 레퍼런스 복제는 대상이 네트워크 주소를 가질 때만 원격에서 해소된다 — 디폴트 서브오브젝트는 안전하지만 동적 생성한 비복제 컴포넌트는 null로 도착할 수 있다.
 *
 * 소유 클라이언트는 응답성을 위해 로컬에 먼저 반영하고 서버에 권위 설정을 요청한 뒤, 복제값이 도착하면 정합한다.
 * 대상이 바뀌면 OnLockOnTargetChanged를 쏘므로 락온 태스크가 이를 구독해 추적 대상을 맞춘다.
 */
UCLASS()
class WXCOMBAT_API UWxLockOnManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxLockOnManagerComponent();

	static UWxLockOnManagerComponent* FindComponent(const AActor* Actor);

	/** nullptr을 넘기면 해제된다 */
	void SetLockOnTarget(USceneComponent* InTarget);

	USceneComponent* GetLockOnTarget() const;

	/** 락온 중이라 시점 회전에 쓰이지 않은 시선 입력을 기록한다. */
	void SetLookInput(const FVector2D& InLookInput);

	/** 입력이 없던 프레임에는 0이 반환된다. */
	FVector2D ConsumeLookInput();

	/**
	 * 로컬 예측·권위 적용·복제 도착에서 브로드캐스트된다.
	 * 셋 다 값이 실제로 바뀔 때만 불린다 — 복제 도착도 REPNOTIFY_OnChanged라 로컬 값과 같으면 RepNotify가 생략된다.
	 */
	UPROPERTY()
	FWxOnLockOnTargetChanged OnLockOnTargetChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION(Server, Reliable)
	void ServerSetLockOnTarget(USceneComponent* InTarget);

	UFUNCTION()
	void OnRep_LockOnTarget();

	void ApplyLockOnTarget(USceneComponent* InTarget);

	UPROPERTY(ReplicatedUsing = OnRep_LockOnTarget)
	TObjectPtr<USceneComponent> LockOnTarget;

	/** 로컬 조작 입력이라 복제하지 않는다. */
	FVector2D LookInput = FVector2D::ZeroVector;
};
