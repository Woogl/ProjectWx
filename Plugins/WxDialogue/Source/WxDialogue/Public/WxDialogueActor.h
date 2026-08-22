// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxInteractable.h"
#include "WxDialogueActor.generated.h"

class UWxDialogueComponent;

/**
 * 말을 걸 수 있는 대상의 공통 호스트. 상호작용 계약을 들고 대화 컴포넌트로 넘긴다.
 *
 * 루트를 만들지 않는다 — 파생이 저마다 다른 몸통을 세운다(NPC 는 캡슐+스켈레탈, 말 거는 물체는 메시).
 * 계약은 액터 전용이라 컴포넌트가 들지 않는다. 대화 말고 다른 상호작용(거래 등)이 같은 대상에 붙어도 구현체는 이 액터 하나로 남고, 능력만 컴포넌트로 조립된다.
 */
UCLASS(Abstract)
class WXDIALOGUE_API AWxDialogueActor : public AActor, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxDialogueActor();

	//~ Begin IWxInteractable
	virtual bool CanInteract() const override;

	/**
	 * 복제하지 않는다 — 권위 측 퀘스트 러너만 이 값을 정하므로, 서버가 곧 클라인 싱글/리슨 호스트가 전제다(다른 퀘스트·대화 노드와 같은 전제).
	 */
	virtual void SetInteractionEnabled(bool bEnabled) override;

	virtual void OnInteracted(AActor* Interactor) override;
	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable

protected:
	/** 이 대상의 대화 정의이자 세션 진입점. 시작 노드·표시 이름은 인스턴스별로 지정한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Dialogue")
	TObjectPtr<UWxDialogueComponent> DialogueComponent;

	/**
	 * 지금 말을 걸 수 있는가. 잠긴 채로 시작해야 하는 대상(퀘스트가 탑재되기 전까지 말을 걸 수 없는 NPC)은 BP·배치 인스턴스에서 미리 끈다.
	 * 감지·사거리는 몸통 형상이 재므로 이 값과 무관하다 — 꺼도 스캔에는 걸리고 활성 판정에서 탈락한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|Dialogue")
	bool bInteractionEnabled = true;
};
