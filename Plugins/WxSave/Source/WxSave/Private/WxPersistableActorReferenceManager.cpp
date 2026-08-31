// Copyright Woogle. All Rights Reserved.

#include "WxPersistableActorReferenceManager.h"

#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool FWxPersistableActorReferenceKey::operator==(const FWxPersistableActorReferenceKey& Other) const
{
	return LevelPath == Other.LevelPath && ActorName == Other.ActorName;
}

uint32 GetTypeHash(const FWxPersistableActorReferenceKey& Key)
{
	return HashCombine(GetTypeHash(Key.LevelPath), GetTypeHash(Key.ActorName));
}

bool UWxPersistableActorReferenceManager::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && !World->IsNetMode(NM_Client);
}

void UWxPersistableActorReferenceManager::Deinitialize()
{
	TArray<FWxPersistableActorReferenceKey> Keys;
	PendingCallbacks.GetKeys(Keys);
	for (const FWxPersistableActorReferenceKey& Key : Keys)
	{
		FirePending(Key, nullptr, false);
	}

	TArray<FSoftObjectPath> LevelPaths;
	PendingLevelCallbacks.GetKeys(LevelPaths);
	for (const FSoftObjectPath& LevelPath : LevelPaths)
	{
		FireLevelPending(LevelPath, nullptr);
	}

	RegisteredActors.Empty();
	PendingCallbacks.Empty();
	HandleToKey.Empty();
	PendingLevelCallbacks.Empty();
	LevelHandleToPath.Empty();
	PostRestoredLevelInstances.Empty();

	Super::Deinitialize();
}

bool UWxPersistableActorReferenceManager::IsLevelCurrentlyPostRestored(const FSoftObjectPath& LevelPath) const
{
	const TWeakObjectPtr<const ULevel>* TrackedLevel = PostRestoredLevelInstances.Find(LevelPath);
	return TrackedLevel && TrackedLevel->IsValid() && TrackedLevel->Get() == LevelPath.ResolveObject();
}

void UWxPersistableActorReferenceManager::RegisterActor(
	const FSoftObjectPath& LevelPath,
	FName LastSessionName,
	AActor* Actor)
{
	if (!Actor || LevelPath.IsNull() || LastSessionName.IsNone())
	{
		return;
	}

	RegisteredActors.Add({LevelPath, LastSessionName}, Actor);
}

void UWxPersistableActorReferenceManager::UnregisterActor(
	const FSoftObjectPath& LevelPath,
	FName LastSessionName)
{
	if (!LevelPath.IsNull() && !LastSessionName.IsNone())
	{
		RegisteredActors.Remove({LevelPath, LastSessionName});
	}
}

AActor* UWxPersistableActorReferenceManager::GetPersistedRuntimeActor(
	const FSoftObjectPath& LevelPath,
	FName ActorName) const
{
	if (LevelPath.IsNull() || ActorName.IsNone())
	{
		return nullptr;
	}

	if (const TWeakObjectPtr<AActor>* Registered = RegisteredActors.Find({LevelPath, ActorName}))
	{
		if (AActor* Actor = Registered->Get())
		{
			return Actor;
		}
	}

	const ULevel* Level = Cast<ULevel>(LevelPath.ResolveObject());
	if (!Level)
	{
		return nullptr;
	}

	for (AActor* Actor : Level->Actors)
	{
		if (IsValid(Actor) && Actor->GetFName() == ActorName)
		{
			return Actor;
		}
	}

	return nullptr;
}

FDelegateHandle UWxPersistableActorReferenceManager::ResolveOrRegister(
	const FSoftObjectPath& LevelPath,
	FName ActorName,
	FOnRuntimeActorResolved Callback,
	UObject* Lifetime)
{
	if (LevelPath.IsNull() || ActorName.IsNone())
	{
		Callback.ExecuteIfBound(nullptr, false);
		return FDelegateHandle();
	}

	if (AActor* ResolvedActor = GetPersistedRuntimeActor(LevelPath, ActorName))
	{
		Callback.ExecuteIfBound(ResolvedActor, true);
		return FDelegateHandle();
	}

	if (IsLevelCurrentlyPostRestored(LevelPath))
	{
		Callback.ExecuteIfBound(nullptr, false);
		return FDelegateHandle();
	}

	FPendingCallback Pending;
	Pending.Handle = FDelegateHandle(FDelegateHandle::GenerateNewHandle);
	Pending.Delegate = MoveTemp(Callback);
	Pending.Lifetime = Lifetime;
	Pending.bHasLifetime = Lifetime != nullptr;

	const FWxPersistableActorReferenceKey Key{LevelPath, ActorName};
	PendingCallbacks.FindOrAdd(Key).Add(Pending);
	HandleToKey.Add(Pending.Handle, Key);
	return Pending.Handle;
}

