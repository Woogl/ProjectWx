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

## [2026-09-23] ingest | 피해 결과 반환과 투사체 소비 계약 (raw/notes/2026-09-23-damage-result-contract.md)

## [2026-09-23] compile | 1 source → 0 new articles, 1 updated (combat-damage). DamageResult/호환 API/서버 투사체 결과 소비를 반영했다. 내부 이벤트 순서는 유지하며 빌드·자동화 근거는 Workflow Task에서 별도 관리한다.

## [2026-09-23] ingest | 사망·대화 화면 주인을 컨트롤러 컴포넌트로 이동 (raw/notes/2026-09-23-player-screen-owner.md)

## [2026-09-23] compile | 1 source → 0 new articles, 2 updated (ui, dialogue). 사망·대화 화면 클래스·태그 관찰의 주인이 UWxPlayerLayoutComponent로 바뀐 것과 설정/컨트롤러 BP 배치 기준을 반영했다. 인게임 동작은 사용자 확인.

## 2026-09-23 — Ability Resolver 모듈 소유 반영
- 사용자 요청에 따른 WxUI 이동을 원자료로 수집하고 ui 기사에 통합했다. 기존 클래스 경로 리다이렉트와 WBP 실행 미검증 범위를 기록했다.

## [2026-09-23] ingest | 명시적 DamageRequest와 기존 BP 호환 경계 (raw/notes/2026-09-23-damage-request-contract.md)

## [2026-09-23] compile | 1 source → 0 new articles, 1 updated (combat-damage). 동기 요청 API와 호출자 출처 선택, 기존 BP 어댑터 경계를 반영했다. 실행 검증과 사용자의 이전 단계 테스트 수용은 Workflow Task에서 구분한다.
- 추가 지시 반영: 별도 Resolver 파일 배치를 Ability VM 파일 통합으로 대체하고 새 원자료로 근거를 남겼다.

## [2026-09-23] ingest / compile | 피니셔 피해 행 참조 복구
- 새 원자료 1개를 combat-finisher에 통합했다. 두 Variant의 DT_Damage 행 연결을 복구하고 Blueprint 저장·재로딩을 확인했다. 실제 플레이 검증은 대기다.

## 2026-09-23 — Dialogue 표시 VM 분리
- 사용자 모듈 의존성 원칙과 Dialogue VM 분리를 원자료로 수집하고 ui·dialogue에 통합했다. C++ 빌드·WBP 재로드/컴파일·델리게이트 전달 검증과 인게임 미검증 범위를 구분했다.

## 2026-09-23 — Dialogue Resolver 제거
- 사용자 승인으로 화면이 세션 연결을 소유하고 MVVM Create Instance를 사용하는 구조를 ui·dialogue에 반영했다. 기존 Resolver 설명은 새 원자료로 대체했다.

## [2026-09-23] ingest / compile | 타격별 피해 실행 정의
- 원자료 1개를 combat-damage에 통합했다. Hit의 행 재조회 제거와 로컬 정의의 복사·복제 계약을 기록했다. 실행 검증은 Workflow Task에서 구분한다.
- 최종 검증: WBP 새 생성 설정 재로드·컴파일 및 실제 위젯 수명 자동화(Wx.UI.Dialogue.ScreenLifecycle) 통과. 게임 화면·클릭 진행은 별도 확인 대상으로 유지한다.

## [2026-09-23] ingest / compile | 피해 입력 보관 구조 단순화
- 사용자 검토를 반영한 원자료를 추가하고 combat-damage의 현재 구현을 갱신했다. DamageDefinition과 유효 상태를 제거하고 Spec 및 Context의 추가 효과 목록만 유지한다.

## [2026-09-23] ingest / compile | Hit 처리 함수 분리
- 새 원자료를 combat-damage에 통합했다. 기존 클래스 내부 함수 책임과 추가 효과의 캡처/적용 순서를 기록했다.

## 2026-09-23 — Quest 표시 VM 이전

Quest·QuestObjective 이전의 사용자 결정과 구현 계약을 원자료로 수집하고 quests·ui의 구독 책임 설명을 갱신했다. 빌드·에셋·실행 검증은 quest-presentation-vm 작업 기록에 구분한다.

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

## [2026-09-23] ingest | DataTable 행 이름 변경 시 행 핸들 참조 갱신(DataTableRowFixup) (raw/notes/2026-09-23-datatable-row-fixup.md)

## [2026-09-23] compile | 1 sources → 0 new articles, 1 updated (editor-tools)
- DataTableRowFixup 원자료를 editor-tools에 통합했다. 모듈 표·등록 수명·행 참조 갱신 계약과 범위 밖 항목을 추가하고, 제목에 플러그인을 넣어 이를 인용하는 링크·색인 라벨을 맞췄다. 사용자 에디터 확인은 원자료에 구분해 기록했다.

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

## [2026-09-23] ingest | 보스 표시 세 층 구조와 전투 서브시스템 (raw/notes/2026-09-23-boss-battle-three-layer.md)

## [2026-09-23] compile | 1 sources → 0 new articles, 3 updated (ui, game, editor-tools)
- 모델/WxGame 리졸버/WxUI VM 세 층 규칙과 보스 바 연결(ui), UWxBattleSubsystem·IdentityTags·PostInitializeComponents 재실행에 따른 사망 구독 중복 문제(game), WxMVVMToolset.SetBindingSourcePath와 WBP 전환 함정(editor-tools)을 반영했다. 보스 표시는 인게임 미검증.

## [2026-09-23] ingest | 상호작용 목록 VM의 행 전체 재생성과 문구 출처 원칙 (raw/notes/2026-09-23-interaction-list-vm.md)

## [2026-09-23] ingest | 아이템 VM 단일화와 WxToolset의 enum 변수·MVVM 도구 (raw/notes/2026-09-23-item-viewmodel-unification.md)

## [2026-09-23] compile | 2 sources → 0 new articles, 4 updated (world, ui, inventory, editor-tools)
- world: 스캐너 행 교체·선택 복원 규칙, HUD 목록 VM 연결과 주입 전환 시 주의점, 상호작용 문구 출처 원칙을 추가했다.
- ui: 아이템 VM 단일화(PC당 공유 인벤토리 VM), 상호작용 목록 VM, WxGame에 남은 VM의 미결정, MVVM 변환 함수 위치 제약을 추가하고 표시 VM 절을 규칙→사례→예외 순으로 재배치했다.
- inventory: 화면 표시 연결과 픽업 문구 절을 추가했다.
- editor-tools: WxToolset 도구 등록 수(세 개→네 개)를 정정하고 AddEnumVariable·SetEventDestinationWidgetFunction·변환 함수 인자 검증을 추가했다.
- 모두 HEAD `7d2a20408` 기준 정적 확인이며, 상호작용 목록·퀵슬롯·획득 표시의 인게임 동작은 미검증이다.

## [2026-09-23] refresh | 17 articles checked, 4 updated, 0 flagged, 0 retracted
- 원자료에 기록된 파일별 SHA-256을 현재 파일과 대조하고(113건 중 81건 동일), 변경 파일마다 기준 이후 커밋을 추적했다. 변경 대부분은 기존 09-23 원자료가 이미 반영했고, 미반영은 f98eef471·9556bfc78·ba396fc16·9b41020cc·2bfc61535 다섯 커밋이었다.
- 기사에 적힌 Wx 식별자·태그를 코드와 대조한 결과 없는 이름은 삭제 이력으로 명시된 3건뿐이었고, 깨진 상대 링크는 0건이었다.

## [2026-09-23] ingest | 0 피해 히트스톱 조건과 Hit Cue 발행 주석 정정 (raw/notes/2026-09-23-zero-damage-hitstop.md)

## [2026-09-23] compile | 1 source → 0 new articles, 2 updated (combat-damage, combat)
- combat-damage: 히트스톱을 Hit Cue와 같은 조건(피해 > 0 또는 퍼펙트 가드)으로 맞춘 규칙, 0 피해 타격의 후속 범위, Hit Cue·플로터의 서버 발행, 무적 Immunity가 Damage GE만 막는 범위를 추가했다.
- combat: 시스템 경계와 핵심 흐름의 삭제된 Hit GE 표현을 ApplyDamage → Damage GE로 정정했다.
- 코드 대조와 전체 WxEditor 빌드까지 확인했고 플레이는 미검증이다.

## [2026-09-23] ingest | 대화·퀘스트 화면 클래스 제거와 리졸버 연결 (raw/notes/2026-09-23-screen-classes-to-resolvers.md)

