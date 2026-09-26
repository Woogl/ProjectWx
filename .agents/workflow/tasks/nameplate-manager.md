# Nameplate를 로컬 NameplateManager가 붙이고 떼는 구조로 전환

상태: 완료 · 체크리스트 6/6 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

이전 상태: 제출(2026-09-24, 사용자 "제출하려고 합니다") — 인게임 확인 대기. 이어서 NameplateManager를 WxGame으로 옮기고 오래된 CoreRedirects를 제거해 제출했다(2026-09-24, 사용자 "제출해주세요", 아래 "NameplateManager WxGame 이동" 절, Wiki 반영: raw `2026-09-24-nameplate-manager-wxgame.md`와 UI·게임 조립·전투 문서). Wiki 반영 완료(2026-09-24, UI의 머리 위 Nameplate 절·게임 조립·전투)

- 날짜: 2026-09-23
- 계기: [WxCombat 모듈 리뷰](module_review_WxCombat.md)의 "락온 표시가 대상 ASC 루즈 태그와 태스크가 만든 레티클 위젯으로 흩어져 있다".
- 기획 근거: `Docs/CombatDesign/Nameplate_System.md` 5장. 기본은 숨김이고, 인식 시와 카메라 락온 시에 표시하며, 추적이 끝나면 즉시 숨긴다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 교전 표시 | 게임: 교전 전의 적에는 Nameplate가 없고, 적이 인식하면 뜨고, 추적을 끝내면 바로 사라진다 | 사람 | 통과 | 이우성 2026-09-25 |
| 락온 표시 | 게임: 교전하지 않은 적을 락온하면 Nameplate와 레티클이 뜨고 해제하면 Nameplate가 사라진다(교전 중이면 남는다) | 사람 | 통과 | 이우성 2026-09-25 |
| 락온 대상 전환 | 게임: 락온 대상을 바꾸면 레티클이 새 부위로 즉시 옮겨간다 | 사람 | 통과 | 이우성 2026-09-25 |
| 사망 표시 | 게임: 사망한 적은 락온 중이어도 Nameplate가 사라진다 | 사람 | 통과 | 이우성 2026-09-25 |
| 크기·위치 | 게임: 거리별 크기 변화와 3000cm 밖 숨김이 이전과 같고, 캡슐 윗면 약 90cm 위에 뜨며 공격·피격 모션에 흔들리지 않는다 | 사람 | 통과 | 이우성 2026-09-25 |
| 호스트·클라이언트 구분 | 리슨 서버와 원격 클라이언트: 각자 자기 락온에만 레티클과 Nameplate를 본다 | 사람 | 통과 | 이우성 2026-09-25 |

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

## NameplateSource 정적 목록 제거 · 2026-09-24

- 사용자 결정("네. 진행해주세요"): NameplateSource 정적 목록을 엔진 조회로 바꾼다. 범위 내 구 오버랩 탐색은 기각했다. 콜리전 채널에 묶이고, 반경 3000cm 쿼리가 더 무거우며, 붙어 있는 목록·여유 거리·락온 예외 경로가 그대로 남기 때문이다.
- 구현
  - NameplateManager는 `TObjectIterator<UWxNameplateSourceComponent>`를 돌며 `GetWorld() == World && HasBegunPlay()`로 거른다.
  - 이 대상은 옛 목록과 같다. 엔진이 `bHasBegunPlay`를 BeginPlay 끝에서 켜고 EndPlay에서 끈다(`ActorComponent.cpp:1661`, `:1687`). CDO는 `TObjectIterator`가 기본으로 제외하고, BP 템플릿은 BeginPlay를 하지 않는다. `TActorIterator`도 같은 방식이다. 클래스 해시(`GetObjectsOfClass`)를 조회한 뒤 월드를 거른다(`EngineUtils.h:191`).
  - `UWxNameplateSourceComponent`는 헤더만 있는 마커가 됐다. `WxNameplateSourceComponent.cpp`를 삭제했다. 생성자의 `bCanEverTick = false`는 엔진 기본값과 같다(`TickTaskManager.cpp:2365`).
