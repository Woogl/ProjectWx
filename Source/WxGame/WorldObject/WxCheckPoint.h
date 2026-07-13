// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxCheckPoint.generated.h"

class UGameplayEffect;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 체크 포인트(다크소울 모닥불).
 * 플레이어가 상호작용하면 HP를 최대치로 회복시키고, 충전형 아이템을 리필하며, 적을 리스폰하고, 이 지점을 부활 지점으로 저장한다.
 * HealEffect 프로퍼티에 HP를 MaxHP로 설정하는 GameplayEffect를 지정해야 한다.
 *
 * AWxGimmick 자식이다. 상호작용 시 State 를 Lit(불 켜짐)으로 확정하며, 이 상태는 복제 + SaveGame 으로 지속된다(한 번 불이 붙으면 재로드 후에도 유지).
 * 불 켜짐 비주얼은 GimmickStateTree(자식 BP 에서 ST 에셋 할당)가 Lit 상태에서 적용한다.
 *
 * 부활 지점은 좌표(RespawnTransform)로 저장된다: 상호작용 시 자기 트랜스폼을 SaveSubsystem 에 세팅하고, 로드/사망 재개 시 AWxGameMode::SpawnDefaultPawnAtTransform 이 그 위치로 스폰한다.
 * 신규 세션(저장 없음) 시작지점은 체크포인트가 아니라 레벨에 배치된 일반 APlayerStart(엔진 기본 ChoosePlayerStart)가 담당한다.
 */
UCLASS(Abstract)
class AWxCheckPoint : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxCheckPoint();

protected:
	virtual void BeginPlay() override;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 태스크(불 켜짐 비주얼·인터랙션 토글)가 Context 액터의 컴포넌트로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	/** 상호작용 시 적용할 회복 GameplayEffect. HP를 MaxHP로 설정하는 GE를 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<UGameplayEffect> HealEffect;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);
};
