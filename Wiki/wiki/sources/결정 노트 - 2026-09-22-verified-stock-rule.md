---
type: source
title: "결정 노트 - 2026-09-22-verified-stock-rule"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "Wiki"
  - "워크플로우"
summary: "옛 LLM Wiki 기사의 verified 필드를 WX 전용 인간 확인 방침에서 순정 규칙(편찬·재확인 날짜 기록)으로 되돌린 2026-09-22 사용자 결정 기록"
source_type: decision-note
source_id: src-c86bc26d2911ee492488
sha256: d9e214670aefc1f403a22ab1e7977253c0b37341257a6ab4797675e1f16e0dca
authority: primary
independence_key: ".wiki/raw/notes/2026-09-22-verified-stock-rule.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-22-verified-stock-rule.md"
raw_copy: ".raw/captured/d9e214670aefc1f403a22ab1e7977253c0b37341257a6ab4797675e1f16e0dca.md"
claim_ids:
  - clm-3f9c6d8c1a-c1
  - clm-3f9c6d8c1a-c2
  - clm-3f9c6d8c1a-c3
  - clm-3f9c6d8c1a-c4
key_claims:
  - "사용자는 2026-09-22 옛 LLM Wiki의 verified 필드 규칙을 WX 전용 인간 확인 방침에서 순정 LLM Wiki 규칙으로 완전히 되돌리도록 결정했다."
  - "verified 순정 규칙 전환 노트는 WX 전용 verified 방침 때문에 합성 신선도 점수 상한이 75점이었다고 기록한다."
  - "verified 순정 규칙 전환 이후 schema.md는 verified를 편찬·재확인 날짜로 기록하고 편찬 없는 구조 이관에는 넣지 않도록 규정했다."
  - "verified 순정 규칙 전환 노트는 verified 날짜가 빌드·게임 실행 검증일이 아니라고 명시한다."
---

# 결정 노트 - 2026-09-22-verified-stock-rule

- 원본: `.wiki/raw/notes/2026-09-22-verified-stock-rule.md`
- 원자료 사본: `.raw/captured/d9e214670aefc1f403a22ab1e7977253c0b37341257a6ab4797675e1f16e0dca.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: `.wiki/raw/notes/2026-09-22-verified-stock-rule.md`(제목 "verified 순정 규칙 전환 결정").
- frontmatter: `source: session`, `ingested: 2026-09-22`, 태그 `wx, workflow`.
- 성격: 대화 세션에서 나온 **사용자 확정 결정**과, 근거가 된 LLM Wiki 플러그인 0.25.0 순정 규칙 원문 발췌.
- 옛 LLM Wiki(`.wiki/`) 운영 규칙에 관한 결정이다. 현재 Wiki는 claude-obsidian vault `Wiki/`로 옮겨졌으므로 이 결정이 현재 Wiki에 그대로 적용되는지는 이 노트만으로 알 수 없다.

## 배경 (관찰)

- 2026-09-22 Wiki lint 후 신선도 점수를 논의하다가, WX `schema.md`가 `verified`를 "순정 의미에 맞는 인간 확인 근거가 있을 때만" 기록하도록 순정 편찬 규칙에서 벗어나 있음이 확인됐다.
- 이 이탈 때문에 합성 신선도 점수 상한이 75점이었다.

## 사람의 판단 원문

순정 규칙으로 되돌리자는 제안에 대한 답:

> 사용자 2026-09-22: "네. 완전히 순정 방식으로 합시다."

## 확정 결정과 반영

- 순정 규칙: 기사를 만들거나 갱신(편찬)할 때 `volatility`(기본 `warm`)와 `verified`(당일 날짜)를 frontmatter에 적는다. `refresh`·`librarian`에서 사람이 재확인한 기사도 `verified`를 당일로 갱신한다. lint 자동 수정은 실제 확인 없이 `verified` 날짜를 지어내지 않는다.
- WX `schema.md` 반영 문장: "`verified`는 편찬·재확인 때 그날로 기록하고, 편찬 없는 구조 이관에는 넣지 않는다."
- 같은 날 기사 17개에 `verified: 2026-09-22`를 추가했다.
- 노트의 주의: `verified`는 편찬·재확인 날짜이며 **빌드·게임 실행 검증일이 아니다**.

## 관련 주제

- [[Wiki 운영]]

## 핵심 주장

- 사용자는 2026-09-22 옛 LLM Wiki의 verified 필드 규칙을 WX 전용 인간 확인 방침에서 순정 LLM Wiki 규칙으로 완전히 되돌리도록 결정했다. ^c1
- verified 순정 규칙 전환 노트는 WX 전용 verified 방침 때문에 합성 신선도 점수 상한이 75점이었다고 기록한다. ^c2
- verified 순정 규칙 전환 이후 schema.md는 verified를 편찬·재확인 날짜로 기록하고 편찬 없는 구조 이관에는 넣지 않도록 규정했다. ^c3
- verified 순정 규칙 전환 노트는 verified 날짜가 빌드·게임 실행 검증일이 아니라고 명시한다. ^c4
