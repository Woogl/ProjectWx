// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxDoor.generated.h"

class UStaticMeshComponent;

/**
 * 개폐 문.
 * 콘솔과 상호작용하면 양쪽 문이 반대 방향으로 슬라이드하며 열린다.
 * 구조상 다시 닫을 수도 있으나(Open ──상호작용──> Close), 현재는 Open 상태의 인터랙션을 에셋에서 비활성화해 단방향(열기 전용)으로 동작한다.
 *
 * 상태는 자체 State 태그(Gimmick.Door.*)가 권위 원천이며, 복제·SaveGame 으로 보존된다.
 * State 는 "여닫는 확정 목표"라 상호작용 시점에 곧장 최종값(Open/Close)으로 확정되고, 슬라이드 애니는 StateTree 의 Wx Component Move 가 그 목표를 향한 순수 비주얼로 처리한다(이동할 문 메시·오프셋은 ST_Door 에셋에서 author).
 *
 *   Close (초기) ──상호작용──> Open ──(양방향 시)상호작용──> Close
 *
 * 전이는 ST_Door 의 각 상태 Required Event to Enter(State 태그) 가 구동한다 — State 가 바뀌면 베이스가 그 태그를 ST 이벤트로 보내고, Root 의 재선택 전이가 열린 선택에서 그 상태가 자신을 고른다.
 * State 는 슬라이드와 무관하게 즉시 확정되며, 서버·클라 동일.
 * 시작 시엔 기본 상태(Close)가 Required Event 없이 선택되고(그래서 Root 자식 중 마지막에 둔다), 복원 시엔 저장된 State 태그가 StateTree.Restore 마커와 함께 발행돼 그 상태로 스냅 진입한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxDoor : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxDoor();

	//~ Begin IWxInteractable — 상호작용 시 현재 State 의 반대 목표(Open/Close)로 확정(프롬프트는 베이스 InteractionPrompt).
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;
	//~ End IWxInteractable

protected:

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Component Move 가 Context 액터의 컴포넌트로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorLeft;

	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorRight;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Console;
};
