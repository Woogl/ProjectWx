---
name: ability-guide
description: 게임 코드를 분석하여 기획자 친화적인 어빌리티 가이드 HTML 문서를 생성한다.
user-invocable: true
allowed-tools: Read, Grep, Glob, Write, Agent
---

# 어빌리티 가이드 생성

게임 코드를 분석하여 어빌리티 가이드 HTML 문서를 `Docs/Guides/ability-guide.html`에 생성하라.

## 분석 대상

다음 파일들을 읽고 분석하라:

1. **어빌리티 베이스**: `WxAbility.h` / `WxAbility.cpp` — 활성화 정책, MP 비용, 쿨다운, 아이콘
2. **일반 공격**: `WxAbility_Attack.h` / `WxAbility_Attack.cpp` — 콤보 시스템, 콤보 몽타주 목록
3. **스킬**: `WxAbility_Skill.h` / `WxAbility_Skill.cpp` — 단일 몽타주 스킬
4. **가드**: `WxAbility_Guard.h` / `WxAbility_Guard.cpp` — 방어 판정, 퍼펙트 가드
5. **회피**: `WxAbility_Dodge.h` / `WxAbility_Dodge.cpp` — 무적 프레임
6. **질주**: `WxAbility_Sprint.h` / `WxAbility_Sprint.cpp` — 이동속도 보너스
7. **점프**: `WxAbility_Jump.h` / `WxAbility_Jump.cpp` — 점프 조건
8. **락온**: `WxAbility_LockOn.h` / `WxAbility_LockOn.cpp` — 타겟 추적, 카메라 설정
9. **피격 반응**: `WxAbility_HitReact.h` / `WxAbility_HitReact.cpp` — 일반/가드 피격 분기
10. **그로기**: `WxAbility_Groggy.h` / `WxAbility_Groggy.cpp` — 기절 상태, 지속시간, DP 드레인
11. **사망**: `WxAbility_Death.h` / `WxAbility_Death.cpp` — 사망 시퀀스, 래그돌
12. **궁극기**: `WxAbility_Ultimate.h` / `WxAbility_Ultimate.cpp` — 컷씬, 시간 왜곡, 비용
13. **게임플레이 태그**: `WxGameplayTags.h` — 상태 태그, 이벤트 태그, ANS 태그
14. **어빌리티 셋**: `WxAbilitySet.h` — 캐릭터에 부여되는 어빌리티/이펙트 목록
15. **대미지 계산**: `WxExecCalc_Damage.h` / `WxExecCalc_Damage.cpp` — 가드 감소, 퍼펙트 가드 반사, DP 누적
16. **이펙트**: `WxEffect_Cost.h`, `WxEffect_Cooldown.h`, `WxEffect_Reflect.h` — 비용/쿨다운/반사 이펙트

파일 경로가 변경되었을 수 있으므로 Glob으로 찾아서 읽어라.

## 문서에 포함할 내용

### 대상 독자

이 문서의 대상 독자는 **게임 기획자**이다. 다음 원칙을 지켜라:

- C++ 클래스명, 함수명을 노출하지 않는다 (예: `UWxAbility_Attack`, `ActivateAbility`, `EndAbility`, `TSubclassOf<>` 등)
- 의사코드(`if/else`, `random()`, `Clamp()` 등)를 사용하지 않는다. 조건과 흐름은 자연어 문장으로 설명한다
- "GameplayTag", "GameplayEffect", "AbilityTask", "몽타주", "ANS", "SetByCaller" 같은 GAS/엔진 내부 용어를 사용하지 않는다
- 대신 기획자가 이해할 수 있는 용어를 사용한다 (예: 몽타주 → 애니메이션, ANS_Invincible → 무적 구간, GameplayEffect → 효과)
- 각 어빌리티를 설명할 때 **게임 플레이에서의 체감**을 먼저 설명하고, 구체적 수치/조건은 그 다음에 배치한다

### 필수 섹션

0. **문서 생성일**: 헤더 영역에 문서가 생성된 날짜를 `YYYY-MM-DD` 형식으로 표시하라

