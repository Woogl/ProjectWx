// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"

// 아래 정의를 cpp 로 내리지 못하는 것은 코딩 규칙 6 의 예외다 — 페이로드 타입이 태스크마다 다른 클래스 템플릿이라 인스턴스화 지점이 호출부에 있다.

/**
 * 통보가 올 때까지 Running 으로 머무는 태스크들의 공용 등록부. 등록 하나가 대기 중인 노드 하나다.
 * 완료 통보는 상태가 살아 있는 동안에만 유효한 약한 실행 컨텍스트로 보낸다.
 */
template<typename PayloadType>
struct TWxStateTreeWaitRegistry
{
	struct FWait
	{
		int32 Handle = INDEX_NONE;
		PayloadType Payload;
		FStateTreeWeakExecutionContext Context;
	};

	/** 해제에 쓸 핸들을 돌려준다. 태스크는 이것을 인스턴스 데이터에 담아 두었다가 ExitState 에서 되돌린다. */
	int32 Add(FStateTreeExecutionContext& Context, const PayloadType& Payload)
	{
		FWait& Wait = Waits.AddDefaulted_GetRef();

		// 재사용하지 않으므로 뒤늦은 해제 요청이 엉뚱한 등록을 걷어가지 않는다.
		Wait.Handle = NextHandle++;
		Wait.Payload = Payload;
		Wait.Context = Context.MakeWeakExecutionContext();

		return Wait.Handle;
	}

	void Remove(int32 Handle)
	{
		for (int32 Index = 0; Index < Waits.Num(); ++Index)
		{
			if (Waits[Index].Handle == Handle)
			{
				Waits.RemoveAt(Index);
				break;
			}
		}
	}

	/**
	 * NotifyWorld 의 살아 있는 등록만 술어에 넘기고, true 를 답한 노드를 Succeeded 로 완료시킨다.
	 * 술어는 bool(const PayloadType&, UObject* Owner) 이며, 로케이터 해석 등 오너가 필요한 판정에 그 오너를 쓴다.
	 */
	template<typename PredicateType>
	void FinishMatching(const UWorld* NotifyWorld, PredicateType&& Predicate)
	{
		// 오너가 사라진 등록을 이 자리에서 걷어내므로 역순으로 돈다 — 아직 보지 않은 낮은 인덱스는 밀리지 않는다.
		// FinishTask 는 완료 상태만 세우므로 완료가 스윕 도중 등록을 걷어가지는 않는다.
		for (int32 Index = Waits.Num() - 1; Index >= 0; --Index)
		{
			// 술어나 완료가 등록부를 건드려도 이 항목이 매달리지 않도록 복사해 둔다.
			const FWait Wait = Waits[Index];

			// 트리째 사라진(오너 파괴) 등록은 통보할 곳이 없으므로 이 참에 걷어낸다.
			TStrongObjectPtr<UObject> Owner = Wait.Context.GetOwner();
			if (!Owner)
			{
				Waits.RemoveAt(Index);
				continue;
			}

			// PIE 는 서버·클라 월드가 한 프로세스에 산다. 남의 월드에서 온 통보로 완료되지 않도록 좁힌다.
			if (Owner->GetWorld() != NotifyWorld)
			{
				continue;
			}

			if (Predicate(Wait.Payload, Owner.Get()))
			{
				Wait.Context.FinishTask(EStateTreeFinishTaskType::Succeeded);
			}
		}
	}

private:
	TArray<FWait> Waits;

	int32 NextHandle = 0;
};
