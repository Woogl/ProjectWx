---
type: meta
title: Wiki Log
status: evergreen
created: 2026-09-26
updated: 2026-09-29
tags:
  - meta
  - log
---

# Wiki Log

Newest completed operations appear first.

## 2026-09-29 · ingest-20260929-task-records-b1

새로 완료된 작업 기록(1/2) 6건(31,299바이트)을 수집했다: [[작업 - workflow-codex-mcp-connection]], [[작업 - workflow-codex-terminal-no-daemon]], [[작업 - workflow-remove-redundant-buttons]], [[작업 - workflow-table-error-recurrence]], [[작업 - 지침-SSOT-점검]], [[작업 - wxcombat-review-quick-fixes]]. [[작업 절차(Workflow)]]에 MCP 연결 거부 서버 제외, Codex `--no-daemon` 이어하기, 중복 버튼 제거, 표 없는 질문 절 허용·표 오류 표시, SSOT 점검과 사람 리뷰를 AI 검증으로 대신한 완료 지시를, [[Wiki 운영]]에 claude-obsidian 버전 단일 정본과 sparse 사본의 작업 절차 정본 포함을, [[어빌리티와 GAS]]·[[피해 파이프라인]]·[[모듈 구조와 코드 정리]]에 WxCombat 리뷰 지적 1·3 수정과 지적 2 보류를 더했다. 새 원자료 재확인 기한은 2027-03-28이다. 기획서 30개 파일과 기존 완료 작업 기록 35건은 원장과 해시가 같았고, 30일 안에 재확인 기한이 오는 활성 원자료는 없었다. 남은 새 완료 기록 4건은 다음 묶음에서 수집한다. 게임 빌드·실행 재검증은 하지 않았다.

## 2026-09-28 · ingest-20260928-task-records-b2

완료 작업 기록 7건(152,258바이트)을 한 묶음으로 수집했다: [[작업 - damage-pipeline-structure-review]], [[작업 - exclusive-tag-blocking]], [[작업 - item-viewmodel-unification]], [[작업 - reference-tab-removal]], [[작업 - remove-get-ability-block-tags]], [[작업 - workflow-recheck]], [[작업 - wxgame-review-fixes]]. [[어빌리티와 GAS]]에 GetAbilityBlockTags·ActivationGroup 삭제와 Ability.Action 태그 계층, 생성자 차단·취소 선언, 끼어들기 판정 규칙, 콤보 진행 공통화·재생 속도·회피 방향 정리를 더하고, 2026-09-25 공통 차단 계산 줄에 날짜와 대체 안내를 붙였다. [[피해 파이프라인]]·[[UI 표시 구조]]·[[아이템과 회복]]·[[모듈 구조와 코드 정리]]·[[체크포인트와 리스폰]]·[[적 AI와 몬스터]]에 2026-09-27~28 헤드리스·사람 확인과 남은 경고·과제를, [[작업 절차(Workflow)]]에 워크플로우 재점검 결정과 리뷰·참고 탭 제거를 더했다. 대체된 주장 2건(clm-d9912a3acf-c2, clm-f1cb443cff-c4)을 deprecated로 보존하고 해당 결정 노트 2장에 후속 반영 안내를 붙였다(두 페이지의 재확인 기한 표기를 원장 값 2027-09-26으로 맞춤). 새 원자료 재확인 기한은 2027-03-28이다. 기획서 30개 파일과 나머지 완료 작업 기록 28건은 원장과 해시가 같았고, 30일 안에 재확인 기한이 오는 활성 원자료는 없었다. 게임 빌드·실행 재검증은 하지 않았다.

## 2026-09-28 · ingest-20260928-abilities-b1-v3

완료 기록 첫 묶음 3건(44,173바이트)을 수집했다: [[작업 - ability-directional-section]], [[작업 - ability-resolver-to-wxui]], [[작업 - combo-stage-desync-after-rejection]]. [[어빌리티와 GAS]]의 번호 섹션 모델을 후속 배열 결정으로 대체하고 콤보 단계 이벤트 동기화·검증 한계를 추가했다. [[UI 표시 구조]]에 리졸버 WxGame 복귀를 연결했다. 이전 원자료 요약 2장에 이력 안내를 붙이고 대체된 주장 2건을 deprecated로 보존했다. 기한 재확인 대상은 없었다. 미수집 완료 기록 7건은 다음 묶음으로 남는다. 게임 재실행 검증은 하지 않았다.

## 2026-09-27 · ingest-20260927T2200Z-task-records