- 검증
  - 프로젝트 파일 재생성.
  - WxEditor Development 빌드 성공(`Saved/Logs/BuildDoctor/build_2026-09-24_023433_655_45368.log`).
  - `CompileAllBlueprints`: 적 BP 5종, 플레이어 `BP_Template`·`BP_HGTest`, `BP_PlayerController`, `WBP_Nameplate_Enemy`, `WBP_LockOnReticle` 모두 성공했고 Nameplate 관련 경고는 없다(`Saved/Logs/NameplateSourceBPCompile.log`).
    - Windows PowerShell 5.1이 `-AllowListFile=…txt` 인자를 점에서 잘라 허용 목록을 읽지 못했고, 전체 BP를 컴파일했다.
    - 오류는 모두 이번 변경과 무관하다: 허용 목록 로드 실패, `NiagaraExamples/.../EUB_SnapToActor`의 `SetMobility` 노드. 경고로 기존 `BP_ItemPickup`의 없는 클래스 `WxInteractionComponent` 참조가 보였다.
  - 인게임은 기존 "인게임 확인 항목"과 함께 확인한다.

## NameplateManager WxGame 이동 · 2026-09-24

- 사용자 결정
  - "코드 품질 개선과 단순화가 목적이긴 해요", "아까 얘기하던 4번 진행하죠".
  - 에이전트 정정(사용자 동의 후 진행): `State.Engaged`는 유지한다. `RefreshEngagement`가 교전 규칙을 한 곳에서 계산해 태그로 알리므로, 태그를 없애면 NameplateManager와 뒤잡 판정이 규칙을 각자 다시 계산하게 된다.
  - Reticle 위치 질문("Reticle은 UWxLockOnComponent에 있어야 할 것 같은데"): LockOnComponent는 AI와 시뮬 프록시에도 있는 복제 모델이라 로컬 표시를 두지 않는다. 입력(로컬 락온 대상)과 방식(대상 소유 월드 위젯)이 같아 NameplateManager에 둔다. 클래스 이름은 바꾸지 않았다.
  - "SkeletalMesh 윗면 쓰지 말고 캡슐(Root)의 윗면을 쓰세요": 높이를 `GetUnscaledCapsuleHalfHeight() + HeadClearance`로 바꿨다. 메시 기본 포즈 바운드 방식(9-24 앞 결정)을 대체한다.
- 구현
  - `UWxNameplateManagerComponent`를 WxUI `Component/`에서 WxGame `Controller/`로 옮겼다(`git mv`). `DefaultEngine.ini`에 `ClassRedirects=(OldName="/Script/WxUI.WxNameplateManagerComponent",NewName="/Script/WxGame.WxNameplateManagerComponent")`를 추가했다. 로그는 `LogWxGame`이다.
  - 락온 대상: 빙의 캐릭터(`GetPawn<AWxCharacterBase>()`)의 `GetLockOnComponent()->GetLockOnTarget()`을 직접 읽는다. `LockOnTargetQuery` 델리게이트, `AWxPlayerController::BeginPlay`·`GetLockOnTarget()`, PC의 include 2개를 지웠다.
  - 대상 순회: `TActorIterator<AWxEnemyCharacter>`를 돌며 `HasActorBegunPlay()`로 거른다. 스트리밍 직후 BeginPlay 전인 적은 ASC가 준비되지 않았기 때문이다. `UWxNameplateSourceComponent`(WxUI 헤더)와 적의 서브오브젝트를 지웠다.
  - 표시 조건: `IsAlive() && (bLockedOn || (bInRange && bEngaged))`. `bEngaged`는 `State.Engaged` 태그로 읽는다. `VisibilityRequirements`를 지웠고, 사망 판정은 `Ability.Death` 태그에서 HP 0(`IsAlive`)으로 바뀌었다.
  - 높이: 캡슐에 붙이고, 캡슐 반높이에 `HeadClearance`(30cm)를 더한다.
- 검증
  - 프로젝트 파일 재생성. WxEditor Development 빌드 성공(`Saved/Logs/BuildDoctor/build_2026-09-24_025927_636_10704.log`).
  - 헤드리스 Python으로 확인·재저장(`Saved/Logs/NameplateResave.log`)
    - `BP_PlayerController`의 컴포넌트 클래스는 `/Script/WxGame.WxNameplateManagerComponent`이고, `NameplateWidgetClass`=`WBP_Nameplate_Enemy_C`, `ReticleWidgetClass`=`WBP_LockOnReticle_C`가 유지됐다.
    - 적 BP 5종과 `LV_DevCombat` 배치 액터(`__ExternalActors__/Maps/LV_DevCombat/2/RU/NRCPT86OI84K9H87YKQM54`)를 다시 저장했다. 맵을 불러온 뒤 외부 액터 패키지를 `save_packages`로 저장하면 된다. 저장 뒤 `Content`에서 `WxNameplateSourceComponent` 문자열 검색 결과는 0건이다. 바뀐 Content 파일은 이 6개뿐이다.
  - 새 프로세스 재확인
    - `CompileAllBlueprints`(`Saved/Logs/NameplateMoveBPCompile.log`): 전체 BP를 컴파일했다. 허용 목록 경로는 절대 경로를 줘도 프로젝트 경로가 앞에 붙어 읽히지 않는다. 실패는 무관한 `EUB_SnapToActor` 하나다.
    - `LV_DevCombat`과 관련 BP 6개 재로드(`Saved/Logs/NameplateReload.log`): LogLinker 경고와 Nameplate 관련 경고가 모두 0건이다.
  - 인게임 미검증. 아래 "인게임 확인 항목"으로 확인한다.

