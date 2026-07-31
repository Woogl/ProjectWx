# module_review 확신도 높음 이슈 1차 처리 (10건)

## 계획

### 목표
`/module-review` 가 남긴 10개 모듈 리뷰 문서(기준 커밋 `c37b6fa6`)의 확신도 "높음" 이슈 43건 중, 사용자와 대조해 방침이 확정된 10건을 처리한다. 실제 게임플레이 결함 4건(대화 소프트락·인디케이터 영구 숨김·쿨다운 오표시·사망 전파 누락), 낭비/히치 2건, 코드-문서 불일치 4건이다. 나머지는 이후 세션에서 같은 방식으로 이어간다.

**범위 제외**: WxCombat 「근접 히트가 클라에서 매 틱 돌지만 결과는 전부 버려진다(주석이 사실과 다름)」은 사용자가 별도로 집중 확인하기로 하여 제외 — `WxWeaponBase.cpp` 는 열지 않는다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxDialogue/.../Private/WxDialogueSessionComponent.cpp` | 겹침 진입 시 앞 세션 정리, 태그를 `SetLooseGameplayTagCount` 절대값으로, 시작 실패 시 `CurrentRow` 복원, 실패 경로 4곳 Warning 로그 | 수정 |
| `Plugins/WxUI/.../Private/Indicator/SWxIndicatorCanvas.cpp` | `SetShowAnyIndicators` 가 위젯 Visibility 직접 조작 대신 슬롯 `SetIndicatorVisible` 경유 | 수정 |
| `Plugins/WxUI/.../Private/MVVM/WxViewModel_Ability.cpp` · `Public/MVVM/WxViewModel_Ability.h` | `SeedActiveCooldown()` 추가 — `Initialize` 시점의 활성 쿨다운 GE 를 1회 스캔해 시드 + 티커 등록 | 수정 |
| `Plugins/WxUI/.../Private/Component/WxNameplateComponent.cpp` · `Public/Component/WxNameplateComponent.h` | 표시 판정과 함께 컴포넌트 틱 토글, 스케일 변화 시에만 `SetRenderScale`, `LastRenderScale` 멤버 추가 | 수정 |
| `Plugins/WxCombat/.../Ability/WxAbility_Ultimate.cpp` · `.h` | `OnGiveAbility` 오버라이드로 컷신 시퀀스 비동기 프리로드, 활성화 시 로드된 포인터 사용(미완료 시 동기 폴백) | 수정 |
| `Source/WxGame/Character/WxCharacterBase.cpp` | `HandleDeathTagChanged` 구독을 `InitAbilitySystem` → `PostInitializeComponents` 로 이동 + 초기 태그 1회 확인 | 수정 |
| `Plugins/WxCore/.../Private/WxInteractable.cpp` · `Public/WxInteractable.h` | 쿼리 콜리전 미활성 시 `ensureMsgf` 로 전제 노출, 계약 주석 정정 | 수정 |
| `Plugins/WxInventory/.../WxEquipmentComponent.h` · `WxInventoryManagerComponent.h` · `README.md` | 장비 경로가 호출부 0건인 미완성 배선임을 명시 | 수정 |
| `Plugins/WxQuest/.../WxQuestStateTreeNodes.cpp` · `.h` · `README.md` | 저널 태스크 컴포넌트 부재 분기에 Warning 로그, "Failed" 계약 문구 정정 | 수정 |

### 접근 방식
- **대화 겹침(1)**: 진입부에서 `HasActiveDialogue()` 면 `EndDialogue()` 로 앞 세션(태그·카메라·행)을 접고 새 세션을 연다. 태그는 카운트 가감이 아니라 절대값(`1`/`0`) 지정으로 바꿔 누수 자체를 성립 불가능하게 만든다 — 전수 grep 결과 `State.Dialogue` 를 loose 로 쓰는 곳은 이 컴포넌트 단독이라 안전하다.
- **인디케이터(2)**: 플래그(`bIsIndicatorVisible`)와 실제 Slate Visibility 를 `FSlot::SetIndicatorVisible` 한 창구로만 동기화한다. "플래그는 true 인데 위젯만 Collapsed" 라는 영구 no-op 조합이 구조적으로 만들어지지 않는다.
- **쿨다운 VM(3)**: `UpdateCooldownState` 는 `CooldownDuration > 0` 을 전제로 하고 그 값은 GE 적용 통지에서만 채워지므로, 지연 생성된 VM 은 그 통지를 받을 기회가 없다. 따라서 시드 단계에서 활성 GE 의 `Spec.GetDuration()` 을 먼저 심고 나서 기존 갱신 경로를 그대로 재사용한다.
- **컷신 프리로드(5)**: 어빌리티가 `InstancedPerActor` 라 인스턴스 멤버에 `FStreamableHandle` 을 들 수 있고, 그 핸들이 곧 GC 방지다. 활성화 경로는 동기 폴백을 남겨 프리로드 미완료 시에도 동작이 후퇴하지 않는다.
- **사망 전파(6)**: 같은 파일이 래그돌 구독에 대해 이미 쓰고 있는 패턴(전 머신 필요 → `PostInitializeComponents` + 초기 복제 1회 확인)을 그대로 따른다. 권위 전용 처리는 `HandleDeath` 내부 가드가 계속 담당한다.
- **무음 실패 드러내기(7·8·10)**: 폴백을 도입해 동작을 바꾸는 대신, 전제가 깨진 지점을 개발자에게 보이게만 한다. WxCore 는 로그 카테고리가 없어 `ensureMsgf`(Shipping 컴파일 아웃), 나머지 두 모듈은 이미 있는 `LogWxDialogue`/`LogWxQuest` Warning 을 쓴다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxDialogue/.../Private/WxDialogueSessionComponent.cpp` | 겹침 진입 시 `EndDialogue()` 선행, 태그를 `SetLooseGameplayTagCount(1/0)` 절대값으로, 시작 실패 시 `CurrentRow` 복원, 실패 4경로 Warning 로그, `Advance` 의 "다음 행 해석 실패"를 정상 종료와 분기 | 수정 |
| `Plugins/WxUI/.../Private/Indicator/SWxIndicatorCanvas.cpp` | `SetShowAnyIndicators` 가 위젯 Visibility 직접 조작 대신 양방향 모두 `FSlot::SetIndicatorVisible` 경유 | 수정 |
| `Plugins/WxUI/.../MVVM/WxViewModel_Ability.cpp` · `.h` | `SeedActiveCooldown()` 추가 — 활성 쿨다운 GE 에서 지속시간 시드 후 `UpdateCooldownState(0.f)` 1회 + 티커 등록 | 수정 |
| `Plugins/WxUI/.../Component/WxNameplateComponent.cpp` · `.h` | `RefreshVisibility` 가 표시와 함께 `SetComponentTickEnabled` 토글, `bStartWithTickEnabled=false`, `LastRenderScale` 비교 후에만 `SetRenderScale` | 수정 |
| `Plugins/WxCombat/.../Ability/WxAbility_Ultimate.cpp` · `.h` | `OnGiveAbility` 오버라이드로 컷신 비동기 프리로드(`CutscenePreloadHandle` 보관), 활성화는 `Get()` → 실패 시 동기 폴백 | 수정 |
| `Source/WxGame/Character/WxCharacterBase.cpp` · `.h` | `HandleDeathTagChanged` 구독을 `PostInitializeComponents` 로 이동 + 초기 복제 태그 1회 확인, 헤더 주석 갱신 | 수정 |
| `Plugins/WxCore/.../WxInteractable.cpp` · `.h` | 쿼리 콜리전 미활성 시 `ensureMsgf`, 인터페이스 헤드라인·`IsMeshInRange` 계약 주석 정정 | 수정 |
| `Plugins/WxInventory/.../WxEquipmentComponent.h` · `WxInventoryManagerComponent.h` · `README.md` | 장비 경로(`EquipItemByDef`·`EquipItem`·`RemoveItemInstance`)가 호출부 0건인 미완성 배선임을 명시 | 수정 |
| `Plugins/WxQuest/.../WxQuestStateTreeNodes.cpp` · `.h` · `README.md` | 저널 태스크 2종의 컴포넌트 부재 분기에 `LogWxQuest` Warning, "Failed" 계약 문구를 실제 동작(엔진이 무시)으로 정정 | 수정 |

