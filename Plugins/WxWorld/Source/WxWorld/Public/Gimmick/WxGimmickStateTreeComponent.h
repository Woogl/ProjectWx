// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeComponent.h"
#include "GameplayTagContainer.h"
#include "StateTreeExecutionExtension.h"
#include "WxInteractable.h"
#include "WxSavable.h"
#include "WxGimmickStateTreeComponent.generated.h"

class ACharacter;
class UPrimitiveComponent;
class UWxGimmickStateTreeComponent;

/**
 * 잠든 트리가 깨어나기를 요청할 때 컴포넌트 틱을 다시 켜는 실행 컨텍스트 확장.
 * 엔진 컴포넌트도 같은 역할의 확장을 붙이지만 그 구현체가 모듈 밖으로 export 되지 않아, 저장된 상태에서 트리를 여는 경로에서는 이것을 대신 붙인다.
 */
USTRUCT()
struct FWxGimmickStateTreeExecutionExtension : public FStateTreeExecutionExtension
{
	GENERATED_BODY()

	virtual void ScheduleNextTick(const FContextParameters& Context, const FNextTickArguments& Args) override;

	UPROPERTY()
	TObjectPtr<UWxGimmickStateTreeComponent> Component;
};

/**
 * 상호작용 이벤트의 페이로드. 어느 영역이 눌렸는지는 이벤트 태그가 이미 가르므로 전이는 보통 이걸 볼 필요가 없고, 더 세밀한 전이 조건이 필요할 때 바인딩해 쓴다.
 */
USTRUCT()
struct FWxGimmickInteractEvent
{
	GENERATED_BODY()

	/** 이번 상호작용이 일어난 영역 메시. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	TObjectPtr<UPrimitiveComponent> Source;

	/** 상호작용을 일으킨 주체(플레이어 캐릭터). */
	UPROPERTY(EditAnywhere, Category = "Wx")
	TObjectPtr<AActor> Interactor;
};

/** 지금 켜져 있는 상호작용 영역 하나의 표시·발동 설정. 'Enable Interaction' 태스크가 상태 진입 시 채운다. */
USTRUCT()
struct FWxGimmickInteractionRegion
{
	GENERATED_BODY()

	/** 이 영역이 표시할 HUD 프롬프트. 비면 문구 없이 표시된다. */
	UPROPERTY()
	FText Prompt;

	/** 이 영역을 눌렀을 때 트리에 발행할 이벤트 태그. 비면 StateTree.Interact 를 쓴다. */
	UPROPERTY()
	FGameplayTag InteractEvent;
};

/**
 * 기믹의 상태머신·상호작용·영속을 한 몸에 담는 StateTree 컴포넌트.
 * 이 컴포넌트를 붙이면 어떤 액터든(순수 BP 포함) 기믹이 된다 — 전용 C++ 액터 클래스가 필요 없다.
 *
 * 세 가지 책임이 있다.
 *  - 상호작용 계약(IWxInteractable): 어느 메시가 지금 켜진 영역인지, 그 영역의 프롬프트가 무엇인지 답하고, 상호작용을 ST 이벤트로 올린다.
 *  - 상태 영속(IWxSavable): 지금 활성인 ST 상태의 Tag 를 저장·복제하고, 복원 시 그 상태에서 트리를 연다.
 *  - StateTree 구동: 자동 시작하며, 저장된 상태가 있으면 그 상태를 시작점으로 지정해 시작한다.
 *
 * 상태 구동 패턴:
 *  - 전이는 전부 ST 에셋이 정한다. 권위 측이 상호작용을 받으면 StateTree.Interact 이벤트를 전 피어에 뿌리고, 어느 상태로 갈지는 각 상태의 전이(On Event)가 지목한다.
 *  - 상태 식별은 엔진 순정 상태 Tag 다. 상태 디테일의 Tag 필드에 태그를 달면 그 값이 곧 저장 키가 된다(에셋 안에서 유일해야 한다).
 *  - 권위 측은 틱마다 활성 상태의 Tag 를 StateTag 에 기록한다. 상태 변화는 전부 트리 틱 안에서 일어나므로 이 폴링이 전부를 잡는다.
 *  - 클라는 멀티캐스트 이벤트로 같은 전이를 밟아 비주얼을 따라간다. 어긋난 피어(늦은 참여·스트리밍 인)는 복제된 StateTag 로 그 상태에서 트리를 재시작해 수렴한다.
 *  - 복원·재시작 진입은 SourceStateID 가 무효인 초기 진입이라, 노드들이 별도 마커 없이 스냅·스킵으로 처리한다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent))
class WXWORLD_API UWxGimmickStateTreeComponent : public UStateTreeComponent, public IWxSavable, public IWxInteractable
{
	GENERATED_BODY()

public:
	UWxGimmickStateTreeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin UActorComponent
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 에디터 월드에서 오너의 ActorGuid 를 SaveId 에 심는다(런타임은 심긴 값을 읽기만 한다). 게임 월드에서는 순정 동작 그대로다. */
	virtual void OnRegister() override;
	//~ End UActorComponent

	//~ Begin UBrainComponent — 저장된 상태가 있으면 그 상태를 시작점으로 트리를 연다.
	virtual void StartLogic() override;
	virtual void RestartLogic() override;

	/** 트리가 멈추기 전에 마지막 활성 상태를 기록한다 — 정지 후엔 활성 상태가 비어 읽을 수 없다. */
	virtual void StopLogic(const FString& Reason) override;
	//~ End UBrainComponent

	//~ Begin IWxInteractable
	virtual bool IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const override;

	/** 권위 측에서 상호작용을 받아 전 피어에 이벤트로 뿌린다. 어느 상태로 갈지는 ST 에셋의 전이가 정한다. */
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;

	virtual FText GetInteractionPrompt(const UActorComponent* Source) const override;
	//~ End IWxInteractable

	//~ Begin IWxSavable
	/**
	 * 에디터에서 부여되어 에셋에 직렬화된 슬롯 키(SaveId). 런타임·세션 간 불변이며 쿠킹 빌드에서도 그대로 성립한다.
	 * 오너 경로에서 파생하지 않는다 — World Partition 은 런타임 경로에 스트리밍 셀(그리드 좌표)을 포함하므로, 액터를 옮기거나 PIE 와 패키지 빌드를 오갈 때마다 키가 달라진다.
	 * 대신 배치 후 맵을 한 번 저장해야 키가 에셋에 남는다. 저장 전이거나 런타임 스폰된 기믹은 키가 무효라 저장/복원에서 제외된다.
	 */
	virtual FGuid GetSaveId() const override;
	virtual void OnSaveRestored() override;
	//~ End IWxSavable

	/**
	 * Mesh 영역의 상호작용을 켜고 끄며, 켤 때는 그 영역의 프롬프트와 발동 이벤트 태그도 함께 담는다. 'Enable Interaction' 태스크가 상태 진입 시 자기 대상 메시로 호출한다.
	 * 꺼진 영역은 IsInteractionMeshActive 가 false 를 답해 다음 스캔에서 후보에서 빠지고, 어빌리티의 서버 활성 검증에도 걸린다.
	 * 복제하지 않는다 — ST 가 각 피어에서 실행되어 같은 값에 수렴한다.
	 */
	void SetInteractionEnabled(UPrimitiveComponent* Mesh, bool bEnabled, const FText& Prompt, FGameplayTag InteractEvent);

	/** 이번 상호작용의 당사자(플레이어 캐릭터). 상호작용 이동/몽타주 태스크가 읽는다. 상호작용이 없었으면 null. */
	ACharacter* GetInteractingCharacter() const;

	/** 지금 활성인 상태의 Tag. 활성 leaf 에서 위로 올라가며 처음 만나는 유효 태그를 답한다(태그 없는 중간 상태는 건너뛴다). */
	FGameplayTag GetActiveStateTag();

	/** 실행 컨텍스트 확장이 트리의 깨우기 요청을 전달하는 진입점. 잠들어 꺼둔 컴포넌트 틱을 다시 켠다. */
	void NotifyTickRequested();