1. **어빌리티 총괄표**: 모든 어빌리티를 한눈에 볼 수 있는 요약 테이블
   - 컬럼: 어빌리티 이름, 카테고리(전투/이동/상태), 입력 방식(누르기/홀드/토글/자동), MP 비용, 쿨다운, 한 줄 설명
   - 내부 전용 어빌리티(피격 반응, 사망 등 플레이어가 직접 조작하지 않는 것)는 별도 섹션으로 분리한다

2. **전투 어빌리티 상세** (일반 공격, 스킬, 궁극기):
   - 어빌리티별 상세 설명, 애니메이션 몽타주 타임라인 예시
   - 일반 공격: 콤보 단계 수, 콤보 연결 조건 (콤보 윈도우), 입력 방식
   - 스킬: 발동 조건, 사용 흐름
   - 궁극기: 발동 조건, 컷씬 연출 흐름, 특수 효과(시간 왜곡, 무적), 비용

3. **방어 어빌리티 상세** (가드, 회피):
   - 어빌리티별 상세 설명, 애니메이션 몽타주 타임라인 예시
   - 가드: 입력 방식(홀드), 가드 판정 구간, 퍼펙트 가드 조건과 효과(DP 반사량), 가드 시 대미지 감소율
   - 회피: 입력 방식, 무적 구간 설명

4. **이동 어빌리티 상세** (질주, 점프, 락온):
   - 질주: 속도 보너스 수치, 입력 방식(홀드)
   - 점프: 발동 조건, 사용 불가 상황
   - 락온: 토글 방식, 최대 거리, 카메라 동작, 타겟 상실 조건

5. **반응형 어빌리티** (피격 반응, 그로기, 사망):
   - 이 어빌리티들은 플레이어가 직접 발동하는 것이 아니라 게임 상황에 의해 자동 발동됨을 명시
   - 피격 반응: 일반 피격과 가드 피격의 차이, 무적/사망 중 무시 조건
   - 그로기: 발동 조건(DP 초과), 지속시간, 해제 과정(DP 드레인)
   - 사망: 발동 조건(HP 0), 진행 과정(모든 어빌리티 취소, 사망 애니메이션 또는 래그돌)

6. **상태 태그 정리**:
   - 게임에서 사용되는 주요 상태(사망, 공중, 그로기, 락온)와 판정 구간(무기 충돌, 콤보 윈도우, 무적, 가드, 퍼펙트 가드)을 기획자 용어로 정리
   - 각 상태/판정이 게임 플레이에 미치는 영향을 한 줄로 설명
   - "태그"라는 용어 대신 "상태"와 "판정 구간"으로 표현한다

### 수치 정확성

- 모든 수치는 반드시 코드에서 직접 읽어서 작성하라
- 하드코딩된 상수 (가드 감소율, 퍼펙트 가드 DP 반사량, 질주 속도 보너스, 그로기 지속시간, 궁극기 MP 비용 등)를 정확히 반영하라
- 추측하지 말고 코드에 있는 값만 사용하라

### 언어

- 문서의 모든 텍스트는 한국어로 작성하라
- HTML 태그의 `lang` 속성은 `ko`로 설정하라
- 경어체(~합니다)가 아닌 평서체(~한다, ~이다)를 사용하라

## HTML 스타일

- 다크 테마 기반 (배경 #0f1117, 텍스트 #e2e4ea)
- 카드형 레이아웃으로 각 어빌리티를 구분
- 어빌리티 카테고리별 색상 구분:
  - 전투: 빨강 (#ff6b6b)
  - 방어: 청록 (#4ecdc4)
  - 이동: 초록 (#95e86e)
  - 기타: 노랑 (#ffd93d)
- 각 어빌리티 카드에 카테고리 색상의 좌측 보더를 적용한다
- 다이어그램은 순수 HTML+CSS로 구현한다 (SVG 허용)
- 반응형 레이아웃 (max-width: 960px)
- 외부 의존성 없이 순수 HTML+CSS로 작성
- 중요 수치나 키워드는 `<strong>` 또는 컬러 `<span>`으로 강조한다
- 테이블은 `border-collapse: collapse`를 적용하고, 헤더 행은 어두운 배경(#1a1d27)으로 구분한다

## 출력

`Docs/Guides/ability-guide.html`에 생성하라. 기존 파일이 있으면 덮어쓴다.