## 오래된 CoreRedirects 제거 · 2026-09-24

- 사용자 지시: "오래된 리디렉터 제거 진행합시다. 에디터 켜져있어요." 사용자 확인: "HeadClearance = 90은 제가 의도한 것이에요"(헤더 기본값, 03:21 수정).
- 시작 상태: `DefaultEngine.ini`의 옛 ClassRedirect 4줄(`/Script/WxGame.`→`/Script/WxUI.`의 `WxViewModelResolver_Ability`·`WxViewModel_Quest`·`WxViewModel_QuestObjective`·`WxViewModel_Dialogue`)은 이미 작업 사본에서 빠져 있었다(파일 수정 03:08, 이 세션이 지운 것 아님). 실행 중인 DebugGame 에디터(03:17 시작, DLL 03:16~03:17 빌드)는 그 설정으로 떠 있었다.
- 옛 4개 확인
  - `Content` 문자열 검색으로 참조 WBP 6개를 찾았다. 이 가운데 `WBP_ItemQuickSlot`·`WBP_QuestTracker`·`WBP_DialogueScreen`은 두 모듈 이름을 모두 담고 있어 문자열만으로는 경로를 가릴 수 없었다.
  - 리다이렉트 없이 헤드리스로 로드했을 때 누락 클래스 경고가 0건이었다. `CompileAllBlueprints`도 오류 0, 경고 0이었다. 저장본이 이미 WxUI 경로를 담고 있어 제거가 안전하다.
- 이번 작업의 `WxNameplateManagerComponent` 리다이렉트
  - 실행 중인 에디터에서 MCP로 `BP_PlayerController`를 다시 저장했다(같은 `NameplateWidgetClass` 값을 다시 써서 dirty로 만든 뒤 `save_assets`).
  - 저장 전 확인: 실행 중인 빌드의 네이티브 `HeadClearance`는 30이었다(헤더 90은 아직 빌드 전). BP 저장본에는 `HeadClearance` 이름이 없었다. 따라서 다시 저장해도 오버라이드가 생기지 않는다.
  - 리다이렉트를 지우고 `[CoreRedirects]` 섹션을 없앴다.
- 검증(리다이렉트 0개)
  - 헤드리스 Python: `BP_PlayerController` 컴포넌트 클래스는 `/Script/WxGame.WxNameplateManagerComponent`이고 위젯 클래스 두 값이 유지된다.
  - `CompileAllBlueprints`(허용 목록 `Saved/Automation/RedirectBPAllowList.txt`, 로그 `Saved/Logs/RedirectRemovedBPCompile.log`): 7개 모두 오류 0, 경고 0.
  - 허용 목록은 bash에서 상대 경로(`-AllowListFile=Saved/Automation/...`)로 넘겨야 읽힌다.
- 에셋 리다이렉터(`ObjectRedirector` 패키지)는 없다. 문자열이 걸린 3개(`LS_Template_Ultimate`, `MF_Opacity`, `MF_ExposureCompensation`)는 14~47KB의 일반 에셋이다.
- 인게임은 미검증이다.

## LockOnTargetQuery 단순화 검토 · 2026-09-24

사용자가 "LockOnTargetQuery 사용 방식이 복잡하다"고 제기했고, 검토 후 "나중에 다시 생각해볼게요"로 한 번 보류했다. 이후 "아까 얘기하던 4번 진행하죠"로 4번을 구현했다(다음 절). 아래는 검토 기록이다.

