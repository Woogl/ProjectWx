// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WxPlayerController.generated.h"

class UCommonActivatableWidget;
class UWxDialogueSessionComponent;

/**
 * 플레이어 컨트롤러.
 * 게임플레이 입력(이동/시선/어빌리티)은 AWxPlayerCharacter가, 메뉴 토글 입력은 UWxHUDLayout(CommonUI 액션)이 소유한다.
 *
 * ModularGameplay 컴포넌트 receiver 다 — Experience 주입으로 요청 등록된 컨트롤러 컴포넌트(인벤토리·상호작용 스캐너·PlayerSpawn 등)를 자동 주입받으며, 어떤 컴포넌트가 붙는지 알지 않는다.
 * 주입 컴포넌트를 쓰는 쪽(뷰모델 등)이 직접 조회하고 늦은 도착까지 스스로 감당한다 — 본 클래스는 그 컴포넌트들을 중개하지 않는다.
 *
 * 화면도 중개하지 않는다 — HUD 와 사망 화면은 UWxUIManagerSubsystem 이 이 컨트롤러의 빙의를 직접 따라가며 띄운다.
 * 여기 남은 화면 글루는 대화 창 하나뿐인데, WxDialogue 와 WxUI 가 서로를 참조할 수 없어 양쪽을 아는 게임 모듈이 이어야 하기 때문이다.
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

protected:
	/** 대화 세션 진행·클라 전달 컴포넌트. NPC 상호작용(서버)이 여기로 진입해 소유 클라에서 세션을 연다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	TObjectPtr<UWxDialogueSessionComponent> DialogueSession;

private:
	UFUNCTION()
	void HandleDialogueStarted();

	UFUNCTION()
	void HandleDialogueEnded();

	/** 대화 중 띄워 둔 대화 창. 세션이 끝날 때 이 창을 닫기 위해 기억한다. */
	TWeakObjectPtr<UCommonActivatableWidget> DialogueScreen;
};
