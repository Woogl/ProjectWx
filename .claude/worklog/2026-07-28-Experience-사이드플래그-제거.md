# Experience 엔트리 사이드 플래그 제거

## 계획

### 목표

Experience 에셋의 주입 항목마다 `bClientComponent`/`bServerComponent`를 손으로 켜야 해서, 컴포넌트를 추가할 때마다 사이드를 에셋에서 다시 판단해야 한다. 사이드 판정을 컴포넌트 쪽 사실(CDO 복제 여부 + 컴포넌트 자신의 role·로컬 판정)에서 파생시켜 플래그를 없앤다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxExperienceDefinition.h` | `FWxFrameworkComponentEntry` 삭제, 목록을 `TArray<TSubclassOf<UGameFrameworkComponent>>`로 | 수정 |
| `Source/WxGame/Framework/WxExperienceDefinition.cpp` | 엔트리 생성자만 있던 파일 | 삭제 |
| `Source/WxGame/Framework/WxGameState.h/.cpp` | 넷모드 사이드 필터를 CDO 복제 여부 판정으로 교체, 순회 대상·주석 갱신 | 수정 |
| `Plugins/WxUI/.../Indicator/WxIndicatorManagerComponent.h/.cpp` | `AddIndicator`에 로컬 컨트롤러 가드(원격 사본은 등록증 미발급) + 주석 | 수정 |
| `Source/WxGame/README.md`, 주입 컴포넌트 헤더 주석 | 사이드 플래그 서술 정리, 사이드 제한 주체가 컴포넌트임을 명시 | 수정 |
| `Content/Framework/EXP_Combat.uasset` | 배열 원소 타입 변경으로 목록 소실 → 6종 재기입 | 수정(에셋) |

### 접근 방식

- **복제 컴포넌트는 엔진이 이미 서버로 강제한다**: `UGameFrameworkComponentManager::CreateComponentOnInstance`가 `!CDO->GetIsReplicated() || LocalRole == ROLE_Authority` 조건을 걸어 클라에서는 복제 컴포넌트를 만들지 않는다. 서버 플래그 5개는 이 규칙의 재기입이었다. GameState 는 클라 넷모드에서 복제 컴포넌트 요청만 생략해, 아무것도 만들지 않는 요청이 남지 않게 한다.
- **비복제 컴포넌트의 사이드 제한은 컴포넌트가 스스로 한다**: 퀘스트·스폰은 기존 authority 가드가 그 역할을 이미 하고 있고, 로컬 표시 전용인 인디케이터 매니저에는 같은 성격의 로컬 컨트롤러 가드를 넣는다. 요청은 양쪽에 등록되고 반대편 사본은 빈 껍데기로 남는다.
- **수용한 부작용**: 실수 시 실패 모드가 "안 붙음"에서 "양쪽 실행"으로 바뀌므로 각 컴포넌트 주석에 자기 사이드 제한 책임을 명시한다. 클라 퀘스트 뷰모델이 null 대신 빈 VM 으로 생성되는 변화도 검증에서 확인한다.

### 재기입할 에셋 목록 (변경 전 덤프)

`WxInventoryManagerComponent` · `WxInteractionScannerComponent` · `WxPlayerSpawnComponent` · `WxQuestComponent` · `WxTimeDilationComponent` · `WxIndicatorManagerComponent`

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxExperienceDefinition.h` | 엔트리 구조체 삭제, 목록을 컴포넌트 클래스 배열로 | 수정 |
| `Source/WxGame/Framework/WxExperienceDefinition.cpp` | 엔트리 생성자만 남아 있던 파일 | 삭제 |
| `Source/WxGame/Framework/WxGameState.h/.cpp` | 넷모드 사이드 필터를 CDO 복제 여부 판정으로 교체, 순회 대상·주석 갱신 | 수정 |
| `Plugins/WxUI/.../Indicator/WxIndicatorManagerComponent.h/.cpp` | 원격 사본은 등록증을 발급하지 않는 로컬 가드 + 주석 | 수정 |
| `Plugins/WxQuest/.../WxQuestComponent.h`, `Plugins/WxSave/.../WxPlayerSpawnComponent.h` | 사이드 제한이 컴포넌트 책임임을 주석에 명시 | 수정 |
| `Source/WxGame/README.md` | 주입 규약에서 사이드 플래그 서술 제거, 컴포넌트 자기 가드 규약 추가 | 수정 |
| `Content/Framework/EXP_Combat.uasset` | 프로퍼티 타입 변경으로 소실된 목록 6종 재기입 | 수정(에셋) |

### 구현·결정과 그 이유
- **판정 근거를 엔진 규칙으로 옮김**: 컴포넌트 매니저가 복제 컴포넌트를 authority 액터에서만 만들기 때문에, 서버 플래그는 이미 엔진이 강제하는 규칙의 재기입이었다. 클라에서는 그 요청만 생략해 "아무것도 만들지 못할 요청"이 남지 않게 했다.
- **비복제 컴포넌트의 사이드는 컴포넌트가 스스로 제한**: 퀘스트·스폰은 이미 갖고 있던 authority 가드가 그대로 그 역할을 하고, 로컬 표시 전용인 인디케이터에는 같은 성격의 로컬 컨트롤러 가드를 새로 넣었다. 데이터가 아니라 코드가 사이드를 아는 구조라 컴포넌트를 추가할 때 에셋에서 다시 판단할 필요가 없다.
- **대신 실패 모드가 바뀐 점을 주석으로 보완**: 가드를 빠뜨리면 "안 붙음"이 아니라 "양쪽 실행"이 되므로, 해당 컴포넌트 헤더마다 자기 사이드 제한 책임을 명시했다.
- **검증 결과**: 스탠드얼론 PIE 는 이전과 동일한 6종 부착. Play As Client 에서 복제 3종(인벤토리·스캐너·타임딜레이션)은 클라에 로컬 생성 없이 복제본만 도착했고, 비복제 3종은 양쪽에 붙되 반대편 사본이 무동작임을 확인했다(클라 GameState 에 러너 컴포넌트 없음). 두 구성 모두 새 경고·에러 없음.

### 계획 대비 달라진 점
- 계획대로. 에셋 목록은 예상대로 타입 변경으로 소실됐고(로드 시 경고 1회), 미리 떠 둔 덤프로 재기입했다.

### 후속 과제
- 인디케이터 실제 표시와 클라 퀘스트 HUD 가시성은 육안 확인이 필요하다 — 뷰포트 캡처가 응답 크기 제한에 걸려 자동 확인하지 못했다.
- 퀘스트 복제 정책이 정해지면 클라 GameState 의 빈 퀘스트 사본이 저널 수신처가 된다.