## [2026-09-23] compile | 1 source → 0 new articles, 4 updated (dialogue, quests, ui, editor-tools)
- dialogue·quests: 화면 연결 주체를 제거된 UWxDialogueScreen·UWxQuestTracker에서 WxGame 리졸버로 바꾸고, 대화 진행의 VM 명령 델리게이트와 네이티브 퀘스트 저널 델리게이트, 위젯 생성·파괴를 따르는 구독 수명을 반영했다.
- ui: 세 층 규칙의 연결을 리졸버로 한정하고(연결용 위젯 클래스 금지), 동적·네이티브 델리게이트 구독과 VM→도메인 명령 전달 규칙을 추가했다.
- editor-tools: SetEventDestinationWidgetFunction을 SetEventDestination(뷰모델 함수 지원)으로 정정하고 WBP 부모 교체 순서를 추가했다.
- 커밋 570e72562·6daf3f804 기준이며, 사용자가 대화·퀘스트 추적기의 인게임 동작을 확인했다.

## [2026-09-23] ingest | WxAI 리뷰 후속: 미니언 노드 삭제·도플갱어 이동속도 SPD 소유·Mirror 노드 정리 (raw/notes/2026-09-23-wxai-review-followups.md)

## [2026-09-23] compile | 1 source → 0 new articles, 1 updated (ai)
- ai: 이동 속도는 SPD가 소유하고 BT 노드는 에셋에서 지정한 GE로만 바꾼다는 규칙, 도플갱어 Mirror 노드 구성과 WxEffect_MoveSpeedOverride에 의한 Master 속도 추종·종료 시 복원, 따라 쓴 Sprint가 끝나지 않아 덮어쓰기를 쓰는 이유와 남은 제약을 추가했다. 삭제된 미니언 추종 노드 언급을 걷어냈다.
- 커밋 411e74d7f~0aaaa917d 기준이며, 빌드·자동화 테스트·BT 저장값 확인과 사용자 인게임 확인을 거쳤다.

## [2026-09-24] ingest | Nameplate·락온 Reticle을 로컬 NameplateManager가 붙이고 떼는 구조 (raw/notes/2026-09-24-nameplate-manager.md)

## [2026-09-24] ingest | WxCombat 정리: 구간 GE 노티파이 자기 핸들 제거·처형 피해 어빌리티 직접 적용·퍼펙트 가드 Cue 통합 (raw/notes/2026-09-24-wxcombat-cleanup.md)

## [2026-09-24] ingest | 장치 상태 태그는 루트 StateTree 에셋에서만 발행 (raw/notes/2026-09-24-device-linked-state-tag.md)

## [2026-09-24] compile | 3 sources → 0 new articles, 7 updated (ui, game, combat, combat-abilities, combat-damage, combat-finisher, world)
- ui: 머리 위 Nameplate와 락온 Reticle 절을 추가했다(NameplateSource 등록, 로컬 NameplateManager, VisibilityRequirements 하나, LockOn 대상 예외, 거리 여유·메시 기준 높이·대상 소유 수명, 리다이렉트를 넣지 않은 이유). 보스 사례의 옛 UWxNameplateComponent 표현을 정정했다.
- game: AWxPlayerController의 NameplateManager 부착과 LockOnTargetQuery 바인딩, AWxEnemyCharacter의 교전 태그·NameplateSource를 추가했다.
- combat: 락온 표시 경계와 수정 위치 표(구간 상태 GE·처형 피해·락온)를 추가했다.
- combat-abilities: 몽타주 구간 상태 GE 절(몽타주 인스턴스 ID별 핸들, 스택 하나만 제거, 브랜칭 경로, 비몽타주 미적용)과 발동 소유 효과의 제거 방식을 추가했다.
- combat-finisher: 처형 피해를 UWxFinisherDamageComponent 대신 Finisher 어빌리티가 서버에서 Event.ApplyFinisherDamage를 받아 적용하는 경로로 고쳤다.
- combat-damage: 퍼펙트 가드 Cue의 서버 발행과 GC_Hit·GC_PerfectGuard의 UWxCueNotify_Hit 공유를 반영했다. ExecCalc 캡처 정의 통합은 동작이 같아 본문을 바꾸지 않았다.
- world: 서버가 루트 StateTree 에셋의 상태 태그만 발행하는 규칙을 추가했다.
- HEAD ca84c9aac 코드와 대조했다. Nameplate·처형 피해·퍼펙트 가드 Cue·장치 태그 발행의 인게임 동작은 미검증이다.

## [2026-09-24] refresh | 17 articles checked, 7 updated, 0 flagged, 0 retracted
- 원자료에 기록된 파일별 SHA-256 113건을 현재 파일과 대조했다(74건 동일, 30건 변경, 9건은 삭제된 파일 또는 경로가 아닌 제목). 변경 파일마다 이전 refresh(7606a258d) 이후 커밋을 추적하고, 마지막 Wiki 반영(3f1ca0d0f) 이후 커밋을 함께 확인했다.
- 미반영은 aaf557a09·ce6295184·4c1bf3e52·ca84c9aac·74fc58853·67d288fc5·6f452b728 일곱 커밋이었다. 7c4420fbb는 WxAI 리뷰 후속 원자료가 이미 반영했다. 모듈 리뷰 문서(88f7e88f4·d6a6ca97b·9d4c18fcc)는 Workflow 작업 자료라 Wiki로 옮기지 않았다.
- 6f452b728(FWxWait 이름)과 74fc58853(ExecCalc 캡처 정의)은 동작 변화가 없어 원자료에만 기록했다.

## [2026-09-24] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-24] ingest | WxCore 정리: 쓰지 않는 태그·모듈 클래스 제거 (raw/notes/2026-09-24-wxcore-cleanup.md)

## [2026-09-24] compile | 1 source → 0 new articles, 2 updated (foundation, combat-damage)
- foundation: WxCore는 순수 정의만 두고 게임플레이 로직을 구현하지 않는다는 사용자 확정과 `FDefaultModuleImpl` 등록을 반영했다.
- combat-damage: 읽는 곳이 없던 `Damage.Guarded` 결과 태그 제거를 반영했다. 가드 경감·SP 차감 판정은 바뀌지 않았다.

## [2026-09-24] ingest | NameplateManager를 WxGame으로 옮기고 마커·락온 질의를 제거 (raw/notes/2026-09-24-nameplate-manager-wxgame.md)

## [2026-09-24] compile | 1 source → 0 new articles, 3 updated (ui, game, combat)
- ui: 머리 위 Nameplate 절을 WxGame NameplateManager, 적 클래스 순회, `IsAlive && (LockOn || (거리 && State.Engaged))` 조건, 캡슐 윗면 높이, Reticle을 LockOnComponent에 두지 않는 이유로 고쳤다.
- game: `AWxEnemyCharacter`의 NameplateSource와 PC의 `LockOnTargetQuery` 바인딩 설명을 지우고, NameplateManager가 조립 계층의 연결 코드라는 설명으로 바꿨다.
- combat: 대상 위 Reticle·Nameplate를 붙이는 주체를 WxGame NameplateManager로 고쳤다.

## [2026-09-24] ingest | 오래된 CoreRedirects 제거와 HeadClearance 90 확인을 원자료에 추가 (raw/notes/2026-09-24-nameplate-manager-wxgame.md)

## [2026-09-24] compile | 1 source → 0 new articles, 1 updated (ui)
- ui: Ability 리졸버의 CoreRedirect 유지 문장을 제거 사실로 고쳤다. Nameplate 절의 ClassRedirect 문장을 `BP_PlayerController` 재저장·리다이렉트 없음으로 바꾸고, `HeadClearance` 기본값을 90cm(사용자 의도값)로 고쳤다.

## [2026-09-24] ingest | 모듈화 목적 재정의와 배치 원칙 정립 (raw/notes/2026-09-24-module-principles.md)

## [2026-09-24] compile | 1 source → 1 new article, 2 updated (modules; foundation, game)
- modules(신규): 게임 내부 경계를 목적으로 하는 모듈 계층, 참조 모듈 수 기반 배치 규칙, 도메인 간 전달 선호 순서, WxCore 계약 세 조건, 중간 통합 플러그인 금지, 모듈 이동·리다이렉트 절차, 재검토 신호를 정리했다.
- foundation: 책임과 의존 방향 절에서 WxCore 계약 조건과 배치 규칙을 modules로 연결했다.
- game: 관련 문서에 modules를 연결했다.

## [2026-09-24] ingest | 모듈 원칙 초안 반증 검토와 사용자 확정을 원자료에 추가 (raw/notes/2026-09-24-module-principles.md)

