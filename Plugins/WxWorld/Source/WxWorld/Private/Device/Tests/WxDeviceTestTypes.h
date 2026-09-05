// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Device/WxDevice.h"
#include "StateTreeTaskBase.h"
#include "UObject/CoreNet.h"
#include "WxDeviceTestTypes.generated.h"

/** 자동화 테스트가 추상 베이스의 실제 상호작용 경로를 실행하기 위한 호스트다. */
UCLASS(NotBlueprintable, NotPlaceable, Transient)
class AWxDeviceTestActor : public AWxDevice
{
	GENERATED_BODY()
};

USTRUCT()
struct FWxDeviceTestTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bCompleteOnEnter = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	int32 Value = 0;
};

USTRUCT(meta = (Hidden))
struct FWxDeviceTestTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxDeviceTestTaskInstanceData;
	FWxDeviceTestTask();

	// StateTree GetInstanceDataType의 헤더 정의는 프로젝트 규칙의 예외다.
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/** 프로퍼티 직렬화 테스트에서 객체 참조의 미해소/해소만 제어한다. 네트워크 전송을 모사하지 않는다. */
UCLASS(Transient)
class UWxDeviceTestPackageMap : public UPackageMap
{
	GENERATED_BODY()

public:
	virtual bool SerializeObject(FArchive& Ar, UClass* InClass, UObject*& Object, FNetworkGUID* OutNetGUID = nullptr) override;

	UPROPERTY()
	TObjectPtr<UObject> ResolvedObject;
};