- 지금 경로: WxUI가 델리게이트를 선언하고, `AWxPlayerController::BeginPlay`가 PC의 `GetLockOnTarget()`(빙의 캐릭터 → `UWxLockOnComponent`)에 바인딩한다. 이 우회는 WxUI가 WxCombat을 모르기 때문에 필요하다.
- 후보와 에이전트 판단(4 > 2 > 1 > 3)
  1. 현상 유지(델리게이트).
  2. WxCore 계약 인터페이스(`GetLockOnTarget()`)를 캐릭터가 구현하고 NameplateManager가 Cast한다. 사용자는 처음에 "WxCore에 추가하는 것은 원치 않아요"라고 했다. 현재 쓰는 곳이 NameplateManager 하나뿐이지만, WxGame 밖의 모듈도 락온 대상을 읽게 되면 가장 나은 안이 된다.
  3. WxUI 인터페이스(`IWxNameplateViewer`)를 `AWxPlayerCharacter`가 구현한다. 델리게이트를 인터페이스로 바꿀 뿐이다.
  4. NameplateManager를 WxGame(`Controller/`)으로 옮기고 `GetPawn<AWxCharacterBase>()->GetLockOnComponent()->GetLockOnTarget()`을 직접 부른다. 델리게이트, PC `BeginPlay`·`GetLockOnTarget()`, include 2개가 사라진다. 판정 규칙(Engaged·Death·락온 예외)이 게임 규칙이라 의존 팬아웃으로도 WxGame 자리다. Lyra도 `NameplateManagerComponent`·`NameplateSource`가 게임 콘텐츠(`Content/UI/Indicators`) BP다. 비용은 사용자 결정 2번("WxUI에 둔다")을 뒤집는 것이다. 또 `DefaultEngine.ini`에 ClassRedirect(`/Script/WxUI.` → `/Script/WxGame.WxNameplateManagerComponent`)를 추가해야 하고, `BP_PlayerController`의 위젯 클래스 두 값이 유지되는지 확인해야 한다. `UWxNameplateSourceComponent`는 WxUI에 둔다.
- 기각: WxUI → WxCombat 의존(도메인 간 의존 금지), `UWxLockOnComponent`를 WxCore로 이동(WxCore엔 정의만), 락온 쪽 태그 부착(옛 `State.LockedOn`), `OnLockOnTargetChanged` 구독(무효화 통지 없음), PC 생성자 람다 바인딩(구조 그대로).
- 상호작용 대상 락온·`UWxAbility_Interact` 이전과 함께 본 판단(사용자 "이 부분도 나중에 잘 고민해볼게요")
  - 갈림길은 상호작용 선택(WxWorld `UWxInteractionScannerComponent`)이 락온 대상을 읽는지다. 읽으면 WxGame 밖 소비처가 WxUI·WxWorld 둘이 되어 2번이 최선이다. 이때 4번을 하면 Scanner에 같은 글루가 한 번 더 필요해 헛수고가 된다. 카메라·Reticle만 상호작용 대상에 걸리면 4번 판단이 그대로다.
  - 대상 쪽: `AWxDevice`(WxWorld)·`AWxDialogueActor`(WxDialogue)에는 `UWxLockOnPointComponent`(WxCombat)를 네이티브로 붙일 수 없으므로 BP 컴포넌트 트리로 붙인다(BlueprintSpawnable). `CanBeLockedOn`은 ASC가 없으면 빈 태그로 평가한다. 락온 후보 수집의 Team 필터(Neutral)와 수집 채널은 확인이 필요하다.
  - 서버 상호작용 검증을 "락온 대상과 같은가"로 걸지 않는다. 락온 RPC(폰 컴포넌트)와 상호작용 RPC(PC Scanner)는 다른 액터라 Reliable 순서가 보장되지 않는다.
  - 표시 조건 단순화와 함께 본 판단(2026-09-24, 사용자 "락온 되었을 때 Nameplate 출력되게, 전투 진입한 대상만 Nameplate 출력되게를 더 단순하게 할 수 있지 않나요?")
    - `State.Engaged`는 `AWxEnemyCharacter::RefreshEngagement`가 `IsAlive() && 자기 락온 대상 != nullptr`로 붙인다. 읽는 곳은 NameplateManager와 같은 클래스의 뒤잡 판정(`WxEnemyCharacter.cpp:94`) 둘뿐이다. `Content`·`Config`에서 문자열 검색 결과는 0건이다.
    - 4번이면 NameplateManager가 `TActorIterator<AWxEnemyCharacter>`로 돌며 `IsAlive() && (락온 대상 || (교전 중 && 거리 안))`을 직접 판정한다. 없앨 수 있는 것: `State.Engaged` 태그(WxCore 정의, 루즈 태그 부착 2곳, 뒤잡 판정은 직접 읽기로), `VisibilityRequirements`, `UWxNameplateSourceComponent`(적 서브오브젝트 포함), `LockOnTargetQuery`와 PC 글루. `RefreshEngagement`는 `UWxBattleSubsystem` 통지 때문에 남는다.
    - 비용: NameplateSource 제거로 적 BP 5종과 배치 액터·스포너를 다시 저장해야 한다(옛 클래스 없음 경고). 사망 판정이 `Ability.Death` 태그에서 HP 0(`IsAlive`)으로 바뀐다.
    - 이에 따라 앞의 "4번을 하면 Scanner에 같은 글루가 한 번 더 필요해 헛수고가 된다"를 정정한다. 4번은 태그와 마커 제거라는 자체 이득이 있다. Scanner가 락온을 읽게 되면 그때 그 소비처를 위해 2번을 더하면 된다.
    - 현 위치(WxUI)에서 할 수 있는 것은 조건식 정리뿐이다. 동작은 같다.
  - `UWxAbility_Interact`는 WxGame에 둔다(의존 팬아웃). 베이스 `UWxAbilityBase`(WxCombat, Exclusive ActivationGroup이 Scanner 표시 게이트)와 `FWxStateTreeTask_WaitForInteraction::NotifyInteracted`(WxWorld)를 함께 참조한다. WxWorld로 옮기면 ActivationGroup·ActionPhase 차단이 사라진다. WxCombat으로 옮기면 검증을 통과한 상호작용의 퀘스트 통지를 위해 WxCore 통지 채널이 필요하다. 사용자가 이전을 계획한 이유는 아직 듣지 못했다.

