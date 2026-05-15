// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxSavableInterface.h"
#include "WxGimmick.generated.h"

class UArrowComponent;
class USceneComponent;

/**
 * 상호작용 가능한 월드 오브젝트의 공통 부모.
 *
 * 컴포넌트(메시/InteractionComponent 등)는 자식 클래스가 직접 들고 바인딩한다.
 * 부모는 1회성 발동 플래그(bTriggered) 와 상태 적용 후크(ApplyState) 만 공통으로 제공한다.
 *
 * ApplyState:
 *  - 현재 상태(bTriggered, 자식 자체 State enum 등) 를 시각/인터랙션 등 로컬 효과에 적용하는 후크.
 *  - 호출 경로:
 *      a) 권한 측 MarkTriggered() → ApplyState (서버 즉시 적용)
 *      b) bTriggered 리플리케이션 OnRep → ApplyState (클라 적용)
 *      c) 자식의 BeginPlay 끝부분 → ApplyState (Level Streaming Persistence + WxSave 복원 동기화)
 *      d) 자식 자체 State 변경 시 (Door/Elevator 등의 OnRep_State, 서버 전이 등)
 *      e) BeginPlay 이후 WxSave 복원 (스트리밍 인-스트림) → OnWxSaveRestored → ApplyState
 *  - 부모 기본 구현은 비어있다. 자식이 필요 시 오버라이드.
 *
 * bTriggered:
 *  - 1회성 발동 상호작용(보물상자/경보 콘솔 등)을 위한 공용 플래그.
 *  - Level Streaming Persistence 로 셀 언로드/리로드 사이에 보존된다.
 *  - WxSave 슬롯에도 보존되어 세션 간 발동 상태가 유지된다.
 *  - 다단계 상태/반복 가능한 액터는 사용하지 않을 수 있다.
 *
 * WxSave 통합:
 *  - IWxSavableInterface 구현. UPROPERTY(SaveGame) 필드(bTriggered 등) 가 슬롯에 기록된다.
 *  - 보존 필드는 UPROPERTY(SaveGame) 로 표시한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxGimmick : public AActor, public IWxSavableInterface
{
	GENERATED_BODY()

public:
	AWxGimmick();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin IWxSavableInterface
	virtual void OnWxSaveRestored() override;
	//~ End IWxSavableInterface

protected:
	/** 모든 자식 컴포넌트의 부착 베이스. 자식 클래스는 SetRootComponent 호출 없이 SceneRoot 에 SetupAttachment 한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(ReplicatedUsing = OnRep_bTriggered, SaveGame)
	bool bTriggered = false;

#if WITH_EDITORONLY_DATA
	/** 에디터 정면 표시용 화살표. 베이스에서 SceneRoot 에 부착 완료. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif

	/** 권한 측에서 bTriggered 를 true 로 전환하고 ApplyState 를 즉시 호출. 이미 true 면 노옵.
	 *  서버에서는 OnRep 이 자동 발화하지 않으므로 ApplyState 를 명시 호출하여 후처리를 한 경로로 통일한다. */
	void MarkTriggered();

	/** 현재 상태를 로컬 효과에 적용. 자식이 오버라이드하여 인터랙션 비활성/메시 위치/틱 토글 등을 구현. */
	virtual void ApplyState() {}

private:
	UFUNCTION()
	void OnRep_bTriggered();
};
