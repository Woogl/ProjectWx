# 어빌리티 VM Deinitialize 통지 제거

## 계획

### 목표
`UWxViewModel_Ability::Deinitialize` 가 `Super` 호출 뒤에 붙인 `Set*` 12줄로 표시 필드 통지를 쏜다. 베이스가 "Deinitialize 는 브로드캐스트하지 않는다"를 계약으로 못박아 둔 자리라(`WxViewModel.h`), 통지만 걷고 값 초기화는 조용한 직접 대입으로 남긴다. `module_review_WxUI.md` 발견 #2.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp` | `Deinitialize` 의 `Set*` 12줄을 직접 대입으로 바꾸고 `Super::Deinitialize()` 앞으로 옮긴다 | 수정 |

### 접근 방식
- **파괴 경로에서만 도는 통지다**: 이 VM 의 `Deinitialize` 호출자는 자기 `Initialize` 첫 줄, 베이스 `BeginDestroy`, 테스트뿐이다. `GetOrCreateAbilityViewModel` 은 `NewObject` 직후에만 `Initialize` 해서 그때는 필드가 이미 기본값이고, 부모 `AbilitySystem` VM 도 자식에 전파하지 않고 배열에서 떼기만 한다. 결국 통지가 실제로 나가는 상황은 계약이 정확히 금지한 GC 파괴 경로뿐이다.
- **값 초기화는 남긴다**: 규약이 막는 것은 통지지 초기화가 아니다. 재초기화가 빈 슬롯으로 끝나면 `RefreshBoundAbility` 가 조기 반환해 옛 표시가 그대로 남으므로, `WxViewModel_Item` 처럼 조용한 되돌리기가 제 역할을 한다.
- **`RF_BeginDestroyed` 가드는 두지 않는다**: 형제 `Character` VM 이 그 가드를 둔 이유는 보스 리졸버가 살아 있는 VM 을 밖에서 `Deinitialize` 하기 때문이다. 어빌리티 VM 에는 그런 호출자가 없어 통지 분기를 만들어도 도는 경로가 없다.
- **`HasMultipleCharges` 를 함께 대입한다**: 기존 12줄은 `SetMaxRecharges` 가 파생값까지 갱신해 줘서 빠져 있었다. 직접 대입으로 바꾸면 그 파생이 사라져 `MaxRecharges=0` 인데 `HasMultipleCharges=true` 인 어긋난 상태가 남는다.
- 호출 순서도 계약대로 자기 정리를 먼저 하고 `Super::Deinitialize()` 를 마지막에 부른다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp` | `Deinitialize` 말미의 세터 12줄을 표시 필드 직접 대입 13줄로 바꾸고 `Super::Deinitialize()` 앞에 배치 | 수정 |

### 구현·결정과 그 이유
- **리뷰 지적은 타당했다**: 베이스가 통지 금지를 문서 주석으로 못박은 자리인데 세터를 썼고, 순서까지 `Super` 뒤였다. 게다가 이 VM 은 실사용 호출자가 파괴 경로뿐이라, 그 12줄이 통지를 내는 상황이 계약이 금지한 바로 그 경우 하나였다.
- **가드 대신 세터를 걷었다**: 리뷰는 형제 `Character` VM 의 `RF_BeginDestroyed` 가드를 제안했지만, 그 가드는 보스 리졸버가 살아 있는 VM 을 밖에서 정리하며 "소스가 빠졌다"를 표시로 알려야 해서 생긴 것이다. 어빌리티 VM 에는 그 호출자가 없으니 가드를 붙이면 아무도 타지 않는 분기만 늘어난다. 통지 자체를 없애는 편이 계약과 코드 양쪽에 맞다.
- **값 되돌리기는 남겼다**: 재초기화가 빈 슬롯으로 끝나면 `RefreshBoundAbility` 가 "물던 것과 같다(둘 다 없다)"로 조기 반환해 표시 필드를 다시 채우지 않는다. 조용한 초기화가 그 잔상을 지운다.
- **파생 필드를 손으로 맞췄다**: 세터를 거치지 않게 되면서 `SetMaxRecharges` 가 해 주던 `HasMultipleCharges` 갱신이 사라지므로 함께 대입한다.
- **검증**: UE 5.8 `WxEditor Win64 Development` 빌드가 `Result: Succeeded`.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 인게임 실측 미실시. 파괴 경로만 바뀌었으므로 스킬 슬롯 표시·쿨다운 동작은 불변이어야 한다.
- 리뷰 문서 `module_review_WxUI.md` 발견 #2 는 해소됐고, 다음 `/module-review` 재실행 시 반영된다.
