# Doppelganger 캐릭터 개선

상태: 완료 · 체크리스트 8/8 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

## 요청

- 요청자: woogle · 2026-09-29

> 1. 도플갱어는 중력 영향 받지 않게 수정
> 2. 도플갱어의 콜리전을 NoCollision으로 수정
> 3. 도플갱어의 Master 미러링 위치를 우측 50cm로 수정
> 4. 도플갱어가 Action 어빌리티 완료할 때마다 Master 우측 50cm로 위치 재조정

## 질문

| ID | 질문 | 선택지 | 추천 | 답변 |
| --- | --- | --- | --- | --- |
| Q1 | 중력을 받지 않게 하는 방식을 어떻게 할까요? Mirror Movement 서비스는 지금 Master의 점프·낙하·앉기를 따라 합니다. 중력만 0으로 끄면 점프를 따라 하는 순간 끝없이 떠오르므로 이 흉내도 함께 정리해야 합니다. | Flying 이동 모드: 도플갱어가 Master의 높이(Z)까지 3D로 따라 떠다닙니다. 점프·낙하·앉기 흉내는 없앱니다(앉기는 Flying에서 엔진이 막음). / 중력 배율 0만: 걷기 모드는 그대로 두고 GravityScale만 0으로 합니다. 점프·낙하 흉내를 없애고, 높이는 텔레포트할 때만 Master에 맞춥니다. | Flying 이동 모드를 추천합니다. NoCollision이면 바닥 판정에 기댈 수 없으니 높이를 직접 따라가는 편이 자연스럽고, 엔진 순정 이동 모드라서 떠오름이나 공중 정지 같은 예외 처리가 필요 없습니다. | Master의 Z축 높이와 MovementMode를 똑같이 씁시다. 문제가 된다면 말씀해주세요. · woogle 2026-09-29 |
| Q2 | NoCollision을 어디까지 적용할까요? 캡슐만 끄면 벽과 캐릭터는 통과하지만, 메시 충돌이 남아 있으면 적의 공격 판정에 맞을 수 있습니다. | 캡슐과 메시 모두 NoCollision: 적에게 맞지 않는 유령으로 둡니다. / 캡슐만 NoCollision: 통과만 하고 메시 피격 판정은 유지합니다. | 캡슐과 메시 모두 끄는 쪽을 추천합니다. 도플갱어는 Master의 공격을 따라 하는 분신이라 피격 반응이 끼어들면 미러링이 끊깁니다. 무기(BP_MinionKatana)의 공격 판정은 도플갱어 자신의 충돌과 상관없이 그대로 동작합니다. | 캡슐과 메시 모두 NoCollision: 적에게 맞지 않는 유령으로 둡니다. · woogle 2026-09-29 |
| Q3 | Master와 이동 모드를 똑같이 맞추면 문제가 있습니다. 캡슐이 NoCollision이면 걷기(Walking) 모드가 바닥을 찾지 못해, 엔진이 매 틱 낙하(Falling)로 바꿉니다. 바닥 검사가 캡슐 자신의 충돌 설정을 쓰기 때문입니다. 그러면 ABP_Unarmed가 IsFalling으로 낙하 자세를 고르므로, 땅 위에서도 계속 떨어지는 자세가 나옵니다. 낙하 모드에서는 이동 입력으로 높이를 바꿀 수도 없습니다. 높이는 매 틱 Master의 Z로 직접 맞추기로 하고, 이동 모드는 어떻게 할까요? | Master 상태에 따라 나눔: Master가 공중(IsFalling)이면 도플갱어도 낙하(중력 0), 그 밖에는 비행(Flying)으로 둡니다. Master가 점프·낙하할 때만 점프·낙하 자세를 따라 합니다. 다만 공중에서는 엔진의 공중 제어(AirControl) 값만큼만 좌우로 움직여서, 점프하는 동안에는 옆 위치를 조금 느리게 따라잡습니다. / 항상 비행(Flying): 가장 단순합니다. 높이는 똑같이 따라가지만, Master가 점프해도 도플갱어는 걷기·서기 자세로 함께 떠오릅니다(점프·낙하 자세 없음). | Master 상태에 따라 나누는 방식을 추천합니다. 이동 모드를 똑같이 맞추자는 답변의 뜻(점프·낙하할 때 같은 모습)을 NoCollision 안에서 가장 가깝게 살릴 수 있습니다. 그리고 엔진 순정 모드 두 가지만 씁니다. 높이는 직접 맞추므로 공중에서 끝없이 떠오르는 일도 없습니다. | Master 상태에 따라 나눔: Master가 공중(IsFalling)이면 도플갱어도 낙하(중력 0), 그 밖에는 비행(Flying)으로 둡니다. Master가 점프·낙하할 때만 점프·낙하 자세를 따라 합니다. 다만 공중에서는 엔진의 공중 제어(AirControl) 값만큼만 좌우로 움직여서, 점프하는 동안에는 옆 위치를 조금 느리게 따라잡습니다. · woogle 2026-09-29 |

## 구현 계획

