---
type: source
title: "작업 - ability-directional-section"
created: 2026-09-28
updated: 2026-09-28
status: developing
tags:
  - source
  - 작업-기록
summary: "방향 섹션 선택을 공통 PlayMontage로 옮기고 콤보는 몽타주 배열로 복원한 결정과 단독·네트워크·사람 연출 검증 기록"
source_type: task-record
source_id: src-f8621e669bc408db998c
sha256: c107fac948fcc641a68afe44de2cade27c92fcbe38e09c8a803573638a777080
authority: primary
independence_key: ".agents/workflow/tasks/ability-directional-section.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/ability-directional-section.md"
raw_copy: ".raw/captured/c107fac948fcc641a68afe44de2cade27c92fcbe38e09c8a803573638a777080.md"
claim_ids:
  - clm-c107fac948-c1
  - clm-c107fac948-c2
  - clm-c107fac948-c3
key_claims:
  - "2026-09-25 사용자 결정으로 콤보는 번호 섹션 몽타주 하나 대신 ComboMontages 배열의 단계별 몽타주를 사용하고, 방향 섹션은 각 몽타주 안에서 선택한다."
  - "공통 PlayMontage가 입력 수집·로컬 좌표 변환·첫 방향 재생의 TargetData 동기화를 맡으며, 공용 무입력 기본값은 Forward이고 Dodge는 기존 Backstep·후방 기본값을 유지한다."
  - "작업 기록은 2026-09-27 단독·리슨 서버와 원격 클라이언트 헤드리스 확인 및 woogle의 2026-09-28 코드 리뷰·콤보 블렌딩·방향 연출 통과를 기록한다."
---

# 작업 - ability-directional-section

- 원본: `.agents/workflow/tasks/ability-directional-section.md`
- 원자료 사본: `.raw/captured/c107fac948fcc641a68afe44de2cade27c92fcbe38e09c8a803573638a777080.md`
- 수집: 2026-09-28 UTC · 재확인 기한: 2027-03-28

## 확정 결정

> 사용자 2026-09-25: "방향과 콤보 모두 다 섹션으로 관리하면 너무 어려울 것 같아요. 콤보는 예전처럼 여러개의 몽타주로 쪼개는게 낫겠습니다."

UWxAbility_Combo가 배열과 단계 인덱스를 공유한다. 공격·스킬은 콤보 창 재발동으로, 패턴은 블렌드아웃에서 다음 몽타주로 진행한다. 새 방향 함수는 C++ 전용이며 UFUNCTION으로 노출하지 않는다.

## 구현 관찰

Forward·ForwardRight·Right·BackRight·Back·BackLeft·Left·ForwardLeft를 공용으로 선택한다. 무입력·누락 방향은 Forward로 폴백한다. 반응은 NormalRight처럼 요청 이름을 접두사로 사용하되 정확히 일치하는 기존 섹션을 우선한다. 방향을 독립 재생하려면 다음 섹션 링크를 끊는다. 방향 섹션 없는 몽타주는 기존 시작 위치를 유지한다. 한 활성화에서는 같은 방향을 쓰고 콤보 재발동은 새 입력을 받는다. Dodge의 루트모션 보정·극한 회피는 유지한다.

## 검증 범위와 한계

GA 22개 배열 설정·컴파일·저장 후 별도 프로세스 재로드와 원본 몽타주 29개 복원을 확인했다. 미참조 통합 몽타주 9개는 당시 삭제하지 않았다. 2026-09-27 헤드리스 검증은 실제 입력 경로와 한 프로세스의 리슨 서버·원격 클라이언트를 사용했다. 방향 스킬·반응은 기존 에셋이 없어 인스턴스 교체와 메모리 사본으로 검사했으므로 배포된 방향 에셋 존재를 증명하지 않는다. 임시 테스트 제거 후 빌드도 성공했다.

> woogle 2026-09-28: "통과 · 코드 리뷰", "통과 · 콤보 연결 연출", "통과 · 방향별 연출 모양"

위 결과는 원자료의 실행 이력이며 이번 Wiki 갱신에서 게임을 재실행한 결과가 아니다. 콤보 완료·거절 뒤 단계 동기화의 후속 수정은 [[작업 - combo-stage-desync-after-rejection]]을 따른다.

## 관련 주제

- [[어빌리티와 GAS]]

## 핵심 주장

- 2026-09-25 사용자 결정으로 콤보는 번호 섹션 몽타주 하나 대신 ComboMontages 배열의 단계별 몽타주를 사용하고, 방향 섹션은 각 몽타주 안에서 선택한다. ^c1
- 공통 PlayMontage가 입력 수집·로컬 좌표 변환·첫 방향 재생의 TargetData 동기화를 맡으며, 공용 무입력 기본값은 Forward이고 Dodge는 기존 Backstep·후방 기본값을 유지한다. ^c2
- 작업 기록은 2026-09-27 단독·리슨 서버와 원격 클라이언트 헤드리스 확인 및 woogle의 2026-09-28 코드 리뷰·콤보 블렌딩·방향 연출 통과를 기록한다. ^c3
