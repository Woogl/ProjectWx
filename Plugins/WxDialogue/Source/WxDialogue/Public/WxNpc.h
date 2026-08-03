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
 * 메시 자체가 상호작용 영역이다 — 계약 인터페이스(WxCore)로 자기 메시를 답하므로 콜리전 프리셋·응답과 무관하게 스캐너에 잡힌다(사거리를 형상으로 재므로 쿼리 콜리전은 켜 둔다).
 * 그 쿼리 콜리전이 곧 영역의 on/off 이기도 하다 — 퀘스트가 단계에 맞춰 껐다 켠다.
 * 상호작용 시 상호작용자 컨트롤러의 대화 세션에 자신의 대화 정의를 넘겨 대화를 연다.
 */
UCLASS(Abstract)
class WXDIALOGUE_API AWxNpc : public AActor, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxNpc();

	//~ Begin IWxInteractable — 쿼리 콜리전이 켜져 있는 동안 메시 하나가 영역, 상호작용자 세션에 대화 시작 위임, "[F] Talk to {Name}" 프롬프트.
	virtual bool IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const override;
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;
	virtual FText GetInteractionPrompt(const UActorComponent* Source) const override;
	//~ End IWxInteractable

	/**
	 * 이 NPC 에게 말을 걸 수 있는지를 영역 메시의 쿼리 콜리전으로 토글한다. 'Enable Npc Interaction' 태스크가 퀘스트 단계에 맞춰 호출한다.
	 * 별도의 상태 플래그를 두지 않는 이유는 콜리전이 이미 그 상태이기 때문이다 — 감지(스캐너의 구 오버랩)도 사거리 판정도 이 형상에 걸리므로, 끄면 후보에서 통째로 빠진다.
	 * 이 메시는 몸통 충돌(캡슐)과 무관한 상호작용 감지 전용이라 꺼도 이동·물리에 영향이 없다.
	 * 잠긴 채로 시작해야 하는 NPC(퀘스트가 탑재되기 전까지 말을 걸 수 없는 NPC)는 BP·배치 인스턴스에서 그 메시의 콜리전을 미리 꺼 둔다 — 시작 값을 담는 별도 프로퍼티는 두지 않는다.
	 * 복제하지 않는다 — 권위 측 퀘스트 러너만 이 값을 정하므로, 서버가 곧 클라인 싱글/리슨 호스트가 전제다(다른 퀘스트·대화 노드와 같은 전제).
	 */
	void SetInteractionEnabled(bool bEnabled);

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
