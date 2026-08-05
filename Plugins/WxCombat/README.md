# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 구축한 액션 RPG 전투의 핵심 도메인 플러그인. 어빌리티·어트리뷰트·대미지 파이프라인부터 무기/투사체, 락온 타게팅, 히트박스 AnimNotify, 시간 감속(히트스톱)까지 "때리고 맞고 반응하는" 흐름 전체를 담당한다.

## 책임
**담당**
- 어빌리티 프레임워크: 프로젝트 공용 베이스(`UWxAbilityBase`)와 공격/회피/가드/스킬/궁극기/그로기/피격/사망/락온 등 구체 어빌리티, 데이터 주도 부여(`UWxAbilitySet`), 입력 라우팅 ASC(`UWxAbilitySystemComponent`)
- 스탯: `UWxCombatAttributeSet`의 Vital/Resource/Combat 어트리뷰트와 복제·클램프·파생 계산
- 대미지 파이프라인: `FWxDamageInfo` → GameplayEffect Spec 변환, `WxExecCalc_Damage`(치명타·방어·가드·퍼펙트가드), 상태이상(Burn 등) Effect/ExecCalc/MMC 군
- 쿨다운·코스트: 공용 GE(`WxEffect_Cooldown`/`WxEffect_Cost`) + MMC가 `WxAbilityTableRow`에서 수치를 온디맨드 조회, 다중 충전 판정
- 히트박스/연출: 무기·투사체 콜리전(`AWxWeaponBase`/`AWxProjectileBase`), 공격/무적/콤보윈도/카메라 등 AnimNotify(State), GameplayCue, 히트스톱 시간 감속(`UWxTimeDilationComponent`)
- 타게팅: 락온 관리 복제 컴포넌트(`UWxLockOnManagerComponent`)와 TargetingSystem 필터 태스크, MotionWarping 스냅

**경계 (비담당)**
- 캐릭터 클래스·플레이어 입력 바인딩·Experience 주입 등 게임 조립은 [[WxGame]]에 위임(이 모듈은 컴포넌트·베이스·데이터 타입만 제공)
- 팀 판정·공용 정의 등 foundation은 [[WxCore]]에 위임
- 어빌리티/이펙트/무기의 구체 값과 몽타주 연결은 BP·DataTable 에셋에 위임(C++는 베이스와 규약만)

## 의존성
- **주요 의존**: [[WxCore]] · GameplayAbilities(GAS) · ModularGameplay(Lyra식 GameStateComponent) · EnhancedInput · TargetingSystem · MotionWarping · AIModule · Niagara(private)
- 규칙: 플러그인이므로 「WxCore 외 Wx 플러그인 참조」 검증 — 없음 ✅ (`.uplugin`·`Build.cs` 모두 Wx 참조는 `WxCore`뿐)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 입력→어빌리티 라우팅과 AbilitySet 일괄 부여를 맡는 ASC. 전투 진입 관문 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 추상 베이스. 쿨다운·코스트·히트스톱·후딜 캔슬 규약을 정의 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기값을 한 에셋으로 묶어 ASC에 부여하는 데이터 에셋 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF/Crit 등 전투 스탯과 IncomingDamage 메타 어트리뷰트 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. GE Spec(SetByCaller/추가이펙트)로 변환하는 허브 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxCombatLibrary` | 무기/투사체 밖 경로(광역·부위·환경)의 단일 대미지 적용 진입점 `ApplyDamage` | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 스윙 단위 히트박스 무기 베이스. AnimNotify가 BeginAttack/EndAttack로 구동 | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위)을 서버 권위+클라 예측으로 복제 | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 상속. 입력 발동은 `ActivationInputAction`(복수 입력은 `IsActivationInput`/`GetInputActions` override), 수치는 `AbilityDataRow`(`FWxAbilityTableRow`)에 둔다. 쿨다운/코스트 GE 기본값은 공용 GE "마커" — 그대로면 Row 기반, 다른 GE로 바꾸면 엔진 순정 경로(Row와 상호배타).
- 새 대미지·상태이상: `FWxDamageInfo.AdditionalEffects`에 GE 클래스를 얹거나 `WxEffect_*`/`WxExecCalc_*`/`WxMMC_*` 군을 추가. 최종 대미지는 `WxExecCalc_Damage`가 IncomingDamage 메타로 밀어넣고 `PostGameplayEffectExecute`가 HP 차감.
- 데이터 주도 설정: 부여는 `UWxAbilitySet`(GrantedAbilities/Effects + `WxCombatAttributeInitTableRow`), 어빌리티 수치는 `WxAbilityTableRow`, 대미지는 `WxDamageTableRow`로 DataTable에서 읽는다.
- 리플리케이션/권한: 어트리뷰트·락온·전역 시간감속 모두 서버 권위. 락온·시간감속은 소유 클라 예측 후 복제 정합. 투사체는 서버 스폰·복제(클라는 authority 게이트로 무동작).

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로 흘러가는지(전투 제어 흐름의 출발점)
2. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 모든 어빌리티가 공유하는 쿨다운/코스트/히트스톱/후딜 규약
3. `Source/WxCombat/Public/WxDamageInfo.h` + `Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp` — 대미지가 데이터에서 최종 HP 차감까지 도달하는 파이프라인
4. `Source/WxCombat/Public/Weapon/WxWeaponBase.h` — AnimNotify → 히트박스 → 대미지 적용의 실제 연결 지점

## 관련
- 상위: [[WxGame]](캐릭터·플레이어 컨트롤러·Experience에서 이 모듈의 컴포넌트/에셋을 조립), WxEditor

---
*문서 기준 커밋 `6e08d6d` · 생성일 2026-08-05 · 소스 145파일 — `/readme-writer`로 갱신*
