// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameplayTagContainer.h"
#include "WxInteractionComponent.generated.h"

class UTexture2D;

/**
 * 단일 상호작용 옵션. 하나의 InputTag에 대응하는 프롬프트 1개를 표현.
 * 같은 액터에 여러 옵션이 있을 경우 InputTag로 구분된다 (예: Primary=대화, Secondary=거래).
 */
USTRUCT(BlueprintType)
struct WXWORLD_API FWxInteractionOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Input.Interact"))
	FGameplayTag InputTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWxOnInteractedSignature, AActor*, Instigator, FGameplayTag, InputTag);

/**
 * 액터의 상호작용을 노출하는 컴포넌트.
 * 컴포넌트 자체는 위치/반경 마커 역할만 하고, 오버랩 감지는 플레이어 캐릭터 측에서 수행한다.
 *
 * 흐름:
 *  1) 플레이어의 InteractionSphere가 본 컴포넌트와 오버랩 → PlayerController에 자신을 등록
 *  2) PlayerController가 후보 중 가장 가까운 것을 선택해 UI 프롬프트로 노출
 *  3) 입력 발생 시 PlayerController가 서버 RPC로 TryInteract 호출
 *  4) 서버에서 옵션 검증 → OnInteracted 델리게이트 fire → 액터 로직이 처리
 *
 * 주의: 이 컴포넌트가 부착된 액터는 서버 RPC 경로를 위해 Replicate 되어야 한다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent, PrioritizeCategories = "Wx"))
class WXWORLD_API UWxInteractionComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UWxInteractionComponent();

	const TArray<FWxInteractionOption>& GetOptions() const;

	/**
	 * 서버에서 상호작용을 실행. PlayerController가 RPC를 통해 호출함을 가정.
	 * Options 안에 InputTag와 일치하는 항목이 있어야 OnInteracted가 fire된다.
	 */
	void TryInteract(AActor* Instigator, FGameplayTag InputTag);

	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractedSignature OnInteracted;

protected:
	/** 이 액터가 노출하는 상호작용 옵션 목록 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx")
	TArray<FWxInteractionOption> Options;
};
