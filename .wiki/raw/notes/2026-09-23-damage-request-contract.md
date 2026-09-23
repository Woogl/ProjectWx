---
title: "명시적 DamageRequest와 기존 BP 호환 경계"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage, static-review]
summary: "동기 C++ DamageRequest 입력, 네이티브 호출부의 출처 선택, 기존 Blueprint 호환 어댑터 분리를 확인했다."
revision: 47b7f8bd7
---

# 명시적 DamageRequest와 기존 BP 호환 경계

2026-09-23 HEAD `47b7f8bd7` 및 미커밋 작업 트리 정적 조사다. 이번 범위의 코드 해시는 아래에 기록한다. 빌드·자동화 및 사용자의 이전 단계 플레이 확인은 Workflow Task에서 별도로 관리하며, 이 원자료는 새 단계의 플레이 검증을 뜻하지 않는다.

## 계약

- `FWxDamageRequest`는 SourceASC, Instigator, Causer, Target, SourceAbility, DamageLevel, DamageTableRow, HitResult를 담는 동기 C++ 구조체다. UObject 포인터 수명을 소유하지 않으므로 저장/지연 실행용 객체가 아니다.
- `ApplyDamageRequest`는 명시한 출처를 검증한 뒤 기존 Hit GE 경로에 연결한다. Causer의 Owner나 현재 AnimatingAbility, 구체 투사체 클래스를 추론하지 않는다. Context의 Instigator ASC와 입력 SourceASC가 어긋나면 InvalidSource로 거부한다.
- 무기, 범위 공격, 피니셔는 기존과 동일하게 각 피해 적용 직전 ASC의 AnimatingAbility/레벨을 읽어 요청에 넣는다. 범위 공격은 대상별로 읽으므로 앞선 대상의 반응이 출처 상태를 바꾸는 경우도 기존 시점을 유지한다.
- 투사체는 현재 Owner 기반 출처(자체 ASC가 있으면 우선), 저장된 ProjectileLevel, SourceAbility=nullptr을 전달한다. 반사로 Owner/Instigator가 바뀌어도 발사 레벨은 유지한다. HP/ATK/DEF 전체를 발사 시점에 스냅샷한 것이 아니다.
- `ApplyDamage`와 `ApplyDamageWithResult`의 BP API는 유지한다. 옛 출처 추론은 `WxDamageCompatibility.cpp`가 요청으로 변환한다. 해당 어댑터에는 호환성을 위한 투사체 캐스트와 현재 Ability 조회가 남아 있다.
- DataTable 저작/해석, 계산/반응 순서, 실행 정의 스냅샷은 이번 변경에서 재설계하지 않는다.

## 코드 근거

경로 기준은 `Plugins/WxCombat/Source/WxCombat/`이다.

| 파일 | 확인 범위 | SHA-256 |
|---|---|---|
| [WxDamageRequest.h](../../../Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageRequest.h) | 입력/수명 계약 전체 | `2083c88ddcab1b94d929cf87532e4027ce8ba52aa8674c5d4941c72e1141ecdd` |
| [WxCombatLibrary.h](../../../Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h) | 새 네이티브 API와 기존 BP API | `6fee340306b79dd5806a7f80097d8cd81326f7e91cf099a705ad31239d428fba` |
| [WxCombatLibrary.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp) | ApplyDamageRequest | `24872147c51cfb7f9c47cbf82b20ec4391335cc123c784c55d6ae1c731f10fd0` |
| [WxDamageCompatibility.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageCompatibility.cpp) | 호환 API 구현 전체 | `49c9dfc742fe69c40c9b95c928e82166cc8cdb614592ba8228ab914f6e89a72e` |
| [WxWeaponBase.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp) | ProcessHit | `7038259c2639c05367aaa05f5c106e0e408e4cf9f5bf05c1c9f4fd886ccfa58b` |
| [WxProjectileBase.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp) | HandleHitCollisionOverlap | `3f3a4c7e33b788f565a9781e872023b427ad84b40e6f595330fcf5e2db667390` |
| [WxAnimNotify_AreaDamage.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp) | Notify | `9e044f8b2e4226ef0a5a70e4fe26943c2b5de077a42f09e5f5199e8c159c74fc` |
| [WxFinisherDamageComponent.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp) | HandleFinisherDamageEvent | `58681ff9b3a714f7bd8fe6ef60ca271ca5885a1f971da7b9942f9bbbcb783962` |
