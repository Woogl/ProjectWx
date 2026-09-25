# WX 프로젝트 Wiki

> WX의 게임 규칙·구현·제약과 검증 범위를 관리하는 팀 공유 LLM Wiki입니다.

Last updated: 2026-09-25

## Statistics

- Sources: 82 raw documents
- Articles: 21 compiled wiki articles
- Inventory records: 0 tracked items
- Datasets: 0 manifests
- Outputs: 0 generated artifacts
- Last compiled: 2026-09-25 (IWxUIData 제거·리졸버 연결 반영; 이전: ui의 위젯 속성 바인딩 Prevent, foundation의 기본 충돌 복잡도 Simple as Complex·에디터 스케일러빌리티 High 기본값 반영; 이전: combat-abilities의 차례 회복을 ApplyCooldown SetByCaller로 정정(대기열 MMC의 4초 결함); 이전: 체크포인트 사용자 확인 범위·남은 검증과 디스크 복원 범위를 world·game에 반영; 이전: combat-abilities의 공용 쿨다운 GE·CooldownTags·차례 회복 MMC·순정 쿨다운 태그 규칙 반영; 이전: 생성 목록의 빈 칸 의미·C++ 생성자 링크·노티파이 값 표기 정정과 GE 폐기 필드 제외, config에 생성물 규칙 명시; 이전: 에셋과 C++ 소스에서 생성하는 references/ability-list·effect-list·character-list 추가, ExportAbilitySystemLists.bat과 OpenWiki.bat이 갱신; 이전: refresh로 커밋 추적 후 어빌리티 데이터의 GA_ 에셋 배치, DT_Ability·DT_Effect 제거, 몽타주 섹션 모델, 처형 한 벌 통합, 소비 아이템 선택, 몽타주 섹션 편집 도구를 combat-abilities 등 12개 기사에 반영; 이전: AnimNotify 17종의 짧은 표시 규칙 반영; 이전: editor-tools의 Row 미리보기 기본 구조체 축약 반영; 이전: refresh로 커밋 추적 후 world의 복원 판정 이동·InitialState 제약·장치 트리의 다른 도메인 태스크·연출 태스크, combat·modules의 몽타주 1회 재생 태스크 WxCombat 이관 반영; 이전: ai·combat·combat-groggy의 AI 트리 정지·잠금 주체를 AWxAIController 단독으로 반영; 이전: foundation·world·combat-finisher의 상호작용 계약을 선택지 하나로 통합 반영; 이전: refresh로 원자료 해시 대조 후 ui의 일시정지 해제 규칙·Effect VM 월드 타이머, combat의 소환 상한·Master.* 주인 태그 반영; 이전: game의 래그돌 PhysicsAsset 통일 반영; 이전: combat-abilities의 쿨다운·코스트 무시 구조·AbilitySet SetByCaller 제약·발동 실패 로그 범위, combat의 락온 대상 소유·사망 시 BT 정지 주체 반영; 이전: foundation의 순수 정의 원칙·FDefaultModuleImpl 등록, combat-damage의 Damage.Guarded 제거 반영; 이전: refresh로 원자료 해시 대조 후 ui·game·combat의 NameplateManager·LockOnTargetQuery·State.LockedOn 제거, combat-abilities의 몽타주 구간 상태 GE 자기 핸들 제거, combat-finisher의 처형 피해 어빌리티 직접 적용, combat-damage의 퍼펙트 가드 Cue 통합, world의 루트 에셋 상태 태그만 발행 반영; 이전: ai의 이동 속도 SPD 소유·도플갱어 미러링(Override GE 속도 추종)·미니언 반응 노드 삭제, dialogue·quests·ui·editor-tools의 대화·퀘스트 화면 클래스 제거와 리졸버 연결·VM 명령 델리게이트·이벤트 목적지 도구 일반화, combat-damage의 0 피해 히트스톱 조건·Hit Cue 서버 발행·무적 범위, combat의 삭제된 Hit GE 표현 정정, world의 엘리베이터/FText 규칙, ui·dialogue의 사망·대화 화면 주인 변경, combat-damage의 요청·결과 API 및 투사체 소비 계약 반영, editor-tools의 DataTableRowFixup 행 참조 갱신 반영, ui·game의 보스 표시 세 층 구조·UWxBattleSubsystem·재초기화 문제, editor-tools의 MVVM 소스 경로 도구 반영, refresh로 world·ui·inventory·editor-tools에 상호작용 목록 VM·아이템 VM 단일화·WxToolset 도구 반영)
- Last lint: 2026-09-25 (구조·출처·링크 검사; 게임 실행 검증과 별개)

