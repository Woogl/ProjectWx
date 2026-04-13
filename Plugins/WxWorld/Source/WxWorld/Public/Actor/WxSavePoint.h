// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actor/WxSpawnableInterface.h"
#include "GameFramework/Actor.h"
#include "WxSavePoint.generated.h"

class UGameplayEffect;
class UStaticMeshComponent;
class UWxInteractionComponent;
class UWxPromptWidgetComponent;

/**
 * 세이브 포인트.
 * 플레이어가 상호작용하면 HP를 최대치로 회복시킨다.
 * HealEffect 프로퍼티에 HP를 MaxHP로 설정하는 GameplayEffect를 지정해야 한다.
 */
UCLASS(Abstract, meta = (PrioritizeCategories = "Wx"))
class WXWORLD_API AWxSavePoint : public AActor, public IWxSpawnableInterface
{
	GENERATED_BODY()

public:
	AWxSavePoint();

	// IWxSpawnableInterface
#if WITH_EDITOR
	virtual UStreamableRenderAsset* GetEditorPreviewMesh() const override;
#endif

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxPromptWidgetComponent> PromptWidget;

	/** 상호작용 시 적용할 회복 GameplayEffect. HP를 MaxHP로 설정하는 GE를 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<UGameplayEffect> HealEffect;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InteractingActor);
};
