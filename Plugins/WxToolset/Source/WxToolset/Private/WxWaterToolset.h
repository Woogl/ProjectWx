// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "WxWaterToolset.generated.h"

class AWaterBody;

/**
 * 물 바디 스플라인을 통째로 바꾸는 것만 뚫는다 — ObjectTools 로 SplineCurves 를 쓰면 위치·회전·배율 점 개수가 잠깐 어긋나 엔진 검사에 걸려 에디터가 멈춘다.
 * 배치·머티리얼·지형 변형(bAffectsLandscape) 같은 일반 속성은 SceneTools·ObjectTools 로 다룬다. 닫힘 여부는 물 바디 종류(호수·바다·강)가 정한다.
 */
UCLASS(BlueprintType, Hidden)
class UWxWaterToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 물 바디 스플라인 점을 통째로 바꾸고 점마다 강 폭·깊이를 쓴다.
	 * @param Points 액터 기준 로컬 위치(cm). 강은 Z 가 그 점의 수면 높이다.
	 * @param Widths 점마다 강 폭(cm). 비우면 물 바디 기본값을 쓴다.
	 * @param Depths 점마다 깊이(cm). 비우면 물 바디 기본값을 쓴다.
	 * @param bLinear 점 사이를 직선으로 잇는다(촘촘한 해안선). 거짓이면 곡선.
	 * @return 레벨은 저장하지 않으므로 AssetTools.save_assets 를 따로 부른다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetWaterBodySpline(AWaterBody* WaterBody, const TArray<FVector>& Points, const TArray<float>& Widths, const TArray<float>& Depths, bool bLinear);
};
