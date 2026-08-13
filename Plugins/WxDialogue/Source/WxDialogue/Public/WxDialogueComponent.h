// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "WxInteractable.h"
#include "WxDialogueComponent.generated.h"

class UPrimitiveComponent;

/**
 * 대화 상대 컴포넌트. 이 컴포넌트를 붙이면 어떤 액터든 말을 걸 수 있는 대상이 된다 — 전용 C++ 액터 클래스가 필요 없다(기믹 컴포넌트와 같은 갈래).
 * 어느 노드에서 대화를 시작할지만 보유하고, 세션 진행은 상호작용한 플레이어 측(UWxDialogueSessionComponent)이 소유한다.
 *
 * 상호작용 계약(IWxInteractable)을 호스트 액터가 아니라 이 컴포넌트가 든다.
 * 덕분에 대화 상대를 다루는 코드('Enable Interaction' 태스크)가 호스트 액터 타입을 몰라도 되고, 호스트를 어느 모듈에 두든 무방하다.
 * 한 액터에 계약 구현체가 둘이면(기믹 컴포넌트와 함께 붙는 경우) IWxInteractable::Find 가 어느 쪽을 답할지 정해지지 않는다 — 모듈 경계상 지금은 만들 수 없는 조합이라, 필요해지면 그때 가른다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent))
class WXDIALOGUE_API UWxDialogueComponent : public UActorComponent, public IWxInteractable
{
	GENERATED_BODY()

public:
	const FDataTableRowHandle& GetStartRow() const;

	/** C++ 호스트가 생성자에서 한 번 부른다(순수 BP 호스트는 디테일 패널에서 고른다). */
	void SetAreaMesh(UPrimitiveComponent* Mesh);

	//~ Begin IWxInteractable — 쿼리 콜리전이 켜져 있는 동안 영역 메시가 영역, 상호작용자 세션에 대화 시작 위임, "Talk to {Name}" 프롬프트.
	virtual bool IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const override;

	/**
	 * 이 대상에게 말을 걸 수 있는지를 영역 메시의 쿼리 콜리전으로 토글한다. 'Enable Interaction' 태스크가 퀘스트 단계에 맞춰 호출한다.
	 * 별도의 상태 플래그를 두지 않는 이유는 콜리전이 이미 그 상태이기 때문이다 — 감지(스캐너의 구 오버랩)도 사거리 판정도 이 형상에 걸리므로, 끄면 후보에서 통째로 빠진다.
	 * 영역 메시는 몸통 충돌(캡슐)과 무관한 상호작용 감지 전용이라 꺼도 이동·물리에 영향이 없다.
	 * 잠긴 채로 시작해야 하는 대상(퀘스트가 탑재되기 전까지 말을 걸 수 없는 NPC)은 BP·배치 인스턴스에서 그 메시의 콜리전을 미리 꺼 둔다 — 시작 값을 담는 별도 프로퍼티는 두지 않는다.
	 * 복제하지 않는다 — 권위 측 퀘스트 러너만 이 값을 정하므로, 서버가 곧 클라인 싱글/리슨 호스트가 전제다(다른 퀘스트·대화 노드와 같은 전제).
	 */
	virtual void SetInteractionEnabled(bool bEnabled) override;

	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;
	virtual FText GetInteractionPrompt(const UActorComponent* Source) const override;
	//~ End IWxInteractable

protected:
	/** 비우면 대화가 시작되지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Dialogue", meta = (RowType = "/Script/WxDialogue.WxDialogueTableRow"))
	FDataTableRowHandle StartRow;

	/** 상호작용 프롬프트에 쓰인다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Dialogue")
	FText SpeakerName;

	/**
	 * 상호작용 영역이 되는 오너의 메시. 감지도 사거리도 이 형상으로 재므로 쿼리 콜리전은 켜 둔다.
	 * 오너에서 자동으로 찾지 않는 이유는 메타휴먼 부착물처럼 등록 시점에 생기는 메시가 있으면 무엇이 잡힐지 정해지지 않기 때문이다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|Dialogue")
	TObjectPtr<UPrimitiveComponent> AreaMesh;
};
