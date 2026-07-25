// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxInteractable.h"
#include "WxNpc.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UWxDialogueComponent;

/**
 * 대화 NPC.
 *
 * 메시 자체가 상호작용 영역이며 상시 활성이다 — 계약 인터페이스(WxCore)로 자기 메시를 답하므로 콜리전 프리셋·응답과 무관하게 스캐너에 잡힌다(사거리를 형상으로 재므로 쿼리 콜리전은 켜 둔다).
 * 상호작용 시 상호작용자 컨트롤러의 대화 세션에 자신의 대화 정의를 넘겨 대화를 연다.
 */
UCLASS(Abstract)
class WXDIALOGUE_API AWxNpc : public AActor, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxNpc();

	//~ Begin IWxInteractable — 상시 활성인 메시 하나가 영역, 상호작용자 세션에 대화 시작 위임, "[F] Talk to {Name}" 프롬프트.
	virtual void GetActiveInteractionMeshes(TArray<UPrimitiveComponent*>& OutMeshes) const override;
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;
	virtual FText GetInteractionPrompt(const UActorComponent* Source) const override;
	//~ End IWxInteractable

protected:
	/** 몸통 볼륨. 캐릭터와 동일하게 루트이며 충돌을 담당한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	/** 이 NPC 의 대화 정의. 시작 노드는 인스턴스별로 지정한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxDialogueComponent> DialogueComponent;

	/** NPC 표시 이름. 상호작용 프롬프트에 쓰인다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Dialogue")
	FText NpcName;
};
