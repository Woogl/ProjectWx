---
type: source
title: "결정 노트 - 2026-09-26-ability-ga-play-acceptance"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "GAS"
  - "어빌리티"
  - "테스트"
summary: "어빌리티 테이블 전환을 철회하고 데이터 전용 GA_를 유지한 최종 결정과 HGTest·소환물·조작감·GE·락온 사람 테스트 7개 통과 범위"
source_type: decision-note
source_id: src-c99fdef61765bfc296b1
sha256: 27e6ec2d4ecc9ee37d8a524cc8a3cce8bf87657740cb83a07498e3a91a82c6b5
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-ability-ga-play-acceptance.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-ability-ga-play-acceptance.md"
raw_copy: ".raw/captured/27e6ec2d4ecc9ee37d8a524cc8a3cce8bf87657740cb83a07498e3a91a82c6b5.md"
claim_ids:
  - clm-181814f649-c1
  - clm-181814f649-c2
  - clm-181814f649-c3
key_claims:
  - "어빌리티 테이블 전환 작업은 테이블 전환을 하지 않고 데이터 전용 GA_를 유지하는 것으로 사람이 마무리했다."
  - "GA_ 유지 이후 HGTest·분신·도플갱어·조작감·가드 경감과 버프 아이콘·템플릿 패시브 UP·락온의 사람 테스트 7개가 모두 통과했다."
  - "GA_ 유지 결과의 사람 테스트는 네트워크 구성이 명시되지 않아 예측·복제·BT 타이밍 검증으로 간주되지 않는다."
---

# 결정 노트 - 2026-09-26-ability-ga-play-acceptance

- 원본: `.wiki/raw/notes/2026-09-26-ability-ga-play-acceptance.md`
- 원자료 사본: `.raw/captured/27e6ec2d4ecc9ee37d8a524cc8a3cce8bf87657740cb83a07498e3a91a82c6b5.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `2026-09-26-ability-ga-play-acceptance.md`(제목 "GA_ 유지 결정과 사람 플레이 확인 범위", 수집일 2026-09-26).
- 출처: 작업 기록 `.agents/workflow/tasks/ability-table-driven.md`(SHA-256 `1e0263a661d3eea0ea8bf86b45ef0b2e285292ca862c0c27094affca9e85e21a`)의 상태·요청·테스트 체크리스트, GA_ 복귀 설계·결과, DT_Effect 제거, 락온 프리셋 복귀와 사용자 테스트 결과를 읽었다.
- 체크리스트 근거는 `이우성 2026-09-25`, 결과 접수 시각은 `2026-09-25T17:04:24.338Z`.
- 관련 작업: [[작업 - ability-table-driven]]

## 확정 결정

> 사용자 결과(작업 기록, 접수 2026-09-25T17:04:24.338Z): "이 작업은 테이블 전환을 하지 않는 것으로 마무리되었습니다. 테스트했을 때에도 문제 없었습니다."

- 테이블화는 최종 구조가 아니다. 데이터 전용 GA_를 유지한다.
- 작업 기록의 행 컬럼·세트의 테이블 목록·동적 행 태그 설계는 과거 이력이며 되살리지 않는다. 현재 구현의 세부 계약은 기존 기사와 후속 변경 근거를 따른다.

## 검증 범위(사람이 확인한 것)

사람 테스트 7개 모두 통과.

- HGTest 어빌리티: HGTest가 있는 맵에서 발동과 연출.
- 분신: 분신이 있는 맵에서 어빌리티 발동과 연출.
- 도플갱어: 도플갱어가 있는 맵에서 어빌리티 발동과 연출.
- 조작감: 블렌드 인 0.05초 통일 뒤 전반 조작감.
- 가드 경감·버프 아이콘: 가드 중 피해 절반 감소와 방패 버프 아이콘 표시.
- 템플릿 패시브 UP: 패시브의 UP 5 지급.
- 락온: 어빌리티 데이터의 타게팅 프리셋으로 대상 선택.

## 확인하지 않은 것

- 이번 수집에서 코드·에셋 내부 조사, 빌드·자동화·PIE·플레이를 재실행하지 않았다.
- 테스트 환경에 네트워크 구성이 명시되지 않았으므로 예측·복제·BT 타이밍 전체의 검증으로 확대하지 않는다.
- 다른 작업의 미확인 항목도 이 결과로 통과 처리하지 않는다.
- 기존 GA_ 복귀 원자료의 미확인 설명은 위 7개 범위에서만 대체된다. 상태·승인 정본은 작업 기록에 있다.

## 관련 주제

- [[어빌리티와 GAS]]
- [[작업 절차(Workflow)]]
- [[결정 노트 - 2026-09-25-ability-data-on-ga]]

## 핵심 주장

- 어빌리티 테이블 전환 작업은 테이블 전환을 하지 않고 데이터 전용 GA_를 유지하는 것으로 사람이 마무리했다. ^c1
- GA_ 유지 이후 HGTest·분신·도플갱어·조작감·가드 경감과 버프 아이콘·템플릿 패시브 UP·락온의 사람 테스트 7개가 모두 통과했다. ^c2
- GA_ 유지 결과의 사람 테스트는 네트워크 구성이 명시되지 않아 예측·복제·BT 타이밍 검증으로 간주되지 않는다. ^c3
