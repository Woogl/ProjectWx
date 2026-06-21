# GimmickInteraction → EnableInteraction (컴포넌트별 토글)

## 계획

### 목표
`FWxStateTreeTask_GimmickInteraction`(소유 기믹의 모든 인터랙션 일괄 토글)을 `FWxStateTreeTask_EnableInteraction` 으로 리네임하고, `InteractionComponent` 인자를 받아 그 컴포넌트 하나만 활성/비활성하도록 바꾼다. 인터랙션이 여러 개인 기믹(엘리베이터 3개)에서 개별 영역을 따로 제어하기 위함. 전체 토글 폴백은 두지 않고(null→Failed), 유일 호출자가 사라진 `AWxGimmick::SetInteractionEnabled` 는 데드코드로 제거한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Public/Gimmick/WxGimmickStateTreeNodes.h` | 구조체/InstanceData 리네임, InstanceData `{InteractionComponent, bEnable}`, DisplayName "Wx Enable Interaction", 섹션·개요 주석 갱신, `class UWxInteractionComponent;` 전방선언 | 수정 |
| `Private/Gimmick/WxGimmickStateTreeNodes.cpp` | EnterState 컴포넌트 직접 토글(null→Failed), GetDescription 갱신, include `WxGimmick.h`→`Interaction/WxInteractionComponent.h` | 수정 |
| `Public/Gimmick/WxGimmick.h` | `SetInteractionEnabled` 선언·doc 제거 | 수정 |
| `Private/Gimmick/WxGimmick.cpp` | `SetInteractionEnabled` 정의 제거, 미사용 `WxInteractionComponent.h` include 제거 | 수정 |
| `Config/DefaultEngine.ini` | `[CoreRedirects]` 에 구조체 2종 StructRedirects | 수정 |
| 기믹 doc 주석 4곳 | `Wx Gimmick Interaction`→`Wx Enable Interaction` (`WxElevator.h`, `WxTreasureChest.cpp`, `WxSpawnConsole.cpp`, `WxAlarmConsole.cpp`); AlarmConsole 줄의 잔존 `Wx Play Fx` 정리 | 수정 |
| `WxWorld/README.md` | AWxGimmick 설명 "일괄 토글" 삭제, 태스크 목록 `GimmickInteraction`→`EnableInteraction` | 수정 |

### 접근 방식
- **노드 의미**: 즉시형(무틱). InstanceData `{TObjectPtr<UWxInteractionComponent> InteractionComponent, bool bEnable}`. EnterState 는 컴포넌트 null→`Failed`(ComponentMove/PlayAnimation 의 필수타깃 null→Failed 관례 일치), 아니면 `InteractionComponent->SetInteractionEnabled(bEnable)` 후 `Succeeded`. AWxGimmick 캐스트 제거.
- **GetDescription**: 바인딩 소스명 우선(ComponentMove 패턴), 직접지정 폴백. `Wx Enable Interaction (ConsoleInteraction: enabled)`.
- **데드코드 제거**: `AWxGimmick::SetInteractionEnabled` 선언/정의/관련 doc·include 정리.
- **CoreRedirects**: 구조체 리네임을 리다이렉트해 기존 ST 에셋 노드 유실/오류 방지. 단 신규 `InteractionComponent` 바인딩은 비므로 에셋 재작성은 별도 필요.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Public/Gimmick/WxGimmickStateTreeNodes.h` | `GimmickInteraction`→`EnableInteraction` 리네임, InstanceData `{InteractionComponent, bEnable}`, DisplayName "Wx Enable Interaction", 섹션·개요 주석 갱신, `UWxInteractionComponent` 전방선언 추가 | 수정 |
| `Private/Gimmick/WxGimmickStateTreeNodes.cpp` | EnterState 컴포넌트 직접 토글(null→Failed), GetDescription 바인딩명+상태 표기, include `WxGimmick.h`→`Interaction/WxInteractionComponent.h` | 수정 |
| `Public/Gimmick/WxGimmick.h` | `SetInteractionEnabled` 선언·doc 제거, 클래스 doc 의 일괄 토글 언급 삭제 | 수정 |
| `Private/Gimmick/WxGimmick.cpp` | `SetInteractionEnabled` 정의 제거, 미사용 `WxInteractionComponent.h` include 제거 | 수정 |
| `Config/DefaultEngine.ini` | `[CoreRedirects]` 에 구조체 2종 StructRedirects 추가 | 수정 |
| `WxElevator.h`, `WxTreasureChest.cpp`, `WxSpawnConsole.cpp`, `WxAlarmConsole.cpp` | doc 주석 `Wx Gimmick Interaction`→`Wx Enable Interaction`; AlarmConsole 줄의 잔존 `Wx Play Fx`→`Wx Spawn Niagara / Wx Play Sound` 정리 | 수정 |
| `WxWorld/README.md` | AWxGimmick 설명 "일괄 토글" 삭제, 태스크 목록 `GimmickInteraction`→`EnableInteraction` | 수정 |

