# 체크포인트 SaveGame 전환

상태: 확인 대기 · 체크리스트 2/4 통과
다음 행동: 사람 확인: 재시작 뒤 슬롯 공유 (대기)

- 사용자 요청: UWxCheckpointSubsystem을 제거하고 SaveGame으로 처리.
- 이전 상태: 구현 및 Editor Development 빌드 검증 완료. 인게임 확인 대기.
- 설계·구현: UWxCheckpointSaveGame에 레벨 패키지·Transform을 직렬화한다. 기록 태스크·부활·새 게임 호출부를 전환하고 기존 서브시스템 파일을 삭제했다. 기존 Standalone/동일 맵 제한을 유지하며 사용자 후속 결정에 따라 PIE와 일반 플레이 모두 WxCheckpoint 슬롯을 사용한다. 세분화는 추후 진행한다.
- 실패 처리: 기록 실패는 StateTree Failed, 조회 불가 시 PlayerStart 부활, 새 게임 삭제 실패 시 이동 중단.
- 정적 검증: Source/Plugins/Config의 기존 클래스 참조 없음. git diff --check 통과. UHT 통과.

- 빌드: WxEditor Win64 Development 성공(종료 코드 0, 248.87초). 빌드 로그 `Saved/Logs/BuildDoctor/build_2026-09-25_180627_941_4324.log`. 변경 문서 상대 링크 검사와 Wiki 뷰어 생성도 통과했다.

- 후속 요청: RecordCheckpoint를 SaveCheckpoint로 변경. SaveGame API, StateTree 태스크·InstanceData·파일명·표시명을 변경하고 기존 StateTree 에셋을 위한 두 StructRedirects를 추가했다. WxEditor Win64 Development 재빌드 성공(종료 코드 0, 73.10초). 로그: Saved/Logs/BuildDoctor/build_2026-09-25_181539_326_29416.log. 기존 에셋의 런타임 로드는 미검증.

- 후속 결정: 저장 슬롯을 WxCheckpoint 하나로 통일. GetSlotName의 World 인자와 PIE 분기를 제거했다. 기존 WxCheckpoint_PIE 파일은 자동 이관·삭제하지 않는다.
- 단일 슬롯 변경 검증: WxEditor Win64 Development 빌드 성공(종료 코드 0, 47.10초). 로그: Saved/Logs/BuildDoctor/build_2026-09-25_183146_475_27060.log. 인게임 저장·조회는 미검증.

- 후속 리다이렉트 정리 완료: ST_CheckPoint 리세이브 후 Config의 두 StructRedirects 제거. 리다이렉트 없는 별도 엔진 프로세스의 재로드·StateTree 컴파일·저장 성공(종료 0). 로그: Saved/Logs/CheckpointResave.log, Saved/Logs/CheckpointWithoutRedirects.log. Content/Plugins 에셋에서 이전 구조체 이름 검색 결과 없음. 기존 BP_CheckPoint 사용자 변경은 보존했다.
- 체크리스트 정리(2026-09-27, 확인 대기 일감 정리 요청): '재시작 뒤 부활'은 없는 이어하기 흐름을 가리켰다. 프런트엔드에는 새 게임뿐이고 새 게임은 체크포인트를 지운다(`UWxGameFlowSubsystem::RequestNewGame` → `ResetCheckpoint`). PIE에서 저장한 뒤 Standalone으로 실행하는 확인이 새 프로세스에서 같은 슬롯을 읽어 재시작 뒤 부활까지 보므로 'PIE·일반 플레이 슬롯 공유'와 한 항목으로 합쳤다.