새로 완료된 작업 기록 6건을 수집했다: [[작업 - 이동속도의-어트리뷰트화]], [[작업 - checkpoint-savegame]], [[작업 - spawner-library-removal]], [[작업 - headless-ai-testing]], [[작업 - workflow-wrapup-checks]], [[작업 - comment-cleanup-routine]]. [[캐릭터 스탯과 전투 자원]]과 [[적 AI와 몬스터]]에 이동 속도 SPD(배율) → MOV(cm/s) 전환을 더하고, 기획서 캐릭터 스탯 명세서의 SPD 배율 정의와 어긋남을 미결정·충돌에 적었다. [[체크포인트와 리스폰]]에 2026-09-27 헤드리스 확인(사망·부활·재시작·실패 처리·스포너 재생성)과 부활 위치 컴포넌트 참조 결함 수정을 더하고, 옛 「인게임 미검증」 문장에 노트 시점 날짜를 붙였다. [[작업 절차(Workflow)]]에 헤드리스 테스트 결정(AI가 끝내지 못한 AI 항목은 사람 항목이 아니라 AI 실패), 워크플로우 개선 마무리 확인 통과, 일일 주석 정리 Routine 수정을 더하고, 해소된 「workflow-wrapup-checks 미수집」 미결 줄을 검증 범위로 옮겼다. [[Wiki 운영]]에 대시보드 Wiki 갱신·정기 갱신 사람 확인 통과를 더했다. 새 원자료의 재확인 기한은 다른 완료 작업 기록과 같은 2027-03-25로 정했다. 기획서 30개 파일과 나머지 완료 작업 기록 19건은 원장과 해시가 같았고, 30일 안에 재확인 기한이 오는 활성 원자료는 없었다. 기존 주장의 대체·폐기는 없었다. 게임 빌드·실행 재검증은 하지 않았다.

## 2026-09-26 · ingest-20260926T175000Z-workflow-records

새로 완료된 작업 기록 5건을 수집했다: [[작업 - wiki-claude-obsidian-migration]], [[작업 - workflow-inspection]], [[작업 - dashboard-work-tab-split]], [[작업 - workflow-final-fixes]], [[작업 - harness-legacy-cleanup]]. 모두 워크플로우·Wiki 운영 기록이라 [[작업 절차(Workflow)]]와 [[Wiki 운영]]에 날짜·출처를 붙인 결정 이력으로 더했고, 지금 지킬 규칙은 정본(`.agents/workflow/process/index.md`, `Wiki/README.md`)을 가리켰다. 종합 점검 Q7(정하기 단계 추가 요청은 읽기 전용)이 옛 주장 clm-bda6487128-c2(추가 요청은 모든 명령 허용)를 대체해, 옛 주장을 deprecated로 바꿨다. claude-obsidian 전환 기록(D9·D11)에 따라 [[overview]]와 [[index]]의 「Routine만 쓴다」를 「Wiki 갱신 두 갈래만 쓴다」로 고쳤다.

[[작업 - animnotify-labels]]는 원본이 바뀌어(빌드 로그 링크를 경로 글자로 바꾼 것뿐) 새 사본으로 대체하고 주장 근거만 옮겼다. 새 원자료의 재확인 기한은 완료 작업 기록과 같은 2027-03-25로 정했다. 기획서 30개 파일과 나머지 완료 작업 기록 13건은 원장과 해시가 같았고, 30일 안에 재확인 기한이 오는 활성 원자료는 없었다. 게임 빌드·실행 재검증은 하지 않았다.

## 2026-09-26 · ingest-20260926-task-record-cleanup

완료 작업 기록 8건의 원본이 2026-09-26 기록 정리로 바뀌어 새 사본으로 다시 수집하고 옛 원자료를 대체했다: [[작업 - ability-table-driven]], [[작업 - animnotify-labels]], [[작업 - datatable-row-preview]], [[작업 - dialogue-presentation-vm]], [[작업 - interaction-list-vm-simplification]], [[작업 - nameplate-manager]], [[작업 - ui-data-interface-removal]], [[작업 - workflow-review]]. 옛 상태 줄 제거·중복 요청 병합 같은 정리뿐이라 주장 근거만 새 사본으로 옮겼다. 지식이 바뀐 것은 workflow-review 한 건으로, 작업 절차 도식 점검의 남은 미결 두 가지가 워크플로우 종합 점검의 Q7·Q8로 정해졌다는 후속 줄이다. 이에 따라 [[작업 절차(Workflow)]]의 두 미결 항목을 확정 결정으로 옮기고 주장 하나(clm-d1d69945aa-c5)를 더했다. 새 원자료의 재확인 기한은 각 옛 원자료의 기한(2027-03-25)을 이었다. 기획서 30개 파일과 나머지 완료 작업 기록 6건은 원장과 해시가 같았고, 30일 안에 재확인 기한이 오는 활성 원자료는 없었다. 게임 빌드·실행 재검증은 하지 않았다.

