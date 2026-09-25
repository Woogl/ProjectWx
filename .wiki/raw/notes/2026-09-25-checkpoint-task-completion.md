---
title: "체크포인트 저장 태스크의 실패와 상태 완료 판정"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, world, savegame, statetree]
summary: "SaveCheckpoint의 Failed 반환은 완료 판정 제외 설정 때문에 상태 실패로 집계되지 않는다. 프로젝트와 UE 5.8 엔진 소스의 정적 대조 근거다."
---

# 체크포인트 저장 태스크의 실패와 상태 완료 판정

기준은 HEAD `39f3629a4`의 WxWorld 코드와 로컬 UE 5.8 엔진 소스다. 모듈 리뷰 담당 서브에이전트가 해당 구현과 엔진 테스트를 읽어 대조했다. 게임 실행이나 저장 실패 재현 결과는 아니다. 수정 판단과 미해결 사항은 [WxWorld 리뷰](../../../.agents/workflow/tasks/module_review_WxWorld.md)에 둔다.

## 코드 근거

- [SaveCheckpoint 태스크](../../../Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SaveCheckpoint.cpp)의 17~20행은 `bConsideredForCompletion = false`, `bCanEditConsideredForCompletion = false`를 설정한다. 43~46행은 저장 함수가 실패하면 로그와 함께 `EStateTreeRunStatus::Failed`를 반환한다.
- 프로젝트 파일 SHA-256: `3F99AF643B69E097FEAF1987DAED822B96A0FAE42C9B0A62BC2D2A79D053B944`.
- UE 5.8 `Engine/Plugins/Runtime/StateTree/Source/StateTreeModule/Private/StateTreeExecutionContext.cpp`의 3852행에서 EnterState 결과를 받지만, 3873~3879행은 `IsConsideredForCompletion(StateTaskIndex)`인 태스크만 상태 결과에 합산한다. 제외된 태스크의 Failed는 이 실패 집계에 들어가지 않는다.
- 엔진 실행 파일 SHA-256: `1FA68034E3EC6F6820362855249AE43FEEE9FD403D2FA6272F1118BFBFB1CDF7`.
- UE 5.8 `Engine/Plugins/Runtime/StateTree/Source/StateTreeTestSuite/Private/StateTreeTaskStateTest.cpp` 954~955·973~975·1000~1001행은 완료에서 제외한 태스크가 진입 중 실패해도 Start 결과가 Running인 조건을 테스트한다. 이번 조사에서 그 테스트를 실행하지는 않았다.
- 엔진 테스트 파일 SHA-256: `98F5B3CAD90399B9BBE2A81CC4D81CB993194D5F21237B61910224DD870CC049`.

## 해석과 경계

현재 계약은 ‘저장 실패 시 태스크가 Failed를 반환한다’까지다. 이 반환이 상태 실패 전이 또는 사용자에게 보이는 실패 처리를 보장하지는 않는다. 다른 태스크와 전이의 에셋 구성에 따른 최종 상태는 따로 확인해야 한다.

기존 [사용자 검증 범위](2026-09-25-checkpoint-validation-scope.md)는 저장·조회·삭제 실패의 실행 검증을 포함하지 않았다. 이번 정적 확인은 당시 성공 경로 수용을 취소하거나 새 실행 검증으로 확대하지 않는다.