### 구현·결정과 그 이유
- **컴포넌트별 단일 책임화**: 노드가 `Context.GetOwner()`(AWxGimmick)에 의존해 전부를 토글하던 구조를, 바인딩된 컴포넌트 하나만 토글하도록 좁혔다. 인터랙션이 여럿인 기믹에서 영역별 제어가 가능해지고, 노드가 더 이상 소유 액터 타입을 가정하지 않아 결합이 줄었다.
- **null→Failed**: 컴포넌트가 필수 바인딩이므로 비면 Failed 로 미설정을 드러낸다. 같은 파일의 ComponentMove/PlayAnimation 이 필수 타깃 null 에 Failed 를 반환하는 관례와 일치시켰다.
- **전체 토글 폐기 + 데드코드 제거**: 사용자 결정으로 폴백을 두지 않았다. 그 결과 유일 호출자가 사라진 `AWxGimmick::SetInteractionEnabled` 와 그 전용 include 를 함께 제거했다(방어적·데드코드 지양).
- **CoreRedirects 선반영**: 구조체 리네임을 리다이렉트해, 다음 에디터 기동 시 기존 ST 에셋의 노드가 신규 구조체로 해석되어 유실/오류 없이 재작성 대상으로 남게 했다. 바인딩 자체는 비므로 재작성은 별도 필요.

### 계획 대비 달라진 점
- 계획대로. 추가로 AlarmConsole 주석에 남아 있던(이전 PlayFx 분리 때 누락) `Wx Play Fx` 표기를 같은 줄을 손대는 김에 신규 노드명으로 정리했다.

### 후속 과제
- 기존 ST 에셋의 인터랙션 노드를 상태별로 재작성·재바인딩해야 한다(에디터 작업, 코드 범위 밖):
  - **ST_Door**·**ST_TreasureChest**: 단일 인터랙션 컴포넌트를 각 노드에 바인딩, `bEnable` 재설정.
  - **ST_Elevator**: 기존 전체 토글 1노드를 컴포넌트별 3노드(Platform/CallConsoleA/CallConsoleB)로 분해·배치·바인딩.
  - 재작성 후 BP 저장 시 스냅샷 갱신.

---

## 추가: 인터랙션 컴포넌트 바인딩 노출(AllowPrivateAccess)

EnableInteraction 노드가 컴포넌트를 인자로 바인딩하려면 대상 UPROPERTY 가 노출돼야 한다. EnableInteraction 을 ST 에서 쓰는 5개 기믹의 인터랙션 컴포넌트 7개에 `meta = (AllowPrivateAccess = "true")` + 설명 주석을 추가했다(프로젝트의 바인딩 노출 관례와 동일).

- 대상: `WxDoor`(ConsoleInteraction), `WxElevator`(Platform/CallConsoleA/CallConsoleB), `WxTreasureChest`(InteractionComponent), `WxSpawnConsole`(ConsoleInteraction), `WxAlarmConsole`(ConsoleInteraction).
- 제외: `WxCutsceneTrigger`·`WxCheckPoint`·`WxLaserCorridor` 는 인터랙션 토글을 C++ 에서 직접(또는 토글 없음) 처리하고 EnableInteraction 노드로 바인딩하지 않으므로, 불필요한 선노출을 피해 그대로 둠(요청 시 동일 적용 가능).
