// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/WxInteractableActor.h"
#include "WxItemPickup.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;
class UWxItemDefinition;

/**
 * 아이템(또는 재화) 지급용 픽업.
 *
 * 상호작용 시 Interactor 의 PlayerState(또는 본체) 에서
 * UWxInventoryManagerComponent 를 찾아 ItemDef 를 Count 만큼 지급한 뒤 파괴된다.
 *
 * 외부 스포너(예: 보물 상자) 가 LaunchInDirection() 으로 픽업을 물리 발사할 수 있다.
 */
UCLASS(Abstract)
class AWxItemPickup : public AWxInteractableActor
{
	GENERATED_BODY()

public:
	AWxItemPickup();

	/** 외부 스포너(예: 보물 상자) 가 스폰 직후 지급할 아이템을 주입할 때 사용. 서버 권한에서만 호출. */
	void Initialize(UWxItemDefinition* InItemDef);

	/** 서버 권한에서 픽업을 물리 발사한다. MeshComponent 의 물리 시뮬레이션을 활성화하고 선속도를 부여한다. */
	void LaunchInDirection(const FVector& Direction, float Speed);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 픽업 외관용 나이아가라 이펙트. 시스템 에셋은 BP에서 지정. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	/** 지급할 아이템 정의 */
	UPROPERTY(EditAnywhere, Category = "Wx|Pickup")
	TObjectPtr<UWxItemDefinition> ItemDef;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InteractingActor);
};