## 인게임 확인 항목

1. 교전 전의 적은 Nameplate가 없다. 적이 인식하면 Nameplate가 뜨고, 추적을 끝내면 바로 사라진다.
2. 교전하지 않은 적을 락온하면 Nameplate와 레티클이 뜨고, 해제하면 Nameplate가 사라진다(교전 중이면 Nameplate는 남는다).
3. 락온 대상을 바꾸면 레티클이 새 부위로 즉시 옮겨간다.
4. 사망한 적은 락온 중이어도 Nameplate가 사라진다.
5. 거리에 따른 Nameplate 크기 변화와 3000cm 밖 숨김이 이전과 같다. Nameplate가 적 종류마다 캡슐 윗면에서 약 90cm 위에 뜨고(사용자 의도값), 공격·피격 모션에 흔들리지 않는다.
6. 리슨 서버 호스트와 원격 클라이언트가 각자 자기 락온에만 레티클과 Nameplate를 본다.


## 사용자 테스트 결과 · 2026-09-25T17:11:01.445Z

<!-- test-feedback:request-6a2afc27-b357-4f63-9138-6d234a334e75:submitted -->
- 전달한 사람: 이우성

> 통과 · 교전 표시
> 통과 · 락온 표시
> 통과 · 락온 대상 전환
> 통과 · 사망 표시
> 통과 · 크기·위치
> 통과 · 호스트·클라이언트 구분


## AI 완료 정리 · 2026-09-25T17:11:01.451Z

<!-- test-feedback:request-6a2afc27-b357-4f63-9138-6d234a334e75-cleanup:1 -->
- 전달한 사람: 이우성
- 처리 AI: Codex
- 처리 결과: 정리 완료

AI 요약:

> Nameplate 사람 테스트 6개 통과 범위를 Wiki에 반영했습니다. 게임 코드·에셋·작업 기록·접수 JSON·작업 상태는 변경하지 않았습니다.
> 무관한 기존 변경과 Git 줄바꿈 경고는 그대로 보존했습니다.

> 변경: .wiki/raw/notes/2026-09-26-nameplate-play-acceptance.md에 사람 확인 범위와 출처 해시를 수집했습니다.

> 변경: .wiki/wiki/topics/ui.md의 인게임 미확인 설명을 갱신하고 락온 대상 거리 예외를 명시했습니다.

> 변경: .wiki/raw/notes/_index.md, .wiki/_index.md와 .wiki/log.md를 갱신했습니다.

> 근거: AGENTS.md, 작업 절차·nameplate-manager.md, Wiki config.md·schema.md와 wiki 스킬을 읽었습니다.

> 근거: Get-FileHash로 작업 기록 SHA-256이 처리 전후 접수 해시와 동일함을 확인했습니다.

> 근거: 순정 llm-wiki lint --local --json: 오류·경고·제안 0개, 통과.

> 근거: git diff --check -- .wiki: 종료 코드 0. 관련 문서의 로컬 링크 45개 검사 통과.

> 근거: Export-Wiki.ps1: Wiki·Workflow 뷰어 각각 64개 문서 갱신. 게임 테스트는 재실행하지 않았습니다.
