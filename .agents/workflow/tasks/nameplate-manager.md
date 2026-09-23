# Nameplate를 로컬 NameplateManager가 붙이고 떼는 구조로 전환

상태: 제출(2026-09-24, 사용자 "제출하려고 합니다") — 인게임 확인·Wiki 반영 대기

- 날짜: 2026-09-23
- 계기: [WxCombat 모듈 리뷰](module_review_WxCombat.md)의 "락온 표시가 대상 ASC 루즈 태그와 태스크가 만든 레티클 위젯으로 흩어져 있다".
- 기획 근거: `Docs/CombatDesign/Nameplate_System.md` 5장. 기본은 숨김이고, 인식 시와 카메라 락온 시에 표시하며, 추적이 끝나면 즉시 숨긴다.

## 조사

- 락온 카메라 태스크(`UWxAbilityTask_LockOnCamera`)가 대상 적 ASC에 `State.LockedOn` 루즈 태그를 붙였다. 리슨 호스트와 싱글에서는 이 태그가 적의 권위 ASC에 올라간다. 이 태그를 쓰는 곳은 `WBP_Nameplate_Enemy`의 조건 `ANY(State.LockedOn, State.Engaged)`, 사망 제외 하나뿐이다.
- 같은 태스크가 레티클 `UWidgetComponent`도 직접 만든다. 위젯 클래스는 `GA_Shared_LockOn`의 `ReticleWidgetClass`(`WBP_LockOnReticle`)다.
- Nameplate(`UWxNameplateComponent`)는 모든 적에 네이티브로 붙어 BeginPlay 때 위젯을 만들고, 매 프레임 틱에서 거리를 판정한다.
- `State.Engaged`는 적이 모든 머신에서 복제된 자기 락온 대상으로 스스로 붙인다(`AWxEnemyCharacter::RefreshEngagement`).
- Lyra는 폰의 `NameplateSource`가 등록만 알리고, 로컬 컨트롤러의 `NameplateManagerComponent`(UI 계층)가 인디케이터를 붙이고 뗀다.

## 사용자 결정 (2026-09-23)

1. 교전하지 않은 적도 락온하면 Nameplate가 떠야 한다.
2. Nameplate는 동적으로 붙이고 뗀다. NameplateManager는 WxUI에 둔다.
3. 태그 조건은 NameplateManager에 하나만 둔다.
4. 레티클도 NameplateManager로 옮긴다.

## 설계

- **WxUI `UWxNameplateComponent`**: 적에 붙는 Nameplate 자리(SceneComponent)다. 위젯도 틱도 없고, BeginPlay와 EndPlay에서 정적 목록에 등록하고 해제만 한다. 클래스와 서브오브젝트 이름을 유지해 BP의 높이(`RelativeLocation`)를 살린다. → 2026-09-24에 `UWxNameplateSourceComponent`(ActorComponent)와 메시 기준 높이로 바뀌었다. 아래 "Nameplate 높이·이름 변경" 참조.
- **WxUI `UWxNameplateManagerComponent`**: 플레이어 컨트롤러에 붙고 로컬 컨트롤러에서만 틱한다.
  - Nameplate 판정: 거리 안에 있고, (태그 조건 충족 또는 LockOn 대상의 주인)이면 Nameplate 위젯 컴포넌트를 대상에 붙인다. 조건을 벗어나면 뗀다.
  - 태그 조건: `VisibilityRequirements` 하나. C++ 기본값은 Require `State.Engaged`, Ignore `Ability.Death`다. LockOn 대상에는 Require를 건너뛰고 Ignore만 적용한다.
  - 레티클: LockOn 대상 지점에 붙인다. LockOn 대상이 바뀌면 다시 붙인다.
  - 위젯 클래스·거리·스케일 설정은 NameplateManager가 가진다.
