---
title: "플레이어 래그돌 떨림: 마네킹 메시의 PhysicsAsset을 적과 같은 것으로 통일"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, game, physics, ragdoll]
summary: "플레이어가 쓰는 Quinn 메시의 /Game/Mannequins/Rigs/PA_Mannequin은 기본 자세에서 겹치는 바디 쌍(pelvis–spine_03, spine_05–upperarm_l)의 충돌이 켜져 있어 래그돌이 떨렸다. 2026-04-22에 SKM_Manny만 NiagaraExamples PA로 바꿨던 것을 SKM_Quinn_Simple·SKM_Quinn·SKM_Manny_Simple에도 적용했다."
---

# 플레이어 래그돌 떨림과 PhysicsAsset 통일

2026-09-24 사용자 테스트에서 발견했다. 플레이어가 죽으면 래그돌이 덜덜 떨리고 적은 그렇지 않았다.

## 조사 결과

- 플레이어·적 모두 같은 `GA_Shared_Death`를 쓰고 `DeathMontage`가 비어 있어, 사망 즉시 래그돌이 된다. 차이는 메시와 PhysicsAsset이다.
  - 플레이어(`BP_Template` Player·`BP_HGTest`), 미니언, 도플갱어: `SKM_Quinn_Simple` + `/Game/Mannequins/Rigs/PA_Mannequin`
  - 적(`BP_Template` Enemy·`BP_Soldier`·`BP_Sandbag`): `SKM_Manny` + `/Game/NiagaraExamples/Gallery/SkeletalMesh/Mannequins/Rigs/PA_Mannequin`
- 두 PA는 바디 22개·컨스트레인트 23개의 구성과 객체 이름까지 같다. 다른 점은 두 가지다.
  - 다리·팔 캡슐: 플레이어 PA가 더 두껍다(thigh r9.3·calf r8.0·upperarm r7.1 대 8.0·6.0·6.0).
  - 바디 간 충돌 비활성 표: 플레이어 PA는 231쌍 중 227쌍이 꺼져 있고, 적 PA는 92쌍만 꺼져 있다.
- 플레이어 PA에서 충돌이 켜진 4쌍 중 2쌍은 기본 자세부터 겹쳐 있다. 적 PA는 이 4쌍이 모두 꺼져 있다.

  | 충돌이 켜진 쌍 | 기본 자세 관통 |
  |---|---|
  | pelvis – spine_03 | 7.6cm |
  | spine_05 – upperarm_l | 3.5cm |
  | lowerarm_r – clavicle_r | 없음 |
  | foot_r – thigh_r | 없음 |

  겹친 바디를 충돌이 밀어내고 고정(LOCKED) 컨스트레인트가 다시 당겨 매 프레임 맞선다. 이것이 떨림의 원인이다.
- 적 PA는 139쌍의 충돌이 켜져 있지만 기본 자세에서 겹치는 쌍이 없다. 가장 가까운 쌍도 양발 3.6cm, 양 허벅지 4.4cm 여유가 있다.
- 이력: 커밋 `0461c95cd`(2026-04-22, "래그돌 버그 수정 (Physics asset 교체)")가 `SKM_Manny`만 NiagaraExamples PA로 바꿨다. Quinn 메시들과 `SKM_Manny_Simple`은 결함 있는 PA에 남아 있었다.

## 조치

- `SKM_Quinn_Simple`·`SKM_Quinn`·`SKM_Manny_Simple`의 `PhysicsAsset`을 적과 같은 NiagaraExamples PA로 바꿨다. 이제 `/Game/Mannequins/Rigs/PA_Mannequin`을 참조하는 에셋은 없다(삭제하지 않음).
- 이 방식을 고른 이유: 플레이어 PA의 겹치는 쌍만 끄면 그 PA는 자기 충돌이 거의 없어져 팔다리가 몸을 뚫는다. 통일하면 모든 캐릭터가 같은 품질의 자기 충돌을 갖는다.
- 남은 점: 정본 PA가 예제 폴더(NiagaraExamples)에 있다. 예제 콘텐츠를 지우면 모든 마네킹 메시의 PA가 끊긴다.

## 조사 방법(재사용)

- 충돌 비활성 표(`UPhysicsAsset::CollisionDisableTable`)는 UPROPERTY도 스크립트 API도 아니다. uasset 바이너리에서 `int32 개수 + (int32 바디A, int32 바디B, uint32 값) × 개수` 형태의 구간을 찾아 읽었다. 값은 항상 0이고 항목이 있으면 비활성이다.
- 바디 인덱스는 `SkeletalBodySetup_N` 객체 번호 순서와 같았다. 이 대응에서 인접 뼈 21쌍이 적 PA 표에 모두 들어 있는 것으로 검증했다.
- 겹침은 스켈레톤 기준 자세(`AnimPoseExtensions.GetReferencePose`)에 캡슐을 배치해 선분 간 거리와 반지름 합으로 계산했다. 박스는 외접구로 근사했다.
- 에디터가 떠 있으면 그 에디터가 연 에셋은 커맨드릿에서 저장할 수 없다(파일 이동 오류 32). 에디터 MCP의 `ObjectTools.set_properties`와 `AssetTools.save_assets`로 바꿨다.

인게임 재확인은 사용자 몫으로 남았다.