<!-- test-feedback:request-b19e39f0-456d-450f-8b76-526941908a5f:1 -->
## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 사망·부활·새 게임 | 게임: 체크포인트 저장 뒤 사망·부활, 새 게임 초기화 | 사람 | 통과 | 이우성 2026-09-25 |
| 재시작 뒤 슬롯 공유 | 에디터에서 체크포인트가 있는 맵을 직접 열고 PIE로 체크포인트에 저장한 뒤 PIE를 끝낸다. 같은 맵을 Standalone Game으로 실행해 사망·부활하면 저장한 체크포인트에서 부활한다. 프런트엔드의 새 게임은 체크포인트를 지우므로 거치지 않는다. | 사람 | 대기 |  |
| 저장·조회 실패 처리 | 자동화 테스트: 저장 실패 시 태스크 Failed, 조회 불가 시 PlayerStart 부활, 슬롯 삭제 실패 시 새 게임 이동 중단 | AI | 대기 |  |
| 코드 리뷰 | UWxCheckpointSaveGame·SaveCheckpoint 태스크·부활·새 게임 호출부 | 사람 | 통과 | 이우성 2026-09-25 |

## 사용자 테스트 결과 · 2026-09-25T10:46:18.347Z

- 확인자: 이우성
- 제출 결과: 이상 없음
- 제출 당시 코드 식별값: `e939b31ed877673e1a3d957abbdd3938208cb82762548711572da2ec83df1a31`
- AI 처리 상태: 확인 필요

확인한 항목:

> 저장 후 사망·부활, 재접속, 새 게임 초기화를 확인한다.

AI 처리 결과:

> 제출 버전의 확인 범위는 이상 없음으로 대조했고 Wiki 정리를 수행했습니다. 작업 중 외부 코드 변경과 남은 검증·리뷰가 있어 전체 완료는 보류합니다.

> 변경: 기존 world·game Wiki에 체크포인트 저장 범위, 사용자 확인 범위, 남은 검증을 구분해 반영했습니다.

> 변경: 검증 근거 원자료와 출처 링크·색인·통계·Wiki 로그를 갱신하고 뷰어를 재생성했습니다.

> 변경: 게임 코드·에셋, Task Markdown, tasks/index.md, 테스트 접수 JSON은 직접 수정하지 않았습니다.

> passed · Task와 최신 결정 대조: Get-FileHash 결과가 접수 SHA-256 f19c518ef423547f46ad136b59e38c8c5956ebcee28358754c9b7733e8463215와 시작·종료 시 일치했습니다. node .agents/scripts/Wiki-AI.cjs --current '체크포인트 SaveGame 전환' 결과 pending=null, 연결된 후속 변경 없음.

> passed · 사용자 테스트 수용 범위 대조: 이우성, 2026-09-25T10:46:18.347Z, passed: 저장 후 사망·부활, 재접속, 새 게임 초기화. 시작 시 codeVersion(root,true)가 제출 값 e939b31ed877673e1a3d957abbdd3938208cb82762548711572da2ec83df1a31과 일치했습니다. 사용자 보고 근거이며 AI 재실행 결과는 아닙니다.

> failed · 종료 시 코드 버전 일치: 마지막 codeVersion(root,true)는 0c24d1a3f796f80d4b99014892af4d1f271b65df7f1c5336ab74b99b16979990으로 제출 값과 다릅니다. 시작 시 확보한 파일 해시와 비교해 외부 Workflow 스크립트 변경을 감지했습니다.

> passed · 기존 실행 로그 대조: rg로 build_2026-09-25_183146_475_27060.log의 Result: Succeeded·47.10초를 확인했습니다. CheckpointResave.log와 CheckpointWithoutRedirects.log에서 ST_CheckPoint 컴파일·저장 성공 및 0 errors를 확인했습니다. 신규 실행은 아닙니다.

> passed · Wiki 반영과 순정 Lint: .wiki/config.md·schema.md와 wiki 스킬에 따라 기존 world.md·game.md 및 raw/notes/2026-09-25-checkpoint-validation-scope.md에 반영했습니다. 설치된 순정 llm-wiki lint --local --json 결과 critical·warning·suggestion 모두 0, 종료 코드 0.

> passed · 문서 링크와 형식: Node 검사로 변경 문서 6개의 상대 링크 170개 모두 존재함을 확인했습니다. 원자료 78개·기사 21개로 통계를 대조했고, 변경 Wiki 파일의 git diff --check가 통과했습니다.

