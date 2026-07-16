// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CommonLazyImage.h"
#include "CoreMinimal.h"

#include "WxLazyImage.generated.h"

class UTexture2D;

/**
 * MVVM 바인딩 친화적 1-arg setter 를 추가한 UCommonLazyImage 확장.
 *
 * UE 5.7 의 MVVM 바인딩 픽커는 인자가 1개인 함수만 노출하기 때문에, 부모의 SetBrushFromLazyTexture(SoftPtr, bool) 같은 다인자 함수는 픽커에서 보이지 않는다.
 * 이 클래스는 bMatchSize=false 가 고정된 1-arg 래퍼를 제공해 MVVM 바인딩 타겟으로 쓰일 수 있게 한다.
 * 비동기 로드/취소/throbber 처리는 LazyImage 의 기본 동작에 그대로 위임된다.
 *
 * 사용법(WBP):
 *   1) Image 자리에 본 위젯 배치
 *   2) View Bindings: SetLazyTexture(함수) <- ViewModel 의 TSoftObjectPtr<UTexture2D> 필드
 */
UCLASS()
class WXUI_API UWxLazyImage : public UCommonLazyImage
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetLazyTexture(const TSoftObjectPtr<UTexture2D>& LazyTexture);
};