추가 질문은 없습니다. Q1은 Q3의 답으로 정했습니다. Q2는 캡슐과 메시 모두 NoCollision, Q3는 Master가 공중이면 낙하(중력 0), 그 밖에는 비행입니다.

1. 중력 무시 (BP_Doppelganger, MCP로 CDO 수정)
   - CharacterMovement의 GravityScale을 0으로, DefaultLandMovementMode를 Flying으로 바꿉니다. 소환 직후 첫 틱 전에도 떨어지지 않게 하기 위해서입니다.
2. 콜리전 (BP_Doppelganger)
   - CollisionCylinder(캡슐)와 CharacterMesh의 콜리전 프리셋을 NoCollision으로 바꿉니다.
   - 캡슐 충돌이 없으므로 서비스의 IgnoreActorWhenMoving 설정·해제(WxBTService_MirrorMovement.cpp:60,130)를 지웁니다.
3. 우측 50cm (WxBTService_MirrorMovement.h:33)
   - LocalOffset 기본값을 FVector(0.f, 50.f, 0.f)로 바꿉니다. BT_Doppelganger에는 이 값을 덮어쓴 곳이 없습니다. 다른 BT가 이 서비스를 쓰지 않는지는 구현할 때 MCP로 다시 확인합니다.
4. Action이 끝날 때 재조정 (WxBTService_MirrorMovement.cpp HandleAbilityEnded)
   - 끝난 어빌리티의 GetAssetTags().HasTag(WxGameplayTags::Ability_Action)일 때만 bPendingAbilityEndTeleport를 세웁니다.
   - 정상 완료와 취소는 구분하지 않습니다. 지금처럼 몽타주 어빌리티가 모두 끝난 틱에 Master 우측 50cm로 텔레포트하고, 콤보가 이어지는 동안에는 기다립니다.
   - 헤더 주석(18줄)을 'Action 어빌리티 종료'로 고칩니다.
5. 높이와 이동 모드 맞추기 (WxBTService_MirrorMovement.cpp TickNode)
   - 매 틱 Master가 IsFalling이면 MOVE_Falling, 아니면 MOVE_Flying으로 둡니다. 모드가 다를 때만 SetMovementMode를 부릅니다. 이 서비스는 도플갱어의 CMC보다 먼저 틱합니다(기존 선행 조건). 그래서 그 틱의 이동부터 새 모드가 적용되고, ABP_Unarmed의 IsFalling도 Master와 같아집니다.
   - 높이는 매 틱 도플갱어의 Z만 Destination.Z(Master Z + LocalOffset.Z)로 직접 맞춥니다(스윕 없음). 낙하 모드로 바꿀 때는 Velocity.Z를 0으로 둡니다. 중력이 0이므로 이것만으로 떠오르지 않습니다.
   - 수평 추종은 지금처럼 오차 보정과 2D 이동 입력으로 합니다. 낙하 중에는 엔진이 AirControl 비율로 입력을 줄이므로 옆 위치를 조금 느리게 따라잡습니다(별도 처리 없음).
   - 비행 속도: MOV는 MaxWalkSpeed에만 반영됩니다. 그래서 매 틱 MaxFlySpeed를 FollowSpeed(Master MaxWalkSpeed×1.25)로 맞추고, Release에서는 클래스 기본값으로 되돌립니다. 앉기 속도를 다루던 방식과 같습니다.
   - 바인딩할 때 낙하 모드·속도를 복사하던 코드(131-135)를 지웁니다.
   - 점프·앉기 흉내(169-173)와 MaxWalkSpeedCrouched 대입을 지웁니다. 또 PreviousJumpCount 멤버와 그 대입, Release의 앉기 속도 복원·UnCrouch·StopJumping, HandleAbilityActivated·FaceMaster 분기의 StopJumping도 지웁니다.
   - 지연 텔레포트 조건(188)에서 도플갱어 쪽 IsMovingOnGround를 빼고 'Master가 땅 위'만 남깁니다(도플갱어는 이제 바닥 판정이 없습니다).
6. 검증
   - 에디터를 끈 상태로 빌드하고 'Result:' 줄로 성공을 판정합니다.
   - Tests/에 임시 자동화 테스트를 만들어 헤드리스 PIE에서 도플갱어를 소환하고 다음을 확인합니다.
     - GravityScale 0, 캡슐·메시 NoCollision
     - Master가 땅 위일 때는 Flying, Master 점프 중에는 Falling이고, 도플갱어의 Z가 Master의 Z와 같으며 계속 떠오르지 않음
     - 비행 중 MaxFlySpeed가 추종 속도와 같음. 해제 뒤에는 기본값으로 돌아옴
     - 정지 상태에서 Master 로컬 기준 우측 50cm ± ArrivalRadius에 수렴
     - Action 어빌리티(약공격)가 끝나면 우측 50cm로 텔레포트
     - Action이 아닌 어빌리티(락온·질주)가 끝나면 텔레포트하지 않음
   - 확인한 뒤 임시 테스트를 지우고 다시 빌드합니다.