> passed · 뷰어 생성: pwsh -NoProfile -File .agents/scripts/Export-Wiki.ps1 종료 코드 0. Saved/Wiki/knowledge.html과 index.html을 각각 59개 문서로 생성했습니다.

> passed · 기존 변경과 보호 대상 보존: 시작 시 해시를 확보한 기존 게임 코드·에셋과 Task·tasks/index.md·접수 JSON의 변경은 0건입니다. 외부 스크립트 변경은 보존했으며 커밋·푸시·외부 메시지·자동 백업을 수행하지 않았습니다.

> not_run · 신규 빌드·추가 플레이·사람 코드 리뷰: 이번 요청은 제출 결과 대조와 문서 정리 범위이므로 신규 빌드·플레이는 실행하지 않았습니다. 실패 경로·슬롯 공유의 제출 증거와 사람 코드 리뷰 수용이 없어 humanChecks에 남겼습니다.

> 남은 확인: 기존 SaveGame 전환·명칭 변경·단일 슬롯·리다이렉트 제거 변경에 대한 코드 리뷰 수용을 확인해야 합니다.

> 남은 확인: 저장 실패 시 StateTree Failed, 조회 불가 시 PlayerStart 부활, 삭제 실패 시 새 게임 이동 중단을 확인해야 합니다.

> 남은 확인: PIE와 일반 플레이 간 WxCheckpoint 슬롯 공유를 확인해야 합니다.

> 남은 확인: ‘재접속’에 완전 재시작 후 같은 맵 부활이 포함됐는지 범위만 확인하면 됩니다. 이미 수행했다면 재실행할 필요는 없습니다.

> 남은 확인: 동시 변경이 끝난 최신 버전에 기존 테스트 결과를 적용할 수 있는지 확인해야 합니다.

> 남은 확인: 작업 중 외부 Workflow 스크립트 변경으로 코드 식별값이 제출 값과 달라졌습니다. 기존 테스트 수용을 변경 후 버전에 적용할 수 없습니다.

> 남은 확인: 기존 구현에 대한 사람의 코드 리뷰 수용 기록이 없습니다.

> 남은 확인: 제출의 ‘재접속’이 프로세스 완전 종료·재시작 후 같은 맵 부활까지 포함하는지는 원문만으로 확인되지 않습니다.

> 접수 후 코드가 변경되었습니다. 변경 범위를 확인하고 다시 테스트하세요.


<!-- test-feedback:request-6c455897-e95e-4c9a-a5ed-98506beca8b0:1 -->
## 사용자 테스트 결과 · 2026-09-25T11:03:28.794Z

- 확인자: 이우성
- 제출 결과: 이상 없음
- 제출 당시 코드 식별값: `c26ac32f156ed92e094db0f7460d426dd41e944126457b28dee0cddd990d4638`
- AI 처리 상태: 확인 필요

확인한 항목:

> 저장 후 사망·부활, 재접속, 새 게임 초기화를 확인한다.

- 처리 AI: Codex

AI 처리 결과:

> 제출한 세 항목의 ‘이상 없음’을 확인했습니다. 기존 Wiki 정리는 이미 반영되어 있어 유지했습니다. 코드 버전 대조 제한과 남은 테스트·코드 리뷰로 전체 완료는 보류합니다.

> 변경: 신규 재사용 지식이 없어 Wiki 중복 기록을 생략했습니다.

> 변경: 파일 수정·삭제, 커밋·푸시·외부 메시지·자동 백업을 수행하지 않았습니다.

> passed · Task 및 최신 결정 대조: Get-FileHash -Algorithm SHA256 결과가 시작·종료 시 접수 값 0f9970aa26a111cdb855ebd027edddce22c4a866b9d2ea8f31b3997e3f326eed와 일치했습니다. node .agents/scripts/Wiki-AI.cjs --current '체크포인트 SaveGame 전환' 결과 pending=null, 연결된 후속 변경 없음.