FDelegateHandle UWxPersistableActorReferenceManager::AddOnLevelPostRestoreCallback(
	const FSoftObjectPath& LevelPath,
	FOnLevelPostRestored Callback,
	UObject* Lifetime)
{
	if (LevelPath.IsNull())
	{
		Callback.ExecuteIfBound(nullptr);
		return FDelegateHandle();
	}

	if (IsLevelCurrentlyPostRestored(LevelPath))
	{
		Callback.ExecuteIfBound(Cast<ULevel>(LevelPath.ResolveObject()));
		return FDelegateHandle();
	}

	FPendingLevelCallback Pending;
	Pending.Handle = FDelegateHandle(FDelegateHandle::GenerateNewHandle);
	Pending.Delegate = MoveTemp(Callback);
	Pending.Lifetime = Lifetime;
	Pending.bHasLifetime = Lifetime != nullptr;

	PendingLevelCallbacks.FindOrAdd(LevelPath).Add(Pending);
	LevelHandleToPath.Add(Pending.Handle, LevelPath);
	return Pending.Handle;
}

void UWxPersistableActorReferenceManager::UnregisterResolveCallback(FDelegateHandle Handle)
{
	if (!Handle.IsValid())
	{
		return;
	}

	if (const FSoftObjectPath* FoundLevelPath = LevelHandleToPath.Find(Handle))
	{
		const FSoftObjectPath LevelPath = *FoundLevelPath;
		LevelHandleToPath.Remove(Handle);
		if (TArray<FPendingLevelCallback>* Callbacks = PendingLevelCallbacks.Find(LevelPath))
		{
			Callbacks->RemoveAll([Handle](const FPendingLevelCallback& Callback)
			{
				return Callback.Handle == Handle;
			});
			if (Callbacks->IsEmpty())
			{
				PendingLevelCallbacks.Remove(LevelPath);
			}
		}
		return;
	}

	const FWxPersistableActorReferenceKey* FoundKey = HandleToKey.Find(Handle);
	if (!FoundKey)
	{
		return;
	}

	const FWxPersistableActorReferenceKey Key = *FoundKey;
	HandleToKey.Remove(Handle);
	if (TArray<FPendingCallback>* Callbacks = PendingCallbacks.Find(Key))
	{
		Callbacks->RemoveAll([Handle](const FPendingCallback& Callback)
		{
			return Callback.Handle == Handle;
		});
		if (Callbacks->IsEmpty())
		{
			PendingCallbacks.Remove(Key);
		}
	}
}

void UWxPersistableActorReferenceManager::OnLevelPostRestore(const ULevel* Level)
{
	if (!Level)
	{
		return;
	}

	const FSoftObjectPath LevelPath(Level);
	PostRestoredLevelInstances.Add(LevelPath, Level);

	TArray<FWxPersistableActorReferenceKey> KeysToFire;
	for (const TPair<FWxPersistableActorReferenceKey, TArray<FPendingCallback>>& Pair : PendingCallbacks)
	{
		if (Pair.Key.LevelPath == LevelPath)
		{
			KeysToFire.Add(Pair.Key);
		}
	}

	for (const FWxPersistableActorReferenceKey& Key : KeysToFire)
	{
		AActor* ResolvedActor = GetPersistedRuntimeActor(Key.LevelPath, Key.ActorName);
		FirePending(Key, ResolvedActor, ResolvedActor != nullptr);
	}

	FireLevelPending(LevelPath, const_cast<ULevel*>(Level));
}

void UWxPersistableActorReferenceManager::FirePending(
	const FWxPersistableActorReferenceKey& Key,
	AActor* ResolvedActor,
	bool bSuccess)
{
	TArray<FPendingCallback> Callbacks;
	if (!PendingCallbacks.RemoveAndCopyValue(Key, Callbacks))
	{
		return;
	}

	for (FPendingCallback& Callback : Callbacks)
	{
		HandleToKey.Remove(Callback.Handle);
		if (!Callback.bHasLifetime || Callback.Lifetime.IsValid())
		{
			Callback.Delegate.ExecuteIfBound(ResolvedActor, bSuccess);
		}
	}
}

void UWxPersistableActorReferenceManager::FireLevelPending(
	const FSoftObjectPath& LevelPath,
	ULevel* Level)
{
	TArray<FPendingLevelCallback> Callbacks;
	if (!PendingLevelCallbacks.RemoveAndCopyValue(LevelPath, Callbacks))
	{
		return;
	}

	for (FPendingLevelCallback& Callback : Callbacks)
	{
		LevelHandleToPath.Remove(Callback.Handle);
		if (!Callback.bHasLifetime || Callback.Lifetime.IsValid())
		{
			Callback.Delegate.ExecuteIfBound(Level);
		}
	}
}
