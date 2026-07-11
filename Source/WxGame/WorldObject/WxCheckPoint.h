// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "WxCheckPoint.generated.h"

class UGameplayEffect;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 체크 포인트.
 * 플레이어가 상호작용하면 HP를 최대치로 회복시키고, 이 지점을 부활 지점으로 저장한다.
 * HealEffect 프로퍼티에 HP를 MaxHP로 설정하는 GameplayEffect를 지정해야 한다.
 *
 * 영속 State·StateTree 가 없는 반복형 즉시 효과(다크소울 모닥불)라 기믹 인프라(AWxGimmick) 대신 APlayerStart 를 상속한다.
 * APlayerStart 상속으로 배치된 인스턴스가 GameMode 의 ChoosePlayerStart 흐름에서 부활/시작 지점 후보가 된다.
 *
 * 식별자(PlayerStartTag)는 디자이너가 인스턴스마다 고유하게 직접 지정한다(예: "CP_Chapel"). 부활 지점은 이 태그로 저장·복원된다.
 * 최초 접속(저장된 시작지점 없음) 시 시작지점은 bIsDefaultStart 으로 지정한다.
 * 태그 미지정·중복·bIsDefaultStart 다중은 CheckForErrors(Map Check) 가 에디터에서 잡는다.
 */
UCLASS(Abstract)
class AWxCheckPoint : public APlayerStart
{
	GENERATED_BODY()

public:
	AWxCheckPoint(const FObjectInitializer& ObjectInitializer);

	/** 최초 접속 시작지점으로 지정됐는지 여부. 스폰 컴포넌트가 시작지점 후보 판정에 사용한다. */
	bool IsDefaultStart() const;

#if WITH_EDITOR
	//~ Begin AActor
	// 복제본은 원본의 PlayerStartTag·bIsDefaultStart 을 복사받는다. 초기화해 "명백히 미완성" 상태로 만들어 아래 CheckForErrors 가 잡게 한다.
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	// Map Check: 태그 미지정·다른 PlayerStart 와 태그 중복·bIsDefaultStart 다중 지정을 에러로 리포트한다.
	virtual void CheckForErrors() override;
	//~ End AActor
#endif

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	/** 상호작용 시 적용할 회복 GameplayEffect. HP를 MaxHP로 설정하는 GE를 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<UGameplayEffect> HealEffect;

	/** true 면 이 레벨 최초 접속(저장된 시작지점 없음) 시 시작지점으로 사용된다. 레벨당 정확히 1개에만 지정한다. */
	UPROPERTY(EditInstanceOnly, Category = "Wx")
	bool bIsDefaultStart = false;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);
};