## [2026-09-24] compile | 1 source → 0 new articles, 1 updated (modules)
- modules: 참조 수 배치 대신 책임 기반 배치(시험 질문)로 바꿨다. 통로 우선순위를 성격별 선택으로 고쳤다. 계약 조건 1을 "소비자의 책임이 한 도메인 안"으로 고치고, 기능 단위 플러그인을 조립 계층으로 허용했다. WxCore에 상태 없는 헬퍼를 허용했다. 다른 도메인 베이스 클래스가 필요한 코드는 미결정 절로 분리했다.

## [2026-09-24] ingest | WxCombat 불필요한 장치 정리를 원자료에 추가 (raw/notes/2026-09-24-wxcombat-machinery-cleanup.md)

## [2026-09-24] compile | 1 source → 0 new articles, 2 updated (combat-abilities, combat)
- combat-abilities: 쿨다운 무시(순정 Immunity·RemoveOther GE)와 코스트 무시(태그+AbilityBase) 구조, AbilitySet GrantedEffects의 SetByCaller 미설정 시 1초 지속, Wx 고유 발동 실패는 로그가 없는 이유를 추가했다.
- combat: 락온 대상은 UWxLockOnComponent 하나가 들고 태스크가 매 틱 읽는다는 점과 사망 시 BT 정지는 AWxAIController가 맡는다는 점을 추가했다. 빌드 통과, 인게임 미검증.

## [2026-09-24] ingest | 플레이어 래그돌 떨림 원인과 PhysicsAsset 통일을 원자료에 추가 (raw/notes/2026-09-24-ragdoll-physics-asset.md)

## [2026-09-24] compile | 1 source → 0 new articles, 1 updated (game)
- game: 마네킹 메시 네 개가 NiagaraExamples PA를 쓰고 /Game/Mannequins PA는 겹치는 바디 쌍의 충돌 때문에 쓰지 않는다는 점, 정본 PA가 예제 폴더에 있다는 제약을 추가했다. 인게임 미검증.

## [2026-09-24] ingest | WxUI 리뷰 후속: 일시정지 해제 규칙 정정·Effect VM 월드 타이머·인디케이터 여백 상수 (raw/notes/2026-09-24-wxui-review-followups.md)

## [2026-09-24] ingest | 소환물 주인 태그를 State.MinionMaster.*에서 Master.*로 변경 (raw/notes/2026-09-24-master-tag-rename.md)

## [2026-09-24] compile | 2 sources → 0 new articles, 2 updated (ui, combat)
- ui: 정지를 `FCanUnpause` 대리자와 함께 걸고, 게임모드는 대리자 없는 정지를 해제 때 그냥 지운다는 제약을 일시정지 절에 추가했다(UE 5.8 `AGameModeBase::ClearPause` 대조, C++·에셋에 다른 정지 주체 없음). Effect VM이 남은 시간을 월드 타이머로 갱신해 정지 중에는 멈춘다는 점을 추가했다.
- combat: 수정 위치 표에 소환 상한·주인 태그(`Master.*`) 행을 추가했다.
- 인디케이터 여백 상수화(`08a1ef928`)와 확인 팝업 헬퍼 인라인(`a4c28a949`)은 기사가 다루지 않는 세부라 원자료에만 기록했다. 인게임 동작은 미검증이다.

## [2026-09-24] refresh | 17 articles checked, 3 updated, 0 flagged, 0 retracted
- 원자료에 기록된 파일별 SHA-256 113건을 현재 파일과 대조했다(72건 동일, 32건 변경, 9건은 삭제된 파일 또는 경로가 아닌 제목). 변경 파일마다 이전 refresh 기준(ca84c9aac) 이후 커밋을 추적했다.
- 미반영은 c4dee8382(소환물 주인 태그)와 WxUI 리뷰 후속 eb92e99a7·08a1ef928·1b15a61ff·a4c28a949였다. 나머지 커밋은 wxcore-cleanup·nameplate-manager-wxgame·wxcombat-machinery-cleanup·ragdoll-physics-asset·module-principles 원자료가 이미 반영했다. WxUI 모듈 리뷰 문서(bf9c872ca)는 Workflow 작업 자료라 Wiki로 옮기지 않았다.
- foundation은 같은 날 편찬됐지만 `verified`가 2026-09-22로 남아 있어, 본문(충돌 채널·프리셋·DefaultGame 등록·공용 계약)을 HEAD d76e48717과 다시 대조하고 `verified`를 맞췄다.

## [2026-09-24] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-24] ingest | 상호작용 계약을 선택지 하나로 통합 (raw/notes/2026-09-24-interaction-contract-options-only.md)

## [2026-09-24] compile | 1 source → 0 new articles, 3 updated (foundation, world, combat-finisher)
- foundation: `IWxInteractable`은 선택지를 내고 선택지가 비면 상호작용할 수 없다는 계약으로 고쳤다(`CanInteract`·`GetInteractionPrompt` 삭제).
- world: 스캐너는 선택지가 빈 대상의 행을 만들지 않고, 서버는 거리와 선택지 유효성으로 자격까지 검증한다고 고쳤다.
- combat-finisher: 적의 자격 판정 위치를 `GetInteractionOptions`로 고치고, 문구는 넘겨받은 상호작용자의 Finisher 어빌리티에서 찾으며 그 어빌리티가 없으면 선택지를 내지 않는다는 점을 추가했다. `36fbb4371` 위 작업, 빌드 통과, 인게임 미검증.

## [2026-09-24] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-24] ingest | AI 트리 정지·잠금을 AWxAIController 단독으로 (raw/notes/2026-09-24-ai-brain-control-single-owner.md)

## [2026-09-24] compile | 1 source → 0 new articles, 3 updated (ai, combat, combat-groggy)
- ai: "트리 정지와 잠금" 절을 추가했다. 사망은 `OnDeath` → `StopLogic`, 그로기는 `Ability.Groggy` 태그 → `LockResource(Reaction)`이고 전투 쪽은 트리를 건드리지 않는다.
- combat: 경계 문장의 사망 BT 정지 주체를 트리 정지·잠금 전체의 주체로 넓혔다.
- combat-groggy: 수명 도식과 본문에서 어빌리티의 AI 일시정지·재개를 지우고 "AI 트리 잠금" 절을 추가했다. 작업 트리 코드 기준, 빌드 통과, 사용자 인게임 확인.

## [2026-09-24] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-24] ingest | 장치 StateTree 정리: 복원 판정 이동·중복 장치 제거·몽타주 태스크 WxCombat 이관·연출 태스크 수정 (raw/notes/2026-09-24-device-statetree-cleanup.md)

## [2026-09-24] compile | 1 source → 0 new articles, 3 updated (world, combat, modules)
- world: `FWxDeviceExecutionPolicy` 문단을 "복원 판정" 절로 바꿨다(`UWxDeviceStateTreeComponent::IsRestoring`, 상태 적용·일회성 효과 태스크 분류). 다른 도메인 태스크가 서버의 InitialState 적용을 실제 진입으로 보는 제약과 저작 규칙, 순정 해법 보류(사용자 결정)를 추가했다. 장치 트리의 다른 도메인 태스크(`Actor.InteractingCharacter` 바인딩, 몽타주 1회 재생)와 연출 태스크의 피어별 동작(레벨 시퀀스 카메라 컷은 당사자 피어만, 사운드 원샷)을 추가했다. 종료 판정이 순정 `IsRunning` 대신 실행 상태를 보는 이유와 상호작용 가능 여부가 대기 노드 등록만 본다는 점을 반영했다.
- combat: 수정 위치 표에 `FWxStateTreeTask_PlayMontageOnce` 행을 추가하고 의존에 StateTree를 넣었다.
- modules: 배치 사례 표에 몽타주 1회 재생 태스크(효과 도메인 배치)를 추가했다.
- 대기 등록부 템플릿 인라인(`b32c1f622`)은 기사가 다루지 않는 세부라 원자료에만 기록했다. 빌드는 `36fbb4371`까지만 기록이 있고 인게임 동작은 확인하지 않았다.

## [2026-09-24] refresh | 17 articles checked, 3 updated, 0 flagged, 0 retracted
- 직전 refresh 커밋(`72c6cbe68`) 이후 커밋 17건을 추적했다. 상호작용 계약(`db3c57be4`)과 AI 트리 제어(`142fab5d6`)는 이미 반영돼 있었다. 주석 전용 4건(`f6f5fa1e1`·`440155e6f`·`edfab5951`·`50e433672`, 주석 외 줄 변경 없음 확인)과 Workflow 리뷰 문서 3건(`378c0decf`·`861a8b494`·`36e95ff5e`)은 반영 대상이 아니다.
- 미반영은 장치 StateTree 정리 8건(`b32c1f622`·`844010a94`·`f6b4af9d4`·`9e922a15b`·`36fbb4371`·`80e3e370b`·`8f06b1ffc`·`70495c0e7`)이었다. 영향 기사는 world·combat·modules이고, 나머지 14개 기사는 이 변경과 겹치는 서술이 없다(Wiki 본문 검색으로 확인).

