---
type: source
title: "결정 노트 - 2026-09-26-workflow-row-actions-final"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
  - "UI"
summary: "대시보드 행 버튼 최종 결정으로 진행 중 기록에서도 기록 열기를 빼고, 배지 행의 버튼 줄 어긋남을 고정 폭 칸으로 고치며 사이드바 소개 문구를 없앤 노트."
source_type: decision-note
source_id: src-95007bbf6cf68642599a
sha256: f8dd930f0cdb2a8c1ebbcac0d0d4299d68adc9e79a417af538a45db70e807949
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-workflow-row-actions-final.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-workflow-row-actions-final.md"
raw_copy: ".raw/captured/f8dd930f0cdb2a8c1ebbcac0d0d4299d68adc9e79a417af538a45db70e807949.md"
claim_ids:
  - clm-8922cc3e49-c1
  - clm-8922cc3e49-c2
  - clm-8922cc3e49-c3
key_claims:
  - "2026-09-26 사용자 결정으로 Workflow 대시보드의 확인 대기·진행 중 행에는 작업 진행 버튼만 두고 기록 열기를 두지 않는다."
  - "상태 배지가 있는 행의 버튼 줄 어긋남은 auto 폭 버튼 칸 때문이었고 160px 고정·오른쪽 정렬로 고쳤다."
  - "리뷰·참고 행에 기록 열기 버튼이 있는지는 원자료 안의 구현 설명과 테스트 기술이 서로 다르게 적는다."
---

# 결정 노트 - 2026-09-26-workflow-row-actions-final

- 원본: `.wiki/raw/notes/2026-09-26-workflow-row-actions-final.md`
- 원자료 사본: `.raw/captured/f8dd930f0cdb2a8c1ebbcac0d0d4299d68adc9e79a417af538a45db70e807949.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `.wiki/raw/notes/2026-09-26-workflow-row-actions-final.md`(frontmatter `ingested: 2026-09-26`, `source: "MANUAL"`).
- 사용자가 대시보드를 테스트하며 내린 결정이다. **앞선 결정을 이어서 바꿈**: [[결정 노트 - 2026-09-26-workflow-dashboard-row-actions]] (확인 대기에는 기록 열기 없음, 완료에는 작업 진행 없음)에 더해 진행 중 기록에서도 기록 열기를 뺐다.

## 사람의 판단 원문

> 사용자 2026-09-26: "진행 중도 기록 열기 버튼 없애주세요"

> 사용자 2026-09-26: ""사람이 작업하고 판단하는 공간"이라는 왼쪽 탭 문구 제거해주세요"

또 "AI 확인 요청" 표시가 있는 행의 버튼 줄이 맞지 않는 화면을 알려줬다.

## 최종 행 버튼(확정 결정)

| 분류 | 버튼 |
| --- | --- |
| 확인 대기·진행 중 | 작업 진행만 |
| 완료·리뷰·참고 | 기록 열기만 |

생성 문서에 아직 없는 새 기록 안내도 기록 열기 자리에만 나온다(`.agents/scripts/wiki-viewer/workflow.js`).

## 구현 관찰

- 줄 어긋남 원인: 행마다 따로 그리는 그리드의 버튼 칸이 `auto`라, 상태 배지가 붙은 행만 칸이 넓어져 다음 행동 열과 버튼이 왼쪽으로 밀렸다. 버튼 칸을 160px 고정·오른쪽 정렬로 바꿨고, 1000px 이하에서는 행 아래로 내려가 왼쪽 정렬이다.
- 사이드바 소개 문구 요소(`#purpose`)와 문구를 넣던 스크립트 줄·전용 스타일을 지웠다. 사이드바는 Workflow 화면에서만 보여 Wiki 화면에는 영향이 없다.

## 검증 범위

- 자동 테스트 TestWikiViewer: 확인 대기·진행 중 행은 작업 진행만, 완료 행은 기록 열기만, 리뷰·참고 행은 버튼 없음(원자료의 테스트 기술이 위 표의 리뷰·참고 기록 열기와 다르게 적혀 있다).
- 헤드리스 Edge 1920px: 확인 대기 17행 모두 다음 행동 열 왼쪽(877)과 버튼 오른쪽(1743)이 같았고 배지가 있는 체크포인트 행도 같았다. 900px에서는 버튼 칸이 행 아래 왼쪽에 모였다.
- 이후 사람 확인: [[결정 노트 - 2026-09-26-workflow-human-verification]]에서 "대시보드" 항목(분류별 행 버튼과 배지 행 정렬)이 통과로 전달됐다.

## 미결정·충돌

- 리뷰·참고 행의 버튼: 구현 관찰은 "완료·리뷰·참고 행은 기록 열기만"이라 적고 TestWikiViewer 기술은 "리뷰·참고 행은 버튼 없음"이라 적어 원자료 안에서 표현이 어긋난다.

## 관련 주제

- [[작업 절차(Workflow)]]
- [[UI 표시 구조]]

## 핵심 주장

- 2026-09-26 사용자 결정으로 Workflow 대시보드의 확인 대기·진행 중 행에는 작업 진행 버튼만 두고 기록 열기를 두지 않는다. ^c1
- 상태 배지가 있는 행의 버튼 줄 어긋남은 auto 폭 버튼 칸 때문이었고 160px 고정·오른쪽 정렬로 고쳤다. ^c2
- 리뷰·참고 행에 기록 열기 버튼이 있는지는 원자료 안의 구현 설명과 테스트 기술이 서로 다르게 적는다. ^c3
