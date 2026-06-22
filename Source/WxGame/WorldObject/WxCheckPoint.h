// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "WxCheckPoint.generated.h"

class UGameplayEffect;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 체크 포인트.
 * 플레이어가 상호작용하면 HP를 최대치로 회복시킨다.
 * HealEffect 프로퍼티에 HP를 MaxHP로 설정하는 GameplayEffect를 지정해야 한다.
 *
 * 영속 State·StateTree 가 없는 반복형 즉시 효과(다크소울 모닥불)라 기믹 인프라(AWxGimmick) 대신 APlayerStart 를 상속한다.
 * APlayerStart 상속으로 배치된 인스턴스가 GameMode 의 ChoosePlayerStart 기본 흐름에서 부활 지점 후보가 된다.
 */
UCLASS(Abstract)
class AWxCheckPoint : public APlayerStart
{
	GENERATED_BODY()

public:
	AWxCheckPoint(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	/** 상호작용 시 적용할 회복 GameplayEffect. HP를 MaxHP로 설정하는 GE를 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<UGameplayEffect> HealEffect;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);
};
