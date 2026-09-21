// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeComponent.h"
#include "GameplayTagContainer.h"
#if WITH_EDITOR
#include "UObject/PropertyText.h"
#endif
#include "WxDeviceStateTreeComponent.generated.h"

class ACharacter;

/** 복제 사이의 여러 진입은 최신 진입으로 합쳐지며 과거 연출을 큐로 재생하지 않는다. */
USTRUCT()
struct FWxDeviceStateSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	FName StateTagName;

	/** 서버의 태그 상태가 바뀔 때마다 1 오른다. 0은 미수신이다. 클라는 직전+1 이면 라이브로, 그 밖(첫 수신·건너뜀)이면 복원으로 들어간다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	uint32 EntrySerial = 0;

	/** 선택적 실행 문맥. 참조가 미해소여도 상태는 적용하고, 해소되면 당사자만 갱신한다. */
	UPROPERTY()
	TObjectPtr<ACharacter> Interactor;

	/** 이 진입을 일으킨 작동 신호의 선택지 값. 상태 태그만으로는 알 수 없는 목적지(엘리베이터 정차 지점)를 클라가 같은 값으로 재현하게 한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	int32 SelectedOptionValue = INDEX_NONE;
};

/**
 * 장치 트리는 서버와 클라가 같은 에셋을 각자 돌리고, 어느 태그 상태에 있을지는 서버가 정한다.
 * 서버는 활성 태그 상태가 바뀔 때마다 스냅샷을 발행한다. 클라는 그 통지를 받은 순간에만 판단해, 그 상태가 아니면 그 상태로 들어간다 — 통지 사이에 제 타이머로 앞서가도 되감지 않는다.
 * 태그는 루트 에셋에서 유일한 상태 식별자다. 같은 태그로의 재진입과 한 틱 안에 지나간 태그 상태는 발행되지 않으므로, 클라에 보여야 할 연출은 잠깐이라도 머무는 별도 태그 상태에 둔다.
 */
UCLASS()
class UWxDeviceStateTreeComponent : public UStateTreeComponent
{
	GENERATED_BODY()

public:
	UWxDeviceStateTreeComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void StartLogic() override;
	virtual void RestartLogic() override;
	virtual bool IsRunning() const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	FGameplayTag GetStateTag() const;
	FString DescribeSynchronization() const;
	bool IsRestoringState() const;

#if WITH_GAMEPLAY_DEBUGGER
	virtual FString GetDebugInfoString() const override;
#endif

protected:
	UFUNCTION()
	void OnRep_StateSnapshot(const FWxDeviceStateSnapshot& Previous);

	UPROPERTY(VisibleInstanceOnly, Transient, ReplicatedUsing = OnRep_StateSnapshot, Category = "Wx")
	FWxDeviceStateSnapshot StateSnapshot;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (GetOptions = "GetInitialStateOptions"))
	FName InitialState;

private:
	void SynchronizeAfterStart();
	void PublishState();
	void EnterState(FGameplayTag Tag, bool bRestore);
	void ApplyInteractor();
	FGameplayTag FindActiveStateTag() const;
	bool HasState(FGameplayTag Tag) const;

#if WITH_EDITOR
	UFUNCTION()
	TArray<FPropertyTextFName> GetInitialStateOptions() const;
#endif

	FGameplayTag InitialTarget;
	bool bRestoringState = false;
};
