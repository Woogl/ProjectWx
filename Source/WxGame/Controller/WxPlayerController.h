// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WxPlayerController.generated.h"

class AWxCharacterBase;
class AWxPlayerCharacter;
class UWxActivatableWidget;
class UWxDialogueSessionComponent;

/**
 * 플레이어 컨트롤러.
 * 게임플레이 입력(이동/시선/어빌리티)은 AWxPlayerCharacter가, 메뉴 토글 입력은 UWxHUDLayout(CommonUI 액션)이 소유한다.
 *
 * ModularGameplay 컴포넌트 receiver 다 — GameMode 가 요청 등록한 컨트롤러 컴포넌트(인벤토리·상호작용 스캐너·PlayerSpawn 등)를 자동 주입받으며, 어떤 컴포넌트가 붙는지 알지 않는다.
 * 주입 컴포넌트를 쓰는 쪽(뷰모델 등)이 직접 조회하고 늦은 도착까지 스스로 감당한다 — 본 클래스는 그 컴포넌트들을 중개하지 않는다.
 */
UCLASS()
class WXGAME_API AWxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWxPlayerController(const FObjectInitializer& ObjectInitializer);

	/** 소유 클라의 대화 세션 컴포넌트. 뷰모델 리졸버가 조회해 대사 표시·넘기기 요청을 잇는다. */
	UWxDialogueSessionComponent* GetDialogueSession() const { return DialogueSession; }

	//~ Begin AActor
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor

	virtual void OnRep_Pawn() override;
protected:
	virtual void OnPossess(APawn* InPawn) override;

	/** 대화 세션 진행·클라 전달 컴포넌트. NPC 상호작용(서버)이 여기로 진입해 소유 클라에서 세션을 연다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	TObjectPtr<UWxDialogueSessionComponent> DialogueSession;

	/** 플레이어 캐릭터에 빙의하면 Game 레이어에 띄울 HUD. 미지정이면 동작 없음. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|UI")
	TSubclassOf<UWxActivatableWidget> GameHUDWidgetClass;

	/** 빙의된 캐릭터 사망 시 Menu 레이어에 띄울 위젯. 미지정이면 동작 없음. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|UI")
	TSoftClassPtr<UWxActivatableWidget> DeathScreenWidgetClass;

	/** 대화 세션이 열리면 Game 레이어에 띄울 대화 위젯. 미지정이면 동작 없음. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|UI")
	TSoftClassPtr<UWxActivatableWidget> DialogueWidgetClass;

private:
	void PushGameHUD();

	void BindCharacterDeath(APawn* InPawn);

	UFUNCTION()
	void HandleCharacterDeath(AWxCharacterBase* DeadCharacter);

	UFUNCTION()
	void HandleDialogueStarted();
};
