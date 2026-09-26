---
title: "AnimNotify 타임라인 이름 축약 규칙"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, animation, editor]
summary: "17종 Notify의 라벨을 종류와 대표 값 하나로 통일한 사용자 합의와 구현 근거."
---

# AnimNotify 타임라인 이름 축약 규칙

2026-09-25 사용자가 긴 라벨 제안을 줄이도록 요청하고 `종류: 대표 값 하나` 형식을 승인했다. 소켓·프리셋·오프셋·정지 거리·FOV는 Details에서 확인한다. Row·에셋 이름을 임의로 자르지 않으며 클래스 이름 끝 `_C`만 제거한다. 미설정은 `None`, 스냅 이동·회전 모두 비활성은 `Off`다.

표시 종류는 Attack, Area, Finisher, Victim, Cutscene, Projectile, Summon, Despawn, Effect, Slow, Rush, Snap, Camera, Noise다. 고정 표식은 Recovery, Combo Window, Use Item이다. Rush는 LockOn/Master/Minion, Snap은 Move/Turn/Move+Turn/Off, Camera는 Follow/Fixed로 구분한다. Slow는 소수점 두 자리 배율, Noise는 cm 거리다.

코드 정적 확인 근거이며 에디터 화면 검증은 포함하지 않는다. 빌드 결과는 [작업 기록](../../../.agents/workflow/tasks/animnotify-labels.md)에 별도로 남긴다.

## 구현 근거

### WxAnimNotify_ReportNoise

[WxAnimNotify_ReportNoise.cpp](../../../Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp) · SHA-256 `60d18ac774198ee0c335fb078b020d47bbe7d9920b024beea12c9a6d013e0767`

```cpp
FString UWxAnimNotify_ReportNoise::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Noise: %scm"), *FString::SanitizeFloat(HearingDistance, 0));
}
```

### WxAnimNotify_AreaDamage

[WxAnimNotify_AreaDamage.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp) · SHA-256 `4241a9460b00d9f9863f2441db130e8dd786d3790e9a6584bc2f4fc874debf0b`

```cpp
FString UWxAnimNotify_AreaDamage::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Area: %s"), DamageDataRow.IsNull() ? TEXT("None") : *DamageDataRow.RowName.ToString());
}
```

### WxAnimNotify_DespawnMinion

[WxAnimNotify_DespawnMinion.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_DespawnMinion.cpp) · SHA-256 `b662529ca88118e07af974252f9c4b49d8852b9051f98c5fa0abfc4c69da2c0a`

```cpp
FString UWxAnimNotify_DespawnMinion::GetNotifyName_Implementation() const
{
	FString ClassName = MinionClass ? MinionClass->GetName() : TEXT("None");
	ClassName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("Despawn: %s"), *ClassName);
}
```

### WxAnimNotify_FinisherDamage

[WxAnimNotify_FinisherDamage.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_FinisherDamage.cpp) · SHA-256 `3d8c20c8223ac18d55d4fec307c15e3e240fb5f38ca2703978a983df8fafe2a5`

```cpp
FString UWxAnimNotify_FinisherDamage::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Finisher: %s"), DamageDataRow.IsNull() ? TEXT("None") : *DamageDataRow.RowName.ToString());
}
```

### WxAnimNotify_FinisherVictim

[WxAnimNotify_FinisherVictim.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_FinisherVictim.cpp) · SHA-256 `03134665144c7399a49a90c59e28d5240b8dde93ab1d5dfaa2c9958e902ebb36`

```cpp
FString UWxAnimNotify_FinisherVictim::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Victim: %s"), VictimMontage ? *VictimMontage->GetName() : TEXT("None"));
}
```

### WxAnimNotify_SkillCutscene

[WxAnimNotify_SkillCutscene.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_SkillCutscene.cpp) · SHA-256 `60eca12506f070588f0b0c41dac89982bd7148db0ab9158b9d566554c6efcd9e`

```cpp
FString UWxAnimNotify_SkillCutscene::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Cutscene: %s"), Sequence ? *Sequence->GetName() : TEXT("None"));
}
```

### WxAnimNotify_SpawnMinion

[WxAnimNotify_SpawnMinion.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_SpawnMinion.cpp) · SHA-256 `d7b1b28d23137c06b128714668c987eeca13bb7240b7e758edb1979a095b1003`

```cpp
FString UWxAnimNotify_SpawnMinion::GetNotifyName_Implementation() const
{
	FString ClassName = MinionClass ? MinionClass->GetName() : TEXT("None");
	ClassName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("Summon: %s"), *ClassName);
}
```

### WxAnimNotify_SpawnProjectile