> passed · 사용자 확인 범위 대조: 이우성의 2026-09-25T11:03:28.794Z 접수 결과는 passed이며, 범위는 저장 후 사망·부활, 재접속, 새 게임 초기화입니다. 메모는 비어 있습니다. 제출 버전 c26ac32f156ed92e094db0f7460d426dd41e944126457b28dee0cddd990d4638에 대한 사용자 보고이며 AI 재실행 결과는 아닙니다.

> not_run · 현재 코드 버전 및 Git 변경 대조: git status --short와 git rev-parse HEAD가 detected dubious ownership으로 거부되었습니다. 보호 설정을 변경하지 않아 제출 버전 일치 여부와 git diff --check는 확인하지 못했습니다.

> passed · 기존 실행 로그 대조: rg로 Saved/Logs/BuildDoctor/build_2026-09-25_183146_475_27060.log의 Result: Succeeded와 47.10초를 확인했습니다. CheckpointResave.log·CheckpointWithoutRedirects.log에서 ST_CheckPoint 컴파일·저장 성공 및 0 errors·1 warning을 확인했습니다. 신규 실행은 아닙니다.

> passed · Wiki 반영 생략 근거: wiki 스킬과 .wiki/config.md·schema.md에 따라 world.md·game.md 및 checkpoint-validation-scope 원자료를 대조했습니다. 저장 범위·실패 처리·단일 슬롯·리다이렉트 제거·남은 검증이 이미 반영되어 있습니다. 이번 제출은 같은 범위의 결과이므로 중복 편찬을 생략하고, 새 접수의 수용 기록은 Workflow 서버에 맡깁니다.

> passed · Wiki 링크·출처·색인: 읽기 전용 Node 검사로 관련 문서 3개의 상대 링크 58개와 출처 경로 23개가 모두 존재함을 확인했습니다. 원자료 79개·기사 21개는 통계와 일치하며, topics 10개·notes 79개는 각각 색인 행 수와 일치했습니다.

> not_run · 신규 빌드·플레이·순정 Lint·뷰어 생성: 이번 요청은 제출 결과 대조와 문서 정리이며 코드·에셋 변경이 없어 빌드·플레이를 재실행하지 않았습니다. Wiki 변경도 없어 순정 Lint와 뷰어 재생성을 생략했습니다. 추가 테스트와 사람 코드 리뷰는 humanChecks에 남겼습니다.

> 남은 확인: SaveGame 전환·SaveCheckpoint 명칭 변경·단일 슬롯·리다이렉트 제거에 대한 코드 리뷰 수용을 확인해야 합니다.

> 남은 확인: 저장 실패 시 StateTree Failed, 조회 불가 시 PlayerStart 부활, 삭제 실패 시 새 게임 이동 중단을 확인해야 합니다.

> 남은 확인: PIE와 일반 플레이 간 WxCheckpoint 슬롯 공유를 확인해야 합니다.

> 남은 확인: 제출한 ‘재접속’에 프로세스 완전 종료·재시작 후 같은 맵 부활이 포함됐는지만 확인해야 합니다. 이미 수행했다면 재실행은 필요 없습니다.

> 남은 확인: Git 저장소 소유권 보호로 제출 코드 버전과 현재 버전을 대조하지 못했습니다. safe.directory 설정이나 내부적으로 예외를 적용하는 codeVersion 함수는 실행하지 않았습니다.

> 남은 확인: 기존 구현에 대한 사람의 코드 리뷰 수용 기록이 없습니다.


## 사용자 테스트 결과 · 2026-09-25T17:34:37.620Z

<!-- test-feedback:request-e4608759-fa55-47a2-b5bf-ff9e6ef94281:submitted -->
- 전달한 사람: 이우성

> 통과 · 사망·부활·새 게임
> 통과 · 코드 리뷰


## 사용자 테스트 결과 · 2026-09-25T17:34:44.877Z

<!-- test-feedback:request-b38d4c19-1f56-4c43-a827-492d1b0324da:submitted -->
- 전달한 사람: 이우성

> 통과 · 코드 리뷰
