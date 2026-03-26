---
name: stat-guide
description: 게임 코드를 분석하여 스탯(어트리뷰트) 가이드 HTML 문서를 생성한다.
user-invocable: true
allowed-tools: Read, Grep, Glob, Write, Agent
---

# 스탯 가이드 생성

게임 코드를 분석하여 스탯(어트리뷰트) 가이드 HTML 문서를 `Docs/stat-guide.html`에 생성하라.

## 분석 대상

다음 파일들을 읽고 분석하라:

1. **어트리뷰트 정의**: `WxCombatAttributeSet.h` — 어트리뷰트 목록, 타입, 복제 여부
2. **어트리뷰트 로직**: `WxCombatAttributeSet.cpp` — 클램프 규칙, 사망/그로기 판정, MaxHP 비율 유지
3. **대미지 공식**: `WxDamageExecCalc.cpp` — 대미지 계산 파이프라인 (방어 배율, 치명타, 가드 감소, DP 누적, MP 회복, 퍼펙트 가드 반사)
4. **그로기 시스템**: `WxAbility_Groggy.cpp` — DP 드레인 속도, 그로기 해제 조건
5. **어빌리티 비용**: `WxAbility.cpp` / `WxAbility.h` — MP 비용 검사 및 차감
6. **MP 회복**: `WxEffect_MPRecovery.h` — 적중 시 MP 회복량
7. **이동속도**: `WxCharacterBase.cpp` — SPD 어트리뷰트와 MaxWalkSpeed 관계
8. **대시**: `WxAbility_Sprint.cpp` — 대시 중 SPD 보너스
9. **초기값 설정**: `WxAbilitySet.h` / `WxAbilitySet.cpp` — 어트리뷰트 초기화 방식

파일 경로가 변경되었을 수 있으므로 Glob으로 찾아서 읽어라.

## 문서에 포함할 내용

### 필수 섹션

0. **문서 생성일**: 헤더 영역에 문서가 생성된 날짜를 `YYYY-MM-DD` 형식으로 표시하라
1. **어트리뷰트 요약 테이블**: 모든 어트리뷰트의 이름, 카테고리(Vital/Combat/Meta), 범위, 설명
2. **생존 스탯 상세** (HP, MP, DP): 각 스탯의 역할, 클램프 범위, 경계값 동작 (사망, 그로기), 그로기 DP 드레인 공식
3. **전투 스탯 상세** (ATK, DEF, SPD, CritRate, CritDMG): 각 스탯의 공식, DEF 경감률 예시 표, 치명타 계산 예시
4. **대미지 계산 파이프라인**: 무적 체크 → 퍼펙트 가드 → 기본 대미지 → 치명타 → 가드 감소 → HP/DP 적용, 각 단계별 공식
5. **전체 계산 예시**: 일반 공격, 가드, 퍼펙트 가드 상황별 수치 계산
6. **상태 전이**: 사망(State.Dead), 그로기(State.Groggy) 발동 조건 및 동작
7. **어빌리티 비용 시스템**: MP 비용 검사 및 차감

### 수치 정확성

- 모든 공식과 수치는 반드시 코드에서 직접 읽어서 작성하라
- 하드코딩된 상수 (가드 감소율, MP 회복량, 드레인 틱레이트 등)를 정확히 반영하라
- 추측하지 말고 코드에 있는 값만 사용하라

## HTML 스타일

- 다크 테마 기반 (배경 #0f1117, 텍스트 #e2e4ea)
- 카드형 레이아웃으로 각 스탯을 구분
- 공식은 monospace 코드 블록으로 표시
- 스탯 카테고리별 색상 구분 (HP 계열: 빨강, MP 계열: 파랑, DP 계열: 노랑, 공격: 주황, 방어: 청록, 속도: 초록, 치명타: 보라)
- 반응형 레이아웃 (max-width: 960px)
- 외부 의존성 없이 순수 HTML+CSS로 작성

## 출력

`Docs/stat-guide.html`에 생성하라. 기존 파일이 있으면 덮어쓴다.
