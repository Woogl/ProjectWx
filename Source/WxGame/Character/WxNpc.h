// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxNpc.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UWxDialogueComponent;
class UWxMetaHumanComponent;

/**
 * 대화 NPC.
 *
 * 폰이 아닌 액터다 — 이동·AI 가 없어도 되는 대화 상대라, 캐릭터와 같은 캡슐+메시 구성만 직접 갖춘다.
 * 대화와 상호작용 계약은 대화 컴포넌트가 든다. 이 클래스가 하는 일은 몸통을 세우고 그 컴포넌트들을 얹는 합성뿐이다.
 * 합성 대상이 대화(WxDialogue)와 외형(WxGame)에 걸쳐 있어 게임 모듈에 둔다.
 */
UCLASS(Abstract)
class WXGAME_API AWxNpc : public AActor
{
	GENERATED_BODY()

public:
	AWxNpc();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	/** 이 메시가 곧 상호작용 영역이다 — 대화 컴포넌트에 영역으로 넘긴다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	/** 이 NPC 의 대화 정의이자 상호작용 계약. 시작 노드·표시 이름은 인스턴스별로 지정한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxDialogueComponent> DialogueComponent;

	/** 메타휴먼 부착물(페이스·그룸·복장)을 바디 메시에 조립하는 컴포넌트. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Visual")
	TObjectPtr<UWxMetaHumanComponent> MetaHumanComponent;
};
