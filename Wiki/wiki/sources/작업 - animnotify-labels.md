---
type: source
title: "작업 - animnotify-labels"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "AnimNotify"
  - "에디터"
summary: "AnimNotify 17종의 타임라인 표시 이름을 종류: 대표 값 형식의 짧은 라벨로 바꾼 작업 기록으로, 사람 확인 2/2 통과로 완료됐다."
source_type: task-record
source_id: src-a23a1d7fb94ec48c6dd4
sha256: 6d65717f2c0a5202d98bd731bd6bb04b1303101563323c07be80adcf6b8a9ba8
authority: primary
independence_key: ".agents/workflow/tasks/animnotify-labels.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/animnotify-labels.md"
raw_copy: ".raw/captured/6d65717f2c0a5202d98bd731bd6bb04b1303101563323c07be80adcf6b8a9ba8.md"
claim_ids:
  - clm-9d2b6fff03-c1
  - clm-9d2b6fff03-c2
  - clm-9d2b6fff03-c3
key_claims:
  - "animnotify-labels 작업은 AnimNotify 17종의 타임라인 라벨을 종류: 대표 값 하나 형식으로 바꾸고 Row·에셋 이름은 보존했다."
  - "animnotify-labels 작업은 표시 함수와 새 override 선언만 바꾸고 실행 코드는 바꾸지 않았음을 diff 비교로 확인했다."
  - "animnotify-labels 작업의 라벨 값 일치와 가독성은 이우성이 2026-09-25 에디터에서 통과로 확인했다."
---

# 작업 - animnotify-labels

- 원본: `.agents/workflow/tasks/animnotify-labels.md`
- 원자료 사본: `.raw/captured/6d65717f2c0a5202d98bd731bd6bb04b1303101563323c07be80adcf6b8a9ba8.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

몽타주 타임라인에 보이는 AnimNotify 17종의 표시 이름을 `종류: 대표 값 하나` 형태의 짧은 라벨로 정리한 작업 기록이다. 상태는 완료(체크리스트 2/2 통과)다.

## 요청과 합의

- 사용자 합의(2026-09-25): `종류: 대표 값 하나`의 짧은 라벨로 17종을 수정하도록 요청했다(기록에는 원문 인용 없이 요약으로만 남아 있다).
- 제출 범위: 사용자가 본 세션 작업물만 제출하도록 명시했다. 본 세션 Notify 코드 24개와 관련 문서 7개만 단일 커밋하고 기존 스테이징 항목은 유지했다.

## 확정 규칙

- 범위: `WxCombat` 15종, `WxAI` ReportNoise, `WxInventory` UseItem. 표시 함수와 새 override 선언만 바꾼다.
- Row·에셋 이름은 그대로 보존하고, 클래스명 끝 `_C`를 떼며, 미설정은 `None`, 스냅 비활성은 `Off`로 표시한다.
- 수치 없는 고정 표식은 Recovery / Combo Window / Use Item이다.

## 검증 범위

- AI 정적 검사: diff 공백 검사, 17종 표시 함수 외 실행 코드 불변 비교, Wiki lint·링크 검사 통과.
- AI 빌드: Editor Development 재검증 성공(`Saved/Logs/BuildDoctor/build_2026-09-25_034044_655_34628.log`, Result Succeeded). 첫 실행은 별도 작업 중인 AI 헤더와 UHT 생성 매크로 행 번호 불일치로 실패했지만 Notify 17종은 컴파일됐고, 다른 세션 빌드 종료 뒤 재실행에서 성공했다. 현재 작업 트리 전체 빌드였다는 점을 기록이 구분한다.
- 사람 확인(통과, 이우성 2026-09-25): 몽타주 타임라인에서 17종이 `종류: 대표 값` 라벨로 보이고 값이 설정과 같음, 라벨이 겹치거나 잘리지 않고 읽힘.
- AI 완료 정리는 Wiki 반영과 lint 0건을 보고했고, 빌드·에디터 테스트는 재실행하지 않았다.

## 관련 주제

- [[에디터 도구]]
- [[작업 - animnotify-categories]]
- <!--wl-->결정 노트 - 2026-09-25-animnotify-labels
- <!--wl-->결정 노트 - 2026-09-26-animnotify-label-acceptance

## 핵심 주장

- animnotify-labels 작업은 AnimNotify 17종의 타임라인 라벨을 종류: 대표 값 하나 형식으로 바꾸고 Row·에셋 이름은 보존했다. ^c1
- animnotify-labels 작업은 표시 함수와 새 override 선언만 바꾸고 실행 코드는 바꾸지 않았음을 diff 비교로 확인했다. ^c2
- animnotify-labels 작업의 라벨 값 일치와 가독성은 이우성이 2026-09-25 에디터에서 통과로 확인했다. ^c3