### 구현·결정과 그 이유
- **대화 태그를 절대값으로**: 겹침 정리만으로도 이번 경로는 막히지만, 카운트 가감은 새 겹침 경로가 생기면 같은 결함이 되살아난다. 소비자(UI 매니저)가 0↔비0 전이만 듣는 구조라 잔량 1이 곧 영구 소프트락이므로, 기록자가 단독임을 전수 확인하고 절대값으로 바꿔 결함을 구조적으로 제거했다.
- **인디케이터 복구 시 전 슬롯을 켜 둠**: `SetShowAnyIndicators(true)` 는 곧바로 이어지는 슬롯 순회가 대상별로 다시 접으므로 과다 표시가 남지 않는다. 방향을 나눠 한쪽만 창구를 거치게 하면 지금 고친 비대칭이 그대로 재발한다.
- **쿨다운 시드가 두 단계인 이유**: `UpdateCooldownState` 는 `CooldownDuration > 0` 을 전제로 하는데 그 값은 GE 적용 통지에서만 채워진다. 통지를 놓친 VM 에는 그 전제가 없으므로, 활성 GE 에서 지속시간을 먼저 심고 나머지 계산은 기존 경로에 그대로 맡겼다(판별식 중복 없음).
- **컷신 동기 폴백 유지**: 부여 직후 바로 발동하는 경우엔 프리로드가 미완료다. 폴백을 지우면 그 순간 컷신이 통째로 스킵되므로, 히치 제거는 얻되 동작은 후퇴시키지 않는 쪽을 택했다.
- **사망 구독 이동에 초기 확인을 동반**: 시뮬 프록시는 구독보다 초기 복제가 먼저 도착할 수 있어 이벤트만으로는 이미 죽은 캐릭터를 놓친다. 같은 파일의 래그돌 구독이 이미 쓰는 보정이라 형태를 맞췄다.
- **`IsMeshInRange` 에 폴백 대신 ensure**: 바운즈 폴백은 "판정 기준이 대상마다 갈린다"는 이유로 이미 의도적으로 배제된 선택이다. 런타임 동작을 바꾸지 않고 전제가 깨진 사실만 개발 빌드에서 드러내되, 계약 문구가 "프리셋을 내려도 안전하다"로 읽히던 부분을 함께 고쳐 오해의 근원을 없앴다.

