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
