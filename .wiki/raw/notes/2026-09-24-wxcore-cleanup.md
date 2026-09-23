---
title: "WxCore 정리: 쓰지 않는 태그·모듈 클래스 제거"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, foundation, damage, architecture]
summary: "읽는 곳이 없는 Damage.Guarded와 사용처가 없는 Device.Locked 네이티브 태그를 지웠다. WxCore는 순수 정의만 두는 모듈이라 빈 FWxCoreModule을 없애고 FDefaultModuleImpl로 등록한다. WxAI의 MirrorMovement 순간이동 자동화 테스트와 테스트용 friend 선언도 지웠다."
---

# WxCore 정리

2026-09-24 커밋 `db5898fa8`(태그)·`6ec2d9e8f`(모듈 클래스)·`b746867c3`(테스트). WxCore 소스 전체를 읽고 태그마다 WxCore 밖 C++(`Source`·`Plugins`)와 `Content`·`Plugins/*/Content`·`Config` 바이너리 문자열을 검색해 사용처를 셌다.

## 사용자 결정

- "WxCore에서 게임플레이 로직 구현 안하고, 순수 정의만 할거라서요." — 빈 `StartupModule`/`ShutdownModule`만 있던 `FWxCoreModule`을 없애는 근거.
- `Damage_Guarded`·`Device_Locked` 제거, `Tests/WxMirrorMovementAbilityTeleportTest` 제거를 지시했다.

## 변경

- `Damage.Guarded`: ExecCalc(`UWxExecCalc_Damage`)가 일반 가드 히트에 붙이기만 하고 읽는 C++·에셋이 없었다. 2026-09-23 정방향 흐름 재설계 때 `Damage.PerfectGuarded`와 함께 추가됐고, 같은 날 `FWxDamageResult`(`bGuarded` 포함)가 삭제된 뒤로 읽는 곳이 없다. 태그와 부착 분기만 지웠고, 가드 경감·SP 차감에 쓰는 `bGuardHit` 판정은 그대로다.
- `Device.Locked`: 2026-09-22 `f87b5e3ae`에서 공용 잠금 상태로 추가됐지만 StateTree 에셋을 포함해 참조가 0건이었다. 같은 커밋에서 잠금은 받는 쪽 대기 태스크 활성 여부로 판단하게 바뀌었다.
- `FWxCoreModule`: `WxCoreModule.h`를 포함하는 곳이 없었다. 헤더를 지우고 `IMPLEMENT_MODULE(FDefaultModuleImpl, WxCore)`로 등록한다.
- WxAI `FWxMirrorMovementAbilityTeleportTest`(`Wx.AI.MirrorMovement.AbilityTeleport`, `282071607`에서 추가)와 `UWxBTService_MirrorMovement`의 `friend` 선언을 지웠다. 같은 폴더의 `WxPatrolPathSpawnTest.cpp`는 이번 지시 범위 밖이라 남겼다.

## 조사만 하고 남긴 후보

- C++·에셋 참조가 모두 0건: `Ability.Skill.4`·`Cooldown.Skill.4`(+ `UWxEffect_Cooldown_Skill_4`), `Ability.Pattern.4`~`9`. 클래스 이름은 끝의 `_N`이 FName 숫자로 분리 저장되어 문자열 검색으로 에셋 참조를 확정할 수 없다(태그 `A.B.4`는 분리되지 않는다).
- `AWxDevice::GetInteractionPrompt`는 `GetInteractionOptions`를 재정의해 호출되지 않지만 `IWxInteractable`의 순수 가상이라 남아 있다.

## 검증

WxEditor Win64 Development 빌드 성공(`Saved/Logs/BuildDoctor/build_2026-09-24_021953_543_33296.log`). 동작 변화가 없는 제거라 플레이는 확인하지 않았다.

근거: [태그 선언](../../../Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h), [모듈 등록](../../../Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp), [대미지 ExecCalc](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp), [Mirror 서비스](../../../Plugins/WxAI/Source/WxAI/Public/WxBTService_MirrorMovement.h).
