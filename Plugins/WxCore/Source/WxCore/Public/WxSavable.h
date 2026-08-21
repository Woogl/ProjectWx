// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "UObject/Interface.h"
#include "WxSavable.generated.h"

/**
 * 액터가 WxSave 의 슬롯 저장/로드 라이프사이클에 참여한다는 마커 + 후크. IWxInteractable 과 마찬가지로 액터만 구현한다.
 * 영속 상태를 컴포넌트가 들더라도 계약은 호스트 액터가 든다(예: 장치 상태 Tag 는 StateTree 컴포넌트의 SaveGame 필드지만 IWxSavable 은 AWxDevice 가 든다). 직렬화 대상은 어느 쪽이든 액터와 그 컴포넌트 전체다.
 *
 * 본 인터페이스를 구현한 액터는:
 *  - 체크포인트 저장 시: UPROPERTY(SaveGame) 필드가 GetSaveId() 의 WxSaveId 키로 슬롯에 기록된다. 액터도 컴포넌트도 전부 기본값이면 기록하지 않는다 — 복원해도 레벨이 세워 둔 값 그대로라 결과가 같다.
 *    이때는 액터 트랜스폼도 함께 빠지므로, SaveGame 필드 변화 없이 위치만 움직이는 액터는 그 변화를 SaveGame 필드로 들어야 한다.
 *  - 슬롯 로드 시: 동일 WxSaveId 의 기록을 찾으면 SaveGame 필드가 복원된 뒤 OnSaveRestored() 가 호출된다.
 *
 * 인터페이스 정의는 WxCore 에 위치하여, WxSave 와 WxSave 소비 도메인 (예: WxWorld) 이 서로 직접 의존하지 않게 한다.
 */
UINTERFACE(MinimalAPI, NotBlueprintable, meta = (CannotImplementInterfaceInBlueprint))
class UWxSavable : public UInterface
{
	GENERATED_BODY()
};

class WXCORE_API IWxSavable
{
	GENERATED_BODY()

public:
	/**
	 * WxSave 슬롯 레코드의 안정적 키. 에디터에서 1회 부여되어 에셋에 직렬화되므로 런타임·세션 간 불변이다.
	 * AActor::GetActorGuid() 는 에디터 전용(WITH_EDITOR) API 라 쿠킹 빌드에서 쓸 수 없어, 영속 UPROPERTY 로 키를 들고 간다.
	 * 유효하지 않은(IsValid()==false) 값을 반환하면 해당 액터는 저장/복원에서 제외된다.
	 */
	virtual FGuid GetSaveId() const = 0;

	/** SaveGame 필드 복원 직후 호출. BeginPlay 이전에 호출될 수 있다. */
	virtual void OnSaveRestored();
};