테스트 체크리스트 초안
- (AI) 빌드 성공
- (AI) BP_Doppelganger 설정값: 중력 0, 기본 이동 모드 Flying, 캡슐·메시 NoCollision
- (AI) 이동 모드와 높이가 Master를 따르고, 떠오르지 않음
- (AI) 비행 속도가 추종 속도를 따르고, 해제하면 복원됨
- (AI) 우측 50cm 수렴
- (AI) Action이 끝날 때만 재조정
- (사람) 코드 리뷰: WxBTService_MirrorMovement.h/.cpp, BP_Doppelganger(CMC의 GravityScale 1→0·DefaultLandMovementMode Walking→Flying, 캡슐·메시 콜리전 프리셋 → NoCollision)
- (사람) 게임에서 도플갱어를 소환한 뒤 이동·점프·공격 콤보를 합니다. 벽을 통과하며 Master 오른쪽 50cm에 같은 높이로 붙어 다니는지 봅니다. 땅 위에서는 걷기 자세, Master가 점프할 때만 점프·낙하 자세인지 봅니다. 점프 중에 옆으로 따라잡는 속도와 콤보가 끝날 때마다 제자리로 순간 이동하는 모습이 어색하지 않은지 봅니다.

구현 승인: woogle 2026-09-29

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 빌드 성공 | 에디터를 끈 상태로 Build.bat WxEditor Win64 Development 실행 후 Result 줄 확인(임시 테스트를 지운 뒤 다시 빌드) | AI | 통과 | 몽타주 종료 수정 뒤 [1/5] Compile WxBTService_MirrorMovement.cpp Result: Succeeded, 임시 테스트 삭제 뒤 재빌드도 Result: Succeeded |
| BP_Doppelganger 설정값 | MCP로 CDO 조회 + 헤드리스 PIE에서 스폰한 인스턴스 검사: 중력 0, 기본 이동 모드 Flying, 캡슐·메시 NoCollision | AI | 통과 | 이전 처리 결과 유지(이번 수정은 BP를 건드리지 않음): MCP 재조회 gravityScale 0·MOVE_Flying·profile NoCollision, 자동화 테스트 검사 통과 |
| 이동 모드와 높이가 Master를 따르고 떠오르지 않음 | 헤드리스 PIE: Master 정지 상태와 Jump 뒤 3초 동안 매 프레임 모드와 Z 비교, 착지 뒤 다시 확인 | AI | 통과 | 이전 처리 결과 유지(이번 수정은 이동 모드·높이 코드를 건드리지 않음): 점프 공중 표본 9, 모드 불일치 0, 최대 Z 오차 0.000 |
| 비행 속도가 추종 속도를 따르고 해제하면 복원됨 | 헤드리스 PIE: MaxFlySpeed와 Master MaxWalkSpeed×1.25 비교 → 블랙보드 Master 키를 비워 해제 → 기본값 비교 → 다시 연결 | AI | 통과 | 이전 처리 결과 유지(이번 수정 범위 밖): 625.0=FollowSpeed, 해제 뒤 600.0=CDO |
| 우측 50cm 수렴 | 헤드리스 PIE: 전방 200cm에 소환하고 일정 시간 뒤 Master+회전(0,50,0)과의 수평 거리가 ArrivalRadius(15) 이하인지 확인 | AI | 통과 | 이번 테스트에서도 소환 4초 뒤 대기 거리 14.11cm |
| 몽타주가 끝날 때마다 재조정 | 헤드리스 게임 모드(LV_DevCombat): 도플갱어 몽타주 A 재생 → 3000cm 밀기 → 0.1s 뒤 거리 → 몽타주 B로 A를 끊음(콤보 다음 단계 모사) → 0.1s 간격 거리 표본 → B 재생 중 다시 밀기 → Montage_Stop → 0.3s 뒤 거리 | AI | 통과 | A 재생 중 3057.51cm(순간 이동 없음); B 재생 중 A 종료 시 3080.04→9.12cm(B playing=1, animating ability=0); B 재생 중 밀어낸 뒤 3011.96cm; B 종료 뒤 0.00cm. Result={Success} |
| 코드 리뷰 | WxBTService_MirrorMovement.h/.cpp 변경, BP_Doppelganger(CMC GravityScale 2→0·DefaultLandMovementMode Walking→Flying·BrakingDecelerationFlying 0→2000, 캡슐·메시 콜리전 프리셋 Custom→NoCollision). 이번에 추가로 볼 점: 재조정 신호가 OnAbilityEnded(Action 필터)에서 AnimInstance OnMontageEnded(모든 몽타주, 끊긴 것 포함)로 바뀌었고, 지연 텔레포트의 '애니메이팅 어빌리티 없음' 조건이 빠짐 | 사람 | 통과 | woogle 2026-09-29 |
| 게임 플레이 확인 | 게임에서 도플갱어를 소환한 뒤 이동·점프·공격 콤보를 합니다. 벽을 통과하며 Master 오른쪽 50cm에 같은 높이로 붙어 다니는지 봅니다. 땅 위에서는 걷기 자세, Master가 점프할 때만 점프·낙하 자세인지 봅니다. 콤보 단계마다(이전 몽타주가 끝날 때, 다음 단계 시작 약 0.2초 뒤) 제자리로 순간 이동하고 마지막 몽타주가 끝날 때 다시 맞추는지, 그 모습이 어색하지 않은지 봅니다. | 사람 | 통과 | woogle 2026-09-29 |

