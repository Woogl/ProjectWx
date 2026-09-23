# Wiki Activity Log

## [2026-09-22] init | 사용자 승인으로 WX Wiki를 팀 공유 프로젝트 로컬 .wiki 구조로 전환. 기존 문서 18개와 상태 메타데이터를 raw/notes에 보존하고 지식·운영 기사 16개로 연결. 코드·에셋 재검증 아님.

## [2026-09-22] lint | local command: 0 critical, 0 warnings, 0 suggestions, 21 auto-fixed

## [2026-09-22] lint | 순정 로컬 lint: critical/warning/suggestion/info 0. 저장소 링크 44개 문서 오류 0; 뷰어 검색·Workflow 모의 실행·Edge Mermaid 검증 통과. 게임 코드·에셋 재검증 아님.

## [2026-09-22] schema | 순정 기능과 중복되는 readme-writer 스킬을 제거하고 WX 모듈 근거 확인 지침만 schema.md에 통합.

## [2026-09-22] compile | Workflow의 최신 확정 기준과 Wiki 과거 사본의 경계, 읽기 전용 조사, 완료 후 지식 반영 절차 및 웹 완료 버튼의 실제 범위를 명확히 함.

## [2026-09-22] lint | 18 checks, 0 critical, 17 warnings, 3 suggestions, 0 candidates, 0 auto-fixed. 보고 전용: 관련 문서 역링크 누락 16건, inbox/_index.md가 순정 --inbox 수집 대상이 되는 문제 1건. 게임 코드·에셋 재검증 아님.

## [2026-09-22] lint | 정정: 18 checks, 0 critical, 16 warnings, 3 suggestions, 0 candidates, 0 auto-fixed. 직전 기록의 inbox/_index.md 경고는 순정 init이 만드는 색인이라 철회하며, 관련 문서 역링크 누락 16건만 유효.

## [2026-09-22] compile | 사용자 결정: 기획은 속도보다 충실하게 검토하되 기존 구조를 신규 기획의 허용 조건으로 삼지 않음. 기획 검토 프롬프트와 절차에 Wiki·원자료 대조 및 변경·확장·대체 방안 검토를 반영.

## [2026-09-22] compile | 기획 절차를 입력·검토·판단·확정과 핵심 검토 원칙으로 축약. 신중한 조사와 신규 기획 수용에 관한 기존 합의는 유지.

## [2026-09-22] schema | 사용자 승인으로 Wiki 링크만 담은 내부 모듈 README 9개를 제거하고 모듈 설명·탐색을 Wiki로 통합. 루트 README는 유지. 기존 raw 원문은 당시 기록으로 보존.

## [2026-09-22] compile | process 공통·설계·테스트·완료·기록 지침을 사람의 판단과 AI 수행 중심으로 축약. 기획 문서를 design.md로 변경하고 관련 링크·뷰어 경로를 갱신.

## [2026-09-22] compile | 단계 완료 시 Task 기록, 작업 완료 시 Wiki 반영으로 절차 통합. 별도 records.md를 제거하고 공통·완료 규칙과 관련 참조를 갱신.

## [2026-09-22] compile | 사용자 요청으로 테스트 결과 수용 버튼의 표시를 설계 확정으로 변경. 테스트 수용·완료 동작은 유지하며 안내를 갱신.

## [2026-09-22] compile | 사용자 정정에 따라 테스트 수용 버튼 표시를 기획 확정으로 수정. 기존 수용 동작 유지.

## [2026-09-22] compile | 사용자 요청으로 테스트 수용 버튼 표시를 테스트 완료로 변경. 기존 수용·완료 동작 유지.

## 2026-09-22 — Workflow 기획서 검토 전환

사용자 결정을 raw/notes/2026-09-22-workflow-review.md에 수집하고 wiki/references/wiki-workflow.md에 통합했다. 개발자의 기획서 검토 역할과 Workflow 검색 제거를 반영했다. 원자료 색인·원자료 수를 갱신했다. TestWikiViewer·TestWikiSpaces·TestWikiAI 통과; 실제 AI 응답 품질·브라우저 육안 검증 아님.

## [2026-09-22] ingest | 현재 작업 트리의 모듈·핵심 C++·설정·기획 조사 원자료 15개 수집. 파일별 SHA-256과 행 번호 발췌. 기존 Workflow 결정 원자료 1개 보존. 빌드·실행·바이너리 에셋 미검증.

