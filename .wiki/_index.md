# WX 프로젝트 Wiki

> WX의 게임 규칙·구현·제약과 검증 범위를 관리하는 팀 공유 LLM Wiki입니다.

Last updated: 2026-09-23

## Statistics

- Sources: 47 raw documents
- Articles: 17 compiled wiki articles
- Inventory records: 0 tracked items
- Datasets: 0 manifests
- Outputs: 0 generated artifacts
- Last compiled: 2026-09-23 (world의 엘리베이터/FText 규칙, ui·dialogue의 사망·대화 화면 주인 변경, combat-damage의 요청·결과 API 및 투사체 소비 계약 반영, editor-tools의 DataTableRowFixup 행 참조 갱신 반영, ui·game의 보스 표시 세 층 구조·UWxBattleSubsystem·재초기화 문제, editor-tools의 MVVM 소스 경로 도구 반영, refresh로 world·ui·inventory·editor-tools에 상호작용 목록 VM·아이템 VM 단일화·WxToolset 도구 반영)
- Last lint: 2026-09-23 (구조·출처·링크 검사; 게임 실행 검증과 별개)

## Quick Navigation

- [All Sources](raw/_index.md)
- [Concepts](wiki/concepts/_index.md)
- [Topics](wiki/topics/_index.md)
- [References](wiki/references/_index.md)
- [Outputs](output/_index.md)

## 프로젝트 지식

- [전투 · WxCombat](wiki/topics/combat.md)
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
