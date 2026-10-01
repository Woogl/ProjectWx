// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotify_SpawnProjectile.h"
#include "Weapons/WxProjectileBase.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotify_SpawnProjectile::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->AttackColor;
}
#endif

FString UWxAnimNotify_SpawnProjectile::GetNotifyName_Implementation() const
{
	FString ClassName = ProjectileClass ? ProjectileClass->GetName() : TEXT("None");
	ClassName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("Projectile: %s"), *ClassName);
}
