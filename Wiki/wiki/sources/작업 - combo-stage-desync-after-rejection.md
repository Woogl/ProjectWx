---
type: source
title: "작업 - combo-stage-desync-after-rejection"
created: 2026-09-28
updated: 2026-09-28
status: developing
tags:
  - source
  - 작업-기록
summary: "입력 발동 이벤트 데이터에 콤보 단계를 실어 정상 완료·서버 거절 뒤 단계 불일치를 고친 작업과 반복 검증·잔여 입력 누락 기록"
source_type: task-record
source_id: src-290cbcdca11539b033a8
sha256: 6a3793cf9df3641d1190138dab622a1f3f82a43908d1ddc8cb0d83cbf5260244
authority: primary
independence_key: ".agents/workflow/tasks/combo-stage-desync-after-rejection.md"
review_state: active
refresh_due: 2026-12-28
original_paths:
  - ".agents/workflow/tasks/combo-stage-desync-after-rejection.md"
raw_copy: ".raw/captured/6a3793cf9df3641d1190138dab622a1f3f82a43908d1ddc8cb0d83cbf5260244.md"
claim_ids:
  - clm-6a3793cf9d-c1
  - clm-6a3793cf9d-c2
  - clm-6a3793cf9d-c3
key_claims:
  - "2026-09-29(KST) 승인된 구현은 입력 발동 측이 콤보 단계를 EventMagnitude로 보내고 서버가 ServerTryActivateAbilityWithEventData 경로에서 받은 단계를 사용하도록 하며, 거절 초기화 ResetCombo·NotifyAbilityFailed 안은 폐기했다."
  - "UWxAbility_Combo는 이벤트 데이터 단계가 범위 밖이면 첫 단을 사용하고, 이벤트 데이터 없는 AI·도플갱어 미러링·서버 단독 발동은 기존 단계 진행을 유지한다."
  - "작업 기록의 네트워크 테스트 27회 중 26회는 전 항목 통과했으며, 완료·거절 뒤 다음 입력의 단계 일치는 27회 모두 통과했지만 첫 입력 누락 1회의 원인은 미확인이다."
---

# 작업 - combo-stage-desync-after-rejection

- 원본: `.agents/workflow/tasks/combo-stage-desync-after-rejection.md`
- 원자료 사본: `.raw/captured/6a3793cf9df3641d1190138dab622a1f3f82a43908d1ddc8cb0d83cbf5260244.md`
- 수집: 2026-09-28 UTC · 재확인 기한: 2026-12-28

## 문제와 확정 결정

서버는 재발동을 위한 종료와 클라이언트가 먼저 끝낸 정상 완료를 모두 취소 아닌 원격 종료로 받아 콤보 단계를 남길 수 있었다. 거절만 초기화하는 안은 정상 완료 뒤 client L / server LLL 불일치를 덮지 못했다.

> 이우성 2026-09-29(KST): "네. 그 방법으로 합시다."

사용자는 클라이언트가 단계를 발동 이벤트 데이터로 보내고 서버가 따르는 안을 승인했다. 원자료의 한국 날짜는 그대로 보존하며 이 문서의 수집 날짜는 UTC 2026-09-28이다.

## 구현 관찰

ASC의 AbilityInputActionTriggered·TryActivateByInputAction은 private TryActivateInputAbility를 거친다. 콤보는 GetNextComboIndex() 결과를 EventMagnitude에 넣어 InternalTryActivateAbility를 부르고 엔진의 기존 이벤트 데이터 RPC를 탄다. 활성 인스턴스 재발동만 다음 단이며 그 밖은 첫 단이다. 비콤보는 기존 TryActivateAbility를 쓴다. 서버는 범위 검사만 하고 클라이언트 단계를 신뢰한다(PvE 전제).

## 검증 범위와 미해결

헤드리스 에디터 PIE 리슨 서버·원격 클라이언트에서 정상 콤보, 완료 뒤 입력, 서버만 첫 발동·재발동 거절을 반복했다. 수정 전 대조 실행은 client L / server LLL로 실패했고 수정 뒤 A2·B·C의 다음 입력은 27회 모두 양쪽 AM_Template_Attack_L이었다. 후딜 회피·호스트 재입력·선입력과 단독 PIE 도플갱어 미러링도 통과했다. 임시 테스트·Build.cs 의존 제거 후 Development 빌드가 성공했고 woogle의 코드 리뷰 통과가 기록돼 있다.

27회 중 1회는 C의 첫 입력이 클라이언트 발동·서버 RPC 없이 사라져 콤보 창을 보지 못했다. 추가 로그 뒤 15회 연속 재현되지 않았으나 원인은 확인하지 못했다. 콤보 단계 수정과 무관하다는 판단은 추정이며 해결됐다고 쓰지 않는다. 지연·패킷 손실·전용 서버 전체 검증으로 확대하지 않는다.

## 관련 주제

- [[어빌리티와 GAS]]

## 핵심 주장

- 2026-09-29(KST) 승인된 구현은 입력 발동 측이 콤보 단계를 EventMagnitude로 보내고 서버가 ServerTryActivateAbilityWithEventData 경로에서 받은 단계를 사용하도록 하며, 거절 초기화 ResetCombo·NotifyAbilityFailed 안은 폐기했다. ^c1
- UWxAbility_Combo는 이벤트 데이터 단계가 범위 밖이면 첫 단을 사용하고, 이벤트 데이터 없는 AI·도플갱어 미러링·서버 단독 발동은 기존 단계 진행을 유지한다. ^c2
- 작업 기록의 네트워크 테스트 27회 중 26회는 전 항목 통과했으며, 완료·거절 뒤 다음 입력의 단계 일치는 27회 모두 통과했지만 첫 입력 누락 1회의 원인은 미확인이다. ^c3
