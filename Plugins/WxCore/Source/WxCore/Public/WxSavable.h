// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxSavable.generated.h"

/**
 * LSP가 액터 또는 소유 컴포넌트의 설정된 속성을 복원한 뒤 후처리가 필요한 액터의 계약.
 * 식별과 바이트 직렬화는 Level Streaming Persistence가 오브젝트 경로를 기준으로 담당한다.
 * 인터페이스는 WxCore에 두어 WxSave와 소비 도메인이 서로 직접 의존하지 않게 한다.
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
	/** LSP 설정에 포함된 액터/소유 컴포넌트 속성 복원 직후 호출. BeginPlay 이전일 수 있다. */
	virtual void OnSaveRestored();
};
