---
title: "verified 순정 규칙 전환 결정"
source: "session"
type: notes
ingested: 2026-09-22
tags: [wx, workflow]
summary: "사용자 결정으로 기사 verified를 WX 전용 인간 확인 방침 대신 LLM Wiki 순정 규칙(편찬·재확인 때 그날 기록)으로 전환했다. 순정 규칙 원문과 반영된 schema.md 발췌."
---

# verified 순정 규칙 전환 결정

## 사용자 결정

2026-09-22 Wiki lint 후 신선도 점수 논의에서, WX `schema.md`가 `verified`를 "순정 의미에 맞는 인간 확인 근거가 있을 때만" 기록하도록 순정 편찬 규칙에서 이탈해 있음이 확인되었다. 이 이탈 때문에 합성 신선도 상한이 75점이었다. 순정 규칙으로 되돌리는 제안에 사용자가 "네. 완전히 순정 방식으로 합시다."라고 결정했다.

## 순정 규칙 원문 (LLM Wiki 플러그인 0.25.0)

`skills/wiki-manager/references/wiki-structure.md`
```text
489: Wiki articles carry a `volatility` field that controls how quickly their freshness score decays. The `verified` field records when a human last confirmed the article's conclusions are still accurate.
```

`skills/wiki-manager/references/compilation.md`
```text
105: When creating or updating a wiki article, set `volatility` and `verified` in frontmatter. Default `volatility` to `warm`. Set `verified` to today's date. The full rubric — what each tier means, when to use it, how the decay differs — lives in `references/wiki-structure.md` § Volatility Classification. The short version: news/trends sources → `hot`, foundational/historical sources → `cold`, everything else → `warm` (the safe default). The author can override during review.
```

`commands/compile.md`
```text
61: 5.5. **Self-validation pass**: For every article touched in step 5 (new or updated), re-read the frontmatter and verify:
62:    - `sources:` is a non-empty list resolving to existing raw files, OR `compiled-from: conversation` is set
63:    - `volatility:` is set to `hot`, `warm`, or `cold`
64:    - `verified:` is set to today's date
```

`commands/refresh.md`
```text
81: On user selection:
82: - **skip**: No changes. Update `verified` date to today (human confirmed it's still accurate).
...
87: After all actions, update the article's `verified` date to today.
```

`commands/librarian.md`
```text
128: 6. For articles the user selects to verify: update `verified:` to today in the article's frontmatter.
```

`skills/wiki-manager/references/linting.md`
```text
350: **Auto-fix**: Add `volatility: warm` as the safe default that puts the article into the standard monitoring cadence. Do not invent a `verified:` date unless verification was actually performed; use existing `updated:`/`verified:` dates only for freshness scoring.
```

## .wiki/schema.md
- [저장소 원문](<../../schema.md>)
- SHA-256: `729dc26ad6b9797ac0c7e2215a9f8f0570e1f6e1b9b2a085a0c8e2a2ccc4a9e4`
```text
...
38: - 날짜 필드 `created`·`updated`·`verified`는 순정 규칙을 따른다. `verified`는 편찬·재확인 때 그날로 기록하고, 편찬 없는 구조 이관에는 넣지 않는다.
...
```

같은 날 기사 17개에 당일 편찬일 `verified: 2026-09-22`를 추가했다. `verified`는 편찬·재확인 날짜이며 빌드·게임 실행 검증일이 아니다.