## 2026-09-26 · ingest-20260926-freshness-review

기획서 30개 파일(중복 사본 포함 29개 원자료)과 완료 작업 기록 14개의 SHA-256을 원장·manifest와 대조했으며 새 수집 대상은 없었다. 옛 결정 노트 66건은 캡처 사본의 해시를 확인했다. 30일 안에 재확인 기한이 오는 활성 원자료 109건 전부를 한 트랜잭션에서 재확인했다. retrieved_at·ingested_at, 주장과 원자료 본문은 보존했다.

재확인 기한은 자료 성격에 따라 정했다. 변경 중인 전투·캐릭터·적·퀘스트 요구/초안 10건은 2026-11-25, 개별 시스템·도구 규격 14건은 2026-12-25, 초기 기획·역기획 5건과 완료 작업 기록 14건은 2027-03-25, 고정된 과거 결정 노트 66건은 2027-09-26이다. 저장소 원자료 변경은 다음 갱신에서도 해시로 대조한다. 게임 빌드·실행 재검증은 하지 않았다.

## 2026-09-26 · ingest-20260926-task-link-refresh

완료 작업 기록 3건의 원본이 바뀌어 원자료를 새로 수집하고 옛 사본을 대체했다: [[작업 - animnotify-labels]], [[작업 - nameplate-manager]], [[작업 - workflow-review]]. 옛 LLM Wiki(`.wiki`) 제거에 따라 링크를 텍스트로 바꾼 변경뿐이라 주제 페이지는 고치지 않았다.

## 2026-09-26 · ingest-20260926-overview

첫 전체 수집(기획서 29건, 완료 작업 기록 14건, 옛 Wiki 결정 노트 66건)을 마치고 vault 개요를 한국어로 채웠다.

## 2026-09-26 · ingest-20260926T055111Z-b11

옛 Wiki 결정 노트(6/6) 11건 수집. 원자료: [[결정 노트 - 2026-09-26-workflow-human-verification]], [[결정 노트 - 2026-09-26-workflow-image-removal]], [[결정 노트 - 2026-09-26-workflow-korean-record-names]], [[결정 노트 - 2026-09-26-workflow-legacy-removal]], [[결정 노트 - 2026-09-26-workflow-process-diagram]], [[결정 노트 - 2026-09-26-workflow-result-fields-by-kind]], [[결정 노트 - 2026-09-26-workflow-row-actions-final]], [[결정 노트 - 2026-09-26-workflow-ssot-copies]], [[결정 노트 - 2026-09-26-workflow-state-from-record-only]], [[결정 노트 - 2026-09-26-workflow-tasks-guide-removed]], [[결정 노트 - 2026-09-26-workflow-web-tasks]]. 갱신한 주제: [[Wiki 운영]], [[작업 절차(Workflow)]].

## 2026-09-26 · ingest-20260926T055107Z-b10

옛 Wiki 결정 노트(5/6) 11건 수집. 원자료: [[결정 노트 - 2026-09-25-workflow-task-records]], [[결정 노트 - 2026-09-25-workflow-test-feedback]], [[결정 노트 - 2026-09-26-ability-ga-play-acceptance]], [[결정 노트 - 2026-09-26-animnotify-label-acceptance]], [[결정 노트 - 2026-09-26-cooldown-play-acceptance]], [[결정 노트 - 2026-09-26-interaction-list-play-acceptance]], [[결정 노트 - 2026-09-26-nameplate-play-acceptance]], [[결정 노트 - 2026-09-26-refresh-commit-trace]], [[결정 노트 - 2026-09-26-row-preview-acceptance]], [[결정 노트 - 2026-09-26-ui-data-display-acceptance]], [[결정 노트 - 2026-09-26-workflow-dashboard-row-actions]]. 갱신한 주제: [[UI 표시 구조]], [[Wiki 운영]], [[상호작용과 장치]], [[어빌리티와 GAS]], [[에디터 도구]], [[작업 절차(Workflow)]], [[피해 파이프라인]].

## 2026-09-26 · ingest-20260926T055103Z-b09

