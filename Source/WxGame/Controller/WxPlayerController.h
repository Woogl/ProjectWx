// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "WxPlayerController.generated.h"

class AWxPlayerCharacter;
class UInputMappingContext;
class UInputAction;
class UAbilitySystemComponent;
class UWxActivatableWidget;
class UWxInteractionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractionTargetChangedSignature, UWxInteractionComponent*, NewTarget);

/** Enhanced Input Action과 Gameplay Tag를 매핑하는 단일 항목 */
USTRUCT(BlueprintType)
struct FWxInputAbilityBinding
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "Input"))
	FGameplayTag InputTag;
};

/**
 * 플레이어 컨트롤러.
 * 입력 관련 데이터(MappingContext, InputAction, InputConfig)를 소유.
 * 현재 빙의 중인 캐릭터의 Health ViewModel을 Global Collection에 등록/관리.
 */
UCLASS()
class WXGAME_API AWxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UInputMappingContext* GetDefaultMappingContext() const;
	const UInputAction* GetMoveAction() const;
	const UInputAction* GetLookAction() const;
	const TArray<FWxInputAbilityBinding>& GetAbilityInputBindings() const;

	/** 플레이어 캐릭터의 InteractionSphere가 오버랩 시 호출 */
	void RegisterInteractionCandidate(UWxInteractionComponent* Component);
	void UnregisterInteractionCandidate(UWxInteractionComponent* Component);

	UWxInteractionComponent* GetCurrentInteractionTarget() const;

	/** 캐릭터 입력 핸들러가 호출. 현재 타깃에 대해 InputTag로 상호작용 시도 (필요 시 서버 RPC로 위임) */
	void TryInteractCurrent(FGameplayTag InputTag);

	/** 현재 상호작용 타깃이 변경될 때 fire. UI/ViewModel이 구독해 프롬프트를 갱신 */
	UPROPERTY(BlueprintAssignable, Category = "Wx|Interaction")
	FWxOnInteractionTargetChangedSignature OnInteractionTargetChanged;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> LookAction;

	/** 어빌리티 입력 바인딩 설정. InputAction → InputTag 매핑 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TArray<FWxInputAbilityBinding> AbilityInputBindings;

private:
	void PushGameHUD(AWxPlayerCharacter* PlayerCharacter);

	void InitializePlayerAbilitySystemViewModel(UAbilitySystemComponent* ASC);

	void UpdateCurrentInteractionTarget();

	UFUNCTION(Server, Reliable)
	void Server_TryInteract(UWxInteractionComponent* TargetComponent, FGameplayTag InputTag);

	UPROPERTY(Transient)
	TObjectPtr<UWxActivatableWidget> GameHUD;

	TArray<TWeakObjectPtr<UWxInteractionComponent>> InteractionCandidates;

	TWeakObjectPtr<UWxInteractionComponent> CurrentInteractionTarget;
};