protected:
	virtual void BeginPlay() override;

	/**
	 * 복제된 StateTag 를 추종한다 — 로컬 트리가 그 상태에 있지 않으면 그 상태에서 재시작해 스냅으로 맞춘다.
	 * 정상 경로에선 멀티캐스트 이벤트가 먼저 도착해 이미 같은 상태이므로 노옵이다. 늦은 참여·스트리밍 인·이벤트 유실이 이 경로로 수렴한다.
	 */
	UFUNCTION()
	void OnRep_StateTag();

	/**
	 * 지금 활성인 상태의 Tag(복제 + SaveGame). 권위 측이 틱마다 갱신하며, 이 값이 곧 세이브 슬롯에 담기는 기믹의 상태다.
	 * 상태에 Tag 를 달지 않으면 저장되지 않으므로, 영속이 필요한 상태에는 반드시 Tag 를 지정한다.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_StateTag, SaveGame, VisibleAnywhere, Category = "Wx")
	FGameplayTag StateTag;

	/**
	 * 지금 상호작용이 켜져 있는 영역들. 멤버십 자체가 활성 상태라 따로 담는 bool 이 없다.
	 * 'Enable Interaction' 태스크가 상태 진입 시 넣고 뺀다. 로컬 전용(복제·SaveGame 아님) — 각 피어의 ST 가 같은 값으로 수렴시킨다.
	 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UPrimitiveComponent>, FWxGimmickInteractionRegion> InteractionRegions;

	/**
	 * 이번 상호작용의 당사자(플레이어 캐릭터). 로컬 전용이며 멀티캐스트가 각 피어에 같은 값을 채운다.
	 * 비영속이라 복원 시엔 비어 있고, 그때는 이동/몽타주 태스크가 초기 진입으로 보아 건너뛴다.
	 */
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> InteractingCharacter;

private:
	/** 상호작용을 전 피어에 알린다. 각 피어가 당사자를 기록하고 자기 트리에 그 영역의 발동 이벤트를 발행한다. 태그는 권위 측에서 정해 실어 보내, 피어마다 영역 설정이 잠시 어긋나도 같은 이벤트가 뜬다. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Interact(UPrimitiveComponent* Source, AActor* Interactor, FGameplayTag EventTag);

	/**
	 * 저장된 StateTag 가 가리키는 상태에서 트리를 시작한다. 태그가 없거나 에셋에서 찾지 못하면 순정 시작(루트 선택)에 맡긴다.
	 * 엔진의 StartTree 가 시작 파라미터를 하드코딩하고 protected·비가상이라, 시작 상태를 넣는 경로만 여기서 다시 조립한다(엔진 업그레이드 시 확인 지점).
	 */
	void StartTreeAtSavedState();

	/** 권위 측에서 활성 상태의 Tag 를 StateTag 에 반영한다. 트리가 상태를 바꾸는 지점(시작·틱) 뒤에서 호출한다. */
	void RefreshStateTag();

	/** WxSave 슬롯 레코드의 안정적 키. 에디터에서 부여되어 에셋에 직렬화되고, 런타임/세션 간 불변이다. */
	UPROPERTY()
	FGuid SaveId;
};