옛 Wiki 결정 노트(4/6) 11건 수집. 원자료: [[결정 노트 - 2026-09-25-checkpoint-single-slot]], [[결정 노트 - 2026-09-25-cooldown-single-ge]], [[결정 노트 - 2026-09-25-exclusive-submission-cleanup]], [[결정 노트 - 2026-09-25-exclusive-tag-blocking]], [[결정 노트 - 2026-09-25-row-preview-empty-struct]], [[결정 노트 - 2026-09-25-row-preview-tooltip]], [[결정 노트 - 2026-09-25-save-checkpoint-rename]], [[결정 노트 - 2026-09-25-spawner-library-removal]], [[결정 노트 - 2026-09-25-ui-data-interface-removal]], [[결정 노트 - 2026-09-25-workflow-convenience-review]], [[결정 노트 - 2026-09-25-workflow-simplification]]. 갱신한 주제: [[UI 표시 구조]], [[Wiki 운영]], [[모듈 구조와 코드 정리]], [[어빌리티와 GAS]], [[에디터 도구]], [[작업 절차(Workflow)]], [[적 AI와 몬스터]], [[체크포인트와 리스폰]].

## 2026-09-26 · ingest-20260926T055059Z-b08

옛 Wiki 결정 노트(3/6) 11건 수집. 원자료: [[결정 노트 - 2026-09-24-module-principles]], [[결정 노트 - 2026-09-24-nameplate-manager-wxgame]], [[결정 노트 - 2026-09-24-nameplate-manager]], [[결정 노트 - 2026-09-24-wxcombat-cleanup]], [[결정 노트 - 2026-09-24-wxcombat-machinery-cleanup]], [[결정 노트 - 2026-09-24-wxcore-cleanup]], [[결정 노트 - 2026-09-25-ability-block-policy-centralization]], [[결정 노트 - 2026-09-25-ability-data-on-ga]], [[결정 노트 - 2026-09-25-animnotify-labels]], [[결정 노트 - 2026-09-25-checkpoint-redirect-cleanup]], [[결정 노트 - 2026-09-25-checkpoint-savegame]]. 갱신한 주제: [[UI 표시 구조]], [[그로기·경직·피니시]], [[모듈 구조와 코드 정리]], [[상호작용과 장치]], [[어빌리티와 GAS]], [[에디터 도구]], [[적 AI와 몬스터]], [[체크포인트와 리스폰]], [[플레이어 캐릭터와 조작]], [[피해 파이프라인]].

## 2026-09-26 · ingest-20260926T055050Z-b07

옛 Wiki 결정 노트(2/6) 11건 수집. 원자료: [[결정 노트 - 2026-09-23-datatable-row-fixup]], [[결정 노트 - 2026-09-23-dialogue-presentation-vm]], [[결정 노트 - 2026-09-23-hit-processing-functions]], [[결정 노트 - 2026-09-23-interaction-list-vm]], [[결정 노트 - 2026-09-23-item-viewmodel-unification]], [[결정 노트 - 2026-09-23-player-screen-owner]], [[결정 노트 - 2026-09-23-screen-classes-to-resolvers]], [[결정 노트 - 2026-09-23-zero-damage-hitstop]], [[결정 노트 - 2026-09-24-ai-brain-control-single-owner]], [[결정 노트 - 2026-09-24-device-statetree-cleanup]], [[결정 노트 - 2026-09-24-interaction-contract-options-only]]. 갱신한 주제: [[UI 표시 구조]], [[그로기·경직·피니시]], [[모듈 구조와 코드 정리]], [[상호작용과 장치]], [[아이템과 회복]], [[어빌리티와 GAS]], [[에디터 도구]], [[적 AI와 몬스터]], [[퀘스트와 대화]], [[피해 파이프라인]].

## 2026-09-26 · ingest-20260926T055046Z-b06

옛 Wiki 결정 노트(1/6) 11건 수집. 원자료: [[결정 노트 - 2026-09-22-current-workflow]], [[결정 노트 - 2026-09-22-verified-stock-rule]], [[결정 노트 - 2026-09-22-workflow-closure]], [[결정 노트 - 2026-09-22-workflow-dashboard]], [[결정 노트 - 2026-09-22-workflow-review]], [[결정 노트 - 2026-09-23-ability-resolver-module]], [[결정 노트 - 2026-09-23-apply-damage-unification]], [[결정 노트 - 2026-09-23-boss-battle-three-layer]], [[결정 노트 - 2026-09-23-damage-context-cleanup]], [[결정 노트 - 2026-09-23-damage-forward-flow]], [[결정 노트 - 2026-09-23-damage-four-arguments]]. 갱신한 주제: [[UI 표시 구조]], [[Wiki 운영]], [[모듈 구조와 코드 정리]], [[보스 전투]], [[어빌리티와 GAS]], [[에디터 도구]], [[작업 절차(Workflow)]], [[피해 파이프라인]].

