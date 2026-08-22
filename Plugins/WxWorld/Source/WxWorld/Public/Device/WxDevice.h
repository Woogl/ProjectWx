// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeDelegate.h"
#include "WxInteractable.h"
#include "WxSavable.h"
#include "WxDevice.generated.h"

class ACharacter;
class UWxDeviceStateTreeComponent;

/**
 * 상호작용이 켜져 있는 동안의 HUD 프롬프트와, 눌렸을 때 트리에 알릴 자리.
 * '상호작용 켜기' 태스크가 상태 진입 시 자기 몫을 하나 넘긴다 — 상태마다 발행자가 다르므로 전이에 페이로드 비교 조건이 필요 없다.
 */
USTRUCT()
struct FWxDeviceInteractionBinding
{
	GENERATED_BODY()

	UPROPERTY()
	FText Prompt;

	/** 눌렸을 때 발행할 ST 델리게이트. 어느 상태로 갈지는 이것을 듣는 전이가 정한다. */
	UPROPERTY()
	FStateTreeDelegateDispatcher Dispatcher;

	/** 위 발행에 쓰는 실행 컨텍스트. 발행이 트리 틱 밖에서 일어나므로 태스크가 진입 시 자기 것을 넘겨 둔다. */
	FStateTreeWeakExecutionContext Context;
};

/**
 * StateTree 로 자기 상태를 구동하는 월드 장치(문·상자·체크포인트·엘리베이터)의 공통 호스트.
 *
 * 루트를 만들지 않는다 — 파생 BP 가 저마다 다른 몸통을 세운다.
 * 상호작용을 받아 남의 장치를 미는 AWxTriggerDevice 와 이름 계열을 이루되 상속 관계는 없다 — 발동 장치는 상태를 들지 않아 트리가 없다.
 *
 * 상태의 실행·소유(복제·SaveGame StateTag)·ST 에셋 저작은 전부 UWxDeviceStateTreeComponent 가 맡는다 — 상태 구동 패턴은 그 클래스 doc-comment 참조.
 * 이 액터에 남는 것은 상호작용 표면(IWxInteractable·프롬프트·당사자)과 세이브 신원(IWxSavable·SaveId)뿐이다.
 * 상호작용·복원 신호는 전부 액터가 받아 컴포넌트에 전달한다 — 스캐너·어빌리티·발동 장치·세이브가 보는 계약 상대는 액터 하나다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxDevice : public AActor, public IWxInteractable, public IWxSavable
{
	GENERATED_BODY()

public:
	AWxDevice();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#if WITH_EDITOR
	/** 저장 직전에 SaveId 를 자기 ActorGuid 로 확정한다(런타임은 심긴 값을 읽기만 한다). */
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
#endif

	//~ Begin IWxInteractable — 활성·프롬프트는 ST 가 상태마다 세팅한 값이다.
	virtual bool CanInteract() const override;

	/** 남의 트리('상호작용 켜기' Target 갈래)가 이 장치를 잠그고 여는 진입점. 프롬프트·발행자가 없는 토글이라 켜도 눌림이 트리에 닿지 않는다. */
	virtual void SetInteractionEnabled(bool bEnabled) override;

	/** 권위 측에서 상호작용을 받아 당사자를 기록하고 지금 상태의 발행자를 트리에 발행한다. 어느 상태로 갈지는 ST 에셋의 전이가 정하고, 그 결과가 복제되어 클라에 전해진다. */
	virtual void OnInteracted(AActor* Interactor) override;

	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable

	//~ Begin IWxSavable
	/**
	 * 에디터에서 부여되어 에셋에 직렬화된 슬롯 키(SaveId). 런타임·세션 간 불변이며 쿠킹 빌드에서도 그대로 성립한다.
	 * 액터 경로에서 파생하지 않는다 — World Partition 은 런타임 경로에 스트리밍 셀(그리드 좌표)을 포함하므로, 액터를 옮기거나 PIE 와 패키지 빌드를 오갈 때마다 키가 달라진다.
	 * 대신 배치 후 맵을 한 번 저장해야 키가 에셋에 남는다. 저장 전이거나 런타임 스폰된 장치는 키가 무효라 저장/복원에서 제외된다.
	 */
	virtual FGuid GetSaveId() const override;

	/** 복원은 컴포넌트의 StateTag 세팅이 전부다 — 컴포넌트에 알리면 트리가 라이브 전이로 그 상태에 수렴한다. */
	virtual void OnSaveRestored() override;
	//~ End IWxSavable

	/**
	 * 이 장치를 지목한 발동 장치가 눌렸을 때 그 장치가 서버 권위에서 부른다. 당사자를 기록하고 트리에 Event.Interact 를 보낸다.
	 * 어느 상태로 갈지는 그 이벤트를 듣는 ST 에셋의 전이가 정하며, 결과는 자기 상호작용과 같이 StateTag 복제로 클라에 전해진다.
	 */
	void NotifyDeviceInteracted(AActor* Interactor);

	/**
	 * 권위가 정한 상태의 Tag. 밖에서 장치 상태를 조건으로 삼는 쪽(층별 호출 레버 등)이 읽는 값이다.
	 * 복제된 필드를 그대로 답하므로 매 틱 게이트로 써도 되고, 모든 피어가 같은 권위 값을 본다.
	 */
	FGameplayTag GetStateTag() const;

	/** 이번 상호작용의 당사자. ST 태스크(이동·몽타주·이펙트)가 대상으로 삼는 캐릭터다. */
	ACharacter* GetInteractingCharacter() const;

	/**
	 * 상호작용을 켜고 끄며, 켤 때는 그 상태의 프롬프트와 발행 자리(Binding)도 함께 담는다 — 끌 때 Binding 은 쓰이지 않는다.
	 * '상호작용 켜기' 태스크가 상태 진입 시 호출한다. 꺼져 있으면 IsInteractionEnabled 가 false 를 답해 다음 스캔에서 후보에서 빠지고, 어빌리티의 서버 활성 검증에도 걸린다.
	 * 복제하지 않는다 — ST 가 각 피어에서 실행되어 같은 값에 수렴한다.
	 */
	void SetInteractionBinding(bool bEnabled, const FWxDeviceInteractionBinding& Binding);

	/**
	 * 이번 상호작용의 당사자(플레이어 캐릭터). 권위가 쓰고 복제로 각 피어에 전해진다 — StateTag 와 같은 액터 채널 갱신에 실리므로 추종 전이 시점에 짝이 맞는다.
	 * 비영속이라 복원 시엔 비어 있고, 그때는 당사자 태스크들이 스스로 스킵한다.
	 */
	UPROPERTY(Replicated, Transient)
	TObjectPtr<ACharacter> InteractingCharacter;

