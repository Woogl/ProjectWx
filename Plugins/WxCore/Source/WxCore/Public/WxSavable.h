// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxSavable.generated.h"

/**
 * LSP 직렬화 전 준비와 속성 복원 후처리가 필요한 오브젝트의 계약.
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
	/** LSP 런타임 액터 허용 목록에 들어 있어도 현재 인스턴스를 기록하지 않아야 하면 false를 반환한다. */
	virtual bool ShouldPersistRuntimeActor() const;

	/** LSP가 설정된 속성을 읽기 직전에 호출한다. 런타임 상태를 저장용 프로퍼티로 투영하는 자리다. */
	virtual void OnSavePreparing();

	/** LSP가 설정된 속성을 복원한 직후 호출한다. BeginPlay 이전일 수 있다. */
	virtual void OnSaveRestored(const TArray<FName>& RestoredPropertyNames);

	/** LSP가 레벨 속성과 런타임 액터 복원을 모두 마친 뒤 호출한다. 저장 데이터가 없는 첫 진입에도 호출된다. */
	virtual void OnPostRestoreLevel();
};
