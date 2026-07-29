// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WxPawnData.generated.h"

/**
 * Experience 가 지정하는 플레이어 폰 구성.
 * Wx 폰은 자기 BP 클래스가 ASC·입력·카메라를 이미 소유하므로 지금은 클래스 선택만 담는다 — 조립식 폰 전제의 Lyra 확장 필드는 대응물이 생길 때 추가한다.
 * 에셋은 네이티브 클래스 인스턴스로만 만든다 — Experience 정의와 같은 스캔 정책.
 */
UCLASS()
class UWxPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 플레이어가 빙의할 폰 클래스. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<APawn> PawnClass;
};