- **LockOnTargetQuery**: NameplateManager의 네이티브 델리게이트다. WxGame `AWxPlayerController`가 빙의 폰의 락온 지점을 돌려주도록 바인딩한다.
- **WxCombat**: 락온 카메라 태스크에서 루즈 태그와 레티클 생성을 걷어낸다. 락온 어빌리티의 `ReticleWidgetClass`는 제거한다.
- **WxCore**: `State.LockedOn` 태그를 제거한다.

## 구현 · 2026-09-23

미커밋 변경이며, 기준은 HEAD `3f1ca0d0f`다. 같은 시간에 다른 세션이 WxCombat 리뷰 항목(`WxEffect_Damage`, PerfectGuard Cue 등)을 고치고 있었고, 그 파일들은 건드리지 않았다.

- `Plugins/WxUI/.../Component/WxNameplateComponent.h/.cpp`: 베이스를 `UWidgetComponent`에서 `USceneComponent`로 바꿨다. 위젯·틱·거리 판정을 없애고 정적 목록 등록과 해제만 남겼다.
- `Plugins/WxUI/.../Component/WxNameplateManagerComponent.h/.cpp` 신규
  - 로컬 컨트롤러에서만 틱을 켠다.
  - 매 프레임 LockOn 대상을 질의해 레티클을 붙이거나 옮긴다. 등록된 자리를 훑어 Nameplate 위젯 컴포넌트를 붙이거나 뗀다.
  - Nameplate 위젯 컴포넌트는 대상 액터 소유로 만든다. 대상이 파괴되면 함께 사라지고, NameplateManager의 EndPlay에서도 직접 뗀다.
  - Character VM 연결과 거리 스케일은 옛 Nameplate 컴포넌트의 코드를 옮겼다.
- `Source/WxGame/Controller/WxPlayerController.h/.cpp`: NameplateManager를 네이티브로 붙이고, BeginPlay에서 `LockOnTargetQuery`를 빙의 캐릭터의 락온 지점에 바인딩했다.
- `UWxAbilityTask_LockOnCamera`: 루즈 태그, 레티클 생성·파괴, `ReticleWidgetClass` 인자를 제거했다. `UWxAbility_LockOn`: `ReticleWidgetClass` 속성을 제거했다.
- `WxGameplayTags`: `State.LockedOn`을 제거했다.

## 검증 · 2026-09-23

- WxEditor Win64 Development 빌드 성공. 첫 시도는 지역 변수 `Character`가 `AController::Character`를 가려 C4458로 실패했고, 이름을 바꿔 통과했다. 로그: `Saved/Logs/BuildDoctor/build_2026-09-23_234058_958_17416.log`. 다른 세션의 미커밋 변경도 함께 빌드됐다.
- `CompileAllBlueprints`(허용 목록 9개): 적 BP 5종, `BP_PlayerController`, `GA_Shared_LockOn`, `WBP_Nameplate_Enemy`, `WBP_LockOnReticle`. 오류 0, 로드 실패 0. 경고는 MCP 플러그인 라이선스 안내 1건뿐이다. 로그: `Saved/Logs/NameplateBPCompile.log`.
- 확인하지 못한 것:
  - 옛 Nameplate의 BP별 높이(`RelativeLocation`)가 그대로 읽히는지. 같은 `USceneComponent` 속성이라 유지될 것으로 보지만 값은 직접 보지 않았다.
  - 인게임 표시·멀티플레이.

## 에셋 반영 · 2026-09-24 (MCP)

사용자가 DebugGame 에디터를 실행한 뒤 MCP로 진행했다. DebugGame DLL은 소스 수정 뒤에 빌드된 것을 확인했다. 세션 시작 때 MCP 연결이 실패해서, 같은 서버(`127.0.0.1:8000/mcp`)에 HTTP JSON-RPC로 직접 호출했다.