## AI 조사 결과 · 2026-09-29T13:02:26.968Z

<!-- test-feedback:request-7b990c65-4450-42de-bbdd-dc41bc0a30f4:3 -->
- 전달한 사람: woogle
- 처리 AI: Claude Code
- 처리 결과: 질문 답변 필요

AI 요약:

> 도플갱어의 추종은 이미 BT_Doppelganger의 Mirror Movement 서비스(WxBTService_MirrorMovement)가 맡고 있습니다. 이 서비스는 Master 기준 LocalOffset 위치를 따라가고, 어빌리티가 끝나면 그 위치로 텔레포트합니다. 따라서 요청 3·4는 이 서비스만 조금 고치면 됩니다.
> - 요청 3: LocalOffset 기본값이 (0,100,0), 즉 오른쪽 100cm이고 BT 에셋에서 덮어쓴 값이 없습니다. 50cm로 줄이면 됩니다.
> - 요청 4: 지금은 어떤 어빌리티든 끝나면 위치를 다시 맞춥니다. 끝난 어빌리티의 에셋 태그가 Ability.Action 아래일 때만 맞추도록 거르면 됩니다.
> 
> 판단이 필요한 것은 요청 1·2의 방식입니다.
> - 중력(요청 1): 서비스가 Master의 점프·낙하·앉기를 따라 하고 있습니다. 중력만 0으로 끄면 점프 순간 도플갱어가 끝없이 떠오릅니다.
> - 콜리전(요청 2): 캡슐만 끌지, 메시까지 꺼서 적에게 맞지 않게 할지 정해야 합니다.
> 
> 소환 순간의 위치(소환 노티파이 LocalSpawnOffset, 전방 200cm)는 이번 요청 범위 밖으로 보고 그대로 둡니다. 소환 뒤에는 추종 위치로 곧 수렴합니다.

> 근거: Read .agents/workflow/tasks/Doppelganger-캐릭터-개선.md — 요청 4건(중력 무시, NoCollision, 우측 50cm 미러링, Action 완료 시 위치 재조정)

> 근거: Read .agents/workflow/process/index.md — 정하기 규칙 확인

> 근거: Read Plugins/WxAI/Source/WxAI/Public/WxBTService_MirrorMovement.h:33 — LocalOffset 기본값 FVector(0,100,0)

> 근거: Read Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp — HandleAbilityEnded(90-94)가 어빌리티 종류를 가리지 않고 bPendingAbilityEndTeleport를 세움, 177-183에서 애니메이팅 어빌리티가 없을 때 Destination으로 TeleportTo; 131-135 낙하 모드 복사, 170-172 앉기·점프 미러링, 185 Error.Z=0, 188 지면 조건부 지연 텔레포트

> 근거: Read Plugins/WxAI/Source/WxAI/(Public|Private)/WxBTTask_MirrorAbility.* — 어빌리티 미러링은 위치에 관여하지 않음

> 근거: Grep BT_Doppelganger.uasset — WxBTService_MirrorMovement·WxBTTask_MirrorAbility 사용, 덮어쓴 속성은 FaceMasterAbilityTags·MoveSpeedEffect(WxEffect_MoveSpeedOverride)뿐, LocalOffset 덮어쓰기 없음

> 근거: Grep BP_Doppelganger.uasset — WxEnemyCharacter 파생, CapsuleComponent·CharacterMesh·WxCharacterMovementComponent·BP_MinionKatana 무기, GravityScale·NoCollision 문자열 없음(현재 기본값)

> 근거: Grep WxAnimNotify_SpawnMinion.cpp:21,35 — 소환 위치는 노티파이 LocalSpawnOffset 전방 200cm

> 근거: Grep WxGameplayTags.h:185 / .cpp:89 — Ability_Action = "Ability.Action"

> 근거: Read memory doppelganger-ability-mirroring·exclusive-action-flag — 미러링 재설계 금지, 액션 판정은 GetAssetTags().HasTag(Ability.Action)


## AI 조사 결과 · 2026-09-29T13:12:13.182Z

<!-- test-feedback:request-46e641ff-987e-4209-b506-65db834b0f5b:1 -->
- 전달한 사람: woogle
- 처리 AI: Claude Code
- 처리 결과: 질문 답변 필요

답변:

> Q1: Master의 Z축 높이와 MovementMode를 똑같이 씁시다. 문제가 된다면 말씀해주세요.
> Q2: 캡슐과 메시 모두 NoCollision: 적에게 맞지 않는 유령으로 둡니다.

AI 요약:

> Q2(캡슐·메시 모두 NoCollision)는 그대로 반영했습니다. Q1은 답변대로 하면 문제가 하나 있어 다시 여쭙니다(Q3).
> 
> 문제는 Master가 걷는 동안 도플갱어를 걷기(Walking) 모드로 둘 수 없다는 점입니다. 걷기 모드는 매 틱 캡슐로 아래를 쓸어 바닥을 찾습니다. 그런데 이 검사는 캡슐 자신의 충돌 설정을 그대로 씁니다. 캡슐이 NoCollision이면 바닥을 끝내 찾지 못하고, 엔진이 곧바로 낙하(Falling)로 바꿉니다. 그러면 ABP_Unarmed가 IsFalling으로 낙하 자세를 고르기 때문에, 도플갱어는 땅 위에서도 계속 떨어지는 자세로 떠 있게 됩니다. 낙하 모드에서는 이동 입력으로 높이를 바꿀 수도 없습니다.
> 
> 그래서 높이는 매 틱 Master의 Z로 직접 맞추고, 이동 모드는 Master가 공중일 때만 낙하, 그 밖에는 비행(Flying)으로 두는 방식을 추천합니다. 이렇게 하면 Master가 점프하거나 떨어질 때만 도플갱어도 점프·낙하 자세를 따라 합니다. 요청 3(우측 50cm)과 요청 4(Action 어빌리티가 끝날 때 위치 재조정)는 이미 정해져 있어 계획에 그대로 넣었습니다.

> 근거: Read .agents/workflow/tasks/Doppelganger-캐릭터-개선.md — 요청 4건, Q1 답변 'Master의 Z축 높이와 MovementMode를 똑같이, 문제가 된다면 말씀', Q2 답변 '캡슐과 메시 모두 NoCollision'

> 근거: Read .agents/workflow/process/index.md — 답변 때문에 새 판단이 생기면 다시 묻는 정하기 규칙 확인

> 근거: Read Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp — 131-135 낙하 모드·속도 복사, 169-173 앉기·점프 흉내, 185 Error.Z=0, 188 양쪽 IsMovingOnGround일 때만 지연 텔레포트, 200-201 수평 이동 입력만 넣음, 60/130 IgnoreActorWhenMoving

> 근거: Read Plugins/WxAI/Source/WxAI/Public/WxBTService_MirrorMovement.h — LocalOffset 기본값 (0,100,0), PreviousJumpCount 멤버, 18줄 '어빌리티 종료' 주석

> 근거: Glob Content/**/*Doppelganger*.uasset — ABS_/BP_/BT_Doppelganger

> 근거: Grep BP_Doppelganger.uasset 'ABP_' — 애님 클래스는 ABP_Unarmed_C

> 근거: Grep Content/Mannequins/Animations/Unarmed/ABP_Unarmed.uasset — IsFalling·GroundSpeed·ShouldMove 사용(점프·낙하 자세를 IsFalling으로 고름)

> 근거: Grep UE_5.8 CharacterMovementComponent.cpp:7058,7110 — ComputeFloorDist가 InitCollisionParams(QueryParams, ResponseParam)로 바닥을 쓸어 봄

> 근거: Grep UE_5.8 MovementComponent.cpp:390-395 — InitCollisionParams가 UpdatedPrimitive(캡슐)의 InitSweepCollisionParams를 씀 → 캡슐 NoCollision이면 바닥을 못 찾아 Walking이 Falling으로 바뀜


## AI 조사 결과 · 2026-09-29T13:17:44.271Z

<!-- test-feedback:request-14d099cf-7a71-417a-b8de-9dcae2e47cc6:1 -->
- 전달한 사람: woogle
- 처리 AI: Claude Code
- 처리 결과: 구현 승인 필요

답변:

> Q3: Master 상태에 따라 나눔: Master가 공중(IsFalling)이면 도플갱어도 낙하(중력 0), 그 밖에는 비행(Flying)으로 둡니다. Master가 점프·낙하할 때만 점프·낙하 자세를 따라 합니다. 다만 공중에서는 엔진의 공중 제어(AirControl) 값만큼만 좌우로 움직여서, 점프하는 동안에는 옆 위치를 조금 느리게 따라잡습니다.

AI 요약:

> 추가 질문은 없습니다. Q3는 답변대로 정했습니다. Master가 공중(IsFalling)이면 도플갱어도 낙하 모드(중력 0)로, 그 밖에는 비행 모드로 둡니다. 높이는 매 틱 Master의 Z로 직접 맞추고, 공중에서는 엔진의 공중 제어(AirControl) 값만큼만 옆으로 움직입니다.
> 
> 다시 조사하다 한 가지를 확인해 계획에 넣었습니다. 이동 속도(MOV)는 걷기 속도(MaxWalkSpeed)에만 반영되고 비행 속도(MaxFlySpeed)에는 반영되지 않습니다(WxCharacterBase.cpp:77,212). 그대로 두면 비행 모드에서 도플갱어가 클래스 기본 비행 속도로 움직여 Master를 따라잡지 못할 수 있습니다. 그래서 추종 서비스가 MaxFlySpeed를 추종 속도와 맞추고, 해제할 때 클래스 기본값으로 되돌립니다. 지금 앉기 속도(MaxWalkSpeedCrouched)를 다루는 방식과 같습니다.
> 
> 구현 계획을 승인해 주세요.

