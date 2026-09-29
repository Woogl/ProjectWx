---
type: source
title: "작업 - Doppelganger-캐릭터-개선"
created: 2026-09-29
updated: 2026-09-29
status: developing
tags:
  - "source"
  - "작업-기록"
  - "도플갱어"
  - "WxAI"
  - "이동"
summary: "도플갱어를 중력 없는 NoCollision 분신으로 바꾸고 Master 우측 50cm 추종·높이와 이동 모드 맞추기·몽타주 종료마다 위치 재조정을 넣은 2026-09-29 완료 작업 기록"
source_type: task-record
source_id: src-eec4ffdf0ec6173c5379
sha256: 63e8005080aa1c44267b201876eddda0789f98bffd8ae33e6a12fac3e21f7634
authority: primary
independence_key: ".agents/workflow/tasks/Doppelganger-캐릭터-개선.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/Doppelganger-캐릭터-개선.md"
raw_copy: ".raw/captured/63e8005080aa1c44267b201876eddda0789f98bffd8ae33e6a12fac3e21f7634.md"
claim_ids:
  - clm-63e8005080-c1
  - clm-63e8005080-c2
  - clm-63e8005080-c3
  - clm-63e8005080-c4
key_claims:
  - "2026-09-29 이후 BP_Doppelganger는 GravityScale 0, 기본 이동 모드 Flying, BrakingDecelerationFlying 2000이고 캡슐·메시 콜리전이 NoCollision이어서 적에게 맞지 않는다."
  - "2026-09-29 이후 WxBTService_MirrorMovement는 Master 우측 50cm(LocalOffset 0,50,0)를 따라가며, Master가 IsFalling이면 MOVE_Falling 아니면 MOVE_Flying으로 두고 Z를 매 틱 직접 맞추며 MaxFlySpeed를 Master MaxWalkSpeed×1.25로 맞춘다."
  - "woogle의 2026-09-29 실패 메모에 따라 도플갱어 위치 재조정은 Action 어빌리티 종료가 아니라 AnimInstance OnMontageEnded의 모든 몽타주 종료(끊긴 것 포함)에서 일어나며, 콤보 중에도 단계마다 우측 50cm로 돌아온다."
  - "도플갱어 개선은 헤드리스 PIE·게임 모드 임시 테스트와 빌드로 AI가 확인했고 woogle이 2026-09-29 코드 리뷰와 게임 플레이 확인을 통과시켜 체크리스트 8/8로 완료됐다."
---

# 작업 - Doppelganger-캐릭터-개선

- 원본: `.agents/workflow/tasks/Doppelganger-캐릭터-개선.md`
- 원자료 사본: `.raw/captured/63e8005080aa1c44267b201876eddda0789f98bffd8ae33e6a12fac3e21f7634.md`
- 수집: 2026-09-29 UTC · 재확인 기한: 2027-03-28

## 개요

woogle이 2026-09-29 요청한 도플갱어 개선 네 가지(중력 영향 없음, NoCollision, Master 미러링 위치 우측 50cm, Action 어빌리티 완료마다 우측 50cm로 재조정)를 다룬 작업 기록이다. 추종은 BT_Doppelganger의 Mirror Movement 서비스(`WxBTService_MirrorMovement`, WxAI)가 맡는다. 웹 처리로 Claude Code가 정하기·구현·수정을 맡았고, 상태는 완료(체크리스트 8/8 통과)다.

## 질문과 결정(woogle 2026-09-29)

- Q1 중력 방식: woogle은 "Master의 Z축 높이와 MovementMode를 똑같이 씁시다. 문제가 된다면 말씀해주세요."라고 답했다.
- Q2 NoCollision 범위: 캡슐과 메시 모두 NoCollision(적에게 맞지 않는 유령).
- Q3: 캡슐이 NoCollision이면 걷기 모드가 바닥을 못 찾아 엔진이 매 틱 낙하로 바꾸므로(바닥 검사가 캡슐 자신의 충돌 설정을 씀), Master가 공중(IsFalling)이면 낙하(중력 0), 그 밖에는 비행(Flying)으로 두고 높이는 매 틱 Master Z로 맞추는 안을 골랐다. 공중에서는 AirControl 비율만큼만 좌우로 움직여 옆 위치를 조금 느리게 따라잡는다.
- 사람 확인 실패 메모(2026-09-29): "어빌리티가 끝날 때마다가 아닌, 몽타주가 끝날 때마다 위치 재조정하게 수정해주세요." 이에 따라 재조정 신호를 몽타주 종료로 바꿨다.