### 계획 대비 달라진 점
- WxCore 인터페이스 헤드라인 주석(`WxInteractable.h:15`)도 함께 고쳤다. 계획엔 `IsMeshInRange` 주석만 있었으나, 기획자를 오도하는 실제 문구는 이쪽이라 같이 정정하지 않으면 조치가 반만 된다.
- WxDialogue 로그를 계획의 4지점에 더해 `StartDialogue` 의 정의 컴포넌트 부재까지 포함해 5지점에 넣었다(같은 성격의 무음 실패).
- 나머지는 계획대로.

### 후속 과제
- **미검증(정적 분석·컴파일까지만)**: 대화 겹침 실제 재현, 인디케이터 알트탭/최소화 후 복구, 쿨다운 도중 메뉴 최초 오픈 시 표시, 원격 클라 사망 전파(PIE 2-클라이언트 필요).
- **확신도 높음 잔여 32건**: 사용자와 하나씩 방침을 정하는 중이다. 다음 차례는 WxSave(디스크 기록 실패 통지·`ResumeTransform` 원시 포인터·`bSaveInProgress` 전역 플래그)부터.
- **범위 제외 1건**: WxCombat 「근접 히트 클라 낭수 연산 + 예측 주석 오류」는 사용자가 별도로 집중 확인 예정.
- 장비 경로는 문서화만 했다. 실제로 쓰려면 트리거를 붙여야 하고, 그때 "늦은 구독자 외형 유실"(pull API 부재)이 함께 드러난다.
