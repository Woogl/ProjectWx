// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/SoftObjectPath.h"
#include "WxSavable.h"
#include "WxPersistableReferencedActorComponent.generated.h"

/** LSP가 런타임 액터를 재생성해 이름이 달라져도 이전 세션 식별자로 다시 찾게 한다. */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXSAVE_API UWxPersistableReferencedActorComponent : public UActorComponent, public IWxSavable
{
	GENERATED_BODY()

public:
	UWxPersistableReferencedActorComponent();

	virtual void OnSavePreparing() override;
	virtual void OnSaveRestored(const TArray<FName>& RestoredPropertyNames) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Wx")
	FSoftObjectPath LastSessionLevelPath;

	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Wx")
	FName LastSessionActorName;

	bool bRegisteredWithManager = false;
};
