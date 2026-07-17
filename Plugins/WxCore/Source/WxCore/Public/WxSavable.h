// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "UObject/Interface.h"
#include "WxSavable.generated.h"

/**
 * 액터가 WxSave 의 슬롯 저장/로드 라이프사이클에 참여한다는 마커 + 후크.
 *
 * 본 인터페이스를 구현한 액터는:
 *  - 체크포인트 저장 시: UPROPERTY(SaveGame) 필드가 GetWxSaveId() 의 WxSaveId 키로 슬롯에 기록된다.
 *  - 슬롯 로드 시: 동일 WxSaveId 의 기록을 찾으면 SaveGame 필드가 복원된 뒤 OnWxSaveRestored() 가 호출된다.
 *
 * 필드 단위 직렬화는 UPROPERTY(SaveGame) 플래그가 처리하므로, 액터는 보존 필드에 해당 플래그만 표시하면 된다.
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

	/** SaveGame 필드 복원 직후 호출. 자식이 시각/인터랙션 동기화 후처리를 한다. BeginPlay 이전에 호출될 수 있다. */
	virtual void OnWxSaveRestored() {}
};