## [2026-09-22] compile | 사용자 요청으로 기존 16개 기사 전체 재편찬 및 편집기 도구 기사 1개 추가. Q-001~003 미결정과 태그 단일 위치 규칙 유지. 두쫀쿠는 사용자 지시로 편찬 제외. schema의 인간 검증 근거 규칙에 따라 verified를 새로 부여하지 않음.

## [2026-09-22] cleanup | 출처 교체와 미결정 보존 후 대체된 legacy 원자료 19개 삭제. 기존 로그와 Workflow 판단 원본은 유지. 민감 문자열 삭제가 아닌 사용자가 요청한 자료 대체이므로 전체 Wiki 문자열 치환은 수행하지 않음.

## [2026-09-22] lint | 전체 재편찬 최종 검사: 원자료 16개·기사 17개, 순정 lint critical/warning/suggestion/info 0. 순정 색인 렌더러로 7개 색인 갱신. 저장소 링크 37개 문서 오류 0. TestWikiViewer·TestWikiSpaces·TestWikiDiagrams 통과. Edge에서 17개 기사·6개 다이어그램 실제 표시 확인. Wiki 화면의 Workflow 전용 함수 호출 오류를 화면 분기로 수정. 이미 삭제된 두쫀쿠 작업의 잔여 링크 정리. 게임 빌드·실행 검증은 수행하지 않음.

## [2026-09-22] lint | local command: 0 critical, 0 warnings, 0 suggestions, 2 auto-fixed

## 2026-09-22 · Workflow 대시보드 증분 반영

- 사용자 요청·코드 관찰·모의 DOM 검증을 raw/notes/2026-09-22-workflow-dashboard.md에 수집하고 기존 wiki/references/wiki-workflow.md에 통합했다. 인간 검증일은 부여하지 않았다.

## 2026-09-22 · Workflow P1·P2 증분 반영

- ingest: 사용자 개선 요청, 코드 해시·구현 관찰·회귀 검증 범위를 raw/notes/2026-09-22-workflow-closure.md에 수집했다.
- compile: 기존 wiki/references/wiki-workflow.md에 승인 버전 검사, 테스트 수용/정리 완료 분리, 승인자 기록과 과거 기록 해석을 통합했다. 인간 검증일은 추가하지 않았다.

## [2026-09-22] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed
- 검증·색인: 순정 Lint 오류 0, 저장소 링크 37개 문서 오류 0, 관련 회귀 6개 통과. 실제 원자료 18개로 상위 색인 통계를 정정하고 뷰어를 재생성했다.

## [2026-09-22] lint | 18 checks, 0 critical, 0 warnings, 3 suggestions, 0 candidates, 0 auto-fixed
- 보고 전용 에이전트 lint: 원자료 18개·기사 17개, 링크 157개(코드 38) 오류 0, 관련 문서 역링크 82쌍 양방향. 제안: inventory·datasets 계층 미사용(schema 방침상 의도), workflow-review 원자료 태그 tooling→workflow 통일 검토. 신선도 전 기사 75점(verified 없음 상한), 2026-10-06부터 70 미만. 게임 코드·에셋 재검증 아님.

## [2026-09-22] lint | 태그 정리: raw/notes/2026-09-22-workflow-review.md의 tooling 태그를 workflow로 통일하고 raw/notes/_index.md 행을 맞춤. 본문·출처 변경 없음, 순정 lint 0건.

## [2026-09-22] schema | 사용자 결정: verified를 순정 규칙으로 전환. 편찬·재확인 때 그날로 기록하고 편찬 없는 구조 이관에는 넣지 않음. schema.md·wiki-operation.md·_index.md 문구 갱신, 오늘 편찬한 기사 17개에 verified: 2026-09-22 추가. 신선도 전 기사 100점. 순정 lint 0건, 저장소 링크 37개 문서 오류 0. PowerShell 7 부재로 뷰어 미재생성. 빌드·실행 검증 아님.

## [2026-09-22] ingest | 그로기 종료 시 몽타주 정지 경로 수정 조사 (raw/notes/2026-09-22-groggy-montage-stop-fix.md)

## [2026-09-22] compile | 1 source → 0 new articles, 1 updated (combat-groggy). 가산 피격 중 그로기 몽타주 정지 수정(c2b1088a0) 반영. combat-finisher는 몽타주 정지를 다루지 않아 변경 없음.

