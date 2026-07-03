// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WxBGMSourceComponent.generated.h"

class UAbilitySystemComponent;

/**
 * 소유 액터(보스 등)의 상태 태그가 켜지는 동안 자신의 BGM 분류 태그를 로컬 BGM 서브시스템에 기여하는 소스 컴포넌트.
 *
 * ActivationTag(예: State.InCombat)를 소유자 ASC 에서 감시하다가, 켜지면 UWxMusicSubsystem 에
 * {this, MusicTag} 를 등록하고 꺼지면(또는 EndPlay) 해제한다. 서브시스템은 활성 소스 중 가장 최근 등록된
 * MusicTag 를 분류 태그로 골라 Chooser 를 평가하므로, 보스전 동안 BGM 이 바뀌고 소스가 빠지면 베이스라인으로 자동 폴백된다.
 * 활성 감지는 캐릭터 코드 수정 없이 자기완결이며(전투 시스템이 이미 부여하는 태그를 재활용), 트리거는 오버랩이 아니라
 * 소유자 상태라 근접·볼륨이 없다. BGM 은 로컬 전용이므로 데디케이티드 서버에서는 동작하지 않는다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXSOUND_API UWxBGMSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxBGMSourceComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// ActivationTag 가 켜져 있는 동안 서브시스템에 기여할 BGM 분류 태그.
	UPROPERTY(EditAnywhere, Category = "Wx|BGM")
	FGameplayTag MusicTag;

	// 소유자 ASC 의 이 태그가 존재하는 동안 소스가 활성화된다(예: State.InCombat).
	UPROPERTY(EditAnywhere, Category = "Wx|BGM")
	FGameplayTag ActivationTag;

	/** ActivationTag 존재 여부 변경 콜백. 활성이면 등록, 비활성이면 해제한다. */
	void HandleActivationTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 현재 활성 상태에 맞춰 서브시스템 등록/해제를 적용한다(중복 호출은 무시). */
	void UpdateRegistration(bool bActive);

	// 소유자에서 해석한 ASC. ActivationTag 이벤트 구독 대상.
	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC;

	// 현재 서브시스템에 등록된 상태인지.
	bool bRegistered = false;
};
