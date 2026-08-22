// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeDelegate.h"
#include "StructUtils/StructView.h"
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
 * 버튼·레버 같은 발동 장치도 이 클래스다 — 누른 상태를 자기 트리로 몰면서 '이벤트 보내기' 태스크로 LinkedDevices 를 민다.
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

	/**
	 * 남의 트리('상호작용 켜기' Target 갈래)가 이 장치의 활성을 끄고 켜는 진입점. 자기 트리가 담아 둔 프롬프트·발행자는 남아 있어 다시 켜면 원래대로 눌린다.
	 * 자기 트리와 같은 값을 쓰므로 나중에 쓴 쪽이 이긴다 — 상태마다 스스로 켜고 끄는 장치(버튼)를 밖에서 잠글 때는 이 진입점이 아니라 그 트리에 잠금 상태를 이벤트로 요청한다.
	 */
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
	 * 다른 장치의 '이벤트 보내기' 태스크가 권위 측에서 부른다. 당사자와 보낸 장치를 기록하고 트리에 EventTag 와 Payload 를 보낸다.
	 * 어느 상태로 갈지는 그 이벤트를 듣는 ST 에셋의 전이가 정하며, 결과는 자기 상호작용과 같이 StateTag 복제로 클라에 전해진다.
	 */
	void NotifyDeviceInteracted(AActor* Interactor, FGameplayTag EventTag, FConstStructView Payload = FConstStructView(), AWxDevice* FromDevice = nullptr);

	/**
	 * 권위가 정한 상태의 Tag. 밖에서 장치 상태를 조건으로 삼는 쪽(층별 호출 레버 등)이 읽는 값이다.
	 * 복제된 필드를 그대로 답하므로 매 틱 게이트로 써도 되고, 모든 피어가 같은 권위 값을 본다.
	 */
	FGameplayTag GetStateTag() const;

	/** 이번 상호작용의 당사자. ST 태스크(이동·몽타주·이펙트)가 대상으로 삼는 캐릭터다. */
	ACharacter* GetInteractingCharacter() const;

	/**
	 * 나를 마지막으로 민 장치. 동작을 마친 장치가 자기를 민 버튼을 되돌려 푸는 자리다('이벤트 보내기' 의 빈 대상 갈래).
	 * 배선이 미는 쪽 → 밀리는 쪽 단방향이라 반대편을 저작으로는 가리킬 수 없다 — 배치 버튼이 미는 장치(피스톤)까지 같은 방식으로 다루려면 이 값이 필요하다.
	 */
	AWxDevice* GetInstigatorDevice() const;

	/**
	 * 상호작용을 켜고 끄며, 켤 때는 그 상태의 프롬프트와 발행 자리(Binding)도 함께 담는다 — 끌 때 Binding 은 쓰이지 않고 담겨 있던 것도 지우지 않는다.
	 * '상호작용 켜기' 태스크가 상태 진입 시 호출한다. 꺼져 있으면 IsInteractionEnabled 가 false 를 답해 다음 스캔에서 후보에서 빠지고, 어빌리티의 서버 활성 검증에도 걸린다.
	 * 복제하지 않는다 — ST 가 각 피어에서 실행되어 같은 값에 수렴한다.
	 */
	void SetInteractionBinding(bool bEnabled, const FWxDeviceInteractionBinding& Binding);

	/**
	 * 이 장치가 발동할 다른 장치들. '이벤트 보내기' 태스크가 대상을 지목하지 않았을 때 이 배열 전부에 이벤트가 나간다.
	 * 배선은 미는 쪽 → 밀리는 쪽 단방향 저작이라 하나가 여럿을(1:N), 한 장치가 여러 발동 장치에(N:1) 걸린다.
	 * 하드 참조라 대상과 함께 로드된다 — 늦은 등록을 따로 다루지 않는 근거다.
	 */
	UPROPERTY(EditInstanceOnly, Category = "Wx")
	TArray<TObjectPtr<AWxDevice>> LinkedDevices;

	/**
	 * 연결 장치에 보내는 이벤트.
	 * 목적지가 여럿인 장치(엘리베이터)의 버튼은 상태 태그를 보내, 받는 트리가 On Event(그 태그)로 가른다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTag TriggerEvent;

	/**
	 * 이번 상호작용의 당사자(플레이어 캐릭터). 권위가 쓰고 복제로 각 피어에 전해진다 — StateTag 와 같은 액터 채널 갱신에 실리므로 추종 전이 시점에 짝이 맞는다.
	 * 비영속이라 복원 시엔 비어 있고, 그때는 당사자 태스크들이 스스로 스킵한다.
	 */
	UPROPERTY(Replicated, Transient)
	TObjectPtr<ACharacter> InteractingCharacter;

	/**
	 * 나를 마지막으로 민 장치. 권위 전용이라 복제하지 않는다 — 되돌려 보내는 것도 권위 트리뿐이다.
	 * 상호작용은 사건이므로 상태처럼 남지 않는다: 복원·레이트조인에선 비어 있고, 그때는 되돌릴 상대도 없다.
	 */
	UPROPERTY(Transient)
	TObjectPtr<AWxDevice> InstigatorDevice;

protected:
	/** 장치 BP 에 ChildActor 로 심긴 발동 장치가 자기를 품은 장치를 스스로 지목한다 — 문+버튼처럼 한 몸으로 저작되는 쌍은 배선이 필요 없다. */
	virtual void BeginPlay() override;

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

	/** 켜져 있는 동안의 프롬프트와 발행 자리. 꺼도 남겨 둔다 — 다시 켤 때 그 자리로 돌아온다. */
	UPROPERTY(Transient)
	FWxDeviceInteractionBinding InteractionBinding;
};
