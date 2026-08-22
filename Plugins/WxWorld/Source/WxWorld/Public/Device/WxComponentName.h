// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxComponentName.generated.h"

class AActor;
class USceneComponent;

/**
 * 장치 액터의 컴포넌트를 이름으로 지목한다. ST 에셋은 레벨 컴포넌트를 직접 가리킬 수 없어 이름이 유일한 저장 형태다.
 * 바인딩으로 대신할 수 없다 — 엔진 바인딩 피커는 소스 프로퍼티에 CPF_Edit 를 요구하는데 BP 컴포넌트 패널로 만든 컴포넌트의 클래스 변수에는 그 플래그가 붙지 않아, 네이티브로 선언하지 않는 한 목록에 뜨지 않는다.
 * 대신 에셋 스키마의 Context 액터 클래스에서 뽑은 컴포넌트 목록 드롭다운으로 고른다(WxEditor 의 FWxStateTreeComponentNameCustomization).
 * NoBinding: 고르는 길을 이 드롭다운 하나로 둔다.
 */
USTRUCT(meta = (NoBinding))
struct FWxComponentName
{
	GENERATED_BODY()

	/** 액터 클래스의 컴포넌트 프로퍼티 이름 — 컴포넌트 패널에 보이는 그 이름이다. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FName Name;

	/** 지목이 비었거나 그런 컴포넌트가 없으면 nullptr. */
	USceneComponent* Resolve(const AActor* Actor) const;
};
