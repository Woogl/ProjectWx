# 기획자용 어빌리티 가이드 생성

프로젝트의 어빌리티 관련 코드를 분석하여 기획자가 Blueprint로 어빌리티를 만들 수 있도록 HTML 가이드 문서를 생성하라.

출력 파일: `Docs/AbilityCreationGuide.html`

## 절차

### 1단계: 어빌리티 코드 분석

다음 파일들을 읽어 BP에 노출되는 속성을 파악하라:

**기본 클래스:**
- `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility.h` — 공통 속성 (ActivationPolicy, ActivationInputTag, AbilityIcon, CooldownDuration, CooldownTag)
- `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility.h`의 `EWxAbilityActivationPolicy` enum 정의 확인

**구체 어빌리티 클래스 (.h + .cpp):**
- `WxAbility_Attack` — ComboMontages, 콤보 로직
- `WxAbility_Skill` — SkillMontage
- `WxAbility_Dodge` — DodgeMontage
- `WxAbility_Guard` — GuardMontage
- `WxAbility_HitReact` — HitReactMontage, GuardHitReactMontage, AbilityTriggers 발동 방식
- `WxAbility_Jump` — 점프 로직
- `WxAbility_Sprint` — 달리기 로직
- `WxAbility_LockOn` — TargetingPreset, CameraInterpSpeed, CameraPitchOffset, MaxDistance, ReticleWidgetClass

**AnimNotifyState / AnimNotify:**
- `WxAnimNotifyState_WeaponCollision` — 무기 충돌 판정 활성화
- `WxAnimNotifyState_ComboWindow` — 콤보 입력 수용 구간
- `WxAnimNotifyState_Invincible` — 무적 상태 부여
- `WxAnimNotifyState_Guard` — 가드 판정 활성화
- `WxAnimNotifyState_TurnAround` — TargetingPreset
- `WxAnimNotify_SpawnProjectile` — ProjectileClass, SpawnSocketName

**데이터 에셋:**
- `WxAbilitySet` — GrantedAbilities, GrantedEffects

**데미지 시스템:**
- `WxDamageExecCalc` — 데미지 공식 확인

### 2단계: Gameplay Tag 확인

`WxGameplayTags.h/cpp`를 읽어 현재 선언된 입력 태그, 어빌리티 태그, ANS 태그 목록을 정리하라.

### 3단계: HTML 문서 생성

`Docs/AbilityCreationGuide.html`에 다음 내용을 포함하는 비주얼 HTML 문서를 작성하라.

**디자인 요구사항:**
- 다크 테마 (배경: #0f1117 계열)
- 컬러 코딩된 타임라인 바로 AnimNotifyState 배치 구간 시각화
- 플로우 다이어그램 (작업 흐름, 데미지 파이프라인)
- 트리 구조로 BP 설정값 표시
- 카드 레이아웃으로 어빌리티 타입별 분리
- 접이식 FAQ
- 외부 라이브러리 없이 순수 HTML + CSS로 작성

**포함할 섹션:**

1. **핵심 구성 요소** — WxAbility, AnimMontage, AnimNotifyState, AnimNotify, GameplayEffect, AbilitySet 역할 설명 카드
2. **작업 흐름** — AnimMontage 준비 → Notify 배치 → Ability BP 생성 → AbilitySet 등록 플로우 다이어그램
3. **어빌리티 공통 설정** — 코드에서 읽은 실제 ActivationPolicy enum 값과 공통 속성 테이블
4. **AnimNotifyState 종류** — 각 NotifyState의 역할, 배치 팁, 컬러 범례 포함
5. **AnimNotify 종류** — SpawnProjectile 설정 속성
6. **어빌리티 종류별 제작** — 각 어빌리티 타입별:
   - BP 설정 속성 테이블 (코드에서 읽은 실제 EditDefaultsOnly 속성)
   - 몽타주 Notify 배치 패턴 (타임라인 바 시각화)
   - 설정 예시 (트리 구조)
   - 접이식
7. **AbilitySet 설정** — 플레이어용, 적용 예시
8. **실전 예제** — 최소 4가지 (근접 콤보, 원거리 스킬, 회피+반격, 적 공격)
9. **데미지 파이프라인** — 플로우 다이어그램 + 실제 코드에서 읽은 데미지 공식 + 계산 예시 테이블
10. **FAQ**

**주의사항:**
- 아직 구현 안된 시스템을 추가하거나 추측하지 말 것.
- 코드 상에 존재하지 않는 변수 이름이나 enum 등을 만들지 말 것.

### 4단계: 기존 파일 비교

기존 `Docs/AbilityCreationGuide.html`이 있으면 읽어서 참고하되, 코드 분석 결과와 다른 부분은 코드 기준으로 수정하라.

### 5단계: 결과 보고

생성 완료 후 문서에 포함된 섹션 목록과 발견된 변경 사항을 보고하라.