## [2026-09-24] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-25] ingest + compile | Row 미리보기 기본 구조체 축약
- 원자료 1개를 수집하고 editor-tools에 초기 기본값 비교와 원문 툴팁 유지 규칙을 반영했다. C++ 컴파일 통과, DLL 점유로 링크 실패, 화면 미검증.

## [2026-09-25] ingest + compile | Row 미리보기 툴팁 통일
- 사용자 후속 요청을 원자료로 수집하고 editor-tools에 셀과 툴팁 표시 통일을 반영했다. 기존 원문 툴팁 유지 정책을 대체한다.

## [2026-09-25] ingest + compile | Row 미리보기 빈 하위 구조체 수정
- HGTest 사용자 화면으로 부모 전체 비교의 누락을 확인했다. 하위 객체별 기본값 비교와 제한 범위를 editor-tools에 반영했다.

## [2026-09-25] ingest + compile | AnimNotify 라벨 축약
- 사용자 합의와 17종 표시 함수 근거를 수집하고 editor-tools에 표시 규칙을 통합했다. 런타임 동작 변경과 에디터 화면 검증은 포함하지 않는다.

## [2026-09-25] ingest | 어빌리티 규칙 변경과 몽타주 섹션 모델 (raw/notes/2026-09-25-ability-montage-section-model.md)

## [2026-09-25] ingest | 어빌리티·GE 데이터를 에셋 한 곳으로: GA_ 복귀, DT_Ability·DT_Effect 제거 (raw/notes/2026-09-25-ability-data-on-ga.md)

## [2026-09-25] compile | 2 sources → 0 new articles, 12 updated
- combat-abilities: "타입과 GA_"(C++ 타입은 규칙, GA_는 콘텐츠, 공격 타입 넷, Abstract 타입, 데이터 배치 규칙, 세트 부여와 같은 클래스 건너뛰기), "몽타주 섹션", "비용과 쿨다운"(CDO에서 읽는 비용·쿨다운 값, `CooldownGameplayEffectClass` 그룹), "검증" 절로 다시 썼다. 콤보 재시작·늦은 노티파이 거르기·컷신 중 입력 차단을 발동 절에 더했다.
- combat-finisher: 한 벌 통합과 `UWxAnimNotify_FinisherVictim`·`UWxAnimNotify_FinisherDamage` 흐름으로 구현 경로를 다시 썼다. Sources 목록에 빠져 있던 상호작용 계약 원자료를 넣었다.
- combat-damage: 섹션 기반 피격 반응, 가드 반응 섹션, 그로기 강등 위치(`UWxEffectComponent_DamageReaction`), 가드 경감값의 GE_ 소유를 반영했다.
- combat-groggy: "그로기 중 피격" 절(넉 계열 강등 위치)을 추가했다.
- combat-resources: 비용을 GA_의 `CostResource`·`CostAmount`로 고쳤다. Q-003 미결정은 그대로다.
- combat: 경계에 GA_ 데이터 배치와 테이블 제거를 적고, 수정 위치 표에 강등·GE 표시·섹션·착지·처형 노티파이·컷신 차단·시체 수명 행을 더했다.
- game: 시체 수명(`AWxCharacterBase::CorpseLifeSpan`)과 착지 섹션 탐색 절을 추가했다.
- ai: "어빌리티 발동" 절(에셋 태그 조회, GA_의 번호 태그)을 추가했다.
- world: 스캐너의 발동 판정이 기본 인스턴스로 한다는 점을 상호작용 계약 절에 더했다.
- inventory: 소비 아이템을 인벤토리가 고르는 흐름(`CanUseConsumable`·`UseConsumable`, `FindConsumableInstance`)으로 도식과 본문을 고쳤다.
- ui: 어빌리티 슬롯 VM의 기본 인스턴스 판정과 버프 목록의 `UWxEffectComponent_UIData` 조회를 보강했다.
- editor-tools: "AnimMontage 섹션 편집" 절(`AppendMontage`·`RenameSection`·`SnapNotifyEndsToSections`·`SnapNotifyStartsToSections`)을 추가했다.
- 코드 서술은 HEAD `d63ce0630`과 대조했다. GA_·몽타주·GE 에셋 값과 PIE 결과는 원자료를 따르며, HGTest·분신·도플갱어·처형·가드 경감·락온은 인게임으로 확인하지 않았다.

## [2026-09-25] refresh | 17 articles checked, 12 updated, 0 flagged, 0 retracted
- 직전 refresh 커밋(`34bb76871`) 이후 커밋 23건을 추적했다. Row 미리보기·AnimNotify 라벨(`9e5529e87`·`b93072ef8`)은 이미 반영돼 있었다. 작업 기록만 바꾼 3건(`4e80be7ba`·`8652e585e`·`d63ce0630`)은 반영 대상이 아니다.
- 미반영은 어빌리티 테이블 구동 전환 1-1·1-2단계 14건(`b4510a45c`~`b652ee043`)과 GA_ 복귀 4건(`5153da936`·`0473e201b`·`e305161ee`·`7c52ce0ce`)이었다. modules·foundation·dialogue·quests와 Wiki 운영 참조 문서는 이 변경과 겹치는 서술이 없다(본문 검색과 대조로 확인).

## [2026-09-25] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-25] ingest + compile | 작업 테스트 결과의 AI 접수

사용자가 제안한 이상 없음/이상 있음 선택과 문제 기록 전달을 raw/notes/2026-09-25-workflow-test-feedback.md에 수집하고 wiki-workflow에 입력·접수·수정/정리·재확인·복구·기록 갱신 경로를 통합했다. Workflow 사용 안내와 테스트·완료 절차도 연결했다. 새 서비스·화면 테스트와 기존 저장/실행 회귀 8개가 통과했으며 실제 AI·브라우저 육안·게임 실행 검증은 별도다.

## [2026-09-25] compile | 어빌리티·이펙트·캐릭터 목록 기사(references/ability-list·effect-list·character-list) 추가. BatchFiles/ExportAbilitySystemLists.bat(.agents/scripts/Export-AbilitySystemLists.ps1)이 GA_·ABS_·GE_·AM_·BT_·DT_·캐릭터 BP 등 에셋의 저장값·참조(몽타주 섹션·노티파이, 속성 초기값, 피해 행과 행 참조 포함)와 C++ 소스를 에디터 없이 읽어 생성하고, OpenWiki.bat이 위키를 열 때 다시 만든다. combat-abilities에 관련 링크 추가. 게임 실행 검증 아님.

- 2026-09-25: 체크포인트 SaveGame 전환 원자료를 수집하고 world 기사의 저장·조회·초기화 계약과 진입점을 갱신했다. 정적 확인이며 인게임 검증은 별도다.

- 2026-09-25: game 기사의 GameInstance 책임·새 게임 삭제 실패·부활 조회 경로를 같은 원자료로 갱신하고 원자료 수를 70개로 대조했다.

- 2026-09-25: 사용자 후속 명칭 변경에 따라 world의 RecordCheckpoint를 SaveCheckpoint로 정정했다. API·태스크·InstanceData·파일명을 함께 변경하고 기존 에셋 호환용 StructRedirects를 추가했다.

- 2026-09-25: SpawnerLibrary 제거 원자료를 수집하고 world에 AWxSpawner::RespawnAll의 C++ 호출·Manual 제외·로드 범위를 반영했다.

- 2026-09-25: 사용자 결정으로 체크포인트 슬롯을 WxCheckpoint 하나로 통일하고 world의 PIE 분리 설명을 갱신했다. 세분화는 추후 진행한다.

- 2026-09-25: ST_CheckPoint 리세이브 후 구조체 리다이렉트 두 항목을 제거하고 별도 프로세스의 로드·StateTree 컴파일·저장 성공을 원자료와 world에 반영했다.

## [2026-09-25] ingest + compile | Workflow 작업 기록 현황

기존 웹 화면을 선호한 사용자 답변과 구현 관찰을 raw/notes/2026-09-25-workflow-task-records.md에 수집하고 wiki-workflow에 표시 원본·분류·갱신 절차·승인 경계를 통합했다. 작업별 상태와 승인 원문은 Workflow에 유지한다. 브라우저 육안 확인은 로컬 파일 URL 보안 정책으로 수행하지 못했으며 모의 DOM·저장·실행 회귀 결과와 구분한다.