- `BP_PlayerController` → `NameplateManagerComponent`: `NameplateWidgetClass`=`WBP_Nameplate_Enemy_C`, `ReticleWidgetClass`=`WBP_LockOnReticle_C`로 설정했다. 컴파일 뒤에도 유지됨을 확인하고 저장했다.
- `WBP_Nameplate_Enemy`: 위젯 자신의 `SetVisibility` 바인딩(`Conv_TagRequirementsToVisibility`, `IgnoreTags=Ability.Death`, `TagQuery=ANY(State.LockedOn, State.Engaged)`)을 삭제했다. 위젯 기본 가시성이 `SelfHitTestInvisible`로 조건 충족 시 값과 같으므로, 붙어 있으면 보인다. 사망 시 제거는 NameplateManager가 한다. 저장본에서 `State.LockedOn` 문자열이 사라졌다.
  - 주의: `ObjectTools.set_properties`로 `bindings` 배열을 다시 쓰면 인스턴스 변환 함수 참조를 `.` 경로로 찾아 실패하고 "could not be set"을 반환한다. 그래도 배열은 기존 원소 자리에 덮여 4개로 줄었고, 남은 바인딩과 참조는 원래와 같았다. 저장 후 별도 프로세스로 다시 불러와 컴파일해 오류 0·경고 0을 확인했다.
- 적 BP 5종의 Nameplate 높이는 모두 `(0,0,120)`으로 C++ 기본값과 같다. 같은 `USceneComponent` 속성이라 BP에서 바꾼 값이 있었다면 유지됐을 것이다. 로드 경고가 없어 다시 저장하지 않았다. 배치 액터도 저장하지 않았다.
- 처형 작업의 후속으로 `BP_Template`(Player)·`BP_HGTest`를 다시 저장했다. 삭제된 `WxFinisherDamageComponent`에 대한 LogLinker 경고가 사라졌다.
- `GA_Shared_LockOn`에 남은 옛 `ReticleWidgetClass` 값은 로드할 때 조용히 버려진다. 다시 저장하지 않았다.
- 별도 프로세스 재검증은 `CompileAllBlueprints`로 했다. 종료 코드 1은 실행 중인 에디터가 8000 포트를 점유해 생긴 HttpListener 오류이며, 컴파일 결과와는 무관하다.

## Nameplate 높이·이름 변경 · 2026-09-24

- 사용자 결정: Nameplate 높이는 캐릭터 메시 기준 머리 위 30으로 통일한다. 기본 포즈 메시 높이로 고정하고(애니메이션 바운드·`head` 본 방식 기각), 캡슐(RootComponent)에 붙인다.
- 사용자 제안(동의): 위치·위젯이 없어진 Nameplate 컴포넌트를 `UWxNameplateSourceComponent`로 바꾼다(Lyra `NameplateSource`와 같은 역할).
- 구현
  - `UWxNameplateSourceComponent`(`WxNameplateSourceComponent.h/.cpp`): 베이스를 `UActorComponent`로 바꿨다. 등록과 해제만 한다. 적의 멤버와 서브오브젝트 이름도 `NameplateSourceComponent`로 바꾸고, 고정 높이 `(0,0,120)`을 지웠다.
  - NameplateManager: Nameplate 위젯을 대상 루트에 붙인다. 높이는 `ACharacter::GetMesh()`의 `GetImportedBounds()`를 메시→루트 상대 변환한 상자의 `Max.Z`에 `HeadClearance`(기본 30cm)를 더한 값이다. Nameplate를 만들 때 한 번 계산한다. 거리 판정은 대상 액터 위치 기준으로 바꿨다.
  - 클래스 리다이렉트는 넣지 않았다. 넣으면 배치 액터에 저장된 옛 서브오브젝트가 새 클래스로 로드되어 소유 컴포넌트로 등록된다. 그러면 한 액터에 NameplateSource가 둘이 될 수 있다. 리다이렉트가 없으면 옛 데이터는 "클래스 없음" 경고와 함께 버려진다.