> 근거: Read .agents/workflow/tasks/Doppelganger-캐릭터-개선.md — 요청 4건, Q1·Q2 답변, 새로 받은 Q3 답변(Master가 공중이면 낙하·중력 0, 그 밖에는 비행, 공중에서는 AirControl만큼 좌우 이동)

> 근거: Read .agents/workflow/process/index.md — 더 물을 것이 없으면 구현 계획·검증·체크리스트 초안을 넘기는 정하기 규칙 확인

> 근거: Read Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp — Release의 앉기 속도 복원·UnCrouch·StopJumping·IgnoreActorWhenMoving(55-60), HandleAbilityActivated의 StopJumping(85), HandleAbilityEnded가 종류를 가리지 않고 bPendingAbilityEndTeleport를 세움(90-94), 바인딩할 때 IgnoreActorWhenMoving·낙하 복사(130-135), FaceMaster 분기의 StopJumping·PreviousJumpCount(143-144), 추종 속도 FollowSpeed=MaxWalkSpeed*1.25를 MOV GE로 씀(152-168), 앉기·점프 흉내(169-173), Error.Z=0(185), 양쪽이 땅 위일 때만 지연 텔레포트(188), 이동 입력은 2D(200-201)

> 근거: Grep MaxWalkSpeed/MaxFlySpeed in Source·Plugins *.cpp — MOV는 WxCharacterBase.cpp:77,212에서 MaxWalkSpeed에만 들어가고, MaxFlySpeed를 쓰는 코드는 없음

> 근거: Glob Content/**/*Doppelganger*.uasset — ABS_/BP_/BT_Doppelganger

> 근거: Grep 'MirrorMovement' BT_Doppelganger.uasset — 1건(이 서비스 사용). Content 전체 Grep은 결과가 없어 다른 BT의 사용 여부는 구현할 때 MCP로 다시 확인


## AI 구현 결과 · 2026-09-29T13:23:21.193Z

<!-- test-feedback:request-47dff516-77a2-4771-ba86-4f46913468e9:1 -->
- 전달한 사람: woogle
- 처리 AI: Claude Code
- 처리 결과: 사람 확인 필요

AI 요약:

> 구현 계획대로 도플갱어를 고쳤고, 헤드리스 PIE 자동화 테스트에서 AI 항목이 모두 통과했습니다. 임시 테스트는 지웠고, 지운 상태에서 다시 빌드해 성공했습니다.
> 
> - **중력·콜리전(BP_Doppelganger)**: CMC의 GravityScale을 0으로, DefaultLandMovementMode를 Flying으로 바꿨습니다. 캡슐과 메시의 콜리전 프리셋은 둘 다 NoCollision입니다. GravityScale은 계획에 적힌 1이 아니라 원래 2였습니다.
> - **우측 50cm**: Mirror Movement 서비스의 LocalOffset 기본값을 (0,50,0)으로 바꿨습니다. 이 서비스를 쓰는 에셋은 BT_Doppelganger 하나뿐입니다.
> - **Action이 끝날 때만 재조정**: 끝난 어빌리티의 에셋 태그가 Ability.Action일 때만 위치를 다시 맞춥니다.
> - **높이·이동 모드**: 매 틱 Master가 공중이면 낙하, 그 밖에는 비행 모드로 둡니다. 높이는 Master의 Z로 직접 맞춥니다. 비행 속도는 추종 속도를 따르고, 해제하면 기본값으로 돌아갑니다. 점프·앉기 흉내, 캡슐 충돌 무시 설정, PreviousJumpCount는 지웠습니다.
> 
> **계획 밖 변경 1건**: BP_Doppelganger의 BrakingDecelerationFlying을 0에서 2000으로 바꿨습니다. 걷기 제동값과 같은 값입니다. 첫 테스트에서 비행 모드에 제동이 없어 도플갱어가 목표 주위를 계속 오가며 멈추지 못했습니다(3초 뒤에도 목표에서 64cm, 6초 동안 15~137cm를 오감). 제동을 넣은 뒤에는 14.3cm에서 멈춥니다. 계획의 '정지 상태에서 우측 50cm 수렴'을 지키기 위한 세부 조정이라 따로 묻지 않았고, 코드 리뷰 항목에 적어 두었습니다.
> 
> 남은 일은 사람 항목인 코드 리뷰와 게임 플레이 확인입니다.

> 변경: Plugins/WxAI/Source/WxAI/Public/WxBTService_MirrorMovement.h: LocalOffset 기본값 (0,100,0)→(0,50,0), 클래스 주석을 'Action 어빌리티 종료'로, PreviousJumpCount 멤버 제거

> 변경: Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp: HandleAbilityEnded가 Ability.Action 에셋 태그일 때만 재조정 예약; 매 틱 Master IsFalling이면 MOVE_Falling(진입 시 Velocity.Z=0), 아니면 MOVE_Flying; Z를 Destination.Z로 직접 맞춤; MaxFlySpeed=FollowSpeed, Release에서 CDO 값으로 복원; 점프·앉기 흉내·MaxWalkSpeedCrouched·UnCrouch·StopJumping·IgnoreActorWhenMoving·바인딩 시 낙하 복사 제거; 제한 시간 텔레포트 조건에서 도플갱어 IsMovingOnGround 제거; CapsuleComponent include 제거

