// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxInteractable.h"
#include "WxItemPickup.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;
class UWxItemDefinition;

/**
 * 아이템(또는 재화) 지급용 픽업.
 *
 * 메시 자체가 상호작용 영역이며 상시 활성이다 — 계약 인터페이스(WxCore)로 자기 메시를 답하므로 WxWorld 를 참조하지 않고도 스캐너에 잡힌다.
 * 응답·프롬프트도 본 액터가 IWxInteractable 로 제공하므로 BP 배선은 필요 없다.
 * 상호작용 시 Interactor 의 인벤토리에 ItemDef 를 지급한 뒤 파괴된다.
 *
 * 외부 스포너(예: UWxRewardLibrary::GrantReward) 가 SetItemDef 로 지급 데이터를 주입하고 LaunchInDirection 으로 물리 발사한다.
 */
UCLASS(Abstract)
class WXINVENTORY_API AWxItemPickup : public AActor, public IWxInteractable
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

	/** 서버 권한에서 픽업을 물리 발사한다. */
	void LaunchInDirection(const FVector& Direction, float Speed);

	//~ Begin IWxInteractable
	virtual bool IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const override;
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;
	virtual FText GetInteractionPrompt(const UActorComponent* Source) const override;
	//~ End IWxInteractable

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 픽업 외관용 나이아가라 이펙트. 시스템 에셋은 Pickup Fragment 에서 적용된다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	UPROPERTY(ReplicatedUsing = OnRep_ItemDef)
	TObjectPtr<UWxItemDefinition> ItemDef;

	/** 지급 수량. 최소 1. */
	UPROPERTY(Replicated)
	int32 Quantity = 1;

private:
	UFUNCTION()
	void OnRep_ItemDef();

	/** ItemDef 의 Pickup Fragment 데이터를 메시/나이아가라 컴포넌트에 동기 로드 후 적용한다. */
	void ApplyPickupVisual();
};
