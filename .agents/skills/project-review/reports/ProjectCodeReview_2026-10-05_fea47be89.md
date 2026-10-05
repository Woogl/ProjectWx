# 프로젝트 코드 리뷰

* 시간: 2026-10-05 11:18 (Asia/Seoul)

* 기준 커밋: fea47be89

* 작업 트리: 소스 미커밋 변경 없음. 미추적 파일은 이전 보고서 `ProjectCodeReview_2026-10-03_e9dbe2a28.md` 하나이고, 코드가 아니어서 검토 대상이 아니다.

## 결과 요약

`Source/WxGame` 게임 런타임 C++의 459개 파일, 32,869줄을 검토했다. 자체 플러그인 3개(BoxComponentVisualizer·DataTableRowFixup·WxToolset)는 모두 Editor 모듈이라 범위에서 뺐다. 기능 폴더 17개 묶음을 모두 끝까지 읽고, 네트워크·수명·GAS 종단·저장/부활 네 관점으로 모듈을 가로지르는 흐름을 따라갔다. 중복 병합과 반증 검증을 거쳐 총 73건을 확정했다. 그중 29건은 10-04 보고서(e9dbe2a28)에서 확정됐지만 아직 반영되지 않은 항목으로, 코드가 그대로임을 확인해 다시 실었다.

| 카테고리 | 🔴 심각 | 🟡 개선 | 🟢 사소 |
| --- | ---: | ---: | ---: |
| 전투·어빌리티 (AbilitySystem·Combat·Animation·Weapons·Targeting) | 1 | 21 | 12 |
| UI·프론트엔드 (UI·FrontEnd) | 1 | 5 | 6 |
| 퀘스트·월드 (Quest·Spawner·Device·Inventory·Interaction·Dialogue·Save·Player) | 0 | 8 | 9 |
| AI | 0 | 2 | 3 |
| 캐릭터·네트워크 (Character·Minion) | 0 | 2 | 1 |
| 공통·개발 (Development·전역 규칙) | 0 | 0 | 2 |
| 합계 | 2 | 38 | 33 |

리뷰 직후 간단하고 안전한 30건을 바로 고쳤다(항목마다 「2026-10-05 반영」으로 표시). 소스는 증분 빌드 성공·경고 없음을 확인했고, 에셋 2건은 다시 읽어 값을 확인했다. PIE 확인은 하지 않았다.

함께 고치면 좋은 짝이 두 쌍 있다. 공격 프리셋 오프셋과 Skill_3 범위 피해 행 누락(D01·D04)은 같은 몽타주·프리셋 에셋을 다룬다. 스포너 처치 대기 정체와 퀘스트 볼륨 재수주(D03·D21)는 같은 에셋 규약(퀘스트 대상 액터의 공간 로드 끄기)으로 함께 풀린다.

## 항목별 세부 사항

### D01. 🔴 공격·락온 타게팅 프리셋의 AOE 오프셋이 몸 앞이 아니라 월드 +X로 더해져, 판정 범위가 바라보는 방향에 따라 달라진다

**발생 조건**: 아래 프리셋을 쓰는 공격을 월드 +X가 아닌 방향을 보고 쓸 때 항상 생기고, 현재 에셋에서 드러난다. 결과가 실제로 갈리는 것은 범위 경계 근처의 대상이다.

- 궁극기 범위 피해: `AM_HGTest_Ultimate_1`(노티파이 3개)과 `AM_Template_Ultimate`의 AreaDamage가 TP_Attack_250을 쓴다.
- 락온 대상이 없을 때 SnapToTarget이 회전 대상을 고르는 범위: TP_Attack_250을 쓰는 몽타주 36개(플레이어 라이트 콤보와 적 패턴 대부분)다.
- SnapToTarget 이동(bSnapLocation): TP_Attack_500(DodgeCounter·Heavy_1/2·Skill_3·Minion_Attack_Heavy)과 TP_Attack_1000(Soldier_Pattern_2 계열)이다.

