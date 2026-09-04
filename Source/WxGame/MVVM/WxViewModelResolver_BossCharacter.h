// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_BossCharacter.generated.h"

class AWxEnemyCharacter;
class UUserWidget;
class UMVVMView;

/**
 * 교전 중인 보스의 표시 데이터를 공용 UWxViewModel_Character 에 실어 위젯에 넘긴다.
 * 위젯이 보스보다 먼저 생길 수 있으므로 생성 시점 조회로 끝내지 않고, 보스의 교전 통지를 구독해 그 자리를 계속 갱신한다.
 *
 * 리졸버는 위젯 클래스 단위로 공유되는 const 객체라 자기 상태를 들 수 없다.
 * 그래서 "지금 보스" 는 월드의 GameState 를 Outer 로 공유되는 뷰모델 한 자리가 쥐며, 통지에 실려 온 보스가 자기 월드의 자리를 찾아간다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_BossCharacter : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;

private:
	void HandleBossEngagementChanged(AWxEnemyCharacter* BossCharacter, bool bEngaged) const;
};
