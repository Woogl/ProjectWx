// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "WxGameplayTagStack.generated.h"

struct FWxGameplayTagStackContainer;

/**
 * 단일 GameplayTag → int32 카운트 페어. FastArray 항목.
 */
USTRUCT(BlueprintType)
struct WXINVENTORY_API FWxGameplayTagStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FWxGameplayTagStack();
	FWxGameplayTagStack(FGameplayTag InTag, int32 InStackCount);

	FString GetDebugString() const;

private:
	friend FWxGameplayTagStackContainer;

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	int32 StackCount = 0;
};

/**
 * GameplayTag 키 기반 카운트 컨테이너. FastArray 로 효율 레플리케이션.
 * 인스턴스별 가변 상태(스택 수, 탄약, 내구도, 강화수치 등)를 일관된 인터페이스로 표현하는 데 사용한다.
 */
USTRUCT(BlueprintType)
struct WXINVENTORY_API FWxGameplayTagStackContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FWxGameplayTagStackContainer();

	/** 권한: Tag 의 카운트에 StackCount 를 더한다. 음수/0 은 무시. */
	void AddStack(FGameplayTag Tag, int32 StackCount);

	/** 권한: Tag 의 카운트를 StackCount 만큼 차감한다. 0 이하가 되면 스택 자체를 제거. */
	void RemoveStack(FGameplayTag Tag, int32 StackCount);

	int32 GetStackCount(FGameplayTag Tag) const;

	bool ContainsTag(FGameplayTag Tag) const;

	//~ Begin FFastArraySerializer
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~ End FFastArraySerializer

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);

private:
	UPROPERTY()
	TArray<FWxGameplayTagStack> Stacks;

	/** 빠른 조회 캐시. 레플리케이션 대상 아님. */
	TMap<FGameplayTag, int32> TagToCountMap;
};

template<>
struct TStructOpsTypeTraits<FWxGameplayTagStackContainer> : public TStructOpsTypeTraitsBase2<FWxGameplayTagStackContainer>
{
	enum { WithNetDeltaSerializer = true };
};