[WxAnimNotify_SpawnProjectile.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_SpawnProjectile.cpp) · SHA-256 `b930d26d670dd7e26592b967437899f279dfa95647c6adf25cd1b93bfff47b9e`

```cpp
FString UWxAnimNotify_SpawnProjectile::GetNotifyName_Implementation() const
{
	FString ClassName = ProjectileClass ? ProjectileClass->GetName() : TEXT("None");
	ClassName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("Projectile: %s"), *ClassName);
}
```

### WxAnimNotify_StartRecovery

[WxAnimNotify_StartRecovery.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_StartRecovery.cpp) · SHA-256 `342512b20a9b47373d6ae47cd5f483fda6ec3bfe86dc60a5509ca5f14246c859`

```cpp
FString UWxAnimNotify_StartRecovery::GetNotifyName_Implementation() const
{
	return TEXT("Recovery");
}
```

### WxAnimNotifyState_ApplyGameplayEffect

[WxAnimNotifyState_ApplyGameplayEffect.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp) · SHA-256 `cc6d032bfc8743990f28e4d6ec2cc0e1fad0d29d06baf5de04b2bab29cf2c8ba`

```cpp
FString UWxAnimNotifyState_ApplyGameplayEffect::GetNotifyName_Implementation() const
{
	FString ClassName = EffectClass ? EffectClass->GetName() : TEXT("None");
	ClassName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("Effect: %s"), *ClassName);
}
```

### WxAnimNotifyState_CameraMove

[WxAnimNotifyState_CameraMove.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp) · SHA-256 `54b8da3a17893b02c74f339bcbb380dcec3ccc3e9eadb9b28e8234a78a0aba75`

```cpp
FString UWxAnimNotifyState_CameraMove::GetNotifyName_Implementation() const
{
	return bAttachToOwner ? TEXT("Camera: Follow") : TEXT("Camera: Fixed");
}
```

### WxAnimNotifyState_ComboWindow

[WxAnimNotifyState_ComboWindow.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ComboWindow.cpp) · SHA-256 `a72663b74ebd4bd47acad1621e2964517b8f1f65ffb9e8e114a6dbd0fe203cef`

```cpp
FString UWxAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("Combo Window");
}
```

### WxAnimNotifyState_Rush

[WxAnimNotifyState_Rush.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp) · SHA-256 `5213fc3618f36e03fc07ebcf3b3ad5558daa25e862a54a9736fffc0c235df0ae`

```cpp
FString UWxAnimNotifyState_Rush::GetNotifyName_Implementation() const
{
	switch (TargetSource)
	{
	case EWxRushTarget::LockOnTarget:
		return TEXT("Rush: LockOn");
	case EWxRushTarget::Master:
		return TEXT("Rush: Master");
	case EWxRushTarget::Minion:
		return TEXT("Rush: Minion");
	default:
		return TEXT("Rush: None");
	}
}
```

### WxAnimNotifyState_SlowTime

[WxAnimNotifyState_SlowTime.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_SlowTime.cpp) · SHA-256 `d252f9b696820318edf48d214b3a58651d1ee07700c00abbc2863cab3c01b48f`

```cpp
FString UWxAnimNotifyState_SlowTime::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Slow: x%.2f"), TimeDilation);
}
```

### WxAnimNotifyState_SnapToTarget

[WxAnimNotifyState_SnapToTarget.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_SnapToTarget.cpp) · SHA-256 `2c295456e9af12c93dd35e32c660dfb993b52f27bc8b73b1bc7b125a3ae6611b`

```cpp
FString UWxAnimNotifyState_SnapToTarget::GetNotifyName_Implementation() const
{
	if (bSnapLocation)
	{
		return bSnapRotation ? TEXT("Snap: Move+Turn") : TEXT("Snap: Move");
	}

	return bSnapRotation ? TEXT("Snap: Turn") : TEXT("Snap: Off");
}
```

### WxAnimNotifyState_WeaponAttack

[WxAnimNotifyState_WeaponAttack.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp) · SHA-256 `7513d494f0eb9d52300de4d236d2fbae6ea2650aeb9e1416b91fecc70e9c07d7`

```cpp
FString UWxAnimNotifyState_WeaponAttack::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Attack: %s"), DamageDataRow.IsNull() ? TEXT("None") : *DamageDataRow.RowName.ToString());
}
```

### WxAnimNotify_UseItem

[WxAnimNotify_UseItem.cpp](../../../Plugins/WxInventory/Source/WxInventory/Private/AnimNotify/WxAnimNotify_UseItem.cpp) · SHA-256 `8163613baaa75ded11f1c4156af42860b2bbe20b69d3e172d0a3e624c34b46d1`

```cpp
FString UWxAnimNotify_UseItem::GetNotifyName_Implementation() const
{
	return TEXT("Use Item");
}
```

