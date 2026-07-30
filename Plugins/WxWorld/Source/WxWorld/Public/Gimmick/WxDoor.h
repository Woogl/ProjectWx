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
 * 동작은 전부 ST_Door 이 정한다 — 각 상태가 자기 인터랙션 가용·프롬프트·문 위치를 선언하고, 상호작용 이벤트를 받는 전이가 다음 상태를 지목한다.
 * 이 클래스는 메시 셋만 들고 있으며, 슬라이드는 Component Move 가 순수 비주얼로 처리한다(이동할 문 메시·오프셋은 에셋에서 author).
 *
 *   Close (초기) ──상호작용──> Open ──(양방향 시)상호작용──> Close
 *
 * 상태의 Tag 가 곧 저장 값이라, 복원 시엔 그 상태에서 트리가 열려 열린 문은 열린 채로 시작한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxDoor : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxDoor();

protected:

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Component Move 가 Context 액터의 컴포넌트로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorLeft;

	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorRight;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Enable Interaction 이 토글 대상으로, 전이 조건이 상호작용 영역 비교 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Console;
};