## [2026-09-25] compile | 어빌리티·이펙트 목록 표기 정정

Export-AbilitySystemLists.ps1을 고쳐 목록을 다시 생성했다. 빈 칸이 "없음"이 아니라 부모 기본값임을 명시하고 어빌리티 타입 칸·GE_ 부모 칸을 C++ 생성자 파일로 링크했다. GE_ 기타 칸에서 엔진이 컴포넌트 값을 되써 두는 5.3 폐기 필드(`UGameplayEffect::PreSave`)를 뺐다. 몽타주 섹션은 시작 시각 순이고 노티파이 `{}` 값은 인스턴스·섹션 대응이 없는 모음임을 정정했다. config.md에 세 목록이 스크립트로만 갱신하는 생성물임을 적었다. 게임 실행 검증 아님.


## [2026-09-25] ingest + compile | 쿨다운 GE 통합

사용자 결정(쿨다운 클래스 통합, CooldownTags 컨테이너 유지, SharesCooldownGroup 미도입, Pattern 번호 태그 유지)과 구현·검증 관찰을 raw/notes/2026-09-25-cooldown-single-ge.md에 수집하고 combat-abilities의 타입과 GA_·비용과 쿨다운·검증 절에 반영했다. 설계 때 놓친 엔진 순정 쿨다운 태그 규칙과 그 보정을 적었다. 어빌리티 목록은 생성기에 CooldownTags 칸을 더해 다시 만들었다. 빌드·데이터 검증 커맨드릿 통과, 인게임 미검증.

## [2026-09-25] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-25] ingest | 체크포인트 SaveGame 검증 범위

raw/notes/2026-09-25-checkpoint-validation-scope.md에 사용자 제출 범위와 적용 버전·기존 근거의 경계를 수집했다. Task SHA-256과 문서 제외 코드 식별값은 접수 값과 일치했다. 사람의 결과 원문·승인 정본과 AI report는 Workflow 서버가 원래 Task에 기록하며, 이 작업은 Task·목차·접수 JSON을 직접 수정하지 않는다.

## [2026-09-25] compile | 체크포인트 검증 범위와 디스크 복원 설명 정리

기존 world·game 기사에 사용자 확인 범위, 예외·슬롯 공유 검증과 코드 리뷰 수용의 미해결 경계, 체크포인트 한 개의 디스크 유지와 전체 게임 상태 복원의 차이를 반영했다. 재접속의 세부 절차는 추정하지 않았으며, 이전 원자료는 변경하지 않았다. 게임 코드·에셋 수정이나 새 플레이 검증이 아니고 작업 전체 완료도 아니다. 원자료 색인·통계와 루트 탐색을 갱신했다.

## [2026-09-25] lint | 체크포인트 문서 정리 검증

순정 llm-wiki lint --local --json 결과 critical·warning·suggestion 0건, 수정 0건. 변경 문서 6개의 상대 링크 170개를 확인했고 누락은 없었다. 실제 파일 수는 원자료 78개·기사 21개이며 기존 루트 색인의 기사 수 20을 21로 맞췄다. git diff --check는 변경한 추적 Wiki 파일에서 통과했다. 게임 플레이·신규 빌드·사람 코드 리뷰의 검증 결과가 아니다.

## [2026-09-25] compile | 체크포인트 사용자 확인의 적용 버전 명시

마무리 해시 비교 중 이 정리 작업이 수정하지 않은 Workflow 스크립트(Wiki-AI.cjs, Workflow-Execution.cjs, Workflow-TestFeedback.cjs, wiki-viewer/test-feedback.js, Wiki-AI-Providers.cjs)의 동시 변경을 관측했다. Task 해시는 접수 값과 같지만 문서 제외 코드 식별값은 e939b31ed877673e1a3d957abbdd3938208cb82762548711572da2ec83df1a31에서 c5f50d478d2cb6a6a6803b2e3a54be0b869086144b9c3315299d3a90c35599f0으로 달라졌다. world의 확인 설명을 정리 시작 시 대조한 제출 버전에 한정했고 기존 raw는 그 시점의 불변 근거로 보존했다. 변경 후 버전 수용은 blocker로 보고하며 다른 작업의 변경은 건드리지 않는다.

## [2026-09-25] ingest + compile | 테스트 결과 처리 AI 선택

사용자의 다른 AI 지원 요청과 구현·CLI 옵션·검증을 raw/notes/2026-09-25-workflow-feedback-providers.md에 수집하고 wiki-workflow에 제공자 선택·변경 재시도·이력·권한 경계를 통합했다. 검토의 읽기 전용과 테스트 결과 실행을 구분한다. 모의 제공자·HTTP·DOM 및 기존 실행 회귀 7개를 통과했고 실제 제공자 AI 요청·브라우저 육안 검증은 별도다.

## [2026-09-25] ingest + compile | 쿨다운 1회 사용 4초 결함 수정

사용자 보고(회피 1회에 쿨다운 4초, UI 진행률 이상)의 원인과 수정을 raw/notes/2026-09-25-cooldown-single-ge.md에 더하고 combat-abilities의 비용과 쿨다운 절을 고쳤다. 엔진이 GE를 활성 목록에 넣은 뒤 지속시간을 재계산해 대기열 MMC가 자기 자신을 셌고, 대기열 계산을 ApplyCooldown의 SetByCaller로 옮겼다. 임시 자동화 테스트로 지속시간을 확인했고 인게임은 미검증.

## [2026-09-25] lint | 어빌리티·이펙트·캐릭터 목록 교차 검증

헤드리스 커맨드릿의 T3D 내보내기와 Python 속성 조회로 세 목록을 스크립트 파서와 독립적으로 대조했다. GA_ 40개(타입·WxAbilitySet·저장값 칸), 몽타주 34개(섹션 순서, 노티파이 클래스·개수·저장값), WxAbilitySet·캐릭터 구성, DT_Damage·DT_CharacterAttribute 행 값, GE_ 6개, C++ 클래스 에셋 사용처, 표에 없는 행 참조 3건이 일치했다. 노티파이의 벡터·트랜스폼 값(`LocalSpawnOffset`)을 적지 않는다는 점이 문서에 없어 머리말에 보강했다. 게임 실행 검증 아님.

## [2026-09-25] ingest + compile | 쿨다운 수정 사용자 확인

수정 뒤 사용자의 인게임 확인("잘 되네요", 회피 쿨다운·UI 진행률)을 raw/notes/2026-09-25-cooldown-single-ge.md와 combat-abilities 검증 메모에 반영했다. 소환물 쿨다운 무시·네트워크 복제는 미확인으로 남긴다.

## [2026-09-25] compile | 어빌리티·이펙트 목록의 C++ 링크 제거

사용자 결정으로 어빌리티 타입 칸과 GE_ 부모 칸의 C++ 생성자 파일 링크를 빼고 클래스 이름만 남겼다. 링크는 해당 클래스 하나의 파일만 가리켜 상위 클래스 기본값을 놓치게 하고 어빌리티 목록의 16%를 차지했다. "빈 칸은 부모 기본값이며 기본값은 C++ 클래스와 상위 클래스의 생성자·헤더 초기값에 있다"는 머리말은 유지했다. 게임 실행 검증 아님.


## [2026-09-25] ingest + compile | refresh: 프로젝트 기본 설정 3건

마지막 Wiki 반영(5c66bfaf9) 이후 미반영 커밋 be1c832e4·fbf8d89e9·ccf16da7b를 raw/notes/2026-09-25-performance-config-defaults.md에 수집했다. 38d4dde08(에셋 리세이브)는 지식 변화가 없어 제외했다. ui에 위젯 속성 바인딩 Prevent를, foundation에 기본 충돌 복잡도 Simple as Complex와 에디터 스케일러빌리티 High 기본값을 반영했다. 설정 의미는 UE 5.8 엔진 소스와 대조했고 WBP_·EUW_ 레거시 바인딩 0건과 C++ complex 트레이스 0건을 재확인했다. 메시 38개 분류는 적용 당시 에디터 조회 결과이며, 에셋 쪽 쿼리·게임 실행·프레임 측정은 검증하지 않았다.

## [2026-09-25] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-25] ingest + compile | 모듈 리뷰의 검증 범위 정정

사용자가 지정한 `39f3629a4` 기준으로 `raw/notes/2026-09-25-checkpoint-task-completion.md`와 `raw/notes/2026-09-25-runtime-lifecycle-review.md`를 수집했다. world에는 체크포인트 태스크의 Failed 반환과 상태 실패 집계의 차이를, game에는 동일 객체 재표시 시 어빌리티 소실·구독 누적의 조건과 새 게임 선택 검사의 한계를, ui에는 컴포넌트 소유 요청과 HUD 내부 메뉴 요청의 정리 범위를 반영했다. 수정 판단은 모듈별 Workflow 리뷰에 유지한다. 이후 외부 작업의 미커밋 코드 변경은 포함하지 않았으며, 기존 사용자 성공 경로 확인을 저장 실패 실행 검증으로 확대하지 않는다. 게임 코드 수정·빌드·PIE 실행은 하지 않았다.

