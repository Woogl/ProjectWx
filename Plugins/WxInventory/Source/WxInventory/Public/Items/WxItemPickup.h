// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxInteractable.h"
#include "WxSavable.h"
#include "WxItemPickup.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;
class UWxItemDefinition;

USTRUCT()
struct FWxPersistedItemPickupState
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<UWxItemDefinition> ItemDefinition;

	UPROPERTY()
	int32 Quantity = 1;

	UPROPERTY()
	bool bSimulatingPhysics = false;

	UPROPERTY()
	FVector LinearVelocity = FVector::ZeroVector;

	UPROPERTY()
	FVector AngularVelocityRadians = FVector::ZeroVector;
};

/**
 * 아이템(또는 재화) 지급용 픽업.
 *
 * 상시 활성이며, 쿼리 콜리전이 켜진 메시 덕에 스캐너에 잡힌다 — 계약이 WxCore 에 있어 WxWorld 를 참조하지 않고도 대상이 된다.
 * 상호작용 시 Interactor 의 인벤토리에 ItemDef 를 지급한 뒤 파괴된다.
 *
 * 외부 스포너(예: UWxRewardLibrary::GrantReward) 가 SetItemDef 로 지급 데이터를 주입하고 LaunchInDirection 으로 물리 발사한다.
 */
UCLASS(Abstract)
class WXINVENTORY_API AWxItemPickup : public AActor, public IWxInteractable, public IWxSavable
{
	GENERATED_BODY()

public:
	AWxItemPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 외부 스포너가 SpawnActorDeferred → FinishSpawning 사이에 지급할 아이템과 수량을 주입할 때 사용.
	 * 서버 권한에서만 호출.
	 */
	void SetItemDef(UWxItemDefinition* InItemDef, int32 InQuantity = 1);

	/** 서버 권한에서만 동작한다. */
	void LaunchInDirection(const FVector& Direction, float Speed);

	//~ Begin IWxInteractable
	virtual void OnInteracted(AActor* Interactor) override;
	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable

	//~ Begin IWxSavable
	virtual void OnSavePreparing() override;
	virtual void OnSaveRestored(const TArray<FName>& RestoredPropertyNames) override;
	//~ End IWxSavable

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 시스템 에셋은 Pickup Fragment 에서 적용된다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	UPROPERTY(ReplicatedUsing = OnRep_ItemDef)
	TObjectPtr<UWxItemDefinition> ItemDef;

	/** 최소 1. */
	UPROPERTY(Replicated)
	int32 Quantity = 1;

	/** 런타임 생성 픽업을 LSP가 다시 구성하는 데 필요한 동적 상태. */
	UPROPERTY(VisibleInstanceOnly, Category = "Wx|Persistence")
	FWxPersistedItemPickupState PersistedState;

private:
	UFUNCTION()
	void OnRep_ItemDef();

	void ApplyPickupVisual();
};
