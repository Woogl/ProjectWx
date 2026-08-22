// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxInteractable.generated.h"

class AActor;

/**
 * 상호작용 대상의 공용 계약. 대상 액터가 구현한다 — 컴포넌트는 구현하지 않는다.
 *
 * 능력이 컴포넌트에 담기더라도(대화·장치) 계약은 액터가 들고 그 컴포넌트로 넘긴다. 그래서 대상 하나당 구현체도 하나이고, 조회는 Cast 한 번이다.
 * 상호작용은 액터 단위다 — 액터 안의 특정 메시만 상호작용 영역이 되는 개념은 없다. 영역을 갈라야 하면 액터를 나눠 배치하고 각자 대상을 지목한다(예: 문에 달린 버튼은 별도 장치 액터이고 그 트리가 문을 민다).
 * 한 대상이 선택지 여럿(말 걸기·거래 등)을 내놓아야 하면, 계약을 컴포넌트로 되돌리는 것이 아니라 여기의 응답을 옵션 목록으로 키운다 — 그때도 액터가 컴포넌트들의 옵션을 모아 답한다.
 * 다만 감지와 사거리는 여전히 콜리전 형상 위에서 돈다. 액터에 쿼리 콜리전이 켜진 프리미티브가 하나도 없으면 스캔에도 사거리에도 걸리지 않는다(스켈레탈이면 피직스 애셋도 필요).
 * 계약이 WxCore 에 있으므로 소비 도메인(예: WxInventory 픽업)이 WxWorld 에 의존하지 않고도 자기 액터를 상호작용 대상으로 만들 수 있다.
 */
UINTERFACE(MinimalAPI, NotBlueprintable, meta = (CannotImplementInterfaceInBlueprint))
class UWxInteractable : public UInterface
{
	GENERATED_BODY()
};

class WXCORE_API IWxInteractable
{
	GENERATED_BODY()

public:
	virtual bool CanInteract() const;
	
	virtual void OnInteracted(AActor* Interactor) = 0;
	
	virtual FText GetInteractionPrompt() const = 0;
	
	/**
	 * '상호작용 켜기' 태스크가 액터를 지목했을 때 부른다.
	 * 켜고 끄는 수단은 구현체가 정한다 — IsInteractionEnabled 의 답이 그에 맞게 바뀌기만 하면 된다(대화 상대는 영역 메시의 쿼리 콜리전을 내린다).
	 * 여닫을 일이 없는 대상(픽업·적)은 구현하지 않는다 — 기본은 무동작이다.
	 */
	virtual void SetInteractionEnabled(bool bEnabled);
};