## [2026-09-22] librarian | scanned 17 articles, 0 stale, 0 low-quality

## [2026-09-22] ingest | 어빌리티 비용·쿨다운 GE 정적 조사 (raw/notes/2026-09-22-current-ability-cost-cooldown.md)

## [2026-09-22] ingest | 대화 정의·호스트·행 데이터와 세션 계약 조사 (raw/notes/2026-09-22-current-dialogue-definition.md)

## [2026-09-22] ingest | 퀘스트 계약·StateTree 태스크·저널 표시 조사 (raw/notes/2026-09-22-current-quest-tasks.md)

## [2026-09-22] ingest | WxCore 태그 정의·로케이터 표시·픽업 상호작용 조사 (raw/notes/2026-09-22-current-core-support.md)

## [2026-09-22] ingest | verified 순정 규칙 전환 결정 (raw/notes/2026-09-22-verified-stock-rule.md)

## [2026-09-22] compile | 5 sources → 0 new articles, 6 updated (combat-abilities, dialogue, foundation, quests, wiki-operation, inventory). 출처 하나뿐이던 5개 기사에 근거 추가, 기존 원자료 current-game·current-ui도 인용. inventory는 foundation 역링크만 추가.

## [2026-09-22] librarian | scanned 17 articles, 0 stale, 0 low-quality

## [2026-09-23] ingest | 인스턴스 구조체 FText 기본값과 에셋 저장 실패 조사 (raw/notes/2026-09-23-instanced-struct-ftext-default-save-fail.md)

## [2026-09-23] compile | 1 source → 0 new articles, 1 updated (world). 엘리베이터 정차 지점 규칙(bWakeOnCall·StopPrompt)과 인스턴스 구조체 FText C++ 기본값 저장 실패 함정(5212bbe3a) 반영. 저장 실패 재현은 에디터 MCP 결과이며 인게임 동작은 미검증.

## [2026-09-23] ingest | 사망·대화 화면 주인을 컨트롤러 컴포넌트로 이동 (raw/notes/2026-09-23-player-screen-owner.md)

## [2026-09-23] compile | 1 source → 0 new articles, 2 updated (ui, dialogue). 사망·대화 화면 클래스·태그 관찰의 주인이 UWxPlayerLayoutComponent로 바뀐 것과 설정/컨트롤러 BP 배치 기준을 반영했다. 인게임 동작은 사용자 확인.
## 2026-09-23 — 표시 VM 모듈 경계와 화면 수명

Ability Resolver의 WxUI 통합과 Dialogue·Quest·QuestObjective의 순수 표시 VM 이전을 원자료 5개로 수집해 ui·dialogue·quests에 반영했다. 도메인 구독은 WxGame 화면 수명이 담당하며 새 Wx 기능 모듈 의존성이나 WxCore 로직은 추가하지 않는다. 빌드·에셋 컴파일·수명 테스트의 검증 범위는 각 작업 기록에 남겼다. 사용자 요청으로 검증용 C++ 테스트와 friend 선언을 제거했다.

## [2026-09-23] ingest | 피해 결과 반환과 투사체 소비 계약 (raw/notes/2026-09-23-damage-result-contract.md)

## [2026-09-23] compile | 1 source → 0 new articles, 1 updated (combat-damage). DamageResult/호환 API/서버 투사체 결과 소비를 반영했다. 내부 이벤트 순서는 유지하며 빌드·자동화 근거는 Workflow Task에서 별도 관리한다.

## [2026-09-23] ingest | 명시적 DamageRequest와 기존 BP 호환 경계 (raw/notes/2026-09-23-damage-request-contract.md)

## [2026-09-23] compile | 1 source → 0 new articles, 1 updated (combat-damage). 동기 요청 API와 호출자 출처 선택, 기존 BP 어댑터 경계를 반영했다. 실행 검증과 사용자의 이전 단계 테스트 수용은 Workflow Task에서 구분한다.
- 추가 지시 반영: 별도 Resolver 파일 배치를 Ability VM 파일 통합으로 대체하고 새 원자료로 근거를 남겼다.

## [2026-09-23] ingest / compile | 타격별 피해 실행 정의
- 원자료 1개를 combat-damage에 통합했다. Hit의 행 재조회 제거와 로컬 정의의 복사·복제 계약을 기록했다. 실행 검증은 Workflow Task에서 구분한다.
- 최종 검증: WBP 새 생성 설정 재로드·컴파일 및 실제 위젯 수명 자동화(Wx.UI.Dialogue.ScreenLifecycle) 통과. 게임 화면·클릭 진행은 별도 확인 대상으로 유지한다.

