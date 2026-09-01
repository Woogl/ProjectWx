// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxInteractable.h"
#include "WxDialogueActor.generated.h"

class USkeletalMeshComponent;
class UWxDialogueComponent;

/**
 * 말을 걸 수 있는 대상의 공통 호스트. 상호작용 계약을 들고 대화 컴포넌트로 넘긴다.
 *
 * 루트를 만들지 않는다 — 파생이 저마다 다른 몸통을 세운다(NPC 는 캡슐+스켈레탈, 말 거는 물체는 메시).
 * 계약은 액터 전용이라 컴포넌트가 들지 않는다.
 */
UCLASS(Abstract)
class WXDIALOGUE_API AWxDialogueActor : public AActor, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxDialogueActor();

	//~ Begin IWxInteractable
	virtual void OnInteracted(AActor* Interactor) override;
	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable

	/** 대사 포즈를 얹을 메시. 스켈레탈 메시가 없는 대상은 비운다. */
	virtual USkeletalMeshComponent* GetPoseMesh() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wx|Dialogue")
	TObjectPtr<UWxDialogueComponent> DialogueComponent;
};
