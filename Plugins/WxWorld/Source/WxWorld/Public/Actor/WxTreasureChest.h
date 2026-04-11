// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxTreasureChest.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;
class UWxInteractionComponent;

/**
 * 보물 상자 (테스트용).
 * 플레이어가 InteractionComponent 범위에 진입하면 프롬프트 위젯이 표시되고,
 * 상호작용 입력 시 서버에서 로그만 출력한다. (실제 보상 로직은 추후 구현)
 */
UCLASS(meta = (PrioritizeCategories = "Wx"))
class WXWORLD_API AWxTreasureChest : public AActor
{
	GENERATED_BODY()

public:
	AWxTreasureChest();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWidgetComponent> PromptWidget;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InteractingActor);
};
