// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxGlobalViewModelSubsystem.generated.h"

class UWxViewModel_Character;
class UWxViewModel_Inventory;
class UUserWidget;
class UMVVMView;

/**
 * 글로벌 뷰모델을 관리하는 로컬 플레이어 서브시스템.
 * 글로벌 뷰모델 Shell을 미리 생성하여 소유하며, 위젯은 전용 리졸버를 통해 가져간다.
 * 보스 등 실제 데이터 소스는 Shell 뷰모델의 Initialize/Deinitialize를 호출하여 내부 상태만 갱신한다.
 */
UCLASS()
class WXGAME_API UWxGlobalViewModelSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UWxViewModel_Character* GetPlayerCharacterViewModel() const;
	UWxViewModel_Character* GetBossCharacterViewModel() const;
	UWxViewModel_Inventory* GetInventoryViewModel() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UWxViewModel_Character> PlayerCharacterViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UWxViewModel_Character> BossCharacterViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UWxViewModel_Inventory> InventoryViewModel;
};

/**
 * 글로벌 VM_PlayerCharacter 용 View Bindings Resolver.
 *
 * 위젯을 소유한 LocalPlayer 의 UWxGlobalViewModelSubsystem 이 미리 생성해 소유 중인 플레이어 캐릭터 Shell 뷰모델을 그대로 반환한다 (새로 생성하지 않음).
 * UWxViewModel_Character 는 WxUI 플러그인 소속이라 본 서브시스템을 참조할 수 없으므로, 리졸버는 게임 모듈인 이 파일에 둔다.
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_PlayerCharacter : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};

/**
 * 글로벌 VM_BossCharacter 용 View Bindings Resolver.
 *
 * 위젯을 소유한 LocalPlayer 의 UWxGlobalViewModelSubsystem 이 미리 생성해 소유 중인 보스 캐릭터 Shell 뷰모델을 그대로 반환한다 (새로 생성하지 않음).
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_BossCharacter : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
