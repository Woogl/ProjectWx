// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxLockOnComponent.generated.h"

class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnLockOnTargetChanged, USceneComponent*, NewTarget);

/**
 * 캐릭터가 겨누는 대상을 담는다 — 플레이어는 락온 어빌리티가, AI 는 AWxAIController 가 퍼셉션 타겟을 채운다.
 * 대상을 액터가 아닌 SceneComponent 단위로 들고 있어 부위별 락온으로 확장할 수 있다.
 * 락온 지점(UWxLockOnPointComponent)은 플레이어 락온의 대상 계약이며, AI 는 겨누는 액터의 루트 컴포넌트를 넣는다.
 *
 * 발사체 방향·몽타주 스냅·타겟팅 필터 등 서버와 시뮬프록시 소비처가 일관된 값을 읽어야 하므로 서버 권위로 전 머신에 복제한다.
 * 컴포넌트 레퍼런스 복제는 대상이 네트워크 주소를 가질 때만 원격에서 해소된다 — 디폴트 서브오브젝트는 안전하지만 동적 생성한 비복제 컴포넌트는 null로 도착할 수 있다.
 *
 * 소유 클라이언트는 응답성을 위해 로컬에 먼저 반영하고 서버에 권위 설정을 요청한다.
 * 대상 선택은 클라이언트를 신뢰하는 정책이라 서버는 요청을 재검증하지 않으며, 사망·거리 이탈 등으로 대상이 무효해지는 것은 소유 클라의 락온 태스크가 폴링해 해제·재탐색을 다시 올려 보낸다.
 */
UCLASS()
class WXCOMBAT_API UWxLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxLockOnComponent();

	/**
	 * 소스가 겨누는 액터를 묻는 소비처용 조회. 컴포넌트가 없거나 대상이 없으면 nullptr.
	 * 조준 지점을 돌려주는 UWxLockOnPointComponent::ResolveLockOnTarget 과 짝이며, 이쪽은 그 지점의 액터를 답한다.
	 */
	static AActor* ResolveLockOnTargetActor(const AActor* Source);

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
	 *
	 * 대상이 세터를 거치지 않고 무효해지는 경우(예: 원격에서 대상 액터가 릴러번시를 잃어 파괴)에는 통지가 없다.
	 * GetLockOnTarget 은 그때도 null 을 답하므로, 그 상태까지 따라가야 하는 표시는 통지가 아니라 질의로 갱신한다.
	 */
	UPROPERTY()
	FWxOnLockOnTargetChanged OnLockOnTargetChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** 소유 클라가 고른 대상을 그대로 반영한다 — 대상은 락온 지점(UWxLockOnPointComponent)이라는 계약도 클라가 지킨다. */
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