> 변경: Content/Character/Doppelganger/BP_Doppelganger.uasset: CharMoveComp GravityScale 2→0, DefaultLandMovementMode Walking→Flying, BrakingDecelerationFlying 0→2000(계획 외, 비행 모드 정지 수렴용); CollisionCylinder·CharacterMesh0 콜리전 프리셋 Custom→NoCollision

> 근거: Read .agents/workflow/process/index.md, .agents/workflow/tasks/Doppelganger-캐릭터-개선.md — 구현 계획·승인(woogle 2026-09-29) 확인

> 근거: Read Plugins/WxAI/Source/WxAI/Public/WxBTService_MirrorMovement.h, Private/WxBTService_MirrorMovement.cpp — 수정 전 코드 확인 후 계획대로 편집

> 근거: Grep Plugins/WxCore/.../WxGameplayTags.h:185 Ability_Action, 엔진 GameplayAbilityTypes.h:300 FAbilityEndedData::AbilityThatEnded

> 근거: grep -rla WxBTService_MirrorMovement Content → BT_Doppelganger.uasset 1건(다른 BT 사용 없음)

> 근거: Build.bat WxEditor Win64 Development → [6/26] Compile WxBTService_MirrorMovement.cpp, Result: Succeeded

> 근거: UnrealEditor -ModelContextProtocolPort=8001 + MCP ObjectTools.get_properties BP_Doppelganger CDO: 수정 전 gravityScale 2, defaultLandMovementMode MOVE_Walking, 캡슐·메시 collisionProfileName Custom(QueryAndPhysics)

> 근거: MCP set_properties: CharMoveComp gravityScale 0, defaultLandMovementMode MOVE_Flying; CollisionCylinder·CharacterMesh0 bodyInstance collisionProfileName/collisionEnabled NoCollision, objectType ECC_WorldStatic → compile_blueprint → save_assets true → 재조회로 값 확인

> 근거: 임시 테스트 Source/WxGame/Tests/WxDoppelgangerMirrorTest.cpp(Wx.Tests.Doppelganger.Mirror, LV_DevCombat PIE) 작성·빌드 성공

> 근거: 1차 실행 UnrealEditor-Cmd -nullrhi -ExecCmds 'Automation RunTests Wx.Tests.Doppelganger.Mirror' → Fail: 정지 수렴 거리 64.08cm, 나머지 통과

> 근거: 2차 실행(0.5s 간격 로그) → 비행 속도가 남아 목표 주위를 오감(dist 15~137cm, 가속 0일 때도 속도 유지); MCP 조회 brakingDecelerationFlying 0, brakingDecelerationWalking 2000

> 근거: MCP set_properties brakingDecelerationFlying 2000 → compile → save true → 재조회 확인

> 근거: 3차 실행 → Result={Success}: 정지 수렴 14.33cm(로컬 X=13.9 Y=46.5), 모드 Flying, MaxFlySpeed 625=FollowSpeed(CDO 600), 점프 공중 표본 9·모드 불일치 0·최대 Z 오차 0.000, 착지 후 Flying·Z 동일, 해제 뒤 MaxFlySpeed 600 복원·재바인딩 뒤 625, 질주(비 Action) 종료 0.3s 뒤 거리 2922.8cm(텔레포트 없음), 약공격(Ability.Action) 종료 0.15s 뒤 거리 0.00cm·Z 오차 0.00

> 근거: rm -rf Source/WxGame/Tests Saved/mcptmp → Build.bat 재빌드 Result: Succeeded

> 근거: git status: M BP_Doppelganger.uasset, M WxBTService_MirrorMovement.cpp/.h (작업 기록 파일 외 다른 변경 없음)


## 사용자 테스트 결과 · 2026-09-29T14:00:02.880Z

<!-- test-feedback:request-e98c8bae-6074-47eb-8462-8e808332ead2:submitted -->
- 전달한 사람: woogle

> 통과 · 코드 리뷰
> 실패 · 게임 플레이 확인: 어빌리티가 끝날 때마다가 아닌, 몽타주가 끝날 때마다 위치 재조정하게 수정해주세요.


## AI 수정 결과 · 2026-09-29T14:00:02.880Z

<!-- test-feedback:request-e98c8bae-6074-47eb-8462-8e808332ead2:1 -->
- 전달한 사람: woogle
- 처리 AI: Claude Code
- 처리 결과: 사람 확인 필요

AI 요약:

