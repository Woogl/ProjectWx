// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WxBGMData.generated.h"

class USoundBase;

/**
 * 하나의 BGM 트랙 정의이자 Chooser 의 결과 타입.
 *
 * 선택은 Chooser 테이블이 담당하고, 트랙별 재생 파라미터(페이드/우선순위)는 곡 자신이 갖게 해
 * 곡마다 다른 페이드 길이를 줄 수 있다. 실제 루프는 Sound 에셋(Sound Cue/Wave 의 Looping)에서 켠다.
 */
UCLASS(BlueprintType)
class WXAUDIO_API UWxBGMData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|BGM")
	TObjectPtr<USoundBase> Sound;

	// 이 곡으로 전환될 때 페이드 인 시간(초).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|BGM", meta = (ClampMin = "0.0"))
	float FadeInTime = 2.f;

	// 이 곡에서 다른 곡으로 벗어날 때 페이드 아웃 시간(초).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|BGM", meta = (ClampMin = "0.0"))
	float FadeOutTime = 2.f;
};
