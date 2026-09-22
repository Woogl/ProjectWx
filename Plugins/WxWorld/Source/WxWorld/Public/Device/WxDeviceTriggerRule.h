// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Device/WxDeviceComponentName.h"
#include "WxInteractable.h"
#include "WxDeviceTriggerRule.generated.h"

class AWxDevice;

/**
 * '작동 대기' 가 누구에게서 무엇을 받아 줄지 정하는 수락 규칙의 베이스. 노드에서 규칙을 비워 두면 누가 보냈든 그냥 받는다.
 * 트리 틱 밖(스캔·수락 검증)에서도 불리므로 규칙은 자기 설정값과 넘겨받은 장치만으로 답한다.
 */
USTRUCT(meta = (Hidden))
struct WXWORLD_API FWxDeviceTriggerRule
{
	GENERATED_BODY()

	virtual ~FWxDeviceTriggerRule();

	/** Sender 가 지금 작동 신호를 보내면 받아 줄 선택지. 문구가 빈 선택지는 「그냥 받는다」는 뜻이라 미는 장치가 자기 프롬프트로 채운다. */
	virtual void GetAcceptedOptions(const AWxDevice& Receiver, const AWxDevice* Sender, TArray<FWxInteractionOption>& OutOptions) const;
};

/**
 * 스플라인을 타는 장치(엘리베이터)의 호출 규칙. 정차 지점은 스플라인 포인트이고 선택지 값은 그 포인트 번호다 — 층이 늘어도 트리는 그대로다.
 *  - 탑승칸 밖의 호출 버튼: 그 버튼에서 가장 가까운 정차 지점 하나. 탑승칸이 이미 거기 있으면 받지 않아 버튼이 잠긴다.
 *  - 탑승칸에 붙은 버튼: 지금 있는 곳을 뺀 모든 정차 지점. 플레이어가 상호작용 목록에서 고른다. 깨우는 상태(bWakeOnCall)에서는 잠긴다.
 * 받아들인 값은 오너 장치의 SelectedOptionValue 에 남으므로 '스플라인 이동' 의 목표 포인트를 Actor.SelectedOptionValue 에 바인딩해 쓴다.
 */
USTRUCT(meta = (DisplayName = "정차 지점 선택"))
struct FWxDeviceTriggerRule_SplineStops : public FWxDeviceTriggerRule
{
	GENERATED_BODY()

	virtual void GetAcceptedOptions(const AWxDevice& Receiver, const AWxDevice* Sender, TArray<FWxInteractionOption>& OutOptions) const override;

	/** 스플라인을 타고 움직이는 탑승칸. 지금 있는 정차 지점과, 보낸 버튼이 탑승칸에 붙어 있는지를 이것으로 가른다. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FWxStateTreeComponentName Platform;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (AllowedClasses = "/Script/Engine.SplineComponent"))
	FWxStateTreeComponentName Spline;

	/**
	 * 비활성 장치를 깨우는 상태에 켠다. 밖 호출만 받고 탑승칸 버튼은 잠근다.
	 * 탑승칸이 이미 있는 정차 지점의 호출도 받으므로, 같은 층에서 깨우면 제자리에서 문만 열리고 다른 층에서 깨우면 그 층으로 온다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	bool bWakeOnCall = false;

	/**
	 * 탑승칸 버튼이 내놓는 정차 지점 선택지의 문구. {0} 은 정차 지점 번호(1부터)다. 밖 버튼은 대기 노드의 Prompt 를 쓴다.
	 * C++ 기본값을 두지 않는다 — 인스턴스 구조체 안의 FText 가 기본값과 같으면 에셋 저장이 실패한다(FortniteMain 커스텀 버전 불일치).
	 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FText StopPrompt;
};
