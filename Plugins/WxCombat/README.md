# WxCombat — 전투 시스템

> Unreal Gameplay Ability System(GAS) 위에 구축한 액션 RPG 전투의 핵심 모듈. 어빌리티/이펙트/어트리뷰트, 대미지 파이프라인, 락온·타게팅, 무기·투사체, 히트 판정용 AnimNotify를 담당한다.

## 책임
**담당**
- GAS 런타임: ASC(`UWxAbilitySystemComponent`), 어트리뷰트 세트, 어빌리티 베이스와 구체 어빌리티(공격/회피/가드/스킬/궁극기/피격/그로기/사망/AI 패턴 등)
- 대미지 파이프라인: `FWxDamageInfo` → GameplayEffect Spec 변환 → `WxExecCalc_Damage` 최종 계산(치명타·가드·퍼펙트가드·무적 판정)
- 어빌리티 부여 데이터: `UWxAbilitySet`(Ability/Effect/Attribute 초기값을 한 에셋에 묶어 일괄 부여), 입력 태그 기반 어빌리티 활성화 라우팅
- 락온/타게팅: `UWxLockOnManagerComponent`, TargetingSystem 필터 태스크, MotionWarping 기반 타겟 스냅
- 무기·투사체·이펙트 존, 히트 판정/카메라/무적 구간을 여닫는 AnimNotify(State), 히트스톱·슬로우 등 시간 왜곡

**경계 (비담당)**
- 어빌리티 UI 표시 데이터(아이콘 등)는 [[WxUI]]의 `UWxAbilityComponent` 파생으로 어빌리티에 EditInline 부착 — WxCombat은 표시 데이터를 정의하지 않음
- 캐릭터/폰 클래스, 입력 매핑 자산, 공용 GameplayTag 정의는 [[WxCore]] 및 게임 모듈([[WxGame]])

## 의존성
- **주요 의존**: [[WxCore]](유일한 Wx 의존), 엔진 플러그인 GameplayAbilities·ModularGameplay·EnhancedInput·TargetingSystem·MotionWarping
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 캐릭터에 붙는 ASC. 입력 태그→어빌리티 활성화 라우팅, AbilitySet 부여 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | Ability/Effect/Attribute 초기값을 묶은 데이터 에셋. 캐릭터 BP가 지정 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 쿨다운/코스트를 DataRow 기반 공용 GE로 처리, 후딜 캔슬·히트스톱 규약 정의 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF·크리·속도 등 스탯과 `IncomingDamage` 메타 어트리뷰트 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. AnimNotify/무기가 채워 Spec으로 변환 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxExecCalc_Damage` | 대미지 최종 계산 ExecutionCalculation(치명타·가드·퍼펙트가드·무적) | `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `UWxCombatLibrary` | 무기/투사체 밖 경로의 대미지 적용 진입점(`ApplyDamage`/`ApplyRawDamage`) | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위) 서버 권위 복제·조회 | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 상속(BP 가능). 쿨다운/코스트 수치는 GE에 직접 넣지 말고 `AbilityDataRow`(`FWxAbilityTableRow`)에 기입 — 베이스가 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` GE에 온디맨드로 채운다. 다른 GE로 바꾸면 엔진 순정 경로로 전환(상호배타).
- **활성화 정책**: `EWxAbilityActivationPolicy`로 입력 트리거(`OnInputTriggered`) / 부여 즉시 자동(`OnGranted`, 패시브) 구분.
- **데이터 주도 설정**: 어트리뷰트 초기값은 `FWxCombatAttributeInitTableRow`, 대미지 프리셋은 `FWxDamageTableRow`(`FWxDamageInfo::FromDataRow`)를 DataTable Row로 참조.
- **새 이펙트**: `Effect/` 아래 `UWxEffect_*` GameplayEffect 파생과 `WxExecCalc_*`/`WxMMC_*` 계산 클래스. 대미지 흐름에 태우려면 `FWxDamageInfo::AdditionalEffects`에 추가.
- **히트 판정**: 무기 스윙은 `WxAnimNotifyState_WeaponAttack`가 `AWxWeaponBase::BeginAttack/EndAttack`을 여닫고, 광역/투사체/피니셔는 각 `WxAnimNotify_*`가 라이브러리·어빌리티에 위임.
- **리플리케이션**: 대미지·투사체 스폰·락온 대상은 서버 권위, 소유 클라는 예측 후 정합. 입력 태그도 클라→서버 동기화.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 쿨다운/코스트/후딜/히트스톱 등 모듈 전반의 어빌리티 규약이 헤더 주석에 응축됨
2. `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터가 무엇을 부여받는지(진입 데이터)의 형태
3. `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` — 대미지 최종 판정 흐름(가드/퍼펙트가드/무적) 한눈에
4. `Source/WxCombat/Public/WxDamageInfo.h` — AnimNotify·무기가 채워 넘기는 대미지 데이터의 계약

## 관련
- 상위: 캐릭터/폰이 ASC와 `UWxAbilitySet`을 물어 사용 — 게임 모듈([[WxGame]]), AI 패턴 어빌리티는 [[WxAI]] 흐름과 맞물림
- 함께: [[WxCore]](공용 정의·태그), [[WxUI]](어빌리티 표시 데이터·HUD 바인딩)

---
*문서 기준 커밋 `a9e6ea8` · 생성일 2026-07-19 · 소스 149파일 — `/readme-writer`로 갱신*