## [2026-09-25] ingest + compile | 퀘스트 전환 예약의 수명

`raw/notes/2026-09-25-quest-transition-lifetime.md`를 근거로 quests에 다음 틱 예약과 새 수주의 수명 조건을 추가했다. 기준은 사용자 지정 `39f3629a4`이며 StateTree 호출 스택의 재진입 회피가 이전 실행의 예약 취소까지 보장하는 것은 아님을 명시했다. 수정 판단은 WxQuest 리뷰에 남긴다. 게임 실행 검증은 하지 않았다.

검증: 순정 llm-wiki lint는 critical·warning·suggestion 0건, CheckWikiLinks는 64문서에서 오류 0건이다. 리뷰 6문서의 기준 커밋·소스 수·발견 개수·근거 행 범위를 확인했고 diff --check를 통과했다. Wiki/Workflow 뷰어를 각각 62문서로 다시 내보냈다.

## [2026-09-25] ingest + compile | UI 데이터 인터페이스 제거

사용자 합의와 구현 관찰을 raw/notes/2026-09-25-ui-data-interface-removal.md에 수집하고 ui·foundation·modules·combat-abilities·editor-tools에 통합했다. WxGame 리졸버의 데이터 전달, WxUI GAS 공통 갱신, 캐릭터 VM 공유 수명, 에디터 썸네일 의존성을 반영했다. 실행 검증과 인간 플레이 수용은 Workflow 작업 기록에서 구분한다.

## [2026-09-25] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-25] ingest + compile | UI 표시 연결 검증

최종 빌드·회귀 3개·리다이렉트 없는 Blueprint 97개 컴파일과 무효화된 어빌리티 슬롯 정리 수정 근거를 수집해 ui에 반영했다. 원격 복제와 실제 화면 확인은 미검증으로 구분했다.

## [2026-09-25] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-25] ingest + compile | Workflow 3단계 단순화와 테스트 체크리스트 (raw/notes/2026-09-25-workflow-simplification.md)

사용자 승인(제안 1~5)과 추가 요청(추가 질문이 없을 때의 구현 승인, AI·사람 담당 테스트 체크리스트)을 수집하고 references/wiki-workflow에 통합했다. 판단과 실행·대시보드·테스트 결과 전달·로컬 서비스 경계를 현재 구조로 바꾸고 폐지한 웹 새 작업 경로는 이전 구조로 줄였다. 기준 HEAD `c9e2efec6`에 미커밋 변경을 포함하며 실제 AI 처리 요청과 사람의 화면 확인은 포함하지 않는다.

## [2026-09-25] lint | local command: 0 critical, 0 warnings, 0 suggestions, 0 auto-fixed

## [2026-09-25] ingest + compile | Exclusive 태그 차단과 도플갱어 발동 조건 면제

사용자 결정·구현 관찰·검증 근거를 raw/notes/2026-09-25-exclusive-tag-blocking.md에 수집하고 combat-abilities·ai에 통합했다. 차단 태그 선언, 콤보 자기 기여 제외, Recovery 수명, IgnoreAbilityActivationTags 이름·면제 범위, 미러링 재시도 제한을 반영했다. 빌드·GAS 회귀 2개·GA 40개의 1,600개 차단 관계·리다이렉트 없는 효과 참조 로드는 통과했고, 사람의 코드 리뷰·플레이·네트워크는 미확인이다.

## [2026-09-25] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-25] ingest + compile | Workflow 편의 점검과 기록 기반 작업 현황 (raw/notes/2026-09-25-workflow-convenience-review.md)

AI·사람 관점 점검 근거와 사용자가 승인한 네 개선을 수집하고 references/wiki-workflow에 통합했다. 작업 현황은 각 기록 제목 아래 상태 줄에서 만들고, 코드 리뷰는 테스트 체크리스트 사람 항목이며, 저장소 전체 코드 식별값 검사는 없앴다. 기준 HEAD `c9e2efec6`에 미커밋 변경을 포함하며 실제 AI 처리 요청과 사람의 화면 확인은 포함하지 않는다.

## [2026-09-25] lint | local command: 0 critical, 0 warnings, 0 suggestions, 0 auto-fixed

## [2026-09-25] ingest + compile | 어빌리티 공통 차단의 ASC 통합 (raw/notes/2026-09-25-ability-block-policy-centralization.md)

자식 공통 차단 제거와 향후 클래스 축소 방향을 수집하고 combat-abilities에 통합했다. GetAbilityBlockTags가 그룹·태그와 명시 선언을 합치고 ASC의 순정 ApplyAbilityBlockAndCancelTags와 콤보 조회가 같은 목록을 사용한다. 빌드·GAS 회귀 3개·GA 40개 차단 관계 1,600건·점프 40건을 확인했다. 코드 리뷰·실제 플레이·네트워크는 사람 확인 대기로 구분한다.

## [2026-09-25] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-25] ingest + compile | 차단 테스트 정리와 주석 정정 (raw/notes/2026-09-25-exclusive-submission-cleanup.md)

사용자의 테스트 제거·주석 정정·제출 요청에 따라 임시 C++ 테스트 2개, 전용 friend, GA 검증 Python 2개를 제거했다. combat-abilities에 테스트가 제출 전 제거됐음을 명시하고 이전 회귀 결과는 실행 당시 근거로 보존했다. 차단 관련 주석 점검 대상 12개 파일에서 주석을 제외한 코드 해시가 동일함을 확인했다. 플레이·네트워크 확인 대기는 유지한다.

## [2026-09-25] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed

## [2026-09-26] ingest + compile | Workflow 웹 새 작업·이어하기와 터미널 창 실행 (raw/notes/2026-09-26-workflow-web-tasks.md)

사용자 결정(웹 안에서 처리, AI 대화는 터미널만, 웹에서 맡긴 구현은 모든 명령 허용, 진행 과정은 터미널 창 표시)에 따른 웹 새 작업·질문 답변·구현 승인·추가 요청·터미널 이어하기를 수집하고 wiki-workflow에 통합했다. 정하기는 읽기 전용, 구현·추가 요청·테스트 결과 처리는 모든 명령 허용이며 AI 처리는 터미널 창 실행기로 돌린다. 자동화 테스트 5개·링크 검사·가짜 AI 실제 창 확인·헤드리스 Edge 캡처를 통과했고 실제 AI 요청과 사람 확인은 대기다.

## [2026-09-26] lint | local command: 0 critical, 0 warnings, 0 suggestions, 0 auto-fixed

## [2026-09-26] ingest + compile | Workflow 웹 새 작업의 한글 기록 이름 (raw/notes/2026-09-26-workflow-korean-record-names.md)

사용자 질문(해시 말고 한글로 추적)에 따라 웹 새 작업의 기록 이름을 접수 식별자 해시 대신 한글 제목으로 바꾼 내용을 수집하고 wiki-workflow에 통합했다. 같은 이름은 -2를 붙이고 같은 접수의 재전송은 처리 이력에서 찾는다. 서버 테스트와 실제 cmd start 창의 한글 경로 전달을 확인했다.

## [2026-09-26] lint | local command: 0 critical, 0 warnings, 0 suggestions, 0 auto-fixed

## [2026-09-26] ingest + compile | Workflow 대시보드 분류별 기록 버튼 (raw/notes/2026-09-26-workflow-dashboard-row-actions.md)

사용자 결정(확인 대기에는 기록 열기 없음, 완료에는 작업 진행 없음)을 수집하고 wiki-workflow의 대시보드 설명에 반영했다. TestWikiViewer로 분류별 버튼 구성을 확인했다.

## [2026-09-26] lint | local command: 0 critical, 0 warnings, 0 suggestions, 0 auto-fixed

## [2026-09-26] ingest + compile | Workflow 대시보드 행 버튼 최종 결정과 표시 정리 (raw/notes/2026-09-26-workflow-row-actions-final.md)

사용자 결정으로 진행 중 기록에서도 기록 열기를 빼 확인 대기·진행 중은 작업 진행만, 완료·리뷰·참고는 기록 열기만 두는 기준을 wiki-workflow에 반영했다. 배지가 있는 행의 버튼 줄 어긋남 수정과 사이드바 소개 문구 제거는 원자료에만 남겼다.

