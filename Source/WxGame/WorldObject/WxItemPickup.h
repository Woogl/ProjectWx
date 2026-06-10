// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxItemPickup.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;
class UWxItemDefinition;

/**
 * 아이템(또는 재화) 지급용 픽업.
 *
 * 상호작용 시 Interactor 의 PlayerController(또는 Interactor 본체) 에서
 * UWxInventoryManagerComponent 를 찾아 ItemDef 를 지급한 뒤 파괴된다.
 *
 * 외부 스포너(예: 보물 상자) 가 LaunchInDirection() 으로 픽업을 물리 발사할 수 있다.
 */
UCLASS(Abstract)
class AWxItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AWxItemPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 외부 스포너(예: 보물 상자) 가 SpawnActorDeferred → FinishSpawning 사이에 지급할 아이템과 수량을 주입할 때 사용. 서버 권한에서만 호출. */
	void SetItemDef(UWxItemDefinition* InItemDef, int32 InQuantity = 1);

	/** 서버 권한에서 픽업을 물리 발사한다. MeshComponent 의 물리 시뮬레이션을 활성화하고 선속도를 부여한다. */
	void LaunchInDirection(const FVector& Direction, float Speed);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	/** 픽업 외관용 나이아가라 이펙트. 시스템 에셋은 BP에서 지정. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	/** 지급할 아이템 정의. 외부 스포너가 SetItemDef 로 주입한다. */
	UPROPERTY(ReplicatedUsing = OnRep_ItemDef)
	TObjectPtr<UWxItemDefinition> ItemDef;

	/** 지급 수량. 외부 스포너가 SetItemDef 로 주입한다. 최소 1. */
	UPROPERTY(Replicated)
	int32 Quantity = 1;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InteractingActor);

	UFUNCTION()
	void OnRep_ItemDef();

	/** ItemDef->DisplayName 을 사용해 인터랙션 프롬프트 텍스트를 "[F] {DisplayName}" 으로 갱신한다. */
	void UpdateInteractionText();

	/** ItemDef 의 Pickup Fragment 데이터를 메시/나이아가라 컴포넌트에 동기 로드 후 적용한다. */
	void ApplyPickupVisual();
};
