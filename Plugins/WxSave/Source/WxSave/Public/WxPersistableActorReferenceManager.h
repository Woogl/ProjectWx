// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "WxPersistableActorReferenceManager.generated.h"

class AActor;
class ULevel;

struct FWxPersistableActorReferenceKey
{
	FSoftObjectPath LevelPath;
	FName ActorName;

	bool operator==(const FWxPersistableActorReferenceKey& Other) const;
};

uint32 GetTypeHash(const FWxPersistableActorReferenceKey& Key);

/** 이전 세션의 레벨 경로·액터 이름을 현재 월드의 액터로 변환한다. */
UCLASS()
class WXSAVE_API UWxPersistableActorReferenceManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE_TwoParams(FOnRuntimeActorResolved, AActor* /*ResolvedActor*/, bool /*bSuccess*/);
	DECLARE_DELEGATE_OneParam(FOnLevelPostRestored, ULevel* /*Level*/);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

	void RegisterActor(const FSoftObjectPath& LevelPath, FName LastSessionName, AActor* Actor);
	void UnregisterActor(const FSoftObjectPath& LevelPath, FName LastSessionName);
	AActor* GetPersistedRuntimeActor(const FSoftObjectPath& LevelPath, FName ActorName) const;

	FDelegateHandle ResolveOrRegister(
		const FSoftObjectPath& LevelPath,
		FName ActorName,
		FOnRuntimeActorResolved Callback,
		UObject* Lifetime = nullptr);
	FDelegateHandle AddOnLevelPostRestoreCallback(
		const FSoftObjectPath& LevelPath,
		FOnLevelPostRestored Callback,
		UObject* Lifetime = nullptr);
	void UnregisterResolveCallback(FDelegateHandle Handle);

	/** 현재 로드된 같은 레벨 인스턴스의 LSP 후처리가 끝났는지 확인한다. */
	bool IsLevelCurrentlyPostRestored(const FSoftObjectPath& LevelPath) const;

	void OnLevelPostRestore(const ULevel* Level);

private:
	struct FPendingCallback
	{
		FDelegateHandle Handle;
		FOnRuntimeActorResolved Delegate;
		TWeakObjectPtr<UObject> Lifetime;
		bool bHasLifetime = false;
	};

	struct FPendingLevelCallback
	{
		FDelegateHandle Handle;
		FOnLevelPostRestored Delegate;
		TWeakObjectPtr<UObject> Lifetime;
		bool bHasLifetime = false;
	};

	void FirePending(const FWxPersistableActorReferenceKey& Key, AActor* ResolvedActor, bool bSuccess);
	void FireLevelPending(const FSoftObjectPath& LevelPath, ULevel* Level);

	TMap<FSoftObjectPath, TWeakObjectPtr<const ULevel>> PostRestoredLevelInstances;
	TMap<FWxPersistableActorReferenceKey, TWeakObjectPtr<AActor>> RegisteredActors;
	TMap<FWxPersistableActorReferenceKey, TArray<FPendingCallback>> PendingCallbacks;
	TMap<FDelegateHandle, FWxPersistableActorReferenceKey> HandleToKey;
	TMap<FSoftObjectPath, TArray<FPendingLevelCallback>> PendingLevelCallbacks;
	TMap<FDelegateHandle, FSoftObjectPath> LevelHandleToPath;
};
