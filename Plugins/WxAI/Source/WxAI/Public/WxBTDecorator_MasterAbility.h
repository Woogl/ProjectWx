// Copyright Woogle. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "WxBTDecorator_MasterAbility.generated.h"

/** 감지한 Master 스킬에 맞는 분기를 고르고 진입 시 태그를 소비한다. */
UCLASS()
class WXAI_API UWxBTDecorator_MasterAbility : public UBTDecorator
{
	GENERATED_BODY()
public:
	UWxBTDecorator_MasterAbility();
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	
protected:
	virtual void OnNodeActivation(FBehaviorTreeSearchData& SearchData) override;
	UPROPERTY(EditAnywhere, Category="Wx|AI", meta=(Categories="Ability"))
	FGameplayTag MasterAbilityTag;
};