> 도플갱어가 위치를 다시 맞추는 시점을 '어빌리티가 끝날 때'에서 '몽타주가 끝날 때마다'로 바꿨습니다.
> - 콤보 중간에도 단계마다 한 번씩 Master 우측 50cm로 돌아옵니다. 다음 단계 몽타주에 끊겨 이전 몽타주가 끝날 때 한 번, 마지막 몽타주가 끝날 때 한 번입니다.
> - 기존 방식은 몽타주를 재생하는 어빌리티가 하나도 남지 않을 때까지 기다렸습니다. 그래서 콤보가 모두 끝나야 한 번만 돌아왔습니다. 이 대기 조건을 뺐습니다.
> - 앞선 요청의 'Action 어빌리티만' 조건도 없앴습니다. 몽타주에는 어빌리티 태그가 없어서, 도플갱어가 재생하는 모든 몽타주가 대상입니다. 도플갱어는 미러링한 어빌리티의 몽타주만 재생하고, NoCollision이라 피격 몽타주도 나오지 않습니다.
> - 헤드리스 게임 모드 임시 테스트를 통과했습니다. 몽타주를 재생하는 동안 3000cm 밀어내도 순간 이동하지 않았습니다. 다음 몽타주가 재생되는 중에 이전 몽타주가 끝나자 9.12cm로 돌아왔습니다. 이때 몽타주를 재생하는 어빌리티는 없었으므로, 어빌리티 종료와 무관하게 동작한 것입니다. 마지막 몽타주가 끝난 뒤에는 0.00cm였습니다.
> - 임시 테스트는 지웠고, 지운 상태에서 다시 빌드해 성공했습니다.
> - 코드가 다시 바뀌어 코드 리뷰와 게임 플레이 확인을 '대기'로 되돌렸습니다.
> - 확인할 점: 이전 몽타주의 종료 통지는 블렌드 아웃이 끝난 뒤에 옵니다. 그래서 콤보 중 순간 이동은 다음 단계가 시작되고 약 0.2초 뒤에 일어납니다.

> 변경: Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp: 바인딩할 때 ASC OnAbilityEnded 대신 ActorInfo AnimInstance의 OnMontageEnded에 HandleMontageEnded를 연결하고 Release에서 해제; HandleMontageEnded가 끊긴 몽타주를 포함한 모든 몽타주 종료에서 재조정을 예약; 지연 텔레포트의 '애니메이팅 어빌리티 없음' 조건 제거(다음 콤보 몽타주 재생 중에도 BT 틱에서 바로 재조정); Ability.Action 필터 제거; Animation/AnimInstance.h include 추가

> 변경: Plugins/WxAI/Source/WxAI/Public/WxBTService_MirrorMovement.h: HandleAbilityEnded → UFUNCTION HandleMontageEnded(UAnimMontage*, bool), bPendingAbilityEndTeleport → bPendingMontageEndTeleport, FAbilityEndedData 전방 선언 → UAnimMontage, 클래스 주석을 '몽타주 종료'로 수정

> 근거: Read .agents/workflow/tasks/Doppelganger-캐릭터-개선.md — 게임 플레이 확인 실패 메모 '어빌리티가 끝날 때마다가 아닌, 몽타주가 끝날 때마다 위치 재조정', 기존 계획·체크리스트 확인

> 근거: Read Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp — HandleAbilityEnded(85-92)가 Ability.Action 종료 때만 bPendingAbilityEndTeleport를 세움, 171줄 지연 텔레포트가 !bAbilityActive(GetAnimatingAbility 없음) 조건이라 콤보 전체가 끝날 때까지 기다림

> 근거: Grep UE_5.8 Engine/Classes/Animation/AnimInstance.h:98,770 — OnMontageEnded는 FOnMontageEndedMCDelegate(동적 멀티캐스트, UAnimMontage*, bool bInterrupted)

> 근거: Build.bat WxEditor Win64 Development(수정 뒤) → [1/5] Compile WxBTService_MirrorMovement.cpp, Result: Succeeded

> 근거: 임시 테스트 Source/WxGame/Tests/WxDoppelgangerMontageTest.cpp(Wx.Tests.Doppelganger.MontageEnd) 작성: LV_DevCombat에서 WxMinionSubsystem으로 BP_Doppelganger 소환 → AM_Template_Attack_Light 재생(A) → 3000cm 밀기 → 같은 몽타주 재생(B)으로 A를 끊음 → 거리 표본 → 다시 3000cm 밀기 → Montage_Stop → 거리 확인

> 근거: UnrealEditor-Cmd -game -nullrhi 1차 실행 → Fail: B 시작 오프셋 0.3s 때문에 B가 섹션 끝에서 먼저 끝나 판정 시점이 어긋남(표본상 A 종료 직후 3062→1.74cm 재조정은 확인)

> 근거: 3차 실행 → Result={Success}: 대기 중 14.11cm, A 재생 중 밀어낸 뒤 3057.51cm(순간 이동 없음), B 재생 중 표본 3064.40→3080.04→9.12cm(B playing=1, animating ability=0), B 재생 중 밀어낸 뒤 3011.96cm, B 종료 뒤 0.00cm

> 근거: rm -rf Source/WxGame/Tests → Build.bat 재빌드 Result: Succeeded

> 근거: git status --short: M BP_Doppelganger.uasset, M WxBTService_MirrorMovement.cpp/.h, ?? 작업 기록 파일 — 다른 변경 없음


## 사용자 테스트 결과 · 2026-09-29T14:14:04.120Z

<!-- test-feedback:request-427d70ff-b0e9-4855-9cdf-2d0cd1d15fdd:submitted -->
- 전달한 사람: woogle

> 통과 · 코드 리뷰
> 통과 · 게임 플레이 확인