## 구현 결과

- BP_Doppelganger: CMC GravityScale 2→0(계획에는 1로 적혔지만 원래 2였음), DefaultLandMovementMode Walking→Flying, BrakingDecelerationFlying 0→2000(계획 밖, 비행 모드에서 목표 주위를 오가며 멈추지 못해 걷기 제동값과 맞춤), 캡슐·메시 콜리전 프리셋 Custom→NoCollision.
- `WxBTService_MirrorMovement`: LocalOffset 기본값 (0,100,0)→(0,50,0)(이 서비스를 쓰는 에셋은 BT_Doppelganger 하나). 매 틱 Master가 IsFalling이면 MOVE_Falling(진입 때 Velocity.Z 0), 아니면 MOVE_Flying으로 두고 Z를 Master 기준 목표 Z로 직접 맞춘다. MaxFlySpeed를 FollowSpeed(Master MaxWalkSpeed×1.25)로 맞추고 Release에서 CDO 값으로 되돌린다.
- 점프·앉기 흉내, MaxWalkSpeedCrouched 대입, PreviousJumpCount, IgnoreActorWhenMoving 설정·해제, 바인딩 때 낙하 모드 복사를 지웠고, 지연 텔레포트 조건에서 도플갱어 쪽 IsMovingOnGround를 뺐다.
- 최종 재조정 신호: ASC OnAbilityEnded(Ability.Action 필터) 대신 AnimInstance `OnMontageEnded`에 `HandleMontageEnded`를 연결해, 끊긴 것을 포함한 모든 몽타주 종료에서 Master 우측 50cm로 텔레포트한다. '애니메이팅 어빌리티 없음' 조건을 빼서 콤보 중에도 단계마다 재조정한다. 이전 몽타주 종료 통지는 블렌드 아웃 뒤에 오므로 콤보 중 순간 이동은 다음 단계 시작 약 0.2초 뒤에 일어난다.

## 검증 범위

- AI 헤드리스 PIE 임시 테스트(뒤에 삭제): 설정값(중력 0·Flying·NoCollision), 점프 공중 표본 9개 모드 불일치 0·최대 Z 오차 0.000, MaxFlySpeed 625=FollowSpeed·해제 뒤 600 복원, 소환 4초 뒤 우측 목표까지 14.11cm.
- 몽타주 종료 재조정(헤드리스 게임 모드 LV_DevCombat): A 재생 중 3000cm 밀어도 순간 이동 없음, 다음 몽타주 B가 A를 끊자 9.12cm로 복귀(그때 애니메이팅 어빌리티 0), B 종료 뒤 0.00cm.
- 빌드: 수정 뒤와 임시 테스트 삭제 뒤 Result: Succeeded.
- 사람: woogle 2026-09-29 코드 리뷰와 게임 플레이 확인 통과(재조정 신호 변경 뒤).

## 관련 주제

- [[적 AI와 몬스터]]

## 핵심 주장

- 2026-09-29 이후 BP_Doppelganger는 GravityScale 0, 기본 이동 모드 Flying, BrakingDecelerationFlying 2000이고 캡슐·메시 콜리전이 NoCollision이어서 적에게 맞지 않는다. ^c1
- 2026-09-29 이후 WxBTService_MirrorMovement는 Master 우측 50cm(LocalOffset 0,50,0)를 따라가며, Master가 IsFalling이면 MOVE_Falling 아니면 MOVE_Flying으로 두고 Z를 매 틱 직접 맞추며 MaxFlySpeed를 Master MaxWalkSpeed×1.25로 맞춘다. ^c2
- woogle의 2026-09-29 실패 메모에 따라 도플갱어 위치 재조정은 Action 어빌리티 종료가 아니라 AnimInstance OnMontageEnded의 모든 몽타주 종료(끊긴 것 포함)에서 일어나며, 콤보 중에도 단계마다 우측 50cm로 돌아온다. ^c3
- 도플갱어 개선은 헤드리스 PIE·게임 모드 임시 테스트와 빌드로 AI가 확인했고 woogle이 2026-09-29 코드 리뷰와 게임 플레이 확인을 통과시켜 체크리스트 8/8로 완료됐다. ^c4
