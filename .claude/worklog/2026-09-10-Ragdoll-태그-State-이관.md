# Ragdoll 태그 State 이관

## 계획

### 목표

`Event.Ragdoll` 은 `Event.*` 네임스페이스 계약("서버가 ASC 로 보내는 Gameplay Event")을 깨는 유일한 항목이다. 이벤트로 발송되는 지점이 코드에도 에셋에도 없고 소비처 세 곳이 전부 지속 상태로 다룬다. 태그를 `State.Ragdoll` 로 옮겨 분류표가 자기 규약을 지키게 한다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` | 선언을 State 구획 끝으로 옮기고 `State_Ragdoll` 로 개명, 이웃 형식의 한 줄 doc 추가 | 수정 |
| `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp` | 정의를 State 블록으로 옮기고 문자열을 `"State.Ragdoll"` 로 변경 | 수정 |
| `Plugins/WxCombat/.../Ability/WxAbility_Death.cpp` | 심볼 교체 | 수정 |
| `Plugins/WxCombat/.../Ability/WxAbility_Death.h` | 주석의 태그 이름 정정 | 수정 |
| `Source/WxGame/Character/WxCharacterBase.cpp` | 심볼 교체 2곳 | 수정 |
| `Config/DefaultGameplayTags.ini` | `GameplayTagRedirects` 항목 추가 | 신규 |

### 접근 방식

- **이벤트화 배제**: 래그돌은 어빌리티 인스턴스가 없는 시뮬 프록시와 late joiner 까지 커버해야 한다. Gameplay Event 는 로컬·일회성이라 복제되지 않고, Multicast RPC 로 뿌려도 사망 이후 접속한 클라에는 닿지 않는다. 지금은 복제된 태그가 초기 상태로 실려 오고 `HasMatchingGameplayTag` 1회 폴링이 그걸 받아내는데 이벤트에는 대응물이 없다. 계약을 지키려다 기능이 퇴행하므로, 구조는 그대로 두고 이름만 옮긴다.
- **리다이렉트 필수**: 태그 문자열이 `Content/Character/Shared/Targeting/` 의 프리셋 5개에 저작돼 있다. 리다이렉트 없이 개명하면 저작 데이터가 조용히 끊긴다.
- **리다이렉트 위치**: `UGameplayTagsSettings` 가 `config=GameplayTags, defaultconfig` 이므로 `Config/DefaultGameplayTags.ini` 의 `[/Script/GameplayTags.GameplayTagsSettings]` 가 현재 위치다. `DefaultEngine.ini` 의 `[/Script/Engine.Engine]` 아래에 넣는 옛 방식은 UE 5.8 에서 deprecated 경로라 시작할 때마다 에러 로그를 뱉는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` | 선언을 Event 구획에서 State 구획 끝으로 이동, `State_Ragdoll` 로 개명, 발행·소비처 한 줄 doc 추가 | 수정 |
| `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp` | 정의를 State 블록으로 이동, 문자열 `"State.Ragdoll"` | 수정 |
| `Plugins/WxCombat/.../Ability/WxAbility_Death.cpp` | `AddLooseGameplayTag` 인자 심볼 교체 | 수정 |
| `Plugins/WxCombat/.../Ability/WxAbility_Death.h` | 클래스 doc 의 태그 이름 정정 | 수정 |
| `Source/WxGame/Character/WxCharacterBase.cpp` | 구독·폴링 심볼 교체 2곳 | 수정 |
| `Config/DefaultGameplayTags.ini` | `GameplayTagRedirects` 로 옛 이름 유지 | 신규 |

### 구현·결정과 그 이유

- **이벤트화 대신 개명**: 래그돌은 어빌리티 인스턴스가 없는 시뮬 프록시와 late joiner 까지 커버해야 하는데, Gameplay Event 는 복제되지 않고 Multicast 도 late joiner 에 닿지 않는다. 계약을 지키려다 기능이 퇴행하므로 구조는 그대로 두고 네임스페이스만 바로잡았다.
- **doc 배치**: 태그 헤더에는 이웃 State 태그들과 같은 형식으로 발행·소비처만 한 줄 적었다. 왜 지속 태그여야 하는지는 이미 `WxAbility_Death.h` 에 있어 중복을 만들지 않았다.
- **리다이렉트 위치**: `UGameplayTagsSettings` 가 `config=GameplayTags, defaultconfig` 라 `Config/DefaultGameplayTags.ini` 가 현재 위치다. `DefaultEngine.ini` 의 `[/Script/Engine.Engine]` 아래에 넣는 옛 방식은 UE 5.8 이 deprecated 로 보고 시작할 때마다 에러 로그를 뱉는다(`GameplayTagRedirectors.cpp:32-56`).
- **타게팅 프리셋 바인딩 미확인**: 옛 이름이 프리셋 5개 에셋에 박혀 있다는 사실은 이름 테이블에서 확인했으나, 어느 필드에 물려 있는지는 바이너리라 열어보지 않았다. 리다이렉트가 필요하다는 결론은 어느 쪽이든 같아 그대로 진행했다.

### 계획 대비 달라진 점

계획대로.

### 후속 과제

- 리다이렉트는 옛 이름을 계속 살려 주지만 에셋 파일 안에는 여전히 `Event.Ragdoll` 이 박혀 있다. 에디터에서 `Content/Character/Shared/Targeting/` 의 프리셋 5개를 다시 저장하면 새 이름이 구워지고, 그 뒤에는 리다이렉트를 걷어낼 수 있다.
- 런타임 미검증. 에디터에서 프리셋을 열어 `IgnoreTags` 가 `State.Ragdoll` 로 해석되는지 보면 리다이렉트가 물렸는지 확인된다.