private:
	/**
	 * 지금 상태의 발행자를 트리에 발행한다. 상호작용을 받아 권위 검증을 마친 뒤 부른다.
	 * 트리 틱 밖에서 부르는 경로라 태스크가 남긴 약참조 컨텍스트를 쓴다. 잠든 트리는 이 발행이 깨우고, 발행 표식은 다음 전이 처리까지 보존된다.
	 */
	void BroadcastInteractionDelegate();

	/** 상태머신 실행기이자 상태(StateTag)의 소유자. ST 에셋 저작도 이 컴포넌트의 State Tree 프로퍼티에서 한다. 이 액터는 상호작용·복원 신호를 여기로 전달한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxDeviceStateTreeComponent> StateTreeComponent;

	UPROPERTY()
	FGuid SaveId;

	/**
	 * 지금 상호작용이 켜져 있는가. '상호작용 켜기' 태스크가 상태 진입 시 정한다.
	 * 로컬 전용(복제·SaveGame 아님) — 각 피어의 ST 가 같은 값으로 수렴시킨다.
	 */
	UPROPERTY(Transient)
	bool bInteractionEnabled = false;

	/** 켜져 있는 동안의 프롬프트와 발행 자리. 끄면 다음 켜짐을 위해 비운다. */
	UPROPERTY(Transient)
	FWxDeviceInteractionBinding InteractionBinding;
};