- 검증: Development 빌드 성공. `CompileAllBlueprints` 9개 오류 0. 적 BP 5종에는 예상한 LogLinker 경고(옛 `WxNameplateComponent` 클래스 없음)만 있다.
- 재저장(2026-09-24): 사용자가 에디터를 닫은 뒤 프로젝트 파일 재생성, DebugGame 빌드를 거쳐 에디터를 다시 띄워 MCP로 진행했다.
  - 적 BP 5종: 저장 후 옛 클래스 참조 0, `NameplateSourceComponent`만 남았다. 변경 상태를 만들려고 같은 값으로 쓴 `bReplicates`는 HEAD에서 바꾼 적이 없는 기본값이라 결과에 남지 않았다.
  - `LV_DevCombat`의 `BP_Soldier`(`NRCPT86OI84K9H87YKQM54`): 태그를 바꿨다 되돌려 변경 상태로 만들고 File → Save All로 그 패키지만 저장했다. MCP `save_actor`·`save_assets`는 외부 액터 패키지를 "에셋 없음"으로 거부했다.
  - `LV_OpenWorld`의 대상 2개는 적이 아니라 `WxSpawner`(`Spawner_BP_Enemy5/6`)였다. 옛 파일에 `b2d2870a6` 이전의 미리보기 자식 액터(적 전체 컴포넌트)가 남아 있었고, 현재 `Transient` 미리보기라 다시 저장하면서 빠졌다(12,264→3,393바이트). 스폰 대상 클래스·라벨은 유지됐다.
  - 스포너 태그를 바꾸자 변경 패키지가 11개가 됐다. 저장 대화상자를 여는 클릭이 모달로 게임 스레드를 막아 MCP가 멈췄고, 사용자가 대화상자에서 직접 11개를 모두 저장했다.
  - 함께 저장된 9개는 이번 작업과 무관하다: PCG 볼륨 1, Landmass 브러시 3, `BP_RoadSplineMesh` 5. 7개는 헤더 24~63바이트만 다르고, 도로 스플라인 2개(`8/K4/4ZEBGBY86…`, `E/C1/86VJIIJ6…`)는 크기가 조금 바뀌었다. 맵을 열 때 이 도구들이 스스로 재생성한 결과로 보인다. 사용자 승인("네", 2026-09-24)으로 에디터를 `LV_FrontEnd`로 옮긴 뒤 9개를 `git checkout`으로 HEAD에 되돌렸다.
- 남은 에셋 변경(이번 작업 12개): 적 BP 5종, 플레이어 `BP_Template`·`BP_HGTest`, `BP_PlayerController`, `WBP_Nameplate_Enemy`, `LV_DevCombat` 배치 액터 1, `LV_OpenWorld` 스포너 2.

## 코드 리뷰 후속 · 2026-09-24

- `/code-review high` 후보 9건을 코드·에셋으로 검증했다. 확인된 결함은 9번(낡은 주석) 하나였다.
  - 3번(가드 예측 적용 시 스택 불일치): 해당 없음. 두 GE는 몽타주 ANS 5개에서만 쓰인다.
  - 7번(`VisualOverride` 외형 메시): 오탐. 코드에 없다.
  - 8번(심리스 트래블 시 틱 미활성): 해당 없음. 심리스 트래블을 쓰지 않는다.
  - 5번(매 틱 전체 순회): 퇴행이 아니다. 예전에는 적마다 컴포넌트가 매 틱 같은 계산을 했다.
- 9번: `WxAbility_LockOn.cpp`에서 레티클을 태스크가 정리한다는 옛 주석 두 곳을 고쳤다.
- 사용자 지시("6번과 4번 제안해주신대로 고쳐주세요")
  - 6번: LockOn 대상에는 `MaxVisibilityDistance`를 적용하지 않는다. 락온 가능 거리(`GA_Shared_LockOn` 2000cm)는 락온 쪽이 정한다.
  - 4번: `VisibilityDistanceHysteresis`(기본 200cm)를 추가했다. 새로 붙일 때는 `MaxVisibilityDistance - 200`보다 가까워야 하고, 이미 붙은 것은 `MaxVisibilityDistance`까지 유지한다.
