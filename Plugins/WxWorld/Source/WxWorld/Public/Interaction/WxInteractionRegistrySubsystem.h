// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "WxInteractionRegistrySubsystem.generated.h"

class UWxInteractionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractionListChanged, const TArray<FText>&, Prompts);

/**
 * 상호작용 레지스트리 서브시스템.
 * 로컬 플레이어마다 자동 생성되어, 현재 범위 안에 있는 UWxInteractionComponent들을 등록 순서로 모은다.
 * HUD 리스트 뷰모델(UWxViewModel_InteractionList)이 이 목록을 표시한다.
 *
 * 범위 진입/이탈한 UWxInteractionComponent가 자신을 register/unregister 한다(플레이어에 컴포넌트를 붙이지 않는다).
 * 선택(SelectedIndex)은 UI 소유이므로 본 서브시스템은 다루지 않는다. 목록만 제공하며 로컬 표시 전용이다.
 */
UCLASS()
class WXWORLD_API UWxInteractionRegistrySubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	/** 범위 진입한 상호작용 컴포넌트 등록. 중복은 무시한다. */
	void RegisterInRange(UWxInteractionComponent* Component);

	/** 범위 이탈한 상호작용 컴포넌트 해제. */
	void UnregisterInRange(UWxInteractionComponent* Component);

	/** 현재 인-레인지 컴포넌트들의 프롬프트 텍스트를 등록 순서로 반환한다. */
	TArray<FText> GetPrompts() const;

	/** 인-레인지 목록 변경 시 발사. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractionListChanged OnListChanged;

private:
	/** 무효 참조 정리 후 목록 변경을 델리게이트로 알린다. */
	void RebuildAndNotify();

	TArray<TWeakObjectPtr<UWxInteractionComponent>> InRangeComponents;
};
