# AddComponents 액션의 ActorClass 제거

## 계획

### 목표
`FWxGameFeatureComponentEntry` 가 「대상 액터 클래스 + 컴포넌트 클래스」 한 쌍을 손으로 적게 하는데, 이 쌍은 항상 중복이다. ModularGameplay 프레임워크 컴포넌트 베이스가 이미 어떤 액터에 붙는지를 선언하고 있기 때문이다. `ActorClass` 를 없애고 대상 액터를 컴포넌트 클래스에서 도출해, 기입 항목을 하나로 줄이고 잘못된 조합을 표현 불가능하게 만든다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` | 엔트리에서 `ActorClass` 제거, `ComponentClass` 타입을 프레임워크 컴포넌트로 좁힘, `TitleProperty`·주석 갱신 | 수정 |
| `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp` | 리시버 클래스 도출 헬퍼 추가, `AddToWorld`·`IsDataValid` 를 그에 맞춤 | 수정 |

### 접근 방식
- **컴포넌트 베이스 → 리시버 액터 도출**: 파일 로컬 static 헬퍼가 `UPawnComponent`→`APawn`, `UControllerComponent`→`AController`, `UPlayerStateComponent`→`APlayerState`, `UGameStateComponent`→`AGameStateBase` 로 판정한다. 네 베이스는 `UGameFrameworkComponent` 아래 형제라 서로 겹치지 않아 검사 순서가 무관하다. 어디에도 속하지 않으면 Error 로그 후 그 엔트리만 건너뛴다.
- **엔트리 구조체는 유지**: 필드가 하나뿐이 되지만, 없애고 소프트 클래스 포인터 배열로 바꾸면 원소 타입이 변해 `WAS_CoreGameplay` 의 엔트리 6건이 유실된다. 타입을 `UActorComponent`→`UGameFrameworkComponent` 로 좁히는 것은 직렬화 형태가 같아 안전하고, 픽커를 도출 가능한 클래스로 제한하는 효과가 있다.
- **검증은 늘리지 않음**: `IsDataValid` 에서 `ActorClass` 검사만 뺀다. 베이스 도출 가능 여부까지 보려면 검증·쿠킹마다 전 컴포넌트 클래스를 동기 로드해야 하고, 남은 구멍은 픽커 제한과 런타임 Error 로그로 충분히 드러난다.
- **에셋 재작성 불필요**: `ComponentClass` 값은 그대로 살아남고, 남은 `ActorClass` 직렬화 데이터는 대응 프로퍼티가 없어 로드 시 버려진다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` | 엔트리에서 `ActorClass` 제거, `ComponentClass` 를 프레임워크 컴포넌트 타입으로 좁힘, `TitleProperty`·주석 갱신 | 수정 |
| `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp` | `WxResolveReceiverClass` 헬퍼 추가, `AddToWorld` 가 그 결과로 요청, `IsDataValid` 의 `ActorClass` 검사 제거 | 수정 |

### 구현·결정과 그 이유
- **대상 액터를 컴포넌트 타입에서 도출**: 프레임워크 컴포넌트 베이스는 이미 "어떤 액터에 붙는가" 를 선언한다. 엔트리가 그걸 한 번 더 적게 하면 두 정보가 어긋날 수 있고, 어긋나도 매니저가 조용히 아무것도 만들지 않아 증상이 안 보인다. 도출로 바꾸니 어긋난 조합 자체가 표현 불가능해졌다.
- **엔트리 구조체 유지**: 필드가 하나뿐이 됐지만 없애지 않았다. 배열 원소 타입을 struct 에서 소프트 클래스 포인터로 바꾸면 기존 에셋의 엔트리가 전부 유실된다. 실익보다 손실이 크다.
- **픽커 제한을 타입으로**: `ComponentClass` 타입을 프레임워크 컴포넌트로 좁혀, 대상 도출이 불가능한 클래스는 애초에 고를 수 없게 했다. 직렬화 형태는 그대로라 기존 값은 살아남는다.
- **검증은 늘리지 않음**: 남은 구멍(네 베이스 밖의 프레임워크 컴포넌트 직파생)까지 `IsDataValid` 로 잡으려면 검증·쿠킹마다 전 컴포넌트 클래스를 동기 로드해야 한다. 비용 대비 이득이 없어 런타임 Error 로그에 맡겼다.
- **베이스 검사 순서 무관**: 네 베이스는 공통 부모 아래 형제라 한 클래스가 둘 이상에 걸리지 않는다. 파생 깊은 쪽부터 볼 필요가 없어 그대로 순서대로 검사한다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 에디터·PIE 실동작 확인 미실시(빌드 검증만). 에셋의 `ComponentClass` 6건은 태그 기반 프로퍼티 직렬화라 살아남는 것이 확실하지만, 다음 에디터 실행 때 `WAS_CoreGameplay` 엔트리와 주입 결과를 한 번 눈으로 확인하면 좋다.
- `WAS_CoreGameplay` 에 남아 있는 `ActorClass` 직렬화 데이터는 로드 시 버려지고 다음 저장에서 정리된다. 별도 작업 불필요.
