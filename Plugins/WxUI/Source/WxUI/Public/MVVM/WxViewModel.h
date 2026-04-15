// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "WxViewModel.generated.h"

UCLASS(Abstract, BlueprintType)
class WXUI_API UWxViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

	/** Shell 상태가 아닌 실제 데이터로 채워진 상태인지 여부. UI 바인딩용 FieldNotify */
	UFUNCTION(BlueprintPure, FieldNotify, Category = "Wx|ViewModel")
	bool IsInitialized() const;

protected:
	virtual void Deinitialize();

	/**
	 * 초기화 상태를 갱신하고 IsInitialized FieldNotify를 브로드캐스트한다.
	 * 호출 규약: 파생 VM은 자신의 Initialize 말미에 SetInitialized(true),
	 * Deinitialize 진입 시 SetInitialized(false)를 반드시 호출해야 한다.
	 */
	void SetInitialized(bool bInValue);

private:
	bool bInitialized = false;
};
