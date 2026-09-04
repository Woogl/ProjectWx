// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Spawnable/WxSpawnable.h"
#include "WxInteractable.h"
#include "Character/WxCharacterBase.h"
#include "WxEnemyCharacter.generated.h"

class AWxSpawner;
class UWxAIBehaviorComponent;
class UWxEnemyComponent;
class UWxLockOnPointComponent;
class UWxNameplateComponent;

/** 전투 AI의 필수 컴포넌트와 기본값만 조립하며, 런타임 기능과 상태는 각 컴포넌트가 소유한다. */
UCLASS(Abstract)
class WXGAME_API AWxEnemyCharacter : public AWxCharacterBase, public IWxSpawnable, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxEnemyCharacter(const FObjectInitializer& ObjectInitializer);

	//~ Begin IWxSpawnable
	virtual void OnSpawnedBy(AWxSpawner* Spawner) override;
	//~ End IWxSpawnable

	//~ Begin IWxInteractable
	virtual bool CanInteract(const AActor* Interactor) const override;
	virtual void OnInteracted(AActor* Interactor) override;
	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wx|AI")
	TObjectPtr<UWxAIBehaviorComponent> AIBehaviorComponent;

	/** 적 역할 로직을 소유하며, 이 조립 클래스가 외부 액터 계약을 컴포넌트에 전달한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Role")
	TObjectPtr<UWxEnemyComponent> EnemyComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx|UI")
	TObjectPtr<UWxNameplateComponent> NameplateComponent;

	/** 메시의 pelvis 본에 부착되어 카메라·캐릭터 시선과 레티클·호밍이 이 위치를 향한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|LockOn")
	TObjectPtr<UWxLockOnPointComponent> LockOnPoint;

};
