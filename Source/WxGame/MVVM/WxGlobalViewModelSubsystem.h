// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Types/MVVMViewModelContext.h"
#include "MVVMGameSubsystem.h"
#include "WxGlobalViewModelSubsystem.generated.h"

class UWxViewModel_Character;
class UWxViewModel_Inventory;
class UMVVMViewModelBase;
class UMVVMViewModelCollectionObject;

/**
 * 글로벌 뷰모델을 관리하는 로컬 플레이어 서브시스템.
 * 글로벌 뷰모델 Shell을 미리 생성하여 MVVM Game Subsystem의 GlobalCollection에 등록한다.
 * 보스 등 실제 데이터 소스는 Shell 뷰모델의 Initialize/Deinitialize를 호출하여 내부 상태만 갱신한다.
 */
UCLASS()
class WXGAME_API UWxGlobalViewModelSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	static FMVVMViewModelContext GetPlayerCharacterContext();
	static FMVVMViewModelContext GetBossCharacterContext();
	static FMVVMViewModelContext GetInventoryContext();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UWxViewModel_Character* GetPlayerCharacterViewModel() const;
	UWxViewModel_Character* GetBossCharacterViewModel() const;
	UWxViewModel_Inventory* GetInventoryViewModel() const;

private:
	UMVVMViewModelCollectionObject* GetGlobalCollection() const;

	template <typename T>
	T* RegisterGlobalViewModel(const FMVVMViewModelContext& Context);

	template <typename T>
	void UnregisterGlobalViewModel(const FMVVMViewModelContext& Context, TObjectPtr<T>& ViewModel);

	UPROPERTY(Transient)
	TObjectPtr<UWxViewModel_Character> PlayerCharacterViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UWxViewModel_Character> BossCharacterViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UWxViewModel_Inventory> InventoryViewModel;
};

template <typename T>
T* UWxGlobalViewModelSubsystem::RegisterGlobalViewModel(const FMVVMViewModelContext& Context)
{
	UMVVMViewModelCollectionObject* GlobalCollection = GetGlobalCollection();
	if (!GlobalCollection)
	{
		return nullptr;
	}

	T* ViewModel = NewObject<T>(this);
	GlobalCollection->AddViewModelInstance(Context, ViewModel);
	return ViewModel;
}

template <typename T>
void UWxGlobalViewModelSubsystem::UnregisterGlobalViewModel(const FMVVMViewModelContext& Context, TObjectPtr<T>& ViewModel)
{
	if (!ViewModel)
	{
		return;
	}

	if (UMVVMViewModelCollectionObject* GlobalCollection = GetGlobalCollection())
	{
		GlobalCollection->RemoveViewModel(Context);
	}

	ViewModel = nullptr;
}
