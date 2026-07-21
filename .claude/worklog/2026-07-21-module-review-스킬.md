# module-review 스킬 신설

## 계획

### 목표
모듈 단위로 코드를 리뷰해 개선점을 문서화하는 `module-review` 스킬을 만든다. `readme-writer`와 동일한 운영 모델(모듈 단위 분석·서브에이전트 격리·provenance 점진 갱신·무인 실행)을 계승해, 사용자가 잠든 사이 자동으로 돌릴 수 있게 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `.claude/skills/module-review/SKILL.md` | 스킬 정의 본문 | 신규 |
| `Docs/Programmer/ModuleReview/` | 산출물 폴더(첫 실행 시 서브에이전트가 문서와 함께 생성) | - |

### 접근 방식
- **운영 모델 계승**: readme-writer의 모듈 발견·상태 판정(missing/foreign/stale/fresh)·provenance 로직을 그대로 재사용. 산출물만 `Docs/Programmer/ModuleReview/<Module>.md`로 중앙화 — 문서가 모듈 소스 **밖**이라 stale 판정에 self-exclude pathspec이 불필요해진다.
- **무수정**: 오케스트레이터에 `Write` 미부여. 서브에이전트는 리뷰 문서 1개만 쓰고 소스는 절대 손대지 않는다(spec-audit의 무수정 원칙).
- **리뷰 루브릭 5차원**: 버그/정확성 · 설계/구조 · 규칙 위반(`CLAUDE.md` 권위) · 중복/복잡도 · 성능/안전. 심각도 🔴 심각 / 🟡 개선 / 🟢 사소. 고신호 우선, 억지 발견·스타일 트집 금지, 근거(`파일:라인`) 필수, 의도된 설계 가능성은 확신도로 표기.
- **아침 다이제스트**: 마무리 보고를 `모듈 | 🔴 | 🟡 | 🟢 | 문서링크` 표로 내어 어디에 불이 났는지 한눈에 보이게 한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `.claude/skills/module-review/SKILL.md` | `module-review` 스킬 정의 본문 | 신규 |

### 구현·결정과 그 이유
- **운영 모델은 readme-writer 복제, 산출물만 재배치**: 모듈 발견·상태 판정(missing/foreign/stale/fresh)·provenance·병렬 서브에이전트 구조를 그대로 계승. 사용자가 이미 검증한 야간 무인 실행 패턴을 재사용하는 것이 신뢰도·학습비용 면에서 최선.
- **산출물을 `Docs/Programmer/ModuleReview/`로 중앙화**: 문서가 모듈 소스 밖이라 stale 판정에서 readme-writer의 self-exclude pathspec 트릭이 불필요해져 로직이 단순해짐. 아침에 한 폴더에서 전 모듈 리뷰를 훑는 다이제스트 용도에도 부합.
- **오케스트레이터에 `Write` 미부여**: 리뷰 문서 쓰기는 서브에이전트가 하고 오케스트레이터는 소스를 손대지 않음을 도구 권한으로 강제(spec-audit의 무수정 원칙 계승).
- **리뷰 루브릭 5차원 + 심각도 3단계 + 확신도**: 고신호 우선·억지 발견 금지·근거(`파일:라인`) 필수. "규칙 위반" 범주의 권위 루브릭은 프로젝트 `CLAUDE.md`로 고정해, 개인 취향이 아니라 명시된 규칙 위반만 잡게 함. 의도된 설계 가능성은 확신도 "낮음"으로 표기해 중립성 유지.

### 계획 대비 달라진 점
- 산출물 위치를 계획 초안의 `Docs/CodeReview/` → 사용자 지시대로 `Docs/Programmer/ModuleReview/`로 변경. 스킬명은 `module-review`로 확정(빌트인 `/code-review` 충돌 회피).

### 후속 과제
- **스모크 테스트 완료(2026-07-21)**: `/module-review WxSave` 시범 실행 → `Docs/Programmer/ModuleReview/WxSave.md` 템플릿대로 정상 생성(provenance·근거·확신도 표기 정확, 🔴0 🟡2 🟢1). 억지 발견 없이 고신호만, 사용자 선호(prefer-inplace/explicit)에 부합하는 보일러플레이트는 의도적으로 미보고 확인. 야간 무인 실행 준비 완료.
- 리뷰 문서는 덮어쓰기 스냅샷 모델이라, "안 고치기로 한" 발견도 코드가 그대로면 재실행마다 재출현할 수 있음(v1 채택). 필요 시 발견 무시(dismiss) 메커니즘은 후속 검토.