## [2026-09-26] lint | local command: 0 critical, 0 warnings, 0 suggestions, 0 auto-fixed

## [2026-09-26] ingest + compile | AnimNotify 색상 분류 정리 (raw/notes/2026-09-26-animnotify-categories.md)

접수 해시와 일치하는 작업 기록의 전체 체크리스트 통과를 확인하고 editor-tools와 foundation에 6개 색상·17종 매핑·Editor config·전처리 보호·사람 확인 범위를 반영했다. 작업 기록과 접수 JSON은 수정하지 않았다. 게임 코드·에셋 수정 및 빌드·플레이 재실행은 없으며 패키지 빌드 미실행 범위를 유지한다.

## [2026-09-26] 검증 | AnimNotify 문서 정리

순정 llm-wiki lint --local --json에서 이번 원자료·기사 관련 지적은 없었다. 전체 결과는 범위 밖 raw/notes/2026-09-26-module-review-contracts.md의 색인 누락 경고 1개·미편찬 제안 1개로 실패했으며 해당 자료는 수정하지 않았다. 대상 문서 3개의 로컬 링크 53개와 UTF-8 읽기, 접수 SHA-256과 작업 기록 일치를 확인했다. Export-Wiki.ps1로 Wiki·Workflow 뷰어 65개 문서를 재생성했다.

## [2026-09-26] ingest + compile | 모듈 리뷰의 재등록 수정과 콤보 배열 계약 (raw/notes/2026-09-26-module-review-contracts.md)

7개 런타임 모듈의 점진 리뷰에서 확인한 C++ 계약을 game·combat·combat-abilities의 해당 문단에만 반영했다. 해결된 재등록·새 게임 지적과 콤보 배열·방향 섹션을 구분하고 이전 승인·사람 테스트 대기는 보존했다. 소스 수정·빌드·게임 실행 검증은 수행하지 않았다.

## [2026-09-26] ingest + compile | Workflow 웹 처리 첫 실제 실행과 무관한 경고 처리 규칙 (raw/notes/2026-09-26-workflow-first-real-run.md)

웹 작업 진행으로 보낸 AnimNotify 테스트 결과를 Codex가 터미널 창에서 처음 끝까지 처리한 관찰과, 다른 세션 원자료의 일시적 lint 경고가 완료를 막은 원인·처리 지시 변경을 wiki-workflow에 반영했다.

## [2026-09-26] lint | local command: 0 critical, 0 warnings, 0 suggestions, 0 auto-fixed

## [2026-09-26] ingest + compile | Workflow AI 결과의 질문·계획은 정하기·추가 요청에서만 받는다 (raw/notes/2026-09-26-workflow-result-fields-by-kind.md)

첫 실제 처리에서 AI가 테스트 결과 처리 결과의 계획 칸에 완료 문장을 넣어 승인 없는 계획 절이 생긴 결함과, 질문·새 계획을 정하기·추가 요청 결과에서만 받도록 한 서버 수정, 완료 목록의 AI 상태 배지 제거를 wiki-workflow에 반영했다.

## [2026-09-26] lint | local command: 0 critical, 0 warnings, 0 suggestions, 0 auto-fixed

## [2026-09-26] ingest + compile | Workflow 상태는 질문·계획·체크리스트에서만 정한다 (raw/notes/2026-09-26-workflow-state-from-record-only.md)

사용자 승인으로 AI가 사람에게 넘기는 것을 질문·구현 계획·체크리스트로 한정하고, 단계별 AI 결과 칸 제한·체크리스트 전부 통과 시 즉시 완료·완료 뒤 정리 분리·미실행 항목의 사람 넘김·기록 충돌 규칙을 wiki-workflow에 반영했다.

## [2026-09-26] lint | local command: 0 critical, 0 warnings, 0 suggestions, 0 auto-fixed

## [2026-09-26] ingest + compile | Workflow 기록 작성 규칙 링크와 tasks 안내 문서 삭제 (raw/notes/2026-09-26-workflow-tasks-guide-removed.md)

사용자 요청으로 대시보드의 기록 작성 규칙 링크와 기록 폴더 안내 문서 tasks/index.md를 없앤 내용을 wiki-workflow에 반영했다.

## [2026-09-26] lint | local command: 0 critical, 0 warnings, 0 suggestions, 0 auto-fixed

## [2026-09-26] ingest + compile | IWxUIData 제거 후 사람의 표시 확인 범위

작업 기록의 테스트 결과를 raw/notes/2026-09-26-ui-data-display-acceptance.md로 수집하고 ui 기사에 코드 리뷰·HUD·버프·보스·이름표 표시 확인 범위를 반영했다. 원격 복제 후 재매칭은 별도 미해결로 유지한다. 과거 raw는 수정하지 않았으며 게임 코드·에셋·작업 기록·접수 JSON·작업 상태를 변경하지 않았다.

## [2026-09-26] ingest + compile | GA_ 유지 결정과 사람 플레이 확인 범위

작업 기록의 최종 결정과 사람 테스트 7개 통과 범위를 raw/notes/2026-09-26-ability-ga-play-acceptance.md로 수집하고 combat-abilities 기사와 색인에 반영했다. 과거 원자료는 보존했고 GA_ 복귀의 플레이 미확인 설명만 갱신했다. 후속 변경·네트워크 검증으로 확대하지 않았으며 게임 코드·에셋·작업 기록·접수 JSON·작업 상태를 변경하지 않았다.


## [2026-09-26] ingest + compile | DataTable Row 미리보기 사람 확인 범위

접수 해시와 일치하는 작업 기록의 사람 테스트 3개 통과를 raw/notes/2026-09-26-row-preview-acceptance.md로 수집하고 editor-tools에 빈 하위 구조체 축약·설정값 보존·셀과 툴팁 일치의 확인 범위를 반영했다. 제출 전 제거된 회귀 테스트 성공은 과거 검증 이력으로 구분했다. 원자료 색인과 루트 통계를 갱신했으며 기존 원자료·게임 코드·에셋·작업 기록·접수 JSON·작업 상태는 수정하지 않았다.

## [2026-09-26] 검증 | DataTable Row 미리보기 Wiki 정리

순정 llm-wiki lint --local --json은 critical·warning·suggestion 0, pass였다. git diff --check -- .wiki는 공백 오류 없이 통과했다. 새 원자료와 editor-tools의 로컬 링크 30개 및 UTF-8 읽기를 확인했고 작업 기록 SHA-256은 접수 해시와 동일했다. Export-Wiki.ps1은 Saved/Wiki의 Wiki·Workflow 뷰어를 각각 64개 문서로 갱신했다. 빌드·자동화·화면 검증 재실행은 하지 않았다.

## [2026-09-26] ingest + compile — Nameplate 로컬 표시의 사람 확인 범위

접수 해시와 일치하는 nameplate-manager 작업 기록에서 사람 테스트 6개 통과를 raw/notes/2026-09-26-nameplate-play-acceptance.md로 수집했다. ui의 인게임 미검증 설명을 해당 범위에서 갱신하고 락온 대상 거리 예외를 유지했다. 원자료 색인과 루트 통계를 갱신했다. 과거 raw·게임 코드·에셋·작업 기록·접수 JSON·작업 상태는 수정하지 않았다.

## [2026-09-26] 검증 — Nameplate Wiki 정리

순정 llm-wiki lint --local --json은 critical·warning·suggestion 0, pass였다. git diff --check -- .wiki는 종료 코드 0으로 공백 검사를 통과했다. 새 원자료와 ui의 로컬 링크 45개 및 UTF-8 읽기를 확인했다. 작업 기록 SHA-256은 접수 해시와 동일했다. Export-Wiki.ps1은 Saved/Wiki의 Wiki·Workflow 뷰어를 각각 64개 문서로 갱신했다. 빌드·게임·멀티플레이 재실행은 하지 않았다.

## [2026-09-26] ingest + compile | Workflow 작업 절차 도식 교체와 서버 동작 점검 (raw/notes/2026-09-26-workflow-process-diagram.md)

사용자 요청으로 바꾼 작업 절차 도식(단계 상자 배치)과 서버·화면 코드 대조 결과를 raw/notes/2026-09-26-workflow-process-diagram.md로 수집했다. references/wiki-workflow의 판단과 실행·웹 새 작업·테스트 체크리스트 절과 출처에 편찬했다. 명확한 지시를 승인으로 보는 지름길이 AI 대화에서만 동작한다는 점과 미결 두 가지(정하기 중 추가 요청의 권한, 완료 직후 추가 요청 재활성)를 적었다. raw 색인 총계를 실제 원자료 수 105로 맞췄다.

## [2026-09-26] 검증 | Workflow 작업 절차 도식 교체 Wiki 정리