## [2026-09-23] ingest / compile | 피해 입력 보관 구조 단순화
- 사용자 검토를 반영한 원자료를 추가하고 combat-damage의 현재 구현을 갱신했다. DamageDefinition과 유효 상태를 제거하고 Spec 및 Context의 추가 효과 목록만 유지한다.

## [2026-09-23] ingest / compile | Hit 처리 함수 분리
- 새 원자료를 combat-damage에 통합했다. 기존 클래스 내부 함수 책임과 추가 효과의 캡처/적용 순서를 기록했다.

## [2026-09-23] ingest / compile | Damage Context 정리
- 미사용 테이블 참조와 중복 수치 제거를 원자료로 수집하고 combat-damage의 현재 저장·복제 계약을 갱신했다.

## [2026-09-23] ingest / compile | DamageResult 평탄화
- 사용자 승인에 따라 enum 제거와 직접 bool 필드 계약을 새 원자료 및 combat-damage에 반영했다.

## [2026-09-23] ingest / compile | ApplyDamage API 통합
- 두 반환 API를 하나로 합친 변경과 호출부 전환을 원자료 및 combat-damage에 반영했다.

## [2026-09-23] ingest / compile | Damage 단일 진입점
- 사용자 추가 지시에 따라 ApplyDamage(Request) 단일 함수와 Blueprint 요청 입력을 현재 계약으로 반영했다. 이전 API 통합 원자료는 경과로 보존한다.

## [2026-09-23] ingest / compile | ApplyDamage 네 인자 복원
- 사용자 선호에 따라 요청 구조체를 제거하고 출처/레벨 추론을 단일 함수에 복원한 현재 계약을 반영했다.

## [2026-09-23] ingest | Damage 결과를 앞으로만 흘리는 구조 (raw/notes/2026-09-23-damage-forward-flow.md)

## [2026-09-23] compile | 1 sources → 0 new articles, 2 updated (combat-damage, combat)
- Hit Wrapper GE·FWxHitEffectContext 제거와 ApplyDamage 판정 → DamageResponse 반응 구조로 combat-damage의 처리 순서·판정·결과 해석 절을 다시 썼다. DamageRequest 제거로 바뀐 진입점 시그니처도 반영했다. combat 토픽의 핵심 클래스 표를 갱신했다.

## [2026-09-23] compile | 1 sources → 0 new articles, 1 updated (combat-damage)
- 회피를 Immunity 차단 델리게이트 구독(Dodge 어빌리티)으로 전환하고 Event.DodgeSuccess·bEvaded 제거를 반영했다.

## [2026-09-23] compile | 1 sources → 0 new articles, 2 updated (combat-damage, combat)
- FWxDamageResult 삭제와 Damage GE 컴포넌트 분할(DamageReaction·PerfectGuard·HitStop)을 반영했다.

## [2026-09-23] compile | 1 sources → 0 new articles, 1 updated (combat-damage)
- ApplyDamage의 적용 여부(bool) 반환을 결과 해석 절에 반영했다.

## [2026-09-23] compile | 1 sources → 0 new articles, 1 updated (combat-damage)
- 사망 요건 GE 일원화, 적대의 ApplyDamage 유지 사유, 방어 판정 ExecCalc 이관, 추가 효과 입력 Context·컴포넌트 이관을 처리 순서·판정 절에 반영했다.

## [2026-09-23] compile | 1 sources → 0 new articles, 1 updated (combat-damage)
- 추가 효과 Spec 생성 시점을 반응 뒤로 바꾼 결정을 판정 절에 반영했다.

## [2026-09-23] compile | 1 sources → 0 new articles, 1 updated (combat-damage)
- Damage.Attack 태그 제거에 맞춰 결과별 반응 표를 정정했다.

## [2026-09-23] lint | combat-damage·combat 정합성 정리
- combat 토픽의 핵심 클래스 표를 현재 대미지 구조(입력 조립·판정/계산·반응 컴포넌트·Immunity 회피)로 정정했다. combat-damage의 출처·검증 메모를 커밋 855ec2eb3 기준으로 정리하고 폐기된 단계 노트를 이력으로 명시했다.
