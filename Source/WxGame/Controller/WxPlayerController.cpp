// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Cheat/WxCheatManager.h"
#include "Components/GameFrameworkComponentManager.h"

AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 개발용 콘솔 치트. 클래스만 지정해 두면 엔진이 Standalone·에디터에서만 실제로 생성한다.
	CheatClass = UWxCheatManager::StaticClass();
}

void AWxPlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// ModularGameplay 컴포넌트 수신 opt-in. 활성 주입 요청(Experience 액션이 등록)의 컴포넌트가 여기에 자동 부착된다.
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AWxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);
}
