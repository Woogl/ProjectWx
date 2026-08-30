// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxDialogueActor.h"
#include "WxNpc.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UWxMetaHumanComponent;

/**
 * 대화 NPC.
 *
 * 폰이 아닌 액터다 — 이동·AI 가 없어도 되는 대화 상대라, 캐릭터와 같은 캡슐+메시 구성만 직접 갖춘다.
 * 대화와 상호작용 계약은 베이스가 든다. 이 클래스가 하는 일은 몸통을 세우고 외형 컴포넌트를 얹는 합성과, 말을 걸 수 있는 순간을 정하는 것뿐이다.
 * 말을 걸 수 있는 순간은 퀘스트가 이 NPC 의 상호작용을 기다리는 동안이다 — 대화 도메인은 퀘스트 대기(WxWorld)를 볼 수 없어 두 도메인이 만나는 여기서 답한다.
 * 합성 대상이 대화(WxDialogue)와 외형(WxGame)에 걸쳐 있어 게임 모듈에 둔다.
 */
UCLASS(Abstract)
class WXGAME_API AWxNpc : public AWxDialogueActor
{
	GENERATED_BODY()

public:
	AWxNpc();

	//~ Begin IWxInteractable
	virtual bool CanInteract(const AActor* Interactor) const override;
	//~ End IWxInteractable

	//~ Begin AWxDialogueActor
	virtual USkeletalMeshComponent* GetPoseMesh() const override;
	//~ End AWxDialogueActor

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	/** 상호작용 감지·사거리를 재는 형상이기도 하다 — 그래서 쿼리 콜리전을 켜 둔다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx|Visual")
	TObjectPtr<UWxMetaHumanComponent> MetaHumanComponent;
};
