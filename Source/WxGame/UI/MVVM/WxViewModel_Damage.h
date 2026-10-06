// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "WxViewModel_Damage.generated.h"

/**
 * AWxDamageFloaterActor 가 피해 한 번마다 만들어 위젯에 넣는다.
 * 표시된 뒤 바뀌지 않으므로 FieldNotify 없이 두고, 위젯은 OneTime 바인딩으로만 읽는다 — OneWay 로 묶어도 갱신 통지는 오지 않는다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Damage : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Damage")
	float Damage = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Damage")
	bool bIsCritical = false;
};
