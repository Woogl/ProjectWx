// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "WxInputBufferComponent.generated.h"

class UInputAction;
class UWxAbilitySystemComponent;
struct FAbilityEndedData;

/** 발동에 실패해 기억해 둔 입력. IA는 어빌리티 CDO가 쥐고 있어 이 컴포넌트보다 오래 살므로 생 포인터로 둔다. */
struct FWxBufferedInput
{
	const UInputAction* Action = nullptr;
	double TriggeredTime = 0.0;
};

/**
 * 선입력. 키 입력의 발동 시도가 실패하면 그 입력을 잠시 기억했다가, 어빌리티가 끝나거나 캔슬 창(콤보 창·후딜)이 열릴 때 다시 시도한다.
 *
 * ASC는 스펙 라우팅(키 상태·발동 시도·InputPressed 중계)만 하고, 무엇을 얼마나 오래 몇 개 기억할지는 여기서 정한다.
 * 이벤트로 트리거되는 어빌리티는 IA 라우팅 경로에 오지 않아 쌓이지 않고, 액션이 아닌 어빌리티의 입력은 쌓지도 비우지도 않는다.
 * 쥔 채 매 프레임 들어오는 홀드는 누른 순간만 새 입력으로 친다 — 쥔 동안은 라이브 경로가 스스로 재시도한다.
 */
UCLASS()
class WXGAME_API UWxInputBufferComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxInputBufferComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InputActionTriggered(const UInputAction* Action);

	/**
	 * 콤보 창·후딜·종료에서 재시도를 요청한다. 이동과 애니메이션 처리 뒤 한 번에 재생한다.
	 * 콤보 창이 이 재생 전에 닫히면 선입력은 다음 타가 아니라 새 발동이 되므로 창은 한 프레임보다 길게 저작한다.
	 */
	void RequestBufferedInputFlush();

private:
	void FlushBufferedInputs();
	void ClearBufferedInputs();
	void HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData);
	FTimerHandle FlushTimerHandle;

	UPROPERTY()
	TObjectPtr<UWxAbilitySystemComponent> AbilitySystemComponent;

	/** 오래된 순. */
	TArray<FWxBufferedInput> BufferedInputs;

protected:
	/**
	 * 실패한 입력을 이 시간(실시간 초) 동안 기억한다. 재시도 지점이 그 안에 오지 않으면 버린다.
	 * 슬로우모션이 게임 시간을 늘려도 플레이어의 시계는 그대로라, 입력이 유효한 길이는 실시간으로 잰다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (ClampMin = "0"))
	float BufferDuration = 0.4f;

	/** 가득 찬 채 새 입력이 오면 가장 오래된 것을 밀어낸다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (ClampMin = "1"))
	int32 MaxBufferedInputs = 1;
};