- 사용자 지적("Focus 대상이 아니라, LockOn 대상 아닌가요?")에 따라 `FocusQuery`를 `LockOnTargetQuery`로 바꿨다. 연결된 대상이 락온 하나뿐이라 `Focus` 일반화는 불필요했다. WxUI가 WxCombat을 include하지 않으므로 모듈 경계는 그대로다.
- 검증: Development 빌드 성공. 실행 중인 DebugGame 에디터에는 이 절의 변경이 없어 DebugGame 재빌드·재시작이 필요하다.
- 1·2번(구간 GE ANS가 클래스 기준으로 제거해 스택형 계약이 필요했음): 사용자가 "스택형이 아닌 GE도 쓸 수 있게" 하고 싶다고 해서 B안(ANS가 자기 핸들만 제거)으로 바꿨다("네 B로 합시다").
  - `UWxAnimNotifyState_ApplyGameplayEffect`가 `TMap<int32, FActiveGameplayEffectHandle>`(저장 안 되는 멤버)에 몽타주 인스턴스 ID별 핸들을 보관하고, 끝에서 그 핸들의 스택 하나만 뺀다.
  - 몽타주 인스턴스 ID는 전역 카운터라 고유하다(`AnimMontage.cpp:1686`). 큐 경로에서는 이벤트 참조의 `FAnimNotifyMontageInstanceContext`로, 브랜칭 포인트 경로에서는 페이로드로 받는다. 엔진 기본 브랜칭 구현이 빈 이벤트 참조를 넘기기 때문이다(`AnimNotifyState.cpp:46`). 브랜칭 노티파이는 큐에서 걸러지므로 두 경로가 겹치지 않는다(`UAnimMontage::FilterOutNotifyBranchingPoints`).
  - 몽타주가 아닌 재생에서는 경고를 남기고 적용하지 않는다.
  - `UWxCombatLibrary::ApplyEffect`가 핸들을 반환하게 바꿨다(호출부는 이 ANS 하나).
  - `UWxEffect_Invincible`·`UWxEffect_PerfectGuard`의 스택 설정은 HEAD로 되돌렸다. `UWxAbilityBase`·`UWxSkillCutsceneComponent`의 핸들 제거는 스택 하나만 빼는 방식을 유지했다(비스택형이면 전체 제거와 같다).
  - 검증: 임시 자동화 테스트 `Wx.Combat.ApplyEffectNotify.OwnHandleOnly`가 통과했다(오류 0, 경고 0). 확인한 경우는 비스택형 GE 두 구간 겹침, 핸들 소유자와의 겹침, 브랜칭 경로 겹침, 몽타주가 아닌 재생의 미적용이다. 로그: `Saved/Logs/ApplyEffectNotifyAutomation.log`. 사용자 지침에 따라 테스트 파일은 삭제하고 다시 빌드해 통과를 확인했다.

## 인게임 확인 항목

1. 교전 전의 적은 Nameplate가 없다. 적이 인식하면 Nameplate가 뜨고, 추적을 끝내면 바로 사라진다.
2. 교전하지 않은 적을 락온하면 Nameplate와 레티클이 뜨고, 해제하면 Nameplate가 사라진다(교전 중이면 Nameplate는 남는다).
3. 락온 대상을 바꾸면 레티클이 새 부위로 즉시 옮겨간다.
4. 사망한 적은 락온 중이어도 Nameplate가 사라진다.
5. 거리에 따른 Nameplate 크기 변화와 3000cm 밖 숨김이 이전과 같다. Nameplate가 적 종류마다 머리 꼭대기에서 약 30cm 위에 뜨고, 공격·피격 모션에 흔들리지 않는다.
6. 리슨 서버 호스트와 원격 클라이언트가 각자 자기 락온에만 레티클과 Nameplate를 본다.