## 2026-09-26 · ingest-20260926T055043Z-b05

완료 작업 기록(2/2) 8건 수집. 원자료: [[작업 - dialogue-presentation-vm]], [[작업 - interaction-list-vm-simplification]], [[작업 - nameplate-manager]], [[작업 - player-screen-classes-to-layout-component]], [[작업 - quest-presentation-vm]], [[작업 - ui-data-interface-removal]], [[작업 - wiki-regeneration]], [[작업 - workflow-review]]. 갱신한 주제: [[UI 표시 구조]], [[Wiki 운영]], [[그로기·경직·피니시]], [[모듈 구조와 코드 정리]], [[상호작용과 장치]], [[어빌리티와 GAS]], [[작업 절차(Workflow)]], [[적 AI와 몬스터]], [[체크포인트와 리스폰]], [[캐릭터 스탯과 전투 자원]], [[퀘스트와 대화]].

## 2026-09-26 · ingest-20260926T055030Z-b02

기획서(그로기·적·보스) 10건 수집. 원자료: [[기획서 - 그로기_시스템]], [[기획서 - 그로기_피니시_시스템_기획서]], [[기획서 - 경직 수정본]], [[기획서 - 뒤잡_시스템]], [[기획서 - 적 규격서]], [[기획서 - 명조 적 시스템 역기획서]], [[기획서 - Elite_monster]], [[기획서 - normal_monster_BT]], [[기획서 - boss_common_bt_draft]], [[기획서 - WX_첫_보스_기획서]]. 갱신한 주제: [[게임 개요와 전투 방향]], [[그로기·경직·피니시]], [[레벨 디자인]], [[보스 전투]], [[상호작용과 장치]], [[어빌리티와 GAS]], [[적 AI와 몬스터]], [[캐릭터 스탯과 전투 자원]], [[피해 파이프라인]].

## 2026-09-26 · ingest-20260926T055017Z-b03

기획서(UI·AI·레벨) 9건 수집. 원자료: [[기획서 - Nameplate_System]], [[기획서 - Patrol_Design]], [[기획서 - Respawn_System]], [[기획서 - WX_AM_기능정리]], [[기획서 - WX_BT_기능정리]], [[기획서 - 레벨_배치용_잡몹]], [[기획서 - Object_Design]], [[기획서 - Wx_Quest]], [[기획서 - 레버_증기_사양서]]. 갱신한 주제: [[UI 표시 구조]], [[레벨 디자인]], [[보스 전투]], [[상호작용과 장치]], [[어빌리티와 GAS]], [[적 AI와 몬스터]], [[체크포인트와 리스폰]], [[퀘스트와 대화]], [[피해 파이프라인]].

## 2026-09-26 · ingest-20260926T055011Z-b01

기획서(전투 기본·PC) 10건 수집. 원자료: [[기획서 - game-concept]], [[기획서 - Core_Combat_System]], [[기획서 - 전투 방향 변경에  따른 시스템 수정 요구사항]], [[기획서 - PC규격서]], [[기획서 - WA_PC_규격서]], [[기획서 - WA_주인공_캐릭터]], [[기획서 - 명조 PC 시스템 역기획서]], [[기획서 - 캐릭터 스탯 명세서]], [[기획서 - 조작키]], [[기획서 - 에스트병 기획서]]. 갱신한 주제: [[게임 개요와 전투 방향]], [[그로기·경직·피니시]], [[보스 전투]], [[상호작용과 장치]], [[아이템과 회복]], [[어빌리티와 GAS]], [[체크포인트와 리스폰]], [[캐릭터 스탯과 전투 자원]], [[플레이어 캐릭터와 조작]], [[피해 파이프라인]].

## 2026-09-26 · ingest-20260926-b04-linkfix

아직 수집되지 않은 결정 노트로 가는 링크를 표시가 남은 일반 텍스트로 바꿨다. 해당 노트가 수집되면 다음 갱신이 링크를 되살린다.

## 2026-09-26 · ingest-20260926T054832Z-b04

완료 작업 기록(1/2) 6건 수집. 원자료: [[작업 - ability-table-driven]], [[작업 - animnotify-categories]], [[작업 - animnotify-labels]], [[작업 - cooldown-unification]], [[작업 - datatable-row-preview]], [[작업 - datatable-row-rename-reference-update]]. 갱신한 주제: [[UI 표시 구조]], [[모듈 구조와 코드 정리]], [[어빌리티와 GAS]], [[에디터 도구]], [[적 AI와 몬스터]].