**판단 근거**: 묶음 리뷰(B10_Combat_Targeting-01)가 찾았고, 서로의 판정을 모르는 검증자 두 명이 각각 확정했다. 엔진 `Engine/Plugins/Experimental/GameplayTargetingSystem/Source/GameplayTargetingSystem/Private/Tasks/TargetingSelectionTask_AOE.cpp`의 생성자(38행)는 `bUseRelativeOffset = false`로 둔다. `GetSourceOffset_Implementation`(312-329행)은 이 플래그가 켜졌을 때만 오프셋을 SourceActor의 Forward/Right/Up 축으로 돌리고, 아니면 `DefaultSourceOffset`을 월드 벡터 그대로 돌려준다. 즉시·비동기·디버그 실행이 모두 `GetSourceLocation + GetSourceOffset`을 쓴다(68·188·129행). `Content/Character/Template/Shared/Targeting/`의 TP_Attack_100·250·500·1000·10000과 TP_LockOn은 `DefaultSourceOffset`이 모두 (50,0,0)이고, 이름표에 `bUseRelativeOffset`이 없어 기본값 false다(.uasset 태그 디코드). 요청을 만드는 [WxRootMotionModifier_SnapToTarget.cpp:76](../../../../Source/WxGame/Targeting/WxRootMotionModifier_SnapToTarget.cpp#L76), [WxAbilityTask_MontageEvents.cpp:137](../../../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp#L137), [WxAbility_LockOn.cpp:244](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_LockOn.cpp#L244)은 모두 SourceActor만 넘긴다. 오프셋을 따로 회전하는 코드나 AOE 서브클래스는 없다. 프리뷰 [WxTargetingPreview.cpp:64](../../../../Source/WxGame/Targeting/WxTargetingPreview.cpp#L64)는 같은 오프셋을 몸 프레임으로 돌려 그린다. 81c04f52f(09-12)의 작업 기록 「후속 과제」에 "정면 50을 의도했다면 `bUseRelativeOffset`을 켜야 한다"가 미해결로 남아 있었고, 이 기록은 35b9b2c32에서 작업 기록 제도와 함께 삭제됐다. 지금 동작을 유지하기로 한 결정은 없다.

**원인**: 프리셋 AOE 태스크에 상대 오프셋 플래그를 켜지 않아 엔진이 오프셋을 월드 축으로 더한다. 프리뷰는 같은 값을 몸 기준으로 그리므로 저작 화면에서는 어긋남이 보이지 않는다.

**영향**: 구 중심이 의도한 자리에서 최대 100cm 벗어난다(월드 -X를 볼 때). 이때 앞쪽 도달 거리는 R+50에서 R−50으로 줄고, 등 뒤 도달 거리는 R−50에서 R+50으로 는다.

| 프리셋(반지름) | 쓰임 | 월드 -X를 볼 때 앞쪽 도달 거리 |
| --- | --- | --- |
| TP_Attack_250 | 궁극기 범위 피해, 락온 없을 때 회전 대상 선택 | 300 → 200cm (−33%) |
| TP_Attack_500 | 스냅 이동 | 550 → 450cm (−18%) |
| TP_Attack_1000 | 스냅 이동 | 1050 → 950cm (−9.5%) |
| TP_LockOn(1500)·TP_Attack_10000 | 락온 후보·패턴 | 영향 미미 |

궁극기가 앞의 적을 놓치고 등 뒤의 적을 맞힐 수 있다. 락온하지 않은 기본 공격은 앞의 적 대신 등 뒤의 적을 향해 돌 수 있다. TP_Attack_100(`AM_HGTest_Skill_3`의 AreaDamage)은 오프셋 비율이 가장 크지만, 그 노티파이에 DamageDataRow가 비어 있어 지금은 피해가 나가지 않는다(D04). 모든 머신이 같은 식으로 계산하므로 네트워크 불일치는 아니다. 플레이어 주력 공격의 판정이 월드 방향에 따라 달라지는 기능 오류라 🔴로 둔다.

**수정 제안**: 프리셋 6개의 AOE 태스크에서 엔진 순정 플래그 `bUseRelativeOffset`을 켠다. 에셋만 고치면 되고 코드 변경은 없다. 프리뷰는 Persona 프리뷰 액터의 회전이 0이면 지금 코드 그대로 런타임과 같아진다. 회전된 프리뷰 액터까지 맞추려면, 상대 모드에서 `GetSourceOffset` 결과를 프리뷰 액터 회전으로 되돌린 뒤 몸 프레임에 얹는다. 고친 뒤에는 월드 -X를 보고 궁극기를 써서 앞쪽 250~300cm의 적이 맞는지 확인한다.

**확신도**: 높음 — 엔진 계산식은 소스로, 오프셋·반지름·플래그 부재는 프리셋 바이너리로 확인했고 독립 검증 두 건이 같은 결론이었다. PIE 재현은 하지 않았다.

**2026-10-05 반영**: 프리셋 6개(TP_Attack_100·250·500·1000·10000, TP_LockOn)의 AOE 태스크에서 `bUseRelativeOffset`을 켰다. 헤드리스 Python으로 고쳤고, 새 프로세스로 다시 읽어 값을 확인했다. 코드는 바꾸지 않았다. PIE 확인은 하지 않았다.

### D02. 🔴 인게임에서 인벤토리 키(B)를 눌러도 아무것도 열리지 않는다

**발생 조건**: 게임 중 HUD가 활성일 때 B를 누르면 항상 재현된다. ESC → 메인 메뉴 → 인벤토리 경로는 동작한다.

**판단 근거**: 묶음 리뷰(B15_UI_Core-02)가 찾았다. 1차 검증은 대체 경로가 있어 🟡로, 판정을 모르는 2차 검증은 항상 재현되는 기능 오류라 🔴로 봤고, 세 번째 검증자가 🔴로 정했다. 실제 HUD는 GM_Combat → BP_PlayerController → `UWxPlayerLayoutComponent::LayoutClass`로 연결된 WBP_GameLayout이고, 같은 키를 처리하는 다른 BP·코드 경로는 없다. 입력 `UI.Action.Inventory`(B, `Config/DefaultInput.ini`)는 [WxHUDLayout.cpp:51](../../../../Source/WxGame/UI/WxHUDLayout.cpp#L51)에서 `PushMenuWidget(InventoryWidgetClass)`를 부른다. 그런데 지금 HUD 레이아웃인 `Content/UI/Widget/WBP_GameLayout.uasset`에는 `InventoryWidgetClass` 이름이 없어 값이 null이다. 그래서 [WxHUDLayout.cpp:93](../../../../Source/WxGame/UI/WxHUDLayout.cpp#L93)에서 바로 돌아간다. 값은 원래 WBP_GameHUDBase에만 있었다. 4bdddcd58(08-12)에서 WBP_GameHUD의 부모가 바뀌며 값을 잃었고, 1f1934a41(08-19)은 이미 쓰이지 않던 베이스를 지웠다. 683aa464a(06-12, 인벤토리 키를 I에서 B로 변경)가 단축키를 유지하려는 의도를 보여 준다. 단축키를 일부러 없앤 기록은 없다.

**원인**: 베이스 BP CDO의 값이 부모 교체 때 자식으로 옮겨지지 않았다.

**영향**: 인벤토리 단축키가 매번 아무 반응 없이 무시된다. 진행이 막히지는 않는다(소비 아이템은 화면 없이 입력으로 쓰이고, 메뉴를 거치는 대체 경로가 있다). 재현 조건이 분명한 상시 기능 오류라 🔴로 두되, 크래시·진행 불가는 아니어서 D01 다음에 둔다.

**수정 제안**: WBP_GameLayout의 `InventoryWidgetClass`에 WBP_Inventory를 지정한다.

**확신도**: 높음 — null이면 바로 돌아가는 코드 경로와 에셋에 값이 없다는 사실로 결과가 정해진다.

**2026-10-05 반영**: WBP_GameLayout의 `InventoryWidgetClass`에 WBP_Inventory를 지정했다. 헤드리스 Python으로 고쳤고, 새 프로세스로 다시 읽어 값을 확인했다. PIE에서 B 키 동작은 확인하지 않았다.

### D03. 🟡 Manual 퀘스트 스포너는 셀이 언로드되면 적이 다시 나오지 않아 「스포너 처치 대기」가 영구 정체된다

**발생 조건**: KillEnemies 스텝 진행 중에 대상 Manual 스포너의 셀이 언로드됐다가 다시 로드된다. 또는 스텝에 들어가는 순간 그 셀이 언로드돼 있다. 지금 LV_OpenWorld에서는 맵 서부·북부 끝까지 갔다 와야 생긴다(D21과 같은 조건).

**판단 근거**: 10-04 보고서 C02(미반영)를 이번 묶음 리뷰(B14_Spawner_Misc-01)가 다시 찾았다. e9dbe2a28 이후 Spawner 폴더는 10-04 C50의 가드 삭제와 주석만 바뀌어 결함 경로는 그대로다. [WxStateTreeTask_TriggerSpawners.cpp:39-46](../../../../Source/WxGame/Spawner/WxStateTreeTask_TriggerSpawners.cpp#L39)은 진입할 때 한 번만 해석한다. 다시 로드된 스포너는 [WxSpawner.cpp:111-114](../../../../Source/WxGame/Spawner/WxSpawner.cpp#L111)에서 Auto일 때만 스폰하고, 부활 때의 `RespawnAll`도 Manual을 건너뛴다. [WxStateTreeTask_WaitSpawnersKilled.cpp:128-139](../../../../Source/WxGame/Spawner/WxStateTreeTask_WaitSpawnersKilled.cpp#L128)는 해석되지 않거나 처치되지 않은 스포너를 계속 기다린다.

**원인**: 스폰 트리거를 받았다는 사실이 스포너의 런타임 상태로만 남는데, 셀 언로드로 그 상태가 사라진다. 대기 태스크는 그대로 기다린다.

**영향**: Main2가 진행 불가가 된다. 레벨을 다시 열거나 D21의 재수주로 퀘스트가 처음부터 다시 시작되는 것 말고는 복구 수단이 없다. 현재 에셋에서 드러나는 경로지만 드문 이동이 필요해 🟡로 둔다.

**수정 제안**: 퀘스트 대상 Manual 스포너를 Is Spatially Loaded=false로 배치하는 에셋 규약을 정한다(D21과 같은 방식). 코드로 고친다면 TriggerSpawners가 상태에 머무는 동안 해석되지 않은 스포너를 낮은 주기로 다시 시도하게 한다.

**확신도**: 높음 — 해석·스폰·대기 세 경로가 코드로 정해지고, 지금 코드에 그대로 있다.

### D04. 🟡 HGTest Skill_3의 범위 피해 노티파이에 DamageDataRow가 비어 있어 범위 타격이 아무 일도 하지 않는다

**발생 조건**: BP_HGTest가 도플갱어가 있는 동안(`Master.Doppelganger` 필요) GA_HGTest_Skill_3을 쓸 때마다 생긴다. 현재 에셋에서 드러난다.

**판단 근거**: 묶음 리뷰(B08)와 GAS 횡단 리뷰(X3)가 각각 범위 밖 의심점으로 적었고, 검증에서 확정했다. `AM_HGTest_Skill_3.uasset`의 DamageDataRow 태그는 WeaponAttack 3개에만 있고 AreaDamage 노티파이에는 없다. 이 노티파이는 d32f2a91b(09-19, Skill3 추가) 때부터 행 없이 저장돼 있다. [WxAbilityTask_MontageEvents.cpp:129](../../../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp#L129) 이하는 프리셋만 검사하고 대상마다 `ApplyDamage`를 부르는데, [WxCombatLibrary.cpp:71](../../../../Source/WxGame/Combat/WxCombatLibrary.cpp#L71) 이하는 행이 없으면 false를 돌려준다. 엔진 `DataTable.h:453-459`는 RowName이 None이면 경고 로그도 남기지 않는다. 에디터 타임라인 이름표만 "Area: None"으로 보인다.

**원인**: 에셋 저작 누락으로 보인다. 행이 빈 노티파이가 조용히 무시된다.

**영향**: Skill_3의 범위 타격이 피해·반응·GP를 전혀 주지 않는다. 무기 구간 3타는 정상이다. D01의 TP_Attack_100 오프셋 문제도 이 때문에 지금은 드러나지 않는다.

**수정 제안**: 기획 결정에 따라 노티파이에 DamageDataRow를 지정하거나 노티파이를 지운다. 에셋만 고치면 된다.

**확신도**: 높음 — 몽타주 태그와 처리 경로로 결과가 정해진다.

### D05. 🟡 체크포인트에서 쉬면 아이템 충전이 쉰 플레이어가 아니라 리슨 호스트(0번 PlayerController)에게 채워진다

**발생 조건**: 리슨 서버에서 원격 클라 플레이어가 BP_CheckPoint와 상호작용해 Lit/Resting 상태로 들어간다. 단독 플레이나 호스트 본인의 상호작용에서는 드러나지 않는다. 게임 안에서 멀티 세션을 여는 흐름은 아직 없어(`RequestNewGame`은 Standalone 전용) 지금은 네트워크 PIE에서만 재현된다.

**판단 근거**: 묶음 리뷰(B13_Inventory-02)와 네트워크 횡단 리뷰(X1_Network-01)가 각각 보고했다. [WxStateTreeTask_RefillItemCharges.cpp:36](../../../../Source/WxGame/Inventory/WxStateTreeTask_RefillItemCharges.cpp#L36)이 `UGameplayStatics::GetPlayerController(Owner, 0)`으로 대상을 고른다. 같은 ST_CheckPoint 상태의 [WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp:32](../../../../Source/WxGame/Device/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp#L32)는 회복 GE를 `GetInteractingCharacter()`에게 건다. 주석이 근거로 든 0번 정책([WxEnemyCharacter.cpp:147](../../../../Source/WxGame/Character/WxEnemyCharacter.cpp#L147))은 받을 당사자가 따로 없는 처치 보상의 정책이다. 그래서 의도 기록으로 볼 수 없다. 메모리 project_no_splitscreen도 "네트워크 멀티플레이(서버 권위·리슨 호스트) 전제는 유효"라고 적는다.

**원인**: 당사자가 있는 체크포인트에서도 리필 대상을 처치 보상의 0번 정책으로 골랐다.

**영향**: 한 번의 휴식에서 대상이 갈린다. 쉰 원격 플레이어는 회복만 받고 물약 충전은 그대로이고, 쉬지 않은 호스트의 충전형 아이템이 가득 찬다.

**수정 제안**: 태스크에 `Category="Input"` 액터 필드를 두고 ST_CheckPoint에서 `Actor.InteractingCharacter`에 바인딩한다. 그 폰의 컨트롤러에서 `UWxInventoryComponent`를 찾아 리필하고, 대상이 없으면 건너뛴다. 기획이 접속한 전원 리필로 정하면, 0번 대신 월드의 PlayerController를 모두 순회한다.

**확신도**: 높음 — 대상 선택이 한 줄로 정해지고, 같은 상태의 다른 태스크가 당사자를 쓰는 것도 코드로 확인했다. 네트워크 PIE 재현은 하지 않았다.

### D06. 🟡 가드 리액션 중 키를 뗐다 다시 눌러도 재입력이 흡수돼, 리액션 뒤 LoopStart로 돌아가며 퍼펙트 가드 창이 열리지 않는다

**발생 조건**: 가드 중 GuardReact(GuardHit 0.5667초 등)가 재생되는 동안 키를 뗐다가, 리액션이 끝나기 전에 다시 누른다.

**판단 근거**: 묶음 리뷰(B05_Abilities_B-02)가 172e7ed0c(10-05, C17 수정)의 부작용으로 찾았다. 키를 떼면 [WxAbility_Guard.cpp:45](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Guard.cpp#L45)가 리액션 종료를 기다린다. 다시 누르면 ASC가 `InputPressed`를 세우지만 가드가 이미 활성이라 발동은 흡수된다. 리액션이 끝나면 [WxAbility_Guard.cpp:99](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Guard.cpp#L99)의 `IsInputHeld()`가 참이 되어 [106행](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Guard.cpp#L106)이 LoopStart를 재생한다. 헤더의 "퍼펙트 가드 창은 첫 입력에만"은 C17 반영 결정이고, 재입력은 그 결정이 다루지 않은 경우다.

**원인**: 홀드와 재입력을 구분하지 않는다. 172e7ed0c 이전에는 Default를 재생해 창이 우연히 열렸다(홀드에도 열려 C17 결함이었다).

**영향**: 가드 피격 직후 이어지는 타격은 키를 다시 눌러도 패링할 수 없고, 리액션이 끝난 뒤 새로 눌러야만 한다.

**수정 제안**: 기획이 허용한다면, 키를 뗀 분기([45-49행](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Guard.cpp#L45))에서 표준 `UAbilityTask_WaitInputPress`를 켜고 그 신호로 Default 재생을 고른다. 이 태스크는 OnPress에서 서버로 이벤트를 보내므로 원격 클라에서도 동작한다. `InputPressed` 오버라이드는 쓰지 않는다. IA_Guard가 Down 트리거라 누르는 동안 매 프레임 불려 홀드와 구분하지 못하고(54474c528이 같은 이유로 지웠다), 서버로 전달되지도 않는다.

**확신도**: 높음 — 입력 경로와 섹션 선택이 코드로 정해진다.

### D07. 🟡 AM_Shared_GuardReact의 GuardHit 섹션 시작(0~0.038초)에 퍼펙트 가드 창이 있어, 입력 없이 패링이 성립할 수 있다

**발생 조건**: 가드 중 넉이 아닌 타격을 막아 GuardHit이 시작되고, 약 38ms(60fps에서 애님 갱신 2~3회) 안에 가드 가능한 타격이 하나 더 닿는다. 두 적의 거의 동시 공격, 근접과 투사체의 겹침이 예다. 창은 서버에만 열린다.

**판단 근거**: 묶음 리뷰(B05_Abilities_B-04)가 찾았다. 몽타주 태그 파싱 결과(AM_Shared_Guard의 C17 T3D 값으로 파서 검증) GuardHit 섹션은 0.0초에 시작하고, `WxPerfectGuard` 구간(WxEffect_PerfectGuard)이 0.0초부터 0.0378초 동안 있다. [WxAbilityTask_MontageEvents.cpp:217](../../../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp#L217) 이하가 서버에서 이 GE를 걸고, [WxEffect_Damage.cpp:127](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L127) 이하는 `Damage.CanGuard`와 `Effect.PerfectGuard` 태그만으로 퍼펙트 가드를 판정한다. 입력은 보지 않는다. 구간은 2026-04-09 AM_GuardHit부터 있었고(당시 0.0609초) 의도를 적은 기록은 없다.

**원인**: 가드 피격 연출 몽타주가 시작부에 퍼펙트 가드 GE 구간을 연다. C17과 같은 부류의 「입력 없는 창」이다.

**영향**: 두 번째 타격이 입력 없이 퍼펙트 가드로 판정되어, 피해 대신 공격자에게 GP 반사와 패리 경직이 들어간다. 창이 짧아 빈도는 낮다.

**수정 제안**: 의도가 아니면 GuardHit의 WxPerfectGuard 구간을 지운다. 연속 타격 보호가 의도라면 GuardReact 헤더에 그 계약을 적되, 보호가 목적이면 패링 처벌까지 주는 퍼펙트 가드는 과하므로 함께 확인한다.

**확신도**: 높음 — 노티파이 시각을 검증된 파서로 확인했고 판정 경로가 코드로 정해진다.

### D08. 🟡 적 네임플레이트 Character VM을 Deinitialize하지 않아 뗀 뒤에도 적 ASC 구독과 매 틱 타이머가 GC까지 남는다

**발생 조건**: 로컬 화면에서 살아 있던 적의 네임플레이트를 뗄 때마다 생긴다(가시 거리 이탈, 교전 해제, 락온 해제, 사망, 컨트롤러 EndPlay).

**판단 근거**: 10-04 보고서 C08(미반영)을 이번 묶음 리뷰(B15_UI_Core-03)와 수명 횡단 리뷰(X2_Lifetime-01)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. VM은 [WxNameplateManagerComponent.cpp:121-132](../../../../Source/WxGame/UI/WxNameplateManagerComponent.cpp#L121)에서 만들어 수동 소스로 넣지만, 떼는 두 곳([97-106](../../../../Source/WxGame/UI/WxNameplateManagerComponent.cpp#L97), [47-54](../../../../Source/WxGame/UI/WxNameplateManagerComponent.cpp#L47))은 `DestroyComponent`만 부른다. 엔진 `UMVVMView::UninitializeSource`(MVVMView.cpp:315-324)는 수동 소스를 정리하지 않고, 같은 VM을 쓰는 보스 리졸버만 [WxViewModelResolver_BossCharacter.cpp:45](../../../../Source/WxGame/UI/MVVM/WxViewModelResolver_BossCharacter.cpp#L45)에서 `Deinitialize`한다.

**원인**: 121행 주석은 VM이 "위젯과 함께 사라진다"고 보지만, 실제로 사라지는 시점은 다음 GC다. 델리게이트 구독(AddUObject)과 타이머는 약참조라 GC 전까지 콜백이 계속 돈다.

**영향**: 떼어 낸 네임플레이트마다 VM 트리가 GC 주기 동안 적의 태그·스펙·GE 변화를 계속 처리하고, 시한부 효과가 있으면 Effect VM 타이머도 매 틱 돈다. 다시 붙을 때마다 새 트리가 생긴다. 현재 에셋에서 늘 생기지만 기능 오류는 아니고 CPU와 델리게이트 목록의 낭비다.

**수정 제안**: 떼는 두 곳에서 `DestroyComponent` 전에 Character VM의 `Deinitialize()`를 부른다. 이 함수가 자식 VM까지 정리한다. VM은 생성할 때 맵 값으로 함께 보관하거나 View에서 꺼내고, 121행 주석도 실제 수명에 맞게 고친다.

**확신도**: 높음 — 정리 누락은 코드와 엔진 소스로 정해진다. 영향 크기는 경미하다.

### D09. 🟡 일반 가드의 SP 소모가 기획의 「경감된 피해량」이 아니라 「경감 후 남은 피해량」이다

**발생 조건**: GuardReductionScale이 0.5가 아닌 가드 GE나 경감 버프가 생기면 드러난다. 지금은 GE_Shared_GuardReduction(0.5)뿐이라, 경감 전 피해가 홀수일 때 반올림으로 1 차이만 난다(예: 25 → 코드 SP 13, 기획 12).

**판단 근거**: 묶음 리뷰(B06_Effects-02)가 찾았다. [WxEffect_Damage.cpp:92](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L92) 이하가 FinalDamage를 Round(Base·Crit·(1−r))로 계산하고, [189행](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L189)이 SP를 FinalDamage만큼 깎으며 [194행](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L194)의 브레이크 판정도 같은 값을 쓴다. 기획 `Docs/CombatDesign/` 캐릭터 스탯 명세서 6.3은 「50% 경감 / 경감된 피해량만큼 SP 소모 / 남은 피해량만큼 HP 감소」로 둘을 나눠 적는다. PC규격서의 「경감시킨 피해량」과도 같은 뜻이다.

**원인**: SP 차감을 경감분 Base·r이 아니라 남은 피해 Base·(1−r)로 잡았다.

**영향**: 경감률이 달라지면 가드 내구가 기획과 반대 방향으로 움직인다. 경감률이 높을수록 SP가 덜 깎인다. 지금은 ±1 차이뿐이다.

**수정 제안**: 경감 전 피해(크리 포함)와 FinalDamage의 차를 SP 차감과 194행 브레이크 판정에 쓴다.

**확신도**: 높음 — 계산식과 현재 GE 값으로 결과가 정해진다.

### D10. 🟡 디버그 카메라를 켜고 끄면 HUD 레이아웃이 다시 만들어져, 폰이 바뀔 때까지 HUD가 돌아오지 않는다

**발생 조건**: 개발 빌드나 PIE에서 `ToggleDebugCamera`(또는 Enable/DisableDebugCamera) 치트를 쓴다. 개발 도구 경로에서만 드러난다.

**판단 근거**: 묶음 리뷰(B15_UI_Core-01)가 찾았다. 엔진 `DebugCameraManager.cpp:74·89`가 `Player->SwitchController`를 부르고, `Player.cpp:162-175`가 `ReceivedPlayerController`를, `LocalPlayer.cpp:738-745`가 `OnPlayerControllerChanged`를 방송한다. Wx는 이 신호를 [WxUIManagerSubsystem.cpp:210](../../../../Source/WxGame/UI/Subsystem/WxUIManagerSubsystem.cpp#L210)에서 구독한다. [HandlePlayerControllerSet](../../../../Source/WxGame/UI/Subsystem/WxUIManagerSubsystem.cpp#L213)은 받을 때마다 기존 레이아웃을 떼고 빈 레이아웃을 새로 만든다. HUD는 [WxPlayerLayoutComponent.cpp:46](../../../../Source/WxGame/UI/WxPlayerLayoutComponent.cpp#L46) 이하가 `OnPossessedPawnChanged`에서만 다시 띄우는데, 디버그 카메라 전환은 원래 PC의 폰을 바꾸지 않는다. Lyra `GameUIPolicy.cpp:47-61`은 PC가 바뀌어도 기존 루트 레이아웃을 다시 붙이고, 신호도 `CommonPlayerController::ReceivedPlayer`에서만 받는다. 이 구독은 3915a329a(05-15, 트래블 시 레이아웃 재생성)에서 들어왔고 디버그 카메라를 다룬 기록은 없다.

**원인**: 맵 이동용으로 만든 "PC가 바뀌면 레이아웃 재생성"이 같은 월드 안의 컨트롤러 교체에도 반응한다.

**영향**: 디버그 카메라를 켜면 HUD와 열려 있던 메뉴·사망·대화 화면이 사라진다. 끄면 다시 빈 레이아웃이 만들어져, 부활 등으로 폰이 바뀔 때까지 HUD가 없다. 그동안 `TrackedPlayerController`는 디버그 카메라 컨트롤러를 가리킨다.

**수정 제안**: Lyra처럼 신호를 Wx 플레이어 컨트롤러의 `ReceivedPlayer`에서 받거나, PC가 바뀌어도 기존 레이아웃을 떼었다가 다시 붙여 재사용한다. 217행 주석 "매번 재생성해도 비용이 작다"는 다시 채우는 쪽이 폰 변경에만 반응한다는 점을 놓쳤으므로 함께 고친다.

**확신도**: 높음 — 엔진 호출 경로와 Wx 처리 코드만으로 결과가 정해진다. PIE 재현은 하지 않았다.

### D11. 🟡 PlayMontageOnce를 EventTag 없는 이벤트로 발동해 원격 플레이어가 대상이면 클라 재생이 빠지고 그 취소가 서버 인스턴스까지 끊는다

**발생 조건**: 리슨 서버나 데디케이티드 서버에서 PlayMontageOnce의 대상이 원격 클라 플레이어일 때다. 발신부는 장치 StateTree의 PlayMontageOnce 태스크와 처형의 짝 몽타주 부여 두 곳이다. 지금 그 태스크를 쓰는 에셋이 없고 처형 피해자는 서버 로컬 AI뿐이라 드러나지 않는다.

**판단 근거**: 10-04 보고서 C14(미반영)를 이번 묶음 리뷰(B04_Abilities_A-03, B05_Abilities_B-06, B11_Device-02)가 다시 찾았고, 네트워크 횡단 리뷰가 엔진 근거를 보강했다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. [WxStateTreeTask_PlayMontageOnce.cpp:41-48](../../../../Source/WxGame/Device/WxStateTreeTask_PlayMontageOnce.cpp#L41)과 [WxAbility_Finisher.cpp:178-184](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Finisher.cpp#L178)는 Payload에 EventTag를 넣지 않는다. 엔진 `ClientActivateAbilitySucceedWithEventData_Implementation`(AbilitySystemComponent_Abilities.cpp:2453)은 EventTag가 유효할 때만 이벤트 데이터를 넘기므로, 클라의 [WxAbility_PlayMontageOnce.cpp:34-42](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_PlayMontageOnce.cpp#L34)는 몽타주 없이 실패해 취소를 복제한다. HitReact·GuardReact·Parry 같은 다른 ServerInitiated 반응과 적 캐릭터가 보내는 처형 이벤트는 EventTag를 채우고 있어 안전하다.

**원인**: 직접 활성화하면서 EventTag를 비워 두었다. 서버는 원격 취소를 존중하는 기본 설정(`bServerRespectsRemoteAbilityCancellation`) 때문에 클라의 취소로 서버 인스턴스도 끝낸다.

**영향**: 원격 플레이어에게 거는 장치 연출 몽타주가 클라에서 재생되지 않고, 서버 쪽 연출도 곧바로 취소된다. 10-04 C03 반영으로 사망 어빌리티가 `Ability.PlayMontageOnce` 제거를 기다리므로, 원격 플레이어가 처형 피해자가 되면 그 대기도 짝 연출 없이 바로 풀린다. 현재 에셋에서는 드러나지 않는다.

**수정 제안**: 두 발신부의 `Payload.EventTag`에 기존 네이티브 태그(예: `Ability_PlayMontageOnce`)를 넣는다.

**확신도**: 높음 — 엔진 복제 경로를 소스로 다시 확인했다. 현재 에셋에서는 드러나지 않는다.

### D12. 🟡 콤보 다음 단의 커밋이 실패하면 입력을 무시하지 않고 진행 중인 단을 취소한다

**발생 조건**: 쿨다운(MaxRecharges 1)이나 SP·MP 비용이 있는 Combo 파생 어빌리티에 콤보 창을 두고, 창 안에서 입력한다. 지금 쿨다운·비용이 있는 GA_HGTest_Skill_1·_3, GA_Template_Skill, Heavy 2종의 몽타주에는 ComboWindow가 없어 드러나지 않는다.

**판단 근거**: 10-04 보고서 C09(미반영)를 이번 묶음 리뷰(B04_Abilities_A-01)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. [WxAbility_Combo.cpp:86-90](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Combo.cpp#L86)은 커밋에 실패하면 무조건 `EndAbility(취소)`이고, 다음 단 입력([107-112](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Combo.cpp#L107))은 앞 단 몽타주가 재생 중인 창 구간에 들어온다. 1단 커밋이 쿨다운 GE를 걸었으므로 2단의 `CheckCooldown`은 반드시 실패한다.

**원인**: "다음 단 불가"와 "현재 단 중단"을 같은 실패 처리로 다룬다.

**영향**: 쿨다운이나 비용이 있는 다단 스킬을 저작하면 2단 입력 순간 1단 모션이 끊긴다. [WxAbility_Combo.h:10](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Combo.h#L10)의 "각 단계마다 비용·쿨다운을 커밋한다"를 따라 저작하기 쉽다. 현재 에셋에서는 드러나지 않는다.

**수정 제안**: 다음 단 진행에서는 엔진 `CommitCheck`로 먼저 확인하고, 실패하면 입력만 무시한 채 앞 단을 계속 재생한다. `EndAbility`는 첫 단 커밋이 실패했을 때만 부른다. 쿨다운을 첫 단에만 걸 의도라면 2단부터는 `CommitAbilityCost`만 한다.

**확신도**: 높음 — 코드 경로만으로 정해진다. 현재 에셋에서는 드러나지 않는다.

### D13. 🟡 패턴이 단계를 넘길 때 ResetActionState를 거치지 않아 앞 단의 Recovery가 다음 단까지 이어진다

**발생 조건**: 다단 패턴의 중간 단 몽타주에 `WxAnimNotify_StartRecovery`를 둔다. 지금 패턴 몽타주에는 하나도 없다.

**판단 근거**: 10-04 보고서 C10(미반영)을 이번 묶음 리뷰(B04_Abilities_A-02)가 다시 찾았다. e9dbe2a28 이후에는 주석 정정과 EndAbility 직전 중복 줄 삭제(10-04 C37·C54 반영)만 바뀌었고, 단계 전환 경로는 그대로다. Combo는 단마다 [WxAbility_Combo.cpp:95](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Combo.cpp#L95)에서 `ResetActionState`로 Blocking에 되돌리지만, [WxAbility_Pattern.cpp:26-27](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Pattern.cpp#L26)은 `PlayMontage`만 부른다. Recovery 단계는 [WxAbilityBase.cpp:164](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L164)에서 다른 어빌리티 차단을 푼다.

**원인**: 단계 전환 경로에서 단계 상태 초기화가 빠졌다.

**영향**: 중간 단에 후딜을 두면 다음 단 전체가 차단이 풀린 채 돈다. BT가 그 사이 다른 패턴이나 스킬을 발동하면 `CancelRecoveringAbilities`가 남은 단을 통째로 끊는다. 현재 에셋에서는 드러나지 않는다.

**수정 제안**: Pattern.cpp 26행과 27행 사이에서 `ResetActionState()`를 부른다.

**확신도**: 높음 — 코드 경로만으로 정해진다. 현재 에셋에서는 드러나지 않는다.

**2026-10-05 반영**: 단계를 넘길 때 `PlayMontage` 전에 `ResetActionState()`를 부른다.

### D14. 🟡 퀘스트 트리의 GiveRewards가 픽업형 보상을 월드 원점에 스폰한다

**발생 조건**: 퀘스트 StateTree(소유자 AWxGameState)의 GiveRewards가 Pickup Fragment를 가진 아이템 행을 지급할 때. 지금 DT_Reward는 Pickup Fragment가 없는 DA_Gold만 주므로 드러나지 않는다.

**판단 근거**: 10-04 보고서 C13(미반영)을 이번 묶음 리뷰(B13_Inventory-01)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. [WxStateTreeTask_GiveRewards.cpp:37-40](../../../../Source/WxGame/Inventory/WxStateTreeTask_GiveRewards.cpp#L37)은 소유자의 액터 트랜스폼을 스폰 기준으로 넘기고, [WxRewardLibrary.cpp:75-82](../../../../Source/WxGame/Inventory/WxRewardLibrary.cpp#L75)는 그 값을 그대로 스폰 위치로 쓴다. 퀘스트 러너의 소유자인 GameState는 루트가 없는 AInfo라 이 값이 원점이다.

**원인**: 장치 트리를 전제로 만든 스폰 위치 계산을 위치 없는 소유자에게도 그대로 쓴다.

**영향**: 픽업형 퀘스트 보상을 추가하면 원점 근처에 떨어져 회수할 수 없다. 현재 에셋에서는 드러나지 않는다.

**수정 제안**: 소유자에 루트 컴포넌트가 없으면 보상 대상 플레이어 폰 위치를 쓴다. 또는 퀘스트 보상은 픽업이어도 인벤토리에 직접 넣는다.

**확신도**: 높음 — 소유자와 트랜스폼 경로를 코드로 확인했다. 현재 에셋에서는 드러나지 않는다.

### D15. 🟡 피해 출처 어빌리티를 AnimatingAbility로 추정해 적 패턴 타격이 다른 어빌리티나 출처 없음·레벨 1로 기록된다

**발생 조건**: 적이 패턴을 휘두르는 중에 다른 몽타주를 트는 어빌리티(가산 일반 피격 등)가 AnimatingAbility를 덮고, 그 뒤 같은 패턴의 무기 타격이나 AreaDamage가 적중한다.

**판단 근거**: 10-04 보고서 C11(미반영)을 이번 묶음 리뷰(B10_Combat_Targeting-02)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. [WxCombatLibrary.cpp:65-66](../../../../Source/WxGame/Combat/WxCombatLibrary.cpp#L65)은 피해 시점 ASC의 `GetAnimatingAbility()`로 출처와 레벨을 정하는데, 엔진은 이 값을 마지막 PlayMontage 호출자로 덮어쓰고 종료할 때 비운다. 이 레벨은 피해 스펙 레벨이 되어 [WxEffectComponent_AdditionalEffects.cpp:30](../../../../Source/WxGame/AbilitySystem/Effects/WxEffectComponent_AdditionalEffects.cpp#L30)의 추가 효과 레벨로 쓰인다. 10-04 보고서가 든 다른 소비처(패시브 발동 식별)는 1508c1632에서 없어졌다.

**원인**: 피해를 낸 어빌리티를 넘겨받지 않고, 피해 시점 ASC의 대표값에서 되묻는다.

**영향**: 레벨별 추가 효과가 생기면 출처 없음·레벨 1로 계산되고, 컨텍스트의 출처 어빌리티도 틀린다. 지금은 AdditionalEffects를 쓰는 행이 없어 드러나지 않는다.

**수정 제안**: 무기 BeginAttack과 AreaDamage 경로에서 구간을 연 어빌리티를 `ApplyDamage`까지 넘긴다. 투사체는 지금처럼 발사 레벨을 쓴다.

**확신도**: 높음 — 엔진의 AnimatingAbility 규칙과 소비처를 코드로 확인했다. 현재 에셋에서는 드러나지 않는다.

### D16. 🟡 CameraMove NotifyEnd가 자기가 바꾼 뷰인지 확인하지 않고 무조건 폰으로 되돌린다

**발생 조건**: 로컬 뷰에 적용되는 CameraMove 구간 두 개가 겹치거나, 구간 도중 다른 시스템(대화 카메라 등)이 뷰 타깃을 바꿀 때. 지금 사용처는 AM_Shared_Finisher 하나이고, 처형 중에는 상호작용이 막혀 대화 카메라도 끼어들 수 없다.

**판단 근거**: 10-04 보고서 C15(미반영)를 이번 묶음 리뷰(B08_Animation_Minion-01)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. [WxAnimNotifyState_CameraMove.cpp:188](../../../../Source/WxGame/Animation/WxAnimNotifyState_CameraMove.cpp#L188)은 현재 뷰 타깃을 확인하지 않고 `SetViewTargetWithBlend(PC->GetPawn())`를 부른다. [38-39행](../../../../Source/WxGame/Animation/WxAnimNotifyState_CameraMove.cpp#L38) 주석대로 적·AI 몽타주의 CameraMove도 각 클라의 로컬 뷰에 적용하는 설계다.

**원인**: End가 현재 뷰 타깃이 이 구간이 스폰한 ACameraActor인지 보지 않는다.

**영향**: 적 몽타주 등에 CameraMove를 저작하는 순간, 먼저 끝난 구간이 진행 중인 다른 구간이나 다른 시스템의 카메라를 끊는다. 폰이 없으면(사망·관전) 뷰가 컨트롤러로 간다. 현재 에셋에서는 드러나지 않는다.

**수정 제안**: End에서 `PC->GetViewTarget()`이 이 오너가 스폰한 ACameraActor일 때만 되돌린다.

**확신도**: 높음 — 코드만으로 정해진다. 현재 에셋에서는 드러나지 않는다.

### D17. 🟡 적 AI 청각만 엔진 기본 팀 판정을 써서 Neutral·무팀 소음원을 적대로 받는다

**발생 조건**: 팀이 `EWxTeam::Neutral`(255)이거나 `IGenericTeamAgentInterface`가 없는 폰이 Jog 애님(소음 노티파이)을 재생하고, 그 위치가 적 AI 청취 범위 안이다. 현재 에셋에서는 드러나지 않는다. 소음 노티파이는 `ABP_Unarmed`의 Jog에서만 나고, 이 ABP를 쓰는 폰은 모두 Player/Enemy 팀이다. Neutral을 쓰는 에셋과 코드는 없다.

**판단 근거**: 묶음 리뷰(B01_AI_Behavior-01)가 찾았다. [WxAIController.cpp:31](../../../../Source/WxGame/AI/WxAIController.cpp#L31) 이하는 청각이 적대만 감지하도록 설정한다. 엔진 `AISense_Hearing.cpp:29·157`은 팀 ID만으로 `FGenericTeamId::GetAttitude`를 부르고, 기본 솔버(`AIInterfaces.cpp:30-36`)는 `A != B ? Hostile : Friendly`다. 프로젝트에 `SetAttitudeSolver` 호출은 없다. 시야는 [WxCharacterBase.cpp:174](../../../../Source/WxGame/Character/WxCharacterBase.cpp#L174) 이하의 Neutral 규칙을 탄다. [WxBTService_UpdateTargetActor.h:30](../../../../Source/WxGame/AI/WxBTService_UpdateTargetActor.h#L30)은 "세 센스 모두 적대만 등록"을 전제로 팀을 다시 확인하지 않는다.

**원인**: 피아 규칙이 두 갈래다. 폰 함수(시야·`HandlePawnHit`)는 Neutral을 Neutral로 보지만, 청각은 기본 솔버라 255도 "다른 팀 = 적대"로 본다.

**영향**: Neutral이나 무팀 폰을 추가하면 그 발소리가 적 AI의 TargetActor가 된다. 공격 판정은 적대만 맞히므로 때리지 못하는 대상을 계속 쫓는다.

**수정 제안**: 엔진 순정 통로인 `FGenericTeamId::SetAttitudeSolver`에 프로젝트 규칙(어느 쪽이든 255이면 Neutral, 같으면 Friendly, 다르면 Hostile)을 등록하고, `AWxCharacterBase::GetTeamAttitudeTowards`가 같은 판정을 부르게 한다. 지금 `WxGame.cpp`는 `FDefaultGameModuleImpl`이라 등록하려면 모듈 클래스가 하나 필요하다. 그게 부담이면 [WxBTService_UpdateTargetActor.cpp:55](../../../../Source/WxGame/AI/WxBTService_UpdateTargetActor.cpp#L55) 이하에서 적대가 아닌 대상을 한 줄로 거르고 헤더 주석을 고친다.

**확신도**: 높음 — 엔진 판정 경로를 소스로 끝까지 확인했고, 해당 소음원이 없다는 것도 에셋 참조로 확인했다.

### D18. 🟡 RandomChoice에서 관찰자 조건으로 강제 진입한 자식이 bAvoidRepeat 기억에 남지 않는다

**발생 조건**: RandomChoice 자식 C에 LowerPriority/Both 조건 데코레이터가 있다. 추첨 때 C가 조건 실패로 관찰자 등록되고, A가 실행되는 동안 C의 조건이 참이 되어 엔진이 C로 바로 들어간다. 그 뒤 RandomChoice에 다시 들어오면 생긴다. 현재 에셋에서는 드러나지 않는다. BT_Soldier·BT_Template의 RandomChoice 4개는 자식 데코레이터가 모두 RandomWeight(중단 불가)뿐이다.

**판단 근거**: 묶음 리뷰(B02_AI_Combat-01)가 찾았다. [WxBTComposite_RandomChoice.cpp:37](../../../../Source/WxGame/AI/WxBTComposite_RandomChoice.cpp#L37) 이하는 이전 자식이 있으면 기록 없이 돌아가고, 기록은 추첨 경로 한 곳([138행](../../../../Source/WxGame/AI/WxBTComposite_RandomChoice.cpp#L138))뿐이다. 엔진은 재시작 요청에서 `OnNodeRestart`로 CurrentChild를 비운 뒤, `BTCompositeNode.cpp:597-601`의 `GetNextChild`가 SearchStart를 보고 C를 바로 고른다. 이때 `GetNextChildHandler`를 거치지 않는다. 70bebd2d0(10-05, C31)은 가중치 0 자식의 관찰자 등록만 막았다.

**원인**: 반복 회피 기준을 추첨 함수 안에서만 기록하는데, 강제 진입은 추첨 함수를 거치지 않는다.

**영향**: 다음 추첨에서 실제로 마지막에 실행된 C가 아니라 중간에 끊긴 A가 빠져, C가 연속으로 다시 뽑힐 수 있다.

**수정 제안**: 37-40행 분기에서 `PrevChild == Memory->CurrentChild`일 때만 `LastChosenChild = PrevChild`로 기록한다. 인덱스 유효성만 보면, 데코레이터 재검사에서 실패해 실행되지 않은 BlockedChild까지 기록해 실제 마지막 자식의 기억을 덮어쓴다.

**확신도**: 높음 — 엔진 재시작·진입 경로를 소스로 확인했고, BT 에셋의 데코레이터 배치도 파싱해 확인했다.

### D19. 🟡 락온 유지는 지점 거리로, 재탐색 후보 제외는 액터 거리로 판정해 경계에서 대상을 번갈아 잃는다

**발생 조건**: 후보 액터 중심은 MaxDistance 안이고 락온 지점은 밖이어야 한다. 지금은 후보가 TP_LockOn(반경 1500)에서 오고 MaxDistance가 2000이라 이 판정이 어떤 후보도 거르지 않는다. 현재 에셋에서는 드러나지 않으며, MaxDistance를 줄이거나 반경을 늘리면 드러난다.

**판단 근거**: 묶음 리뷰(B07_AbilityTasks_Weapons-03)가 찾았다. [WxAbilityTask_LockOnCamera.cpp:55](../../../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_LockOnCamera.cpp#L55) 이하는 지점 컴포넌트 거리로 유지를 판정한다. [WxAbility_LockOn.cpp:136](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_LockOn.cpp#L136)은 액터 중심 거리로 후보를 거르고, [142행](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_LockOn.cpp#L142)에서야 지점을 해석한다.

**원인**: 유지 판정과 재탐색 제외의 거리 기준이 다르다.

**영향**: 경계 구간 후보는 잡히자마자 다시 잃는다. 잃은 대상 하나만 제외하므로(130행) 경계에 후보가 둘 있으면 매 틱 번갈아 갈아타며 서버 RPC가 나가고, 그동안 카메라 추적도 멈춘다.

**수정 제안**: 136행 판정을 142행 이후로 옮겨 `ResolveLockOnTarget` 결과의 `GetComponentLocation()` 거리로 판정한다.

**확신도**: 높음 — 기준 불일치와 현재 설정 값으로 도달 여부가 정해진다.

**2026-10-05 반영**: 지점을 먼저 해석하고, 유지 판정처럼 그 지점 위치로 MaxDistance를 잰다.

### D20. 🟡 체크포인트 부활이 RespawnPoint의 피치·롤을 그대로 써서 기운 채 스폰될 수 있다

**발생 조건**: RespawnPoint의 월드 회전에 피치나 롤이 있는 체크포인트에서 쉰 뒤 부활한다. 현재 에셋에서는 드러나지 않는다. LV_OpenWorld·LV_DevCombat의 체크포인트 배치와 BP_CheckPoint 상대 회전은 모두 Yaw뿐이다(uasset 태그 해석).

**판단 근거**: 저장·부활 횡단 리뷰(X4_Save_Respawn-02)가 찾았다. `SaveCheckpoint` 태스크가 마커의 컴포넌트 트랜스폼을 넘기고, [WxCheckpointSaveGame.cpp:20](../../../../Source/WxGame/Save/WxCheckpointSaveGame.cpp#L20)이 회전을 통째로 저장한다. [WxRespawnLibrary.cpp:48](../../../../Source/WxGame/Player/WxRespawnLibrary.cpp#L48)의 `RestartPlayerAtTransform`은 그 회전으로 스폰한다(엔진 `GameModeBase.cpp:1330·1376-1378`). 반면 엔진 PlayerStart 경로는 Yaw만 쓴다(`GameModeBase.cpp:1214-1222`).

**원인**: 체크포인트 경로가 엔진의 "스폰 시 피치·롤 금지" 정규화를 거치지 않는다.

**영향**: 기울여 배치하면 캡슐이 기운 채 스폰되고, 카메라 피치가 어긋난 채 시작한다.

**수정 제안**: `UWxCheckpointSaveGame::SaveCheckpoint`에서 `FRotator(0, Yaw, 0)`만 저장한다(엔진 순정 규칙과 같다).

**확신도**: 높음 — 엔진 경로 차이는 코드로 정해지고, 현재 배치 값은 에셋에서 확인했다.

**2026-10-05 반영**: 저장할 때 Yaw만 남긴다.

### D21. 🟡 공간 로드되는 퀘스트 수주 볼륨이 셀 재로드 뒤 다시 열려 진행 중이거나 끝낸 Main2를 처음부터 다시 시작시킨다

**발생 조건**: LV_OpenWorld에서 볼륨(약 X=73161, Y=-26774)을 밟아 Main2를 받은 뒤, 볼륨 셀이 언로드될 만큼(로딩 범위 768m) 멀어졌다가 다시 볼륨 자리를 지나간다. 게임플레이 액터가 모두 이 범위 안에 있어서, 콘텐츠가 없는 맵 서부(x < 약 -26400)나 북부(y > 약 51600) 끝까지 갔다 와야 생긴다.

**판단 근거**: 10-04 보고서 C16(미반영)을 이번 묶음 리뷰(B12_Dialogue_Interaction_Quest-01)가 다시 찾았다. e9dbe2a28 이후 바뀐 것은 WxQuestComponent.h 주석(10-04 C66 반영)뿐이다. 볼륨 인스턴스(Content/__ExternalActors__/Maps/LV_OpenWorld/7/A9/5CMPLAXVT7YC1G9LY4PDV8.uasset)와 BP_QuestVolume에는 지금도 bIsSpatiallyLoaded 재정의가 없다. 볼륨 그래프는 StartQuest 뒤 SetActorEnableCollision(false)만 하고, [WxQuestLibrary.cpp:10-17](../../../../Source/WxGame/Quest/WxQuestLibrary.cpp#L10)을 거친 [WxQuestComponent.cpp:34-36](../../../../Source/WxGame/Quest/WxQuestComponent.cpp#L34)은 돌던 퀘스트가 무엇이든 StopLogic → SetStateTreeReference → StartLogic을 한다.

**원인**: 스트리밍으로 리셋되는 볼륨(콜리전 꺼짐 상태)이 리셋되지 않는 GameState의 퀘스트 러너를 덮어쓴다. 셀이 다시 로드되면 볼륨은 패키지에서 새로 만들어져 콜리전이 다시 켜진다.

**영향**: 진행 중인 메인 퀘스트가 첫 스텝으로 돌아가고, 완료한 퀘스트도 다시 시작돼 대화와 GiveRewards(Gold)가 반복된다. 현재 에셋에서 드러나는 경로지만 맵 끝까지 가야 생겨 🟡로 둔다.

**수정 제안**: 볼륨 인스턴스(또는 BP_QuestVolume CDO)의 Is Spatially Loaded를 끈다. "같은 에셋이 돌고 있으면 무시" 같은 코드 보강만으로는 완료 후 재수주를 막지 못한다.

**확신도**: 중간 — 그래프와 로드 설정은 에디터 없이 문자열로 확인했다. 셀이 내려가는 거리는 10-04 계산을 따랐고 플레이로 확인하지 않았다.

### D22. 🟡 극한 회피 전환이 면역 콜백 안에서 무적 GE를 즉시 걷어 같은 투사체나 다른 공격에 곧바로 맞는다

**발생 조건**: 서버에서 회피 무적 중 첫 피해가 면역에 막혀 극한 회피가 성립한 직후, 다음 애님 갱신 전에 공격이 또 닿는다. 이번에 두 번째 공격자 없이 재현되는 경로를 확인했다. BP_Template의 Pattern_2 투사체(BP_Template_Projectile)를 맞기 직전에 회피하면, 투사체가 판정 캡슐에 먼저 닿아 극한 회피가 성립한 뒤 같은 스윕이나 다음 이동 한 걸음에서 몸 메시와 새로 겹친다.

**판단 근거**: 10-04 보고서 C19(미반영)를 이번 묶음 리뷰(B05_Abilities_B-01)가 다시 찾았고, 검증에서 투사체 한 발로 재현되는 경로를 새로 확인했다. e9dbe2a28 이후 MontageEvents는 SlowTime 생성 인자 한 줄만 바뀌어 관련 경로는 그대로다. 면역 콜백([WxAbility_Dodge.cpp:165-172](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Dodge.cpp#L165)) 안에서 Success 섹션을 재생하면, 베이스 `PlayMontageInternal`이 이전 MontageEventsTask를 끝내고 [WxAbilityTask_MontageEvents.cpp:174-183](../../../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp#L174)이 무적 GE를 즉시 걷는다. 투사체는 오버랩마다 무적을 새로 읽고, 회피여도 [WxProjectileBase.cpp:205-209](../../../../Source/WxGame/Weapons/WxProjectileBase.cpp#L205)에서 파괴 없이 반환하며 같은 액터를 거르지 않는다. AM_Shared_Dodge의 Success 섹션 무적 구간은 섹션 시작(예: SuccessForward 7.85~8.65초)에 있어 다음 애님 갱신에서야 다시 걸린다.

**원인**: 섹션 전환을 새 몽타주 재생으로 처리하는 베이스가 구간 GE를 즉시 회수한다. 투사체는 무적 대상을 통과시키면서 같은 대상을 다시 판정한다.

**영향**: 투사체를 정확한 타이밍에 피할수록 극한 회피 연출(SlowTime·Success)과 피격이 함께 일어나고, HitReact가 회피 몽타주를 끊어 회피 반격으로 이어지지 않는다. 현재 에셋(BP_Template의 Pattern_2)에서 드러날 수 있는 경로다. 여러 적에게 동시에 공격받을 때 다른 공격에 맞는 10-04의 경로도 그대로다.

**수정 제안**: Dodge 안에서만 처리한다. Success를 재생하기 전에 같은 무적 GE를 한 스택 걸어 두고, 다음 틱이나 EndAbility에서 그 스택만 걷는다. 투사체에 액터별 중복 제외를 넣는 것은 무적이 정상으로 끝난 뒤의 재판정까지 바꾸므로 근본 수정이 아니다.

**확신도**: 중간 — 무적 제거 경로, 투사체 통과, Success 무적 시각은 코드와 에셋으로 정해진다. 두 번째 오버랩이 다음 애님 갱신 전에 나는지는 스윕 길이와 몸 메시 위치에 달려 있고, 실행으로 확인하지 않았다.

### D23. 🟡 처형 중에도 대상의 GP 드레인·누적이 계속돼 그로기가 도중에 풀리거나 다시 걸린다

**발생 조건**: 그로기 구간의 마지막 1.54초 안에 앞잡을 시작하고, 처형이 대상을 죽이지 않을 때(Sandbag, 반사 GP 그로기, HP가 남은 적).

**판단 근거**: 10-04 보고서 C20(미반영)을 이번 묶음 리뷰(B05_Abilities_B-05)가 다시 찾았다. e9dbe2a28 이후 WxAbility_Groggy.cpp는 몽타주 폴링(10-04 C06 반영)만 바뀌었고 드레인은 그대로다. [WxAbility_Finisher.cpp:62-81](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Finisher.cpp#L62)은 State.FinisherReserved만 달고, [WxAbility_Groggy.cpp:152-171](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Groggy.cpp#L152)의 드레인은 처형과 무관하게 계속된다. [WxEffect_Damage.cpp:204-207](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L204)은 `!bIsGroggy`만 보고 처형 피해를 GP에 다시 쌓는다. 기획은 "앞잡 발동 중 적 스태거 증감이 멈춤"이다.

**원인**: 처형 예약 중 GP 변화를 멈추는 장치가 없다.

**영향**: 처형 도중 그로기가 풀렸다가 처형 피해 GP(86, MaxGP 50)로 다시 걸리고, 그 사이 BT 잠금이 풀린다. 처형 종료 ResetGP가 최종 상태를 정리하고, 처형이 대개 대상을 죽이는 지금 수치에서는 보이는 빈도가 낮다.

**수정 제안**: 누적은 204행 조건에 `!TargetASC->HasMatchingGameplayTag(State_FinisherReserved)`를 더한다. 드레인 정지는 "만료 기반 종료 재도입 금지" 결정을 지키면서, 처형이 대상을 잡기 전에 실패하는 경로까지 함께 설계한다.

**확신도**: 중간 — 코드 경로는 확인했다. 도중에 풀린 뒤 BT가 재개되면서 보이는 영향은 추론이다.

### D24. 🟡 플레이어 잔상이 표시 바디(메타휴먼)가 아니라 숨겨 둔 구동 메시(SKM_Quinn_Simple)를 복사한다

**발생 조건**: 메타휴먼 바디를 단 BP_HGTest가 회피해 GameplayCue.GhostTrail을 낼 때. Player/BP_Template은 Quinn이 곧 보이는 메시라 해당하지 않는다.

**판단 근거**: 10-04 보고서 C29(미반영)를 이번 묶음 리뷰(B03_AbilitySystem_Core-01)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. [WxCueNotify_GhostTrail.cpp:31-43](../../../../Source/WxGame/AbilitySystem/Cues/WxCueNotify_GhostTrail.cpp#L31)은 `GetMesh()`의 스킨 에셋과 포즈를 복사한다. BP_HGTest는 이중 메시 구조이고, [WxMetaHumanComponent.cpp:45-54](../../../../Source/WxGame/Character/WxMetaHumanComponent.cpp#L45)가 바디를 붙인 뒤 리더 메시를 숨긴다.

**원인**: 잔상 원본을 항상 `GetMesh()`로 잡는데, 이중 메시 구조에서는 이것이 보이는 메시가 아니다.

**영향**: BP_HGTest가 회피할 때마다 잔상이 실제 캐릭터와 체형이 다른 Quinn 마네킹 실루엣으로 나온다. 현재 에셋에서 늘 드러나지만 기능 영향은 없는 시각 결함이다.

**수정 제안**: 의도가 아니라면, 메타휴먼 바디가 있을 때 스킨 에셋은 바디 메시로 바꾸고 포즈 원본은 리더 메시를 그대로 쓴다. 본 이름 매핑 결과는 에디터에서 확인해야 한다. 의도된 스타일이면 주석으로 남긴다.

**확신도**: 중간 — 메시 구조는 코드와 에셋 문자열로 확인했다. 화면에서 체형 차이가 얼마나 보이는지와 시각 의도는 확인하지 않았다.

### D25. 🟡 궁극기 컷신 도중 접속한 클라는 시퀀스를 처음부터 끝까지 재생해 서버 세션 종료 뒤에도 입력이 묶인다

**발생 조건**: 다른 플레이어의 궁극기 컷신이 도는 중에 접속을 마치고 GameState 초기 복제를 받는다.

**판단 근거**: 10-04 보고서 C21(미반영)을 이번 묶음 리뷰(B10_Combat_Targeting-03)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. `FWxSkillCutsceneSession`에는 시작 시각이 없고, 초기 복제의 `OnRep_State`가 `bPlaying` 세션을 받으면 [WxSkillCutsceneComponent.cpp:206-215](../../../../Source/WxGame/Combat/WxSkillCutsceneComponent.cpp#L206)에서 Preparing으로 보내 처음부터 재생한다. 서버가 끝내도 [222-226행](../../../../Source/WxGame/Combat/WxSkillCutsceneComponent.cpp#L222)이 이미 재생 중인 로컬 장면을 끝까지 둔다.

**원인**: 늦게 받은 수신자는 경과 시간을 모르고, 서버 종료를 받아도 "이미 재생 중이면 끝까지" 규칙이 그대로 적용된다.

**영향**: 월드 정지가 풀린 뒤에도 그 접속자만 남은 컷신을 보며, 최대 시퀀스 길이만큼 이동·시점 입력이 막힌다.

**수정 제안**: 컴포넌트 `HasBegunPlay()` 이전의 `OnRep_State`가 진행 중인 세션을 받으면 Preparing으로 보내지 않고 세션만 받아들인다. `Session.Id == 0`으로 가르는 방법은 처음부터 접속해 있던 클라도 첫 컷신을 건너뛰게 만들어 쓰지 않는다. 고치지 않을 거라면 이 한계를 결정으로 기록한다.

**확신도**: 중간 — 코드 경로는 확인했다. 접속이 컷신 도중에 끝나야 하는 타이밍 조건이다.

### D26. 🟡 시간 배율이 "가장 최근 요청 우선"이라 컷신 전역 정지 중 시작된 슬로모션이 정지를 풀어 버린다

**발생 조건**: 스킬 컷신(0.001배)이 걸린 뒤 SlowTime 구간(퍼펙트 가드 리액션, 극한 회피 Success)이 시작될 때. 컷신 중에는 입력이 막히고 월드가 거의 멈추므로, 현실적인 경로는 같은 서버 프레임 경합 하나다(A의 궁극기 컷신과 B의 퍼펙트 가드).

**판단 근거**: 10-04 보고서 C28(미반영)을 이번 묶음 리뷰(B10_Combat_Targeting-04)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. [WxTimeDilationSubsystem.cpp:31-45](../../../../Source/WxGame/Combat/WxTimeDilationSubsystem.cpp#L31)의 `ApplyCurrentRequest`는 배율 값이 아니라 핸들이 가장 큰(가장 최근) 요청을 적용한다. 요청자는 컷신(0.001배)과 MontageEvents의 SlowTime 구간이다.

**원인**: 겹친 요청 사이의 우선순위를 배율 강도가 아니라 최근 순서로 정한다.

**영향**: 전원 월드 정지가 계약인 컷신 도중 월드가 슬로모션 배율로 풀려, 1초 안팎 동안 적과 플레이어가 움직인다.

**수정 제안**: `ApplyCurrentRequest`에서 활성 요청 가운데 가장 작은 배율을 적용한다.

**확신도**: 중간 — 선택 규칙은 코드로 정해진다. SlowTime 구간이 섹션 시작 지점에 있는지는 에디터에서 확인하지 않았다.

### D27. 🟡 교차 돌진의 짝 탐색이 KnownTasks만 훑어 예측 클라에서는 "상대 출발점 고정"이 성립하지 않는다

**발생 조건**: 원격 클라 플레이어가 GA_HGTest_Skill_2(분신 쪽으로 돌진)를 쓰고, 서버의 분신이 GA_Minion_Skill_2(주인 쪽으로 돌진)를 낸다. 분신 돌진이 클라 화면에서 먼저 시작된 경우다.

**판단 근거**: 10-04 보고서 C25(미반영)를 이번 묶음 리뷰(B07_AbilityTasks_Weapons-01)가 다시 찾았다. e9dbe2a28 이후 Rush는 `bUseControllerRotationYaw` 저장·복원 삭제(10-04 C40 반영)만 바뀌었다. [WxAbilityTask_Rush.cpp:64-79](../../../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_Rush.cpp#L64)의 `FindRush`는 KnownTasks만 훑는데, 엔진은 복제로 받은 모의 태스크를 TickingTasks에만 넣는다(GameplayTasksComponent.cpp:205-233). 그래서 클라는 분신의 Rush를 찾지 못해 [33-38행](../../../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_Rush.cpp#L33)의 목적지 고정과 [138-147행](../../../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_Rush.cpp#L138)의 짝 맺기를 하지 못한다.

**원인**: 짝 탐색이 로컬에서 활성화한 태스크 목록에만 기댄다.

**영향**: 예측 루트모션의 목적지가 서버와 달라 돌진 끝에 위치가 보정(스냅)된다. 스탠드얼론과 리슨 호스트에서는 드러나지 않는다.

**수정 제안**: 클라에서 KnownTasks에 없으면 상대 ASC의 `GetSimulatedTasks()`에서 Rush를 찾아 복제되는 `StartLocation`을 쓴다. `Target`은 복제되지 않으니 짝 판정은 상대 Rush가 있는지로 대신한다. 이것이 과하면 36행 주석에 서버 한정이라는 점이라도 적는다.

**확신도**: 중간 — KnownTasks 누락은 엔진 소스로 확인했다. 목적지가 실제로 갈리는지는 두 몽타주의 노티파이 순서와 지연에 달려 있다.

### D28. 🟡 위치 보정 재생이나 시뮬 프록시 착지가 착지 섹션 점프를 다시 걸어 몽타주가 Landing 처음으로 되감긴다

**발생 조건**: (a) 소유 클라가 착지 섹션이 있는 몽타주 중에 이미 Landing을 재생하고 있는데, 서버 기준으로 아직 Falling이던 시점의 보정이 온다. (b) 시뮬 프록시가 몽타주 복제로 이미 Landing에 들어간 뒤 Falling→Walking 전이가 늦게 일어난다.

**판단 근거**: 10-04 보고서 C23(미반영)을 이번 묶음 리뷰(B09-04)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. 엔진 `ClientAdjustPosition_Implementation`은 서버 이동 모드를 무조건 다시 적용한다. [WxCharacterMovementComponent.cpp:74-77](../../../../Source/WxGame/Character/WxCharacterMovementComponent.cpp#L74)의 착지 처리는 [99-105행](../../../../Source/WxGame/Character/WxCharacterMovementComponent.cpp#L99)에서 현재 섹션을 보지 않고 `JumpToSectionName(Landing)`을 부른다.

**원인**: 착지 섹션 전이가 멱등이 아닌데, 보정·재생 경로에서도 돈다.

**영향**: (a) 소유 클라의 Landing이 처음부터 다시 재생돼 로컬 몽타주가 서버보다 뒤처지고 후딜 진입이 늦어진다. (b) 시뮬 프록시에 잠깐 히치가 생긴다.

**수정 제안**: 루프 안에서 `MontageInstance->GetCurrentSection() != UWxAbilityBase::LandingSectionName`일 때만 점프한다.

**확신도**: 중간 — 엔진 경로는 확인했고, PIE로는 재현하지 않았다.

**2026-10-05 반영**: 이미 Landing 섹션을 재생 중인 몽타주는 다시 점프하지 않는다.

### D29. 🟡 클라 무브 재연(replay) 중에도 OnJumped가 불려 그 시점의 후딜 어빌리티를 취소한다

**발생 조건**: 소유 클라가 점프한 무브가 서버 ack를 받기 전에 위치 보정이 도착하고, 그 시점에 Recovery 단계인 액션이 있다. RTT가 크거나 보정이 잦을 때만 열리는 좁은 창이다.

**판단 근거**: 10-04 보고서 C22(미반영)를 이번 묶음 리뷰(B09-03)가 다시 찾았다. e9dbe2a28 이후 WxCharacterBase.cpp에는 KillZ 처리(`FellOutOfWorld`)만 더해졌고 `OnJumped_Implementation`은 그대로다. 엔진 `ClientUpdatePositionAfterServerUpdate`는 `bClientUpdating`으로 저장된 무브를 재생하고, 이때 `CheckJumpInput`이 `OnJumped()`를 다시 부른다(CharacterMovementComponent.cpp:13355-13370, Character.cpp:1591-1600). [WxCharacterBase.cpp:110-116](../../../../Source/WxGame/Character/WxCharacterBase.cpp#L110)은 여기서 `CancelRecoveringAbilities`를 부르고, 클라의 취소는 서버로 복제된다.

**원인**: OnJumped를 "실제로 막 점프했다"는 일회성 사건으로 쓰는데, 재연이 과거 무브를 다시 실행하면서 이 사건을 또 낸다.

**영향**: 드물게, 입력한 공격의 후딜이 이유 없이 클라와 서버 모두에서 끊긴다.

**수정 제안**: `OnJumped_Implementation`에서 Super를 부른 뒤 `if (bClientUpdating) { return; }`를 둔다.

**확신도**: 중간 — 엔진 경로는 확인했다. 실제로 일어나는지는 RTT와 보정 타이밍에 달려 있고, PIE로 재현하지 않았다.

**2026-10-05 반영**: `bClientUpdating`(저장된 무브 재연 중)이면 후딜을 끊지 않는다.

### D30. 🟡 원격 클라에서는 어빌리티 부여·제거가 슬롯 VM 재매칭을 부르지 않는다

**발생 조건**: 원격 클라의 로컬 플레이어에서 슬롯 VM이 생긴 뒤 어빌리티 스펙이 복제로 추가·제거되고, 그 뒤 그 ASC에 태그 변화가 없을 때.

**판단 근거**: 10-04 보고서 C26(미반영)을 이번 묶음 리뷰(B16_UI_MVVM_A-01)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. 재매칭은 [WxViewModel_AbilitySystem.cpp:22](../../../../Source/WxGame/UI/MVVM/WxViewModel_AbilitySystem.cpp#L22)에서 엔진 `AbilitySpecDirtiedCallbacks`에만 걸려 있는데, 엔진은 이 통지를 권위에서만 낸다(AbilitySystemComponent_Abilities.cpp:1008-1023). 클라에서는 [WxViewModel_Ability.cpp:251-254](../../../../Source/WxGame/UI/MVVM/WxViewModel_Ability.cpp#L251)의 태그 변화 처리가 우연히 대신해 줄 뿐이다.

**원인**: 슬롯 재매칭이 서버에서만 나오는 엔진 통지에 걸려 있다.

**영향**: 원격 클라에서 첫 스펙 복제보다 HUD가 먼저 구성되면 처음 행동하기 전까지 스킬 슬롯이 빈다. 런타임 부여·회수가 생기면 회수된 아이콘이 남거나 새 스킬이 안 뜬다. 지금은 부여가 빙의 시점이라 거의 드러나지 않는다.

**수정 제안**: `UWxAbilitySystemComponent`에서 `OnGiveAbility`/`OnRemoveAbility`(서버·클라 모두 호출)를 Super 뒤에 재정의하고, 클라일 때 `AbilitySpecDirtiedCallbacks`를 방송한다. [WxViewModel_AbilitySystem.h:25](../../../../Source/WxGame/UI/MVVM/WxViewModel_AbilitySystem.h#L25) 주석도 실제 동작에 맞춘다.

**확신도**: 중간 — 엔진 분기는 확인했다. HUD가 스펙보다 먼저 뜨는 순서가 실제로 생기는지는 PIE로 확인하지 않았다.

### D31. 🟡 클라의 「연결 장치 미로드」 분기가 LinkedDevices의 빈 칸·파괴된 장치까지 열어 서버와 답이 갈린다

**발생 조건**: `bOnlyWhenLinkedAccepts` 장치(ST_Button 계열)의 LinkedDevices에 null 칸이 있거나 연결 장치가 런타임에 파괴됐을 때, 원격 클라에서.

**판단 근거**: 10-04 보고서 C27(미반영)을 이번 묶음 리뷰(B11_Device-01)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다(WxAbility_Interact.cpp는 생성자 주석 한 줄만 바뀌었다). [WxDevice.cpp:41-45](../../../../Source/WxGame/Device/WxDevice.cpp#L41)는 `IsValid`에 실패하면 클라에서만 빈 선택지를 넣고, 서버는 같은 칸을 건너뛴 뒤 [WxAbility_Interact.cpp:83-88](../../../../Source/WxGame/Interaction/WxAbility_Interact.cpp#L83)에서 같은 함수로 다시 검증한다. 이것은 [WxInteractable.h:40](../../../../Source/WxGame/Interaction/WxInteractable.h#L40)의 "클라 표시 게이트와 서버 발동 검증이 같은 답을 받는다" 계약에 어긋난다.

**원인**: `IsValid` 실패를 모두 「클라에 아직 안 들어온 장치」로 해석해 클라에서만 열어 둔다.

**영향**: null 칸이 있는 버튼은 원격 클라에서만 프롬프트가 뜨고, 눌러도 서버 검증에서 조용히 버려진다.

**수정 제안**: 41-45행 분기를 지워 무효 칸은 서버처럼 건너뛴다. 이 분기는 09-21에 사용자가 정한 「미로드 연결 장치는 열어 둔다」의 구현이다. 서로 참조하는 액터는 월드 파티션이 같은 클러스터로 묶어 미로드가 사실상 생기지 않는다는 점과 함께 확인받고, 열어 두는 방침을 유지한다면 null 칸만이라도 분기에서 뺀다.

**확신도**: 중간 — 불일치는 코드만으로 정해진다. 현재 배치에 null 칸이 실제로 있는지는 바이너리로 판정하지 못했다.

### D32. 🟡 새 게임 진입이 맵 로드 단계에서 실패하면 실패 문구 없이 프론트엔드로 돌아온다

**발생 조건**: `RequestNewGame`의 사전 검사(`DoesPackageExist`)를 통과한 목적지 패키지가 출발 월드를 내린 뒤 로드에 실패한다(손상·버전 불일치 등). 현재 에셋에서는 사실상 일어나지 않는 드문 경로다.

**판단 근거**: 묶음 리뷰(B14_Spawner_Misc-02)가 찾았다. 엔진 `UnrealEngine.cpp:15990-16000·16111-16123`에서 실패한 LoadMap은 `PostLoadMapWithWorld(nullptr)`을 보내고, 이것은 [WxGameFlowSubsystem.cpp:93](../../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L93)이 거른다. 이어 `BrowseToDefaultMap`이 프론트엔드를 로드하면 [97-101행](../../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L97)의 비목적지 분기가 `PendingLevel`을 비운다. 그다음 실패 방송이 오면 [115행](../../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L115)의 `!IsBusy()`가 참이라 문구 없이 돌아간다. 10-05 C58 반영 주석([119행](../../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L119))도 이 경로에서는 사실과 다르다.

**원인**: 실패 처리의 게이트가 `IsBusy()`인데, 엔진 폴백 맵 로드가 실패 방송보다 먼저 와서 그 상태를 지운다.

**영향**: 새 게임 진입이 실패해도 안내 없이 메뉴로 돌아온다. 체크포인트는 이미 초기화돼 있다.

**수정 제안**: `HandleTravelFailure`에서 `!IsBusy()` 게이트를 빼고 GameInstance 일치만 본다. 비목적지 분기에서 실패 문구를 세우는 방법은 쓰지 않는다. WBP_MainMenu의 OpenLevel로 정상 복귀할 때도 그 분기를 타기 때문이다. 119행 주석도 실제 순서에 맞게 고친다.

**확신도**: 중간 — 호출 순서는 엔진 소스로 확정되지만, 존재하는 패키지가 로드에 실패하는 상황은 재현하지 않았다.

**2026-10-05 반영**: `HandleTravelFailure`의 `IsBusy()` 게이트를 빼고 GameInstance 일치만 본다. 주석도 실제 순서에 맞게 고쳤다.

### D33. 🟡 큐 방식 즉시 AbilityEvent 노티파이가 같은 애님 갱신 안의 콤보 전환에 묻혀 서버에서 버려질 수 있다

**발생 조건**: 즉시 노티파이와 ComboWindow 시작이 한 번의 몽타주 Advance 안에 같이 지나가야 한다. 현재 에셋의 간격은 AM_HGTest_Attack_1_LLLL(SpawnMinion → ComboWindow) 0.129초, AM_HGTest_Attack_DodgeCounter2 0.371초다. 그래서 정상 프레임에서는 생기지 않고, 서버 프레임이 0.13초 넘게 튈 때만 생긴다.

**판단 근거**: 묶음 리뷰(B07_AbilityTasks_Weapons-02)가 찾았다. [WxAnimNotify_AbilityEvent.h:11](../../../../Source/WxGame/Animation/WxAnimNotify_AbilityEvent.h#L11) 이하의 즉시 노티파이는 분기점 플래그가 없고, 상태 노티파이만 [.cpp:41](../../../../Source/WxGame/Animation/WxAnimNotify_AbilityEvent.cpp#L41)에서 분기점으로 고정한다. 엔진 `AnimMontage.cpp:2847-2877`은 큐 노티파이를 쌓은 뒤 상태 분기점을 동기로 처리한다. ComboWindow가 시작되면 [WxAbilityTask_MontageEvents.cpp:229](../../../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp#L229) 이하에서 다음 단이 바로 재생되고 이전 태스크가 끝난다. 나중에 처리되는 큐 노티파이는 옛 MontageInstanceID를 싣고 와서 새 태스크의 [OwnsMontageSignal](../../../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp#L80)이 거른다.

**원인**: 즉시 노티파이만 Queued로 남아, 동기로 처리되는 상태 분기점보다 늦게 처리된다.

**영향**: 그 단의 분신 소환(같은 방식으로 저작한 투사체·범위 피해도)이 서버에서 빠진다.

**수정 제안**: `UWxAnimNotify_AbilityEvent` 생성자에서 `bIsNativeBranchingPoint = true`로 둔다. 상태 노티파이와 같은 근거이고 `BranchingPointNotify` 경로도 이미 있다.

**확신도**: 중간 — 엔진 처리 순서와 필터 동작은 소스로 확인했다. 실제로 드러나려면 서버 히치가 필요하다.

### D34. 🟡 AdditionalEffects가 피해 0인 히트와 이번 타격으로 죽은 대상에도 추가 효과 GE를 적용한다

**발생 조건**: AdditionalEffects가 있는 피해 행이 (a) 대상을 죽이거나 (b) 가드 경감 등으로 최종 피해 0으로 끝날 때. 지금 DT_Damage에는 AdditionalEffects를 쓰는 행이 없다.

**판단 근거**: 10-04 보고서 C30(미반영)을 이번 묶음 리뷰(B06_Effects-01)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. [WxEffectComponent_AdditionalEffects.cpp:20-23](../../../../Source/WxGame/AbilitySystem/Effects/WxEffectComponent_AdditionalEffects.cpp#L20)은 퍼펙트 가드만 거르고, 같은 GE의 [WxEffectComponent_HitStop.cpp:19-24](../../../../Source/WxGame/AbilitySystem/Effects/WxEffectComponent_HitStop.cpp#L19)처럼 IncomingDamage 기록 > 0을 성립 조건으로 보지 않는다. [WxCombatAttributeSet.cpp:109-116](../../../../Source/WxGame/AbilitySystem/Attributes/WxCombatAttributeSet.cpp#L109)이 이 컴포넌트보다 먼저 Event.Death를 보내므로, 적용 시점에는 사망 어빌리티가 이미 돌고 있다.

**원인**: 형제 컴포넌트가 쓰는 "성립한 히트" 조건과 대상의 Ability.Death를 보지 않는다.

**영향**: 추가 효과 행이 생기면 시체에 상태이상 GE가 걸리고, 타격 연출·반응이 없는 0 피해 히트에 상태이상만 남는다. 현재 에셋에서는 드러나지 않는다.

**수정 제안**: 20행 가드 옆에 HitStop과 같은 피해 기록 조건과 대상의 `Ability_Death` 확인을 둔다. 피해 0에도 거는 것이 의도라면 헤더에 그 계약을 적는다.

**확신도**: 중간 — 코드 경로는 확인했다. 피해 0 갈래는 의도일 가능성을 배제하지 못했다.

### D35. 🟡 CameraMove 임시 카메라 수명이 애니메이션 시간 기준이라 재생이 느려지면 구간 도중에 파괴된다

**발생 조건**: 구간의 실제 길이가 `TotalDuration + BlendOutTime + 1초`를 넘을 때(몽타주 재생 속도가 1 미만이거나 오너 CustomTimeDilation이 1초 넘게 낮아짐). 지금 처형 몽타주는 재생 속도 1이고 히트스톱도 걸리지 않는다.

**판단 근거**: 10-04 보고서 C32(미반영)를 이번 묶음 리뷰(B08_Animation_Minion-02)가 다시 찾았다. e9dbe2a28 이후 관련 코드는 바뀌지 않았다. [WxAnimNotifyState_CameraMove.cpp:79](../../../../Source/WxGame/Animation/WxAnimNotifyState_CameraMove.cpp#L79)의 `SetLifeSpan`은 월드 타이머라 전역 배율만 따르고, 몽타주 재생 속도와 액터 배율은 따르지 않는다. 반면 `TotalDuration`은 애니메이션 시간이다.

**원인**: 누락 대비용 수명을 애니메이션 시간으로 잡았다.

**영향**: 조건이 성립하면 뷰 타깃 카메라가 사라져 시점이 폰으로 툭 튄다. 현재 에셋에서는 드러나지 않는다.

**수정 제안**: 수명 여유를 고정 수 초로 넉넉히 둔다. 정상 경로에서는 NotifyEnd가 시점을 되돌린다.

**확신도**: 중간 — 코드 경로는 확인했다. 현재 에셋에서는 드러나지 않는다.

### D36. 🟡 BeginPlay 전에 해석된 Auto 스포너에 TriggerSpawners가 오면 적이 둘 생기고 하나는 추적을 잃는다

**발생 조건**: TriggerSpawners가 Auto 스포너를 지정하고, 그 스포너 셀이 로드됐지만 아직 BeginPlay 전일 때 스텝에 들어간다. 지금 퀘스트가 지정한 스포너 두 개는 모두 Manual이라 현재 에셋에서는 드러나지 않는다.

**판단 근거**: 묶음 리뷰(B14_Spawner_Misc-03)가 찾았다. UOL 해석은 엔진 `WorldPartitionLevelStreamingPolicy.cpp:285-307`에서 `GetLoadedLevel()`만 요구하므로 BeginPlay 전 셀 액터도 찾는다. 그러면 [WxStateTreeTask_TriggerSpawners.cpp:41](../../../../Source/WxGame/Spawner/WxStateTreeTask_TriggerSpawners.cpp#L41) 이하 → `Respawn`이 적 A를 만들고, 이어 [WxSpawner.cpp:111](../../../../Source/WxGame/Spawner/WxSpawner.cpp#L111)의 Auto 분기가 적 B를 만들어 [159행](../../../../Source/WxGame/Spawner/WxSpawner.cpp#L159)에서 `SpawnedActor`를 덮어쓴다. 5da76fe51(10-05, C50)이 지운 `SpawnedActor` 가드가 이 순서를 막고 있었다.

**원인**: Auto 스포너의 BeginPlay 스폰이 이미 추적 중인 인스턴스를 확인하지 않는다.

**영향**: 적이 하나 더 생기고, A는 이후 Respawn·EndPlay가 치우지 않는다. 둘 다 같은 처치 통지에 묶여 하나만 죽여도 처치 상태가 설 수 있다.

**수정 제안**: `BeginPlay`의 Auto 분기에 "추적 중인 인스턴스가 없을 때만" 조건을 둔다. TriggerSpawners를 BeginPlay를 마친 스포너로 한정하는 방법은 쓰지 않는다. 그 사이에 온 트리거를 Manual 스포너가 잃어 적이 나오지 않는다.

**확신도**: 중간 — UOL 해석 경로는 엔진 소스로 확인했고, 셀 로드 창과 스텝 진입이 겹치는 타이밍은 추론이다.

**2026-10-05 반영**: `BeginPlay`의 Auto 분기를 추적 중인 인스턴스가 없을 때로 한정했다.

### D37. 🟡 한 번의 획득이 여러 슬롯에 나뉘면 획득 토스트가 슬롯 수만큼 갈라져 뜬다

**발생 조건**: 한 번의 지급이 여러 슬롯에 나뉠 때다. 기존 부분 스택을 채우고 남거나, MaxStack을 넘거나, 스택이 안 되는 아이템을 2개 이상 줄 때 생긴다. 현재 에셋에서는 드러나지 않는다. 보상은 DA_Gold(MaxStack 10,000,000)뿐이고, 비스택 아이템은 HUD가 생기기 전 시작 아이템으로만 지급된다.

**판단 근거**: 묶음 리뷰(B17_UI_MVVM_B_Indicator-01)가 찾았다. [WxViewModel_Inventory.cpp:83](../../../../Source/WxGame/UI/MVVM/WxViewModel_Inventory.cpp#L83) 이하는 통지 한 번에 토스트 VM 하나를 만든다. 그런데 [WxInventoryComponent.cpp:276](../../../../Source/WxGame/Inventory/WxInventoryComponent.cpp#L276)·[292행](../../../../Source/WxGame/Inventory/WxInventoryComponent.cpp#L292)은 슬롯마다 통지하고, 클라 복제 경로([70행](../../../../Source/WxGame/Inventory/WxInventoryComponent.cpp#L70) 이하)도 엔트리마다 통지한다.

**원인**: 획득 토스트는 `OnInventoryStackChanged`를 획득 한 건으로 보는데, 이 통지는 슬롯 변경 단위로 나간다.

**영향**: 한 번 얻었는데 토스트가 여러 장 뜨고, 첫 장 수량은 일부만 반영된다.

**수정 제안**: 서버 경로는 `AddItemDefinition`에서 정의 단위 통지를 루프 뒤 합계로 한 번 보낸다. 클라 경로는 같은 콜백 배치 안에서 정의별로 합산해 한 번 보낸다.

**확신도**: 중간 — 발행 경로는 코드로 확정되고, 에셋 값은 에디터가 아니라 바이너리에서 읽었다.

### D38. 🟡 부활로 파괴된 폰을 겨누던 보스가 교전 목록에서 빠지지 않아 보스 HP바가 남을 수 있다

**발생 조건**: 단독 플레이에서 `Character.Boss` 적이 플레이어를 겨누는 중 그로기로 BT가 멈추고, 그 사이 플레이어가 죽어 그로기가 끝나기 전에 부활한다. 이어 BT 서비스가 다시 돌기 전에 GC가 지나가야 한다. `Character.Boss` 태그를 쓰는 에셋이 없어 현재 에셋에서는 드러나지 않는다.

**판단 근거**: 저장·부활 횡단 리뷰(X4_Save_Respawn-01)가 찾았고, 전투 묶음 리뷰(B10)도 범위 밖 의심점으로 같은 가능성을 적었다. [WxLockOnComponent.cpp:50](../../../../Source/WxGame/Targeting/WxLockOnComponent.cpp#L50)은 같은 값이면 방송하지 않는다. 부활이 옛 폰을 파괴하면 GC가 그 폰을 가리키던 `LockOnTarget`을 null로 지운다(엔진 `GarbageCollection.cpp` EliminateGarbage). 이어지는 `SetLockOnTarget(nullptr)`은 null==null이라 방송 없이 끝나, [WxEnemyCharacter.cpp:128](../../../../Source/WxGame/Character/WxEnemyCharacter.cpp#L128)의 `RefreshEngagement`가 돌지 않는다. 그래서 `UWxBattleSubsystem`의 교전 목록에 보스가 남는다. 그로기가 없으면 BT 서비스가 0.1초 안에 사망을 보고 대상을 비워 방송이 나가므로, BT 일시정지가 조건에 들어간다.

**원인**: 교전 목록이 `LockOnTarget` 변경 방송만 따라가는데, 대상이 GC로 무효가 되는 경로에는 방송이 없다. [WxLockOnComponent.h:50](../../../../Source/WxGame/Targeting/WxLockOnComponent.h#L50)은 이런 상태를 질의로 읽으라고 적어 두었다.

**영향**: 보스 HP바가 그 보스가 다시 교전했다가 해제될 때까지 남는다.

**수정 제안**: 헤더 계약대로 읽는 쪽에서 질의한다. `UWxBattleSubsystem::GetCurrentBoss`나 교전 목록을 고르는 곳에서 `AWxEnemyCharacter::IsEngaged()`를 다시 확인한다. 비교식만 바꾸는 방법은 GC 뒤 `LockOnTarget`도 null이라 효과가 없다. 보스 에셋이 생길 때 다루면 된다.

**확신도**: 중간 — GC의 가비지 참조 제거와 BT 일시정지 중 서비스 정지라는 엔진 동작 추론에 기대고, PIE로 재현하지 않았다.

### D39. 🟡 KillZ로 죽은 적의 처치 보상 픽업이 KillZ 아래에 생성돼 곧바로 파괴된다

**발생 조건**: 서버에서 적이 KillZ 아래로 떨어지고, 그 적의 보상에 `UWxItemFragment_Pickup` 아이템이 있다. 현재 에셋에는 Pickup 프래그먼트를 가진 아이템이 없어 모든 보상이 직접 지급되므로 드러나지 않는다.

**판단 근거**: GAS 횡단 리뷰(X3_GAS_Flow-02)가 찾았다. KillZ 사망은 10-05 수정(019324c8b)으로 사망 경로를 타고, [WxEnemyCharacter.cpp:150](../../../../Source/WxGame/Character/WxEnemyCharacter.cpp#L150)이 `GetActorTransform()`에 보상을 준다. [WxRewardLibrary.cpp:74](../../../../Source/WxGame/Inventory/WxRewardLibrary.cpp#L74) 이하가 픽업을 물리로 띄우지만 약 46cm만 오른다. 엔진 물리 동기화가 `CheckStillInWorld`를 불러 Z<KillZ인 픽업을 파괴한다(`Actor.cpp:2321-2325·3373-3382`).

**원인**: 보상 위치가 사망 순간의 액터 위치인데, KillZ 사망에서는 그 위치가 이미 월드 밖이다.

**영향**: 픽업형 보상이 생기면 처치 수와 퀘스트 진행은 오르지만 드랍이 사라진다. 019324c8b 전에는 처치도 드랍도 없었으므로 새로 잃는 것은 없다.

**수정 제안**: 픽업 아이템을 도입할 때, KillZ 사망이면 `GrantReward`의 직접 지급 경로로 돌리거나 보상을 생략한다.

**확신도**: 중간 — 픽업 파괴는 엔진 물리 동기화 경로를 읽어 추론했고 PIE로 확인하지 않았다.

### D40. 🟡 CameraMove 노티파이가 에디터 프리뷰 컴포넌트를 강참조해, 닫은 Persona 프리뷰 월드가 해제되지 않는다

**발생 조건**: AM_Shared_Finisher나 AM_Shared_BackstabFinisher를 애님 에디터에서 열어 CameraMove 구간을 재생·스크럽한 뒤 에디터를 닫는다. 에디터에서만 생기고, 노티파이 인스턴스 하나당 최대 한 월드만 붙잡는다.

**판단 근거**: 묶음 리뷰(B08_Animation_Minion-03)가 찾았다. [WxAnimNotifyState_CameraMove.h:70](../../../../Source/WxGame/Animation/WxAnimNotifyState_CameraMove.h#L70) 이하는 프리뷰 컴포넌트를 UPROPERTY(Transient)로 강참조하고, [.cpp:124](../../../../Source/WxGame/Animation/WxAnimNotifyState_CameraMove.cpp#L124)는 Outer를 월드로 만든다. GC는 Outer를 강참조로 따라가고(`FastReferenceCollector.h:1019`), `PreviewScene.cpp:111-170`의 `Uninitialize`는 월드를 garbage로 표시하지 않은 채 GC를 돌린다. 노티파이 인스턴스는 몽타주 에셋의 서브오브젝트라 에디터 세션 동안 산다.

**원인**: 에셋 수명 객체가 프리뷰 월드 수명 객체를 강참조한다.

**영향**: 닫은 프리뷰 월드 하나(FScene·프리뷰 액터 포함)가 해제되지 않는다. 게임플레이에는 영향이 없다.

**수정 제안**: `FWorldDelegates::OnWorldCleanup`에서 해당 월드의 컴포넌트를 놓는다. 또는 컴포넌트를 프리뷰 액터의 인스턴스 컴포넌트로 붙여 월드가 수명을 쥐게 하고, 노티파이는 `TWeakObjectPtr`로 가리킨다.

**확신도**: 중간 — GC의 Outer 강참조와 프리뷰 월드 해제 경로는 엔진 소스로 확인했고, 메모리 잔류는 측정하지 않았다.

### D41. 🟢 체크포인트 SaveGame이 세션을 넘어 남아 프론트엔드를 거치지 않은 시작에서 지난 체크포인트로 부활한다

**판단 근거**: 10-04 보고서 C51(미반영)을 이번 묶음 리뷰(B09-07)가 다시 찾았고, 디스크에 남는 고정 슬롯([WxCheckpointSaveGame.cpp:24-38](../../../../Source/WxGame/Save/WxCheckpointSaveGame.cpp#L24))을 지우는 곳은 지금도 [WxGameFlowSubsystem.cpp:58-62](../../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L58)의 새 게임 경로뿐이라 PIE로 맵을 바로 실행할 때만 드러난다.

**수정 제안**: 이어하기를 전제하지 않는다면 선택 없이 열린 게임 맵에서도 초기화하거나 GameInstance 수명 값으로 바꾼다.

**확신도**: 높음 — 동작은 코드로 정해지고 수정 여부만 결정 대기다.

### D42. 🟢 자막 유지 시간이 게임 시간으로 흐르는데 단위가 명시돼 있지 않다

**판단 근거**: 10-04 보고서 C45(미반영)를 이번 묶음 리뷰(B17_UI_MVVM_B_Indicator-03)가 다시 찾았고, [WxStateTreeTask_PrintSubtitle.cpp:47](../../../../Source/WxGame/UI/Subtitle/WxStateTreeTask_PrintSubtitle.cpp#L47)은 전역 배율이 적용된 DeltaTime을 누적하는데 [WxSubtitleTableRow.h:26](../../../../Source/WxGame/UI/Subtitle/WxSubtitleTableRow.h#L26)은 "유지할 시간(초)"라고만 적는다.

**영향**: 슬로우·컷신 정지 동안 자막이 늘어나지만, 지금은 이 태스크를 쓰는 StateTree가 없다.

**수정 제안**: 게임 시간이 맞으면 Duration 주석에 "게임 시간(초)"를 적고, 실시간이 맞으면 전역 배율로 나눠 누적한다.

**확신도**: 높음 — 동작은 코드로 정해지고 의도만 미결이다.

### D43. 🟢 `.cpp`에 익명 namespace 8곳과 static 자유 함수 6개가 남아 있다

**판단 근거**: 10-04 보고서 C52(미반영)를 기계 검사와 묶음 리뷰 8건(B03_AbilitySystem_Core-03, B05_Abilities_B-07, B06_Effects-05, B08_Animation_Minion-04, B10_Combat_Targeting-05, B11_Device-04, B12_Dialogue_Interaction_Quest-03, B14_Spawner_Misc-05)이 다시 찾았고, 위치는 아래와 같다.
- 익명 namespace: [WxAbility_Finisher.cpp:16](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Finisher.cpp#L16)(워프 타깃 이름 상수), [WxHitStopComponent.cpp:11](../../../../Source/WxGame/AbilitySystem/WxHitStopComponent.cpp#L11)(배율 상수), [WxAnimNotifyState_SnapToTarget.cpp:13](../../../../Source/WxGame/Animation/WxAnimNotifyState_SnapToTarget.cpp#L13)(워프 타깃 이름 상수 2개), [WxAnimNotify_AbilityEvent.cpp:9](../../../../Source/WxGame/Animation/WxAnimNotify_AbilityEvent.cpp#L9)(`GetSignalReceiver`·`MakePayload` 헬퍼), [WxSkillCutsceneComponent.cpp:26](../../../../Source/WxGame/Combat/WxSkillCutsceneComponent.cpp#L26)(`FWxSkillCutsceneClock` 클래스), [WxDeviceStateTreeComponent.cpp:12](../../../../Source/WxGame/Device/WxDeviceStateTreeComponent.cpp#L12)(`RootInitialStateName` 상수), [WxStateTreeTask_WaitForInteraction.cpp:11](../../../../Source/WxGame/Interaction/WxStateTreeTask_WaitForInteraction.cpp#L11)(전역 대기 등록부와 핸들 카운터), [WxStateTreeTask_WaitSpawnersKilled.cpp:11](../../../../Source/WxGame/Spawner/WxStateTreeTask_WaitSpawnersKilled.cpp#L11)(검사 주기 상수).
- static 자유 함수: [WxEffect_Damage.cpp:60](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L60) `GetDamageStatics`, [:66](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L66) `CalculateDefenseMultiplier`, [:72](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L72) `CalculateBaseDamage`, [:77](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L77) `CalculateCriticalMultiplier`, [:87](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L87) `CalculateGuardMultiplier`, [:92](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L92) `CalculateFinalDamage`.
- 참고: [WxSpawner.cpp:20](../../../../Source/WxGame/Spawner/WxSpawner.cpp#L20)의 이름 있는 namespace `WxSpawnerLabel`도 파일 내부 전용 상수 묶음이다.

**수정 제안**: 상수는 지역 상수나 클래스 static 멤버로, 헬퍼는 호출부 인라인이나 private 멤버로, WaitForInteraction의 전역 대기 등록부는 태스크 클래스의 private static 멤버로 옮긴다. `GetDamageStatics`는 GAS ExecCalc 관용 패턴(Lyra와 같음)이라 예외로 둘지 판단이 필요하다.

**확신도**: 높음 — 기계 검사로 전수 확인했다.

### D44. 🟢 ProjectileBase.h의 PostNetReceiveVelocity가 IGenericTeamAgentInterface 구역 안에 들어가 있다

**판단 근거**: 10-04 보고서 C41(미반영)을 이번 묶음 리뷰(B07_AbilityTasks_Weapons-05)가 다시 찾았고, [WxProjectileBase.h:52-57](../../../../Source/WxGame/Weapons/WxProjectileBase.h#L52)의 인터페이스 구역 안에 AActor 오버라이드인 [56행](../../../../Source/WxGame/Weapons/WxProjectileBase.h#L56)이 그대로 섞여 있다.

**수정 제안**: 56행을 `//~ End IGenericTeamAgentInterface` 뒤로 옮겨 AActor 구역으로 묶는다.

**확신도**: 높음 — 선언 위치만의 문제다.

**2026-10-05 반영**: `PostNetReceiveVelocity`를 인터페이스 구역 밖으로 옮겼다.

### D45. 🟢 가드 자세 복구가 LoopStart 섹션 재생 실패를 처리하지 않는다

**판단 근거**: 묶음 리뷰(B05_Abilities_B-03)가 찾았다. [WxAbility_Guard.cpp:106](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Guard.cpp#L106)이 `PlayMontage` 반환값을 버린다. 섹션이 없으면 [WxAbilityBase.cpp:366](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L366) 이하가 경고 로그를 남기고 false를 돌려준다. 현재 쓰는 AM_Shared_Guard에는 LoopStart가 있어 저작 오류일 때만 생긴다.

**영향**: 플레이어는 키를 뗄 때까지 자세 없이 가드가 유지된다. AI 가드는 BT가 놓으므로 영구히 남지 않는다.

**수정 제안**: false면 `EndAbility`로 끝낸다. 섹션 없이 다시 재생하는 방법은 C17 퍼펙트 가드 창 문제를 되살리므로 쓰지 않는다.

**확신도**: 높음 — 반환값을 무시하는 것이 코드로 정해진다.

**2026-10-05 반영**: LoopStart 재생이 실패하면 `EndAbility`로 끝낸다.

### D46. 🟢 BeyondLeash·AttributeRatio·RandomWeight 데코레이터 설명이 Super를 빼서 BT 그래프에 중단 모드·inversed 표시가 나오지 않는다

**판단 근거**: 묶음 리뷰 두 곳(B01_AI_Behavior-03, B02_AI_Combat-02)이 보고했다. [WxBTDecorator_BeyondLeash.cpp:45](../../../../Source/WxGame/AI/WxBTDecorator_BeyondLeash.cpp#L45), [WxBTDecorator_AttributeRatio.cpp:18](../../../../Source/WxGame/AI/WxBTDecorator_AttributeRatio.cpp#L18), [WxBTDecorator_RandomWeight.cpp:13](../../../../Source/WxGame/AI/WxBTDecorator_RandomWeight.cpp#L13)이 `Super::GetStaticDescription()`을 붙이지 않는다. 엔진 `BTDecorator.cpp:122-145`가 "( aborts …, inversed )"를 만들고, 같은 폴더의 [ObserveAbility](../../../../Source/WxGame/AI/WxBTDecorator_ObserveAbility.cpp#L28)는 이를 앞에 붙인다. BeyondLeash는 BT_Soldier·BT_Template에서 실제로 쓰이며 중단 모드가 폴링 여부를 정한다.

**수정 제안**: 세 데코레이터 모두 ObserveAbility처럼 `Super::GetStaticDescription()`을 앞에 붙인다. 런타임 영향은 없다.

**확신도**: 높음 — 엔진 기본 설명과 같은 폴더의 대조군으로 확인했다.

**2026-10-05 반영**: 세 데코레이터 모두 `Super::GetStaticDescription()`을 앞에 붙였다.

### D47. 🟢 AI 컨트롤러 OnPossess 주석이 촉각(Damage 센스)도 리스너 팀 캐시로 피아를 가른다고 잘못 적는다

**판단 근거**: [WxAIController.cpp:80](../../../../Source/WxGame/AI/WxAIController.cpp#L80). 엔진 `AISense_Damage.cpp:81-98`은 팀 확인 없이 자극을 등록한다. 프로젝트의 촉각은 Damage 센스이고, [WxAIBehaviorComponent.cpp:178](../../../../Source/WxGame/AI/WxAIBehaviorComponent.cpp#L178) 이하는 엔진이 가해자를 가려 주지 않아 직접 거른다고 적는다.

**수정 제안**: 80행을 "청각의 피아 판정이 그 캐시를 쓰므로"로 고친다.

**확신도**: 높음 — 엔진 Damage 센스에 팀 비교가 없다.

**2026-10-05 반영**: 주석에서 촉각을 뺐다.

### D48. 🟢 MirrorAbility가 INIT 매크로가 이미 켠 알림 플래그를 다시 켜고, 중단 시 CleanUp을 두 번 한다

**판단 근거**: [WxBTTask_MirrorAbility.cpp:20](../../../../Source/WxGame/AI/WxBTTask_MirrorAbility.cpp#L20)-21행은 `INIT_TASK_NODE_NOTIFY_FLAGS`(엔진 `BTTaskNode.h:128-142`)가 재정의 여부로 켜는 값을 다시 쓴다. 프로젝트 BT 태스크 중 이 파일만 그렇다. [AbortTask](../../../../Source/WxGame/AI/WxBTTask_MirrorAbility.cpp#L216)와 [OnTaskFinished](../../../../Source/WxGame/AI/WxBTTask_MirrorAbility.cpp#L222)가 모두 CleanUp을 부르는데, 엔진은 정상적인 중단·정지·파괴 경로에서 둘 다 부른다.

**수정 제안**: 20-21행을 지운다. AbortTask 재정의도 지워도 되지만, 컴포넌트가 이미 무효인 경우를 막으려고 남긴 것이라면 둬도 동작 차이는 없다.

**확신도**: 높음 — 매크로 정의와 엔진 호출 경로로 확인했다.

**2026-10-05 반영**: 수동 플래그 대입 두 줄을 지웠다. AbortTask 재정의는 방어 목적일 수 있어 남겼다.

### D49. 🟢 WxKillEnemies 주석이 지워진 처치 수 집계(KillCount)를 거르는 이유로 든다

**판단 근거**: [WxCheatManager.cpp:98](../../../../Source/WxGame/Development/WxCheatManager.cpp#L98). `KillCount`와 집계 로그는 ad3aeec00에서 지워졌고, 지금은 대상마다 처치 로그만 남긴다.

**수정 제안**: "헛도는 GE와 이미 죽은 대상의 처치 로그를 막기 위해 거른다"로 고친다.

**확신도**: 높음 — 파일에 `KillCount`가 없다.

**2026-10-05 반영**: 주석을 「헛도는 GE와 처치 로그를 막기 위해」로 고쳤다.

### D50. 🟢 ASPD 어트리뷰트 주석이 모든 어빌리티 몽타주의 PlayRate라고 적지만 콤보 계열만 쓴다

**판단 근거**: [WxCombatAttributeSet.h:107](../../../../Source/WxGame/AbilitySystem/Attributes/WxCombatAttributeSet.h#L107). `GetMontagePlayRate`는 [WxAbility_Combo.cpp:15](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Combo.cpp#L15) 이하만 ASC 값을 쓰고 베이스는 1을 돌려준다. [WxAbilitySystemComponent.h:59](../../../../Source/WxGame/AbilitySystem/WxAbilitySystemComponent.h#L59)는 같은 사실을 바르게 적는다.

**수정 제안**: "콤보 어빌리티(UWxAbility_Combo 계열) 몽타주 PlayRate로 쓰는 공격 속도 배율(기본 1.0)"로 고친다.

**확신도**: 높음 — PlayRate 소비처를 전수 검색했다.

**2026-10-05 반영**: 주석을 콤보 계열 전용으로 고쳤다.

### D51. 🟢 사망 어빌리티 생성자 주석이 취소 범위와 반응 처리를 실제와 다르게 적는다

**판단 근거**: 묶음 리뷰(B04_Abilities_A-05)가 찾았고 GAS 횡단 리뷰(X3_GAS_Flow)가 보강했다. [WxAbility_Death.cpp:23](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Death.cpp#L23)은 "진행 중인 것은 액션만 끊는다"인데 26-28행은 Action·Sprint·LockOn을 취소한다. 같은 주석의 "활성 반응은 사망 몽타주가 밀어내며 끝나고"도 GA_Shared_Death에 몽타주가 없어 곧바로 래그돌로 넘어가고, 52f51fd45(10-05) 뒤로는 사망 연출이 처형 짝 연출이 끝날 때까지 미뤄지기도 해 맞지 않는다.

**수정 제안**: "진행 중인 것은 액션·질주·락온만 끊는다 — 반응은 취소하지 않는다"로 고치고, 반응이 끝나는 경로 설명은 현재 동작에 맞게 줄인다.

**확신도**: 높음 — 바로 아래 취소 태그 목록과 에셋 목록(사망 몽타주 없음)으로 확인했다.

**2026-10-05 반영**: 취소 범위를 액션·질주·락온으로 고치고, 사망 몽타주가 반응을 밀어낸다는 문장을 지웠다.

### D52. 🟢 Rush의 SourceMontageInstanceID 재대입이 같은 값을 다시 넣는다

**판단 근거**: [WxAbilityTask_Rush.cpp:103](../../../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_Rush.cpp#L103). 98행에서 이 ID로 찾은 인스턴스의 `GetInstanceID()`를 같은 변수에 다시 넣는다.

**수정 제안**: 103행을 지운다.

**확신도**: 높음.

**2026-10-05 반영**: 103행을 지웠다.

### D53. 🟢 IsRestoring 주석이 서버의 InitialState 복원을 빠뜨린다

**판단 근거**: [WxDeviceStateTreeComponent.h:61](../../../../Source/WxGame/Device/WxDeviceStateTreeComponent.h#L61)-63행. 서버는 [.cpp:56](../../../../Source/WxGame/Device/WxDeviceStateTreeComponent.cpp#L56)에서 `EnterState(InitialState, true)`로 복원 플래그를 세운다.

**수정 제안**: "트리 시작(재시작 포함), 서버의 InitialState 적용, 클라가 스냅샷을 따라 요청한 복원 전이"로 고친다.

**확신도**: 높음 — 코드와 주석 차이가 분명하다.

**2026-10-05 반영**: 서버가 시작 직후 InitialState로 요청한 전이를 복원 목록에 넣었다.

### D54. 🟢 UseItem 헤더 주석이 지워진 「인벤토리의 사용 요청」 경로를 설명한다

**판단 근거**: [WxAbility_UseItem.h:12](../../../../Source/WxGame/Inventory/WxAbility_UseItem.h#L12). 5da76fe51(10-05, C47)이 `RequestUseConsumable`을 지웠고, 지금 `Ability.Action.UseItem` 에셋 태그를 쓰는 곳은 HitReact의 취소 태그와 WBP_ItemQuickSlot뿐이다.

**수정 제안**: 문장을 지우거나 "UI 슬롯은 AssetTag(Ability.Action.UseItem)로 이 어빌리티를 지목하며, 발동 경로는 입력과 같다"로 고친다.

**확신도**: 높음 — C++·주석에 남은 참조가 이 한 줄뿐이다.

**2026-10-05 반영**: 지워진 경로를 설명하던 문단을 지웠다.

### D55. 🟢 인벤토리 헤더에 쓰이지 않는 전방 선언과 중복 include가 남아 있다

**판단 근거**: `class UTexture2D;`가 [WxItemInstance.h:11](../../../../Source/WxGame/Inventory/WxItemInstance.h#L11), [WxItemFragment.h:15](../../../../Source/WxGame/Inventory/WxItemFragment.h#L15), [WxItemDefinition.h:12](../../../../Source/WxGame/Inventory/WxItemDefinition.h#L12)에 남아 있지만 아이콘 필드는 모두 `TSoftObjectPtr<UObject>`다. [WxInventoryComponent.h:14](../../../../Source/WxGame/Inventory/WxInventoryComponent.h#L14)의 `class UWxInventoryComponent;`는 그 아래에서 쓰이지 않는다. [WxItemPickup.cpp:18](../../../../Source/WxGame/Inventory/WxItemPickup.cpp#L18)은 헤더가 이미 포함한 `Interaction/WxInteractable.h`를 다시 포함한다.

**수정 제안**: 지운다.

**확신도**: 높음.

**2026-10-05 반영**: 쓰이지 않는 전방 선언 4개와 중복 include를 지웠다.

### D56. 🟢 위젯 push 비동기 액션의 SetBeforePushCallback 경로에 호출자가 없다

**판단 근거**: [WxAsyncAction_PushWidgetToLayer.h:34](../../../../Source/WxGame/UI/Foundation/WxAsyncAction_PushWidgetToLayer.h#L34)-35·61행, [.cpp:62](../../../../Source/WxGame/UI/Foundation/WxAsyncAction_PushWidgetToLayer.cpp#L62)-65·120·161-162행. 2d0871b41(09-15)에서 마지막 호출자가 사라졌다. 10-02 리뷰 Q7이 삭제를 제안했지만 10-04 보고서에는 옮겨지지 않았다.

**수정 제안**: `SetBeforePushCallback`과 `BeforePushCallback` 멤버·실행·해제를 지운다. BP용 `BeforePush`/`AfterPush` 델리게이트는 유지한다.

**확신도**: 높음 — Source 전체에 호출이 없다.

**2026-10-05 반영**: `SetBeforePushCallback`과 `BeforePushCallback` 멤버·실행·해제를 지웠다.

### D57. 🟢 상호작용 어빌리티의 State.Dialogue 차단 주석이 서버·클라 공통 판정처럼 읽히지만, 그 태그는 소유 머신에만 붙는다

**판단 근거**: 네트워크 횡단 리뷰(X1_Network-02)가 찾았다. `State.Dialogue`는 소유 클라 RPC 본문([WxDialogueSessionComponent.cpp:160](../../../../Source/WxGame/Dialogue/WxDialogueSessionComponent.cpp#L160))에서 비복제 loose 태그로 붙는다. 그래서 원격 클라에 대한 서버 차단은 걸리지 않는다. 그런데 [WxAbility_Interact.cpp:29](../../../../Source/WxGame/Interaction/WxAbility_Interact.cpp#L29)는 소유 태그 조건을 서버·클라가 같은 검사로 판정한다고 적고, [35행](../../../../Source/WxGame/Interaction/WxAbility_Interact.cpp#L35)은 발행 머신을 밝히지 않는다. 실제 차단은 클라 표시 게이트가 맡아 겉으로 드러나는 오동작은 없다.

**수정 제안**: 35행 주석에 "소유 머신 로컬 태그라, 원격 클라의 대화 중 차단은 클라 표시 게이트가 맡는다"를 덧붙인다.

**확신도**: 높음 — 태그를 올리고 내리는 곳과 엔진 기본 복제 상태(None)로 확인했다.

**2026-10-05 반영**: 35행 주석에 소유 머신 로컬 태그라 원격 클라의 대화 중 차단은 클라 표시 게이트가 맡는다고 적었다.

### D58. 🟢 DamageReaction 83행 주석의 "일반 반응으로 보낸다"가 27c4ff568 뒤의 동작과 맞지 않는다

**판단 근거**: GAS 횡단 리뷰(X3_GAS_Flow-03)가 찾았다. 같은 히트의 GP로 그로기가 뜨면 [WxEffectComponent_DamageReaction.cpp:60](../../../../Source/WxGame/AbilitySystem/Effects/WxEffectComponent_DamageReaction.cpp#L60)-65행이 반응 태그를 비우고, 빈 태그는 HitReact가 거른다. 그래서 [83행](../../../../Source/WxGame/AbilitySystem/Effects/WxEffectComponent_DamageReaction.cpp#L83)이 말하는 경우에는 Event.Hit만 나가고 반응은 일어나지 않는다.

**수정 제안**: 83행을 "가드를 먼저 끊었으면 브레이크가 아닌 Event.Hit으로 보낸다(반응 태그는 위에서 비웠다)"로 고친다.

**확신도**: 높음 — 동작에는 영향이 없다.

**2026-10-05 반영**: 제안대로 고쳤다.

### D59. 🟢 프론트엔드 상태 문구 6개가 한국어라 인게임 문구 영어 규칙과 어긋난다

**판단 근거**: 오케스트레이터가 줄 번호를 확인하다 발견했고 별도 검증자가 확정했다. [WxGameFlowSubsystem.cpp:41](../../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L41)·[48](../../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L48)·[55](../../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L55)·[60](../../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L60)·[80](../../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L80)·[122](../../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L122)행의 `StatusText`는 [WxFrontEndLibrary.cpp:85](../../../../Source/WxGame/UI/Frontend/WxFrontEndLibrary.cpp#L85)의 `GetTravelStatus`를 거쳐 WBP_FrontEnd에 표시된다. 문구는 09-05·09-07에 들어왔고, 인게임 문구를 영어로 쓴다는 결정(09-22)이 나온 뒤에도 바뀌지 않았다. 다른 인게임 문구(`Talk to {0}`, `Finisher`)는 영어다.

**수정 제안**: 키는 그대로 두고 문구만 영어로 바꾼다. 예: "Starting a new game is only available in single player.", "Level travel failed: {0}".

**확신도**: 높음 — 표시 경로와 결정 기록으로 확인했다.

**2026-10-05 반영**: 문구 6개를 영어로 바꿨다. LOCTEXT 키는 그대로다.

### D60. 🟢 포즈를 유지하는 사망 몽타주에서는 메시 틱 승격이 시체가 사라질 때까지 되돌아가지 않는다

**발생 조건**: GA_Shared_Death(또는 파생)에 포즈를 유지하는 사망 몽타주를 지정할 때이고, 지금은 몽타주가 없어 잠재 상태다.

**판단 근거**: 10-04 보고서 C53(미반영)을 이번 묶음 리뷰(B03_AbilitySystem_Core-02, B04_Abilities_A-04)가 다시 찾았고(10-04 C03 반영의 사망 대기 추가는 이 경로와 무관하다), 틱 복원은 지금도 [WxAbilitySystemComponent.cpp:118](../../../../Source/WxGame/AbilitySystem/WxAbilitySystemComponent.cpp#L118)의 `OnAllMontageInstancesEnded`에만 걸려 있는데 [WxAbility_Death.cpp:87-93](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Death.cpp#L87)은 `ClearAnimatingAbility`만 부른다.

**원인**: 틱 복원 조건이 「모든 몽타주 인스턴스 종료」로 바뀌었는데, 사망 어빌리티의 정리와 [WxAbility_Death.h:40](../../../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Death.h#L40) 주석은 AnimatingAbility 해제로 되돌리던 옛 방식 그대로다.

**영향**: 시체 메시가 `AlwaysTickPoseAndRefreshBones`로 남아, CorpseLifeSpan 기본값 0이면 시체가 사라질 때까지 화면 밖에서도 본을 갱신한다.

**수정 제안**: `HandleDeathMontageElapsed`에서 지금 private인 `RestoreAnimatingMontageMeshTick()`을 부를 수 있게 열어 한 번 부르고, 헤더 주석을 그 동작으로 고친다.

**확신도**: 중간 — 코드 경로는 확인했고, 현재 에셋에서는 드러나지 않는다.

### D61. 🟢 부활 시 스트리밍 완료 대기가 일괄 리스폰보다 먼저라 막 로드된 셀의 적이 같은 프레임에 두 번 생성된다

**판단 근거**: 10-04 보고서 C68(미반영)을 이번 묶음 리뷰(B09-06)가 다시 찾았고, [WxRespawnLibrary.cpp:79-80](../../../../Source/WxGame/Player/WxRespawnLibrary.cpp#L79)은 여전히 `BlockTillLevelStreamingCompleted`로 새 셀의 Auto 스포너가 적을 만들게 한 뒤 `RespawnAll`로 그 적을 파괴하고 다시 만든다.

**수정 제안**: `RespawnAll`을 대기 앞으로 옮기되, 이득이 작을 수 있으니 측정한 뒤 정한다.

**확신도**: 중간 — 실제 프레임 비용은 측정하지 않았다.

### D62. 🟢 UWxEffect_Exceed와 짝 큐·태그는 파생 에셋이 삭제돼 도달할 수 없는 죽은 코드다

**판단 근거**: 10-04 보고서 C55(미반영)를 이번 묶음 리뷰(B03_AbilitySystem_Core-04, B06_Effects-04)가 다시 찾았고, GE_Exceed가 e0e3ecc51(09-09)에서 삭제된 뒤 Abstract인 [WxEffect_Exceed.h:9-16](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Exceed.h#L9)과 `AWxCueNotify_Exceed`([WxCueNotify_Exceed.cpp:12](../../../../Source/WxGame/AbilitySystem/Cues/WxCueNotify_Exceed.cpp#L12)), GC_Exceed, `GameplayCue.Exceed` 태그를 쓰는 곳이 지금도 없다.

**영향**: 살아 있는 기능으로 오해하기 쉽고, 효과 목록에서 ASPD를 바꾸는 GE도 이것 하나뿐이다.

**수정 제안**: 되살릴 계획이 없으면 클래스·큐·태그·GC_Exceed를 한꺼번에 지우고, 되살린다면 큐의 스폰을 WhileActive로 옮기고 멱등하게 만든다.

**확신도**: 중간 — 미사용은 확실하지만 템플릿으로 남긴 의도인지는 기록에 없다.

### D63. 🟢 에셋에서 쓰이지 않는 장치 태스크 4종

**판단 근거**: 10-04 보고서 C64(미반영)를 이번 묶음 리뷰(B11_Device-03)가 다시 찾았고, EnablePlayerInput·PlayLevelSequence·PlayMontageOnce·PlaySound 태스크([WxStateTreeTask_PlaySound.h](../../../../Source/WxGame/Device/WxStateTreeTask_PlaySound.h#L1) 등)는 지금도 Content·Config 어디서도 참조되지 않는다.

**영향**: 검증되지 않은 경로가 남는다(D11의 PlayMontageOnce 태스크가 그 예다).

**수정 제안**: 4종을 지우거나, 디자이너 팔레트로 의도했다면 남긴다(`UWxAbility_PlayMontageOnce`는 처형과 적이 쓰므로 대상이 아니다).

**확신도**: 중간 — 미사용은 확실하지만 팔레트로 남긴 의도인지는 기록이 없다.

### D64. 🟢 히트스톱 GE 적용의 AnimatingAbility 조회와 Context.SetAbility를 읽는 곳이 없다

**판단 근거**: [WxEffect_HitStop.cpp:31](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_HitStop.cpp#L31)-34행. 985aabe9c(09-14)에서 예측 키를 없앤 뒤 출처 기록만 남았다. Source에서 컨텍스트의 `GetAbility`를 읽는 곳은 코스트 MMC([WxEffect_Cost.cpp:40](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Cost.cpp#L40))뿐이다.

**수정 제안**: 31·34행과 `Abilities/GameplayAbility.h` include를 지우고 `MakeEffectContext()` 결과를 그대로 쓴다.

**확신도**: 중간 — C++ 소비처가 없음은 전수 검색했고, BP에서 읽는지는 확인하지 않았다(C++ 전용 GE라 가능성은 낮다).

**2026-10-05 반영**: AnimatingAbility 조회·`SetAbility`와 쓰지 않게 된 include를 지웠다.

### D65. 🟢 상태 GE 8종의 AssetTags 컴포넌트에 조회처가 없다

**판단 근거**: [WxEffect_Exhaust.cpp:20](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Exhaust.cpp#L20), [WxEffect_GuardReduction.cpp:19](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_GuardReduction.cpp#L19), [WxEffect_IgnoreAbilityActivationTags.cpp:18](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_IgnoreAbilityActivationTags.cpp#L18), [WxEffect_IgnoreCosts.cpp:18](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_IgnoreCosts.cpp#L18), [WxEffect_Invincible.cpp:36](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_Invincible.cpp#L36), [WxEffect_PerfectGuard.cpp:19](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_PerfectGuard.cpp#L19), [WxEffect_SkillCutscene.cpp:27](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_SkillCutscene.cpp#L27), [WxEffect_SuperArmor.cpp:25](../../../../Source/WxGame/AbilitySystem/Effects/WxEffect_SuperArmor.cpp#L25). Source에 GE 애셋 태그 질의가 없고, 각 `Effect.*` 태그의 사용처는 모두 부여 태그 확인이다. 2e937bd57(10-05, C56)은 같은 이유로 IgnoreAggro의 AssetTags를 지웠다.

**수정 제안**: C56과 같은 기준으로 8곳의 AssetTags 컴포넌트와 include를 지운다. 관례로 남길 거면 IgnoreAggro와 기준을 맞춘다.

**확신도**: 중간 — C++ 조회처가 없음은 확인했고, BP·ST 그래프의 애셋 태그 질의는 에디터로 확인하지 않았다.

### D66. 🟢 무기 BeginAttack 주석의 "복제하지 않는 차일드 액터 무기" 전제가 실제 구성과 다르다

**판단 근거**: [WxWeaponBase.cpp:51](../../../../Source/WxGame/Weapons/WxWeaponBase.cpp#L51). 같은 파일 [15행](../../../../Source/WxGame/Weapons/WxWeaponBase.cpp#L15)이 `bReplicates = true`이고 BP_Katana·BP_MinionKatana에 덮어쓴 값이 없다. 엔진 `ChildActorComponent.cpp`는 클래스가 복제되면 비권위 쪽에서 서버 복제본을 받는다.

**수정 제안**: 51행을 지운다(50행의 "권위 머신에서만"은 유지).

**확신도**: 중간 — C++·BP 구성은 확인했고, 캐릭터 BP의 ChildActorTemplate 값은 열어 보지 않았다.

**2026-10-05 반영**: 51행을 지웠다.

### D67. 🟢 FellOutOfWorld의 「래그돌 시체가 떨어져 다시 불린다」 설명이 실제 재호출 경로와 다르다

**판단 근거**: [WxCharacterBase.cpp:125](../../../../Source/WxGame/Character/WxCharacterBase.cpp#L125). KillZ 판정(엔진 `Actor.cpp:2326`)은 루트인 캡슐 위치를 보고, 래그돌은 메시만 시뮬레이션한다([278행](../../../../Source/WxGame/Character/WxCharacterBase.cpp#L278)에서 캡슐은 NoCollision). 실제 재호출은 KillZ 아래 남은 캡슐 때문에 서버 CMC 틱(`CharacterMovementComponent.cpp:1709-1714`)이 매 프레임 `CheckStillInWorld`를 부르는 데서 생긴다.

**수정 제안**: "KillZ 아래에 남은 캡슐 때문에 이동 컴포넌트가 매 틱 다시 부르므로 아직 살아 있을 때만 사망시킨다"로 고친다.

**확신도**: 중간 — 엔진 호출 경로는 소스로 확인했고, PIE로 호출 빈도를 찍지는 않았다.

**2026-10-05 반영**: 제안대로 고쳤다.

### D68. 🟢 InputDirection 정렬의 "머신 간 판정이 일치한다" 설명은 서버와 소유 클라 사이에만 맞다

**판단 근거**: [WxTargetingSorterTask_InputDirection.h:13](../../../../Source/WxGame/Targeting/WxTargetingSorterTask_InputDirection.h#L13). 엔진 `p.EnableCharacterAccelerationReplication` 기본값이 0이라 시뮬 프록시는 가속을 받지 않는데, 이 정렬은 SnapToTarget 회전 폴백으로 시뮬 프록시에서도 돈다. 같은 헤더 18행은 "이 판정은 머신마다 갈릴 수 있다"고 따로 인정한다.

**수정 제안**: "서버와 소유 클라이언트가 같은 값을 쓴다(시뮬 프록시는 가속을 받지 않는다)"로 범위를 좁힌다.

**확신도**: 중간 — 엔진 CVar 기본값은 확인했다.

**2026-10-05 반영**: 범위를 서버와 소유 클라이언트로 좁히고 시뮬 프록시는 가속을 받지 않는다고 적었다.

### D69. 🟢 대상이 없는 대화에서도 포즈 몽타주를 비동기 로드하고 실제 원인과 다른 경고를 남긴다

**판단 근거**: [WxStateTreeTask_PlayDialogue.cpp:49](../../../../Source/WxGame/Dialogue/WxStateTreeTask_PlayDialogue.cpp#L49)는 대상 없이 대화를 연다. [WxDialogueSessionComponent.cpp:307](../../../../Source/WxGame/Dialogue/WxDialogueSessionComponent.cpp#L307) 이하 `ApplyCurrentPose`는 대상을 보지 않고 TargetPose를 로드하고, 로드가 끝나면 [362행](../../../../Source/WxGame/Dialogue/WxDialogueSessionComponent.cpp#L362)이 "애님 인스턴스 없음" 경고를 찍는다. 지금 DT_Dialogue에서 TargetPose를 쓰는 행이 퀘스트용인지는 확인하지 못했다.

**수정 제안**: `ApplyCurrentPose` 첫머리에서 `CurrentTarget`이 없으면 돌아간다.

**확신도**: 중간 — 코드 경로는 확실하고, 현재 데이터에서 실제로 일어나는지는 확인하지 못했다.

### D70. 🟢 「다음 퀘스트 시작」 StateTree 태스크와 지연 활성화 경로를 쓰는 에셋이 없다

**판단 근거**: `Content/` 아래 .uasset에 `WxStateTreeTask_StartNextQuest` 참조가 없다. [RequestActivateQuest](../../../../Source/WxGame/Quest/WxQuestComponent.cpp#L39)와 [HandleDeferredActivateQuest](../../../../Source/WxGame/Quest/WxQuestComponent.cpp#L125)를 부르는 곳은 이 태스크([WxStateTreeTask_StartNextQuest.cpp](../../../../Source/WxGame/Quest/WxStateTreeTask_StartNextQuest.cpp))뿐이다. 지금 퀘스트는 볼륨으로만 수주된다.

**수정 제안**: 디자이너 팔레트로 둘지 지울지 정한다. 메모리 project_quest_failed_clears_journal과 헤더가 이 태스크를 재진입 경로로 언급하므로, 남기는 쪽이 의도에 가까울 수 있다.

**확신도**: 중간 — 미사용은 확실하고, 팔레트로 남긴 것인지는 기록이 없다.

### D71. 🟢 WBP_Inventory가 HUD 루트 클래스 UWxHUDLayout을 부모로 둔다

**판단 근거**: `Content/UI/Widget/WBP_Inventory.uasset`의 부모가 `/Script/WxGame.WxHUDLayout`이다. 646188f96(08-19) 전까지는 `WxActivatableWidget`이었다. [WxHUDLayout.h:11](../../../../Source/WxGame/UI/WxHUDLayout.h#L11) 이하는 이 클래스를 "게임 플레이 중 항상 활성화되는 HUD 루트"로 정의하고, WBP_Inventory는 메뉴 레이어에 push된다.

**영향**: 지금은 HUD 액션 바인딩 4개가 Game 모드라 Menu 모드인 인벤토리에서 매칭되지 않아 기능 영향이 없다. InputMode를 바꾸면 ESC·Alt·B가 인벤토리 안에서 HUD처럼 동작한다.

**수정 제안**: WBP_Inventory의 부모를 `UWxActivatableWidget`로 되돌린다(에셋 변경).

**확신도**: 중간 — 부모 클래스와 이력은 확인했다.

### D72. 🟢 ObserveWidgetForGamePause의 "CommonUI 풀에서 재사용" 주석이 현재 경로와 다르다

**판단 근거**: [WxUIManagerSubsystem.cpp:125](../../../../Source/WxGame/UI/Subsystem/WxUIManagerSubsystem.cpp#L125). 이 함수로 오는 위젯은 두 경로 모두 매번 `CreateWidget`으로 새로 만들고, 레이아웃은 풀을 쓰지 않는 `AddWidgetInstance`로만 넣는다.

**수정 제안**: "같은 인스턴스가 다시 push돼도 중복 구독하지 않도록"으로 고치거나 지운다.

**확신도**: 중간 — 두 호출 경로와 엔진 컨테이너 코드로 확인했다.

**2026-10-05 반영**: 「같은 인스턴스가 다시 push돼도 중복 구독하지 않도록」으로 고쳤다.

### D73. 🟢 Attribute 뷰모델 리졸버를 쓰는 에셋이 없다

**판단 근거**: [WxViewModel_Attribute.h:51](../../../../Source/WxGame/UI/MVVM/WxViewModel_Attribute.h#L51) 이하, [.cpp:85](../../../../Source/WxGame/UI/MVVM/WxViewModel_Attribute.cpp#L85) 이하의 리졸버를 참조하는 .uasset이 `Content/`에 없다. bfb526f5c(09-30)로 어트리뷰트 VM 공급이 컨버전 함수 경로로 옮겨진 뒤 남았다. 묶음 리뷰가 함께 올린 Ability 리졸버 미사용 주장은 WBP_ItemQuickSlot도 쓰고 있어 철회했다.

**수정 제안**: Attribute 리졸버를 지운다.

**확신도**: 중간 — 에셋 문자열 검색으로 확인했다.

### 기획 확인 필요

| ID | 확신도 | 문제 | 수정 제안 |
| --- | --- | --- | --- |
| D04 | 높음 | Skill_3의 범위 피해 노티파이에 DamageDataRow가 비어 피해가 없다. | Skill_3에 범위 피해를 둘지, 두려면 어느 DT_Damage 행을 쓸지 정한다. 아니면 노티파이를 지운다. |
| D05 | 높음 | 체크포인트 휴식의 아이템 충전이 쉰 플레이어가 아니라 리슨 호스트에게 간다. | 리필 대상을 쉰 플레이어로 할지, 접속한 전원으로 할지 정한다. 어느 쪽이든 지금 동작은 맞지 않는다. |
| D06 | 높음 | 가드 리액션 중 재입력에 퍼펙트 가드 창이 열리지 않는다. | 리액션 도중 재입력에 퍼펙트 가드 창을 줄지, 준다면 재입력 시점에 열지 리액션이 끝날 때 열지 정한다. |
| D07 | 높음 | GuardHit 섹션 시작 약 38ms에 입력 없는 퍼펙트 가드 창이 있다. | 연속 타격 보호 등 의도된 구간인지 정한다. 의도가 아니면 구간을 지운다. |
| D12 | 높음 | 다단 스킬 2단의 커밋이 실패하면 진행 중인 1단이 끊긴다. | 다단 스킬의 쿨다운·비용을 단마다 걸지, 첫 단에만 걸지 정한다. 어느 쪽이든 다음 단은 커밋 확인 실패 시 입력만 무시하게 고친다. |
| D23 | 중간 | 처형 중에도 대상의 GP 드레인·누적이 계속된다. | 드레인을 멈출 때 그로기 남은 시간을 어떻게 다룰지 정한다. 누적은 FinisherReserved 조건으로 먼저 막을 수 있다. |
| D24 | 중간 | 회피 잔상이 Quinn 마네킹 실루엣으로 나온다. | 실제 캐릭터 외형이어야 하는지, 마네킹 실루엣이 의도된 스타일인지 정한다. |
| D34 | 중간 | 추가 효과가 피해 0 히트와 죽은 대상에도 걸린다. | 가드로 완전히 막힌 히트와 그 타격으로 죽은 대상에도 상태이상을 걸지 정한다. |
| D39 | 중간 | 픽업형 보상을 도입하면 KillZ 사망의 보상이 KillZ 아래에서 사라진다. | KillZ 사망 보상을 직접 지급할지 생략할지 정한다. |
| D41 | 높음 | 체크포인트 저장이 세션을 넘어 남는다. | 이어하기를 전제로 체크포인트를 세션 너머로 이어 쓸지 정한다. |
| D42 | 높음 | 자막 유지 시간이 게임 시간으로 흐른다. | 자막 표시 시간이 슬로우·컷신 정지에 함께 늘어나야 하는지 정한다. |
| D62 | 중간 | Exceed 계열이 도달할 수 없는 죽은 코드다. | Exceed 버프를 폐기했는지, 다시 도입할 계획인지 정한다. |
| D63 | 중간 | 장치 StateTree 태스크 4종을 쓰는 에셋이 없다. | 디자이너 팔레트로 남길지 지울지 정한다. |
| D70 | 중간 | 「다음 퀘스트 시작」 태스크와 지연 활성화 경로를 쓰는 에셋이 없다. | 디자이너 팔레트로 남길지 지울지 정한다. |

### 기각·판정 보류

- 하지 않은 것: PIE·멀티플레이 실행 재현은 하지 않았다. 네트워크·타이밍 항목은 엔진 소스 추론에 기댄다. 빌드와 경고 확인은 미실행이다. BP 그래프·몽타주 노티파이·레벨 배치는 에디터로 열지 않았고, .uasset 문자열·태그 파싱과 `Saved/AbilitySystemLists/` 목록으로만 확인했다(Unreal MCP는 이번 세션에서 연결되지 않았다). 성능은 측정하지 않았다. 10-04 보고서에서 넘어온 29건은 관련 파일이 e9dbe2a28 이후 바뀌지 않았는지 git diff로 확인해 이전 검증을 이어받았다. 파일이 바뀐 5건(D03·D13·D29·D27·D60)과 새 재현 경로가 나온 D22만 다시 검증했다.

| 후보 ID | 판정 | 후보 | 사유 |
| --- | --- | --- | --- |
| B09-05 (10-04 C04) | 기각 | 적 ASC Minimal 복제로 원격 클라에서 적 네임플레이트 이펙트 목록이 빈다 | 지난 보고 뒤 사용자가 1508c1632(10-05 「ASC replication mode 수정」)로 Minimal을 커밋해 결정된 동작이다. |
| B02_AI_Combat-04 (10-04 C63) | 기각 | ObserveAbility가 PerActor 이외 정책의 발동을 비활성으로 읽는다 | InstancedPerExecution에서만 드러나는 결함이고 그 정책은 쓰지 않는다(조치 대상 아님 결정). |
| B09-02 | 기각 | KillZ로 죽은 캐릭터의 래그돌이 끝없이 떨어지며 시뮬레이션된다 | 10-05 019324c8b·de83ad4df가 비파괴와 부활·스포너 정리를 택하며 물리 정지를 일부러 뺐다. 남는 영향은 보이지 않는 미미한 비용이다. |
| X3_GAS_Flow-01 | 기각 | 처형 짝 연출 완주 대기가 소환 미니언의 짧은 시체 수명과 어긋난다 | BP_Minion은 주인(플레이어) 팀을 물려받아 처형 대상이 될 수 없고, 시체 수명을 줄인 다른 에셋도 없다. |
| B03_AbilitySystem_Core-06 | 기각 | 어빌리티 세트 부여의 권위 검사가 호출 사슬에서 반복된다 | 공개 함수의 방어 검사(GE·속성 초기화 포함)이고 줄여도 이득이 거의 없다. |
| B08_Animation_Minion-05 | 기각 | ReportNoise HearingDistance에 하한(ClampMin)이 없다 | 0 이하 = 제한 없음은 엔진 순정 의미이고 현재 에셋은 모두 기본값 300이다. |
| B12_Dialogue_Interaction_Quest-05 | 기각 | 10-04 C66 반영 주석이 재진입 StopLogic 갈래에는 맞지 않는다 | 엔진 두 갈래 중 지금 호출부가 쓰지 않는 쪽에만 틀리고 동작 영향이 없다. |
| B15_UI_Core-07 | 기각 | PlayerLayoutComponent 클래스 주석의 정리 대상에 사망 화면이 들어간다 | 표현이 모호할 뿐이고 cpp 108행에 사망 화면 책임 설명이 이미 있다. |
| B16_UI_MVVM_A-02 (Ability 리졸버 부분) | 기각 | Ability 리졸버가 유일한 사용처에서 늘 nullptr을 돌려준다 | WBP_ItemQuickSlot도 이 리졸버를 쓰고 그 태그 설정은 확인되지 않았다. Attribute 리졸버 부분만 확정했다(D73). |
| B16_UI_MVVM_A-03 | 기각 | 「억제 해제도 같은 핸들로 추가 통지를 보낸다」 주석이 엔진 소스와 다르다 | 엔진 소스에는 그 경로가 없지만 f8c7cb9c4 기록은 중복을 실측했다고 적는다. 같은 핸들 중복 방지는 무해한 방어 코드다. |
| B16_UI_MVVM_A-04 | 기각 | 최대치 생략 규칙이 Attribute VM Initialize에 한 번 더 있다 | public 진입점의 한 줄 방어이고 고쳐도 이득이 거의 없다. |
| B17_UI_MVVM_B_Indicator-02 | 판정 보류 | MarkIndicator 헤더의 TargetLocation 「자동 기록」 설명이 퀘스트 파라미터 경로에서는 성립하지 않아, 좌표를 비우면 월드 원점을 가리킬 수 있다 | 헤더는 인스턴스 직접 편집 경로에서는 맞다. Main1/2 퀘스트 스텝의 실제 TargetLocation 값(에디터 확인)이 없어 결함 여부를 정하지 못했다. |