순정 llm-wiki lint --local은 critical·warning·suggestion 0, PASS였다. git diff --check -- .wiki는 공백 오류 없이 통과했다. 새 원자료와 wiki-workflow의 출처 링크를 확인했고, Export-Wiki 재생성 뒤 링크 검사와 TestWikiViewer를 다시 통과했다.


## [2026-09-26] ingest + compile — AnimNotify 라벨 사람 확인 범위

접수 해시와 일치하는 animnotify-labels 작업 기록의 사람 테스트 두 항목 통과를 raw/notes/2026-09-26-animnotify-label-acceptance.md로 수집하고 editor-tools에 17종 라벨 값 일치와 가독성의 확인 범위를 반영했다. 원자료 색인과 루트 통계를 갱신했다. 과거 raw와 기존 사용자 변경을 보존했으며 게임 코드·에셋·작업 기록·접수 JSON·작업 상태는 수정하지 않았다.

## [2026-09-26] 검증 — AnimNotify 라벨 Wiki 정리

순정 llm-wiki lint --local --json은 critical·warning·suggestion 0으로 pass였다. git diff --check -- .wiki에서 공백 오류가 없었고, 새 원자료와 editor-tools의 로컬 링크 33개 및 UTF-8 읽기를 확인했다. 작업 기록 SHA-256은 접수 해시와 동일했다. Export-Wiki.ps1은 Saved/Wiki의 Wiki·Workflow 뷰어를 각각 64개 문서로 갱신했다. 빌드·에디터 화면 테스트는 재실행하지 않았다.


## [2026-09-26] ingest + compile — 공용 쿨다운 GE 사람 확인 범위

접수 해시와 일치하는 cooldown-unification 작업 기록의 사람 테스트 4개 통과를 raw/notes/2026-09-26-cooldown-play-acceptance.md로 수집했다. combat-abilities에 회피 UI·소환물 쿨다운 무시·리슨 서버와 클라이언트 차례 회복 및 코드 리뷰 확인 범위를 반영하고, 과거 소환물·네트워크 미확인 설명을 해당 범위에서 갱신했다. 원자료 색인과 루트 통계를 갱신했다. 기존 raw·사용자 변경·게임 코드·에셋·작업 기록·접수 JSON·작업 상태는 변경하지 않았다.

## [2026-09-26] 검증 — 공용 쿨다운 GE Wiki 정리

순정 llm-wiki lint --local --json은 critical·warning·suggestion 0으로 pass였다. git diff --check -- .wiki에서 공백 오류가 없었고, 새 원자료와 combat-abilities의 로컬 링크 39개 및 UTF-8 읽기를 확인했다. 원자료는 107개로 색인 통계와 일치하며 작업 기록 SHA-256은 접수 해시와 동일했다. Export-Wiki.ps1은 Saved/Wiki의 Wiki·Workflow 뷰어를 각각 64개 문서로 갱신했다. 빌드·자동화·PIE 테스트를 재실행하지 않았다.

## [2026-09-26] ingest + compile — 상호작용 목록과 엘리베이터 사람 확인 범위

접수 해시와 일치하는 interaction-list-vm-simplification 작업 기록의 사람 테스트 6개 통과를 raw/notes/2026-09-26-interaction-list-play-acceptance.md로 수집했다. world에 목록·선택·범위 이탈과 리스폰·행 문구·버튼 잠금 및 코드 리뷰의 확인 범위를 반영했다. 현재 층 호출 잠금과 비활성 상태의 깨우기 예외를 구분하고, 과거 인게임 미검증 설명을 해당 범위에서 보완했다. 원자료·주제 색인과 루트 통계를 갱신했다. 과거 raw와 기존 사용자 변경을 보존하고 게임 코드·에셋·작업 기록·접수 JSON·작업 상태는 수정하지 않았다.

## [2026-09-26] 검증 — 상호작용 목록 Wiki 정리

순정 llm-wiki lint --local --json은 critical·warning·suggestion·info 0으로 pass였다. CheckWikiLinks.ps1은 66개 문서에서 오류 0이었고 git diff --check -- .wiki에서 공백 오류가 없었다. 새 원자료와 world의 UTF-8 및 로컬 링크를 확인했고 원자료 108개가 색인 통계와 일치했다. 작업 기록 SHA-256은 접수 해시와 동일했다. 빌드·BP 컴파일·게임·에셋 저장은 재실행하지 않았다.

Export-Wiki.ps1로 Saved/Wiki의 Wiki·Workflow 뷰어를 각각 64개 문서로 갱신했다. 루트 탐색에도 사람 확인 범위 링크를 추가하고 뷰어에 반영했다.

## [2026-09-26] ingest + compile | Workflow 구 AI 워크플로우 잔재 제거 (raw/notes/2026-09-26-workflow-legacy-removal.md)

사용자 요청으로 지운 옛 웹 경로 파일·테스트, 옛 절차 문서·4단계 그림 19개와 현재 코드에서 없앤 옛 결과 형식 표시·옛 대시보드 CSS를 raw/notes/2026-09-26-workflow-legacy-removal.md로 수집했다. references/wiki-workflow의 이전 구조·권한 문단과 출처에 편찬했다. Claude 권한 거부 알림이 처리 근거에 남는다는 현재 동작과, 잔재로 보지 않고 남긴 것(회귀 테스트, 절 없는 기록 처리, 워크플로우 문서 PNG 묶기)을 적었다.

## [2026-09-26] 검증 | Workflow 구 AI 워크플로우 잔재 제거 Wiki 정리

순정 llm-wiki lint --local은 critical·warning·suggestion 0, PASS였다. git diff --check -- .wiki는 공백 오류 없이 통과했다. Export-Wiki 재생성 뒤 링크 검사 오류 0건(61문서)과 TestWikiViewer·TestWikiSpaces를 다시 통과했다.

## [2026-09-26] ingest + compile | Workflow 문서 이미지 기능 제거 (raw/notes/2026-09-26-workflow-image-removal.md)

사용자 결정("지금 안쓰면 지웁시다")으로 없앤 문서 이미지 기능(PNG 묶기·Markdown 이미지 표시)과 옛 목차 제목 건너뛰기를 raw/notes/2026-09-26-workflow-image-removal.md로 수집했다. references/wiki-workflow의 파일과 화면·이전 구조 절과 출처에 편찬했다. 화면이 Markdown 이미지를 그리지 않으므로 그림은 Mermaid 도식으로 쓴다는 점을 적었다.

## [2026-09-26] 검증 | Workflow 문서 이미지 기능 제거 Wiki 정리

순정 llm-wiki lint --local은 critical·warning·suggestion 0, PASS였다. git diff --check는 공백 오류 없이 통과했다. Export-Wiki 재생성 뒤 링크 검사 오류 0건과 TestWikiViewer·TestWikiSpaces를 다시 통과했다.

## [2026-09-26] refresh | 18 articles checked, 1 updated, 0 flagged, 0 retracted
- 직전 refresh 커밋(`39f3629a4`) 이후 `bf6596012`까지 커밋 19건(병합 1건 포함)과 작업 트리를 추적했다. 결과는 raw/notes/2026-09-26-refresh-commit-trace.md에 수집했다.
- 이미 반영: IWxUIData 제거(`c9e2efec6`), 재진입·새 게임 수정과 콤보 배열(`64fcd9285`·`519f929fb`·`b6f1e9e8c`, module-review-contracts), Exclusive 차단(`ac6db7670`), AnimNotify 색상 분류(`1689d999f`·`4c9f3934e`), Workflow·모듈 리뷰·작업 기록(`11047815a`·`33733d1bb`·`81c5e03dc`)과 작업 트리의 구 워크플로우 잔재·문서 이미지 기능 제거.
- 반영 대상 아님: 작업 기록·테스트 소스만 바꾼 `d644364be`·`566fb1167`, 레벨 배치만 바꾼 `7da389b85`·`6eb5200de`와 병합 `ad0db6de0`, AGENTS.md 코딩 규칙 `264a71740`, 회의자료 `184a76fec`, 작업 트리의 checkpoint-savegame 기록 변경.
- 새로 반영: `6adb657b9` DefenseConstant 입력 제한 메타 제거를 combat-damage 계산 절에 반영했다(계산 보정은 코드와 대조). 생성 목록 세 개는 Export-AbilitySystemLists.ps1 재실행 결과 unchanged였다. 점검한 기사 18개의 verified를 오늘로 맞췄고 생성 목록은 스크립트 산출물이라 손대지 않았다.

## [2026-09-26] lint | local command: 0 critical, 0 warnings, 0 suggestions, 0 auto-fixed
