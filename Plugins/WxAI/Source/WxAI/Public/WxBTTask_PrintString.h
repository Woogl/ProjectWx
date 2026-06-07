// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WxBTTask_PrintString.generated.h"

/**
 * BT Task: 지정한 메시지를 화면과 로그에 출력한다. (디버그/테스트 용도)
 *
 * 항상 Succeeded 를 반환하므로 트리 흐름을 그대로 통과시킨다.
 */
UCLASS()
class WXAI_API UWxBTTask_PrintString : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UWxBTTask_PrintString();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

protected:
	/** 출력할 메시지 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	FString Message = TEXT("Hello");

	/** 폰 이름을 메시지 앞에 붙일지 여부. 여러 폰을 동시에 테스트할 때 구분에 쓴다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	bool bPrependPawnName = true;

	/** 화면에 메시지가 표시되는 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ScreenDuration = 2.f;

	/** 화면 출력 색상 */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	FColor ScreenColor = FColor::Cyan;
};