## Quick Navigation

- 2026-09-25: `IWxUIData`를 제거하고 도메인 표시 데이터를 WxGame 리졸버에서 연결한다. [WxUI](wiki/topics/ui.md)와 [모듈 경계](wiki/topics/modules.md)에 역할·수명을 반영했다.

- 2026-09-25: 프로젝트 기본 설정 3건을 반영했다. 위젯 속성 바인딩 금지는 [WxUI](wiki/topics/ui.md), 기본 충돌 복잡도와 에디터 스케일러빌리티는 [WxCore](wiki/topics/foundation.md)에 있다.
- 2026-09-25: Workflow 테스트 결과의 AI 선택·변경 재시도·실행 권한 경계를 [도구 구조](wiki/references/wiki-workflow.md)에 반영했다.
- 2026-09-25: [WxWorld](wiki/topics/world.md)에 체크포인트의 사용자 확인 범위와 남은 검증을 구분하고, [WxGame](wiki/topics/game.md)의 체크포인트 디스크 복원 범위를 정정했다. 승인 정본은 Workflow에 유지한다.
- 2026-09-25: 기존 작업의 테스트 결과 접수·AI 수정/정리·재확인·복구 경로를 [도구 구조](wiki/references/wiki-workflow.md)에 반영했다.
- 2026-09-25: Workflow 작업 기록의 분류·확인 범위·다음 행동과 갱신 절차를 [도구 구조](wiki/references/wiki-workflow.md)에 반영했다.

- [All Sources](raw/_index.md)
- [Concepts](wiki/concepts/_index.md)
- [Topics](wiki/topics/_index.md)
- [References](wiki/references/_index.md)
- [Outputs](output/_index.md)

## 프로젝트 지식

- [모듈 경계와 배치 원칙](wiki/topics/modules.md)
- [전투 · WxCombat](wiki/topics/combat.md)
- [어빌리티 목록 · GA_·몽타주](wiki/references/ability-list.md)
- [이펙트 목록 · GE_·피해 행·에셋 사용처](wiki/references/effect-list.md)
- [캐릭터 목록 · WxAbilitySet·속성 초기값](wiki/references/character-list.md)
- [AI · WxAI](wiki/topics/ai.md)
- [공용 기반 · WxCore](wiki/topics/foundation.md)
- [대화 · WxDialogue](wiki/topics/dialogue.md)
- [인벤토리 · WxInventory](wiki/topics/inventory.md)
- [퀘스트 · WxQuest](wiki/topics/quests.md)
- [UI · WxUI](wiki/topics/ui.md)
- [월드 · WxWorld](wiki/topics/world.md)
- [게임 조립 · WxGame](wiki/topics/game.md)
- [Wiki·Workflow 도구 구조](wiki/references/wiki-workflow.md)
- [편집기 도구 · WxEditor·WxToolset·DataTableRowFixup·BoxComponentVisualizer](wiki/references/editor-tools.md)

기존 16개 기사를 현재 근거로 재편찬하고 편집기 도구 설명을 추가했습니다. 신규 정적 조사 원자료 15개와 기존 Workflow 결정 원자료 1개를 사용합니다. 기준 HEAD는 `fe8c943f49401326e1007fedd78a937c9e66db47`이며 미커밋 작업 트리를 포함한 파일별 해시를 출처에 기록했습니다. 문서 날짜(`verified` 포함)는 편찬 기준이며 빌드·게임 실행·바이너리 에셋 검증일이 아닙니다.

## 운영

- [Wiki 설정](config.md)
- [주제 가이드](schema.md)
- [Wiki 운영](wiki/references/wiki-operation.md)
- [Workflow](../.agents/workflow/index.md)
- [작업 로그](log.md)
