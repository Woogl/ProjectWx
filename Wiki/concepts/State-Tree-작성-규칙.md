# State Tree 작성 규칙

State Tree를 쓰는 모든 곳(적·소환물 AI, 장치, 퀘스트 스텝)에 공통인 태스크 작성·저작 규칙과 그 규칙이 기대는 엔진 동작을 모은 문서다.
각 시스템의 트리 구조는 [적 몬스터](적-몬스터.md), [장치와 배치물](장치와-배치물.md), [퀘스트 시스템](퀘스트-시스템.md)에 있다.

## 구현

### 완료 판정
- 상태의 완료 판정에는 스스로 시간을 들여 끝나는 태스크만 참여한다.
- 판정에서 빠지는 태스크는 생성자에서 `bConsideredForCompletion = false`와 `bCanEditConsideredForCompletion = false`를 함께 둬(`WITH_EDITORONLY_DATA` 안) 에디터에서도 되돌릴 수 없다. HEAD의 C++ 태스크 32개 중 17개다.
  - 진입 즉시 끝나는 즉발 태스크 13개: 소리·나이아가라·상호작용자 GE·플레이어 입력·연결 장치 작동·체크포인트 저장, 보상·충전, 퀘스트 제목·목표·다음 퀘스트, 스포너 발동·리스폰.
  - 스스로 끝나지 않는 태스크 4개: AI의 `LockOn`·`MirrorAbility`·`MirrorMovement`, 인디케이터의 `MarkIndicator`.
- 가르는 기준은 이름이 아니라 완료 시점이다. `PlaySound`는 즉발이라 빠지고, `PrintSubtitle`은 자막이 끝날 때 완료해 참여한다.

### 머무는 상태
- 스스로 끝나면 안 되는 상태(대기·잠김·도착 후 정지)에는 엔진 `Delay` 태스크를 `bRunForever`로 하나 두고 판정에 참여시킨다. 영원히 Running이라 상태가 완료되지 않고, '여기 머문다'는 뜻이 에셋에 그대로 남는다.
- 스스로 끝나야 하는 상태에는 지연 완료형 태스크를 하나 이상 둔다. 즉발 태스크만 놓인 상태는 명시 전이로 빠져나가게 짜고 PIE로 확인한다.
- AI 공용 트리의 전투 상태도 락온 태스크가 판정에서 빠져 있어 `Delay`를 함께 둔다(추정, 에셋 미확인).

### 데이터와 바인딩
- 프로퍼티 바인딩은 소스에서 노드 인스턴스 데이터로의 단방향 복사다. 태스크는 바인딩으로 컨텍스트 액터의 멤버에 쓸 수 없다.
- 태스크가 만드는 런타임 데이터(스폰 목록 등)는 그 태스크의 인스턴스 데이터에 두고, 쓰는 쪽은 태스크끼리의 바인딩으로 읽는다. 에셋에서 만드는 태스크를 쓰는 태스크보다 앞에 둔다.
- 액터 멤버 바인딩은 액터에서 태스크로 들어가는 입력에만 쓴다.
- 태스크 인스턴스 데이터는 상태에 들어올 때마다 새로 만들어진다. 상태를 떠났다 돌아와도 이어야 하는 값은 액터 쪽 컴포넌트가 든다(적의 정찰 진행 위치가 `UWxAIBehaviorComponent`에 있는 이유).
- C++ 구조체 태스크는 다이내믹 델리게이트를 구독하지 못한다. 그런 이벤트는 틱에서 상태 변화를 직접 본다(도플갱어 이동 태스크의 몽타주 종료 판정).
- 통지가 트리 틱 밖에서 오고 그 통지 안에서 트리가 멈출 수 있으면, 구독 상태를 인스턴스 데이터 밖 공유 포인터(`TSharedFromThis`)에 두고 `AddSP`로 구독한다(도플갱어 미러링 태스크).

### 컴포넌트 지정
- 태스크가 컨텍스트 액터의 컴포넌트를 가리킬 때는 바인딩 대신 이름 구조체 `FWxStateTreeComponentName`(`NoBinding`)을 쓴다.
- WxEditor의 `FWxStateTreeComponentNameCustomization`이 ST 에셋 스키마의 Context 액터 클래스에서 컴포넌트 프로퍼티를 훑어 드롭다운을 만든다. 네이티브 컴포넌트와 BP 컴포넌트가 함께 잡히고, `AllowedClasses` 메타로 후보를 좁힌다.
- 런타임은 `Resolve`로 액터에서 그 이름의 컴포넌트를 찾는다.
- 장치 태스크(`ComponentMove`·`PlayAnimation`·`SaveCheckpoint`·`SpawnNiagara`·`SplineMove`)와 `WxDeviceTriggerRule`이 쓴다. 구조체는 `Source/WxGame/Device/WxDeviceComponentName.h`에 있다.

### 레벨 액터 지정
- ST 에셋은 레벨 밖에 있어, 배치 액터를 가리킬 때는 `FUniversalObjectLocator`에 `meta = (AllowedLocators = "Actor")`를 붙인다. 단일 멤버와 배열 모두 된다.
- WxEditor의 `FWxActorLocatorCustomization`이 이 메타가 붙은 필드에 액터 픽커(아웃라이너 검색·선택 항목 사용·지우기·스포이드)를 띄운다. `AllowedClasses` 메타(클래스 경로 1개)로 후보를 좁히고, 파라미터 백이 만든 프로퍼티(퀘스트 스텝 루트 파라미터·링크 상태 오버라이드)에도 뜬다.
- 해석은 강제 로드 없는 `SyncFind`로 하며, PIE와 월드 파티션 해석이 들어 있다.
- 쓰는 곳은 스포너 태스크 `TriggerSpawners`·`WaitSpawnersKilled`(배열, 스포너만)와 `WaitMoveToTarget`·`MarkIndicator`·`WaitForInteraction`(단일 `Target`)이다.

### 엔진 동작에 기댄 곳
- 판정 참여 태스크가 0개인 상태는 진입 직후 형제 상태의 완료를 물려받아 곧바로 완료된다(5.8.1에도 남아 있다).
  - 컴파일러가 그런 상태에 '상태 자신' 완료 비트를 덧붙이는데, 진입 때 `ResetStatus(TasksNum)`이 태스크 개수만큼만 지워 그 비트에 형제의 값이 남는다.
  - 완료 전이 탐색은 활성 체인의 가장 얕은 상태부터 돌아, 부모가 먼저 걸리면 자식의 완료 전이는 검토되지 않고 루트로 되감긴다.
  - 그 비트 위치가 태스크 개수라 태스크를 하나 더하거나 빼면 잘 돌던 트리가 깨질 수 있다. 판정 태스크 0개 상태가 있는 에셋은 태스크를 바꿀 때마다 다시 확인한다.
  - 판정 태스크가 하나라도 있으면 그 비트가 붙지 않는다. 엔진 `Delay`는 `bConsideredForScheduling = false`라 머무는 상태에 둬도 그것만으로 트리 틱을 요청하지 않는다.
- 판정에서 빠진 태스크의 반환값은 상태 진입 결과에 합산되지 않는다. 그런 태스크가 Failed를 내도 상태 진입은 막히지 않는다(`EnterState`의 `IsConsideredForCompletion` 분기).
- 엔진은 완료한 태스크를 다시 틱하지 않고 틱하지 않는 태스크는 바인딩 복사도 건너뛴다. 그래서 즉발 태스크에 성능 플래그를 따로 붙이지 않는다.
- 바인딩 피커는 소스 프로퍼티에 `CPF_Edit`를 요구한다. BP 컴포넌트 패널로 만든 컴포넌트의 변수에는 그 플래그가 없어 피커에 뜨지 않고, 이미 저작된 바인딩만 이름으로 계속 해석된다.
- ST 컴파일러(`ValidateNoLevelActorReferences`)는 오브젝트 프로퍼티 값이 레벨 액터로 해석되면 소프트 참조라도 에러를 낸다. UOL은 순수 구조체라 이 검사 밖이다.
- 디테일 패널은 ST 인스턴스 데이터 직속 행에서 버려질 행에 `CustomizeHeader`를 한 번 더 부른다. 그래서 두 커스터마이제이션은 호출마다 위젯을 새로 만들고 호출 사이 상태(플래그·타이머)를 두지 않는다. 엔진 기본 UOL 편집기가 단일 멤버 행에서 값을 못 그리던 것도 이 때문이다.

## 결정
- 2026-08-13 레벨 액터 지정은 UOL(`AllowedLocators = "Actor"`)로 확정했다. 같은 날 오전 소프트 참조와 수주 지점 주입으로 바꿨다가, 에셋 안에서 오버라이드로 저작하는 흐름이 필요해 오후에 되돌렸다. 소프트 참조는 ST 컴파일러가 저장을 막고 런처 엔진이라 검사를 고칠 수 없다. `FSoftObjectPath` 생구조체는 검사는 피하지만 PIE 리매핑과 픽커를 손수 다시 만들어야 하는 UOL의 열화판이다. 둘 다 다시 제안하지 않는다. (사용자 결정)
- 2026-08-14 단일 UOL 멤버에도 전용 픽커가 정상 동작함을 확인해 '배열 필수' 제한을 걷었다. 대상 수는 의미대로 고른다. (작업 기록, 커밋 72020bb3d)
- 2026-08-21 상태 완료 판정은 스스로 시간을 들여 끝나는 태스크에만 맡기고, 즉발 태스크는 코드에서 판정 제외를 잠갔다. 즉발 태스크가 같은 상태의 대기 태스크보다 먼저 완료로 잡혀 상태가 곧바로 끝나는 사고가 반복됐고, 상태마다 `TasksCompletion=All`을 손으로 지정하는 방어는 자주 빠졌다. 이동·재생 태스크를 판정에 남긴 것은 문·엘리베이터 이동, 상자 애니메이션, 퀘스트 대화처럼 그 완료가 곧 상태의 끝인 자리가 있어서다. (사용자 결정, 커밋 954ab75b5)
- 2026-08-22 머무는 상태에는 `Delay`(`bRunForever`)를 판정에 참여시켜 둔다. `ST_Button`·`ST_Elevator`에 태스크를 더하자 완료 비트 함정이 깨어나, 잠금 상태가 매 프레임 재진입하고 순차 자식 상태가 한 프레임에 관통해 보낸 이벤트가 버려졌다. (작업 기록)
- 2026-08-22 태스크의 컴포넌트 지목을 바인딩에서 이름 드롭다운으로 바꿨다(08-24에 `FWxComponentName`에서 `FWxStateTreeComponentName`으로 개명). 장치 BP의 컴포넌트를 네이티브로 올려 바인딩을 되살리는 안은 장치 종류별 C++ 서브클래스를 만들지 않는 방향과 충돌해 다시 제안하지 않는다. (사용자 결정, 커밋 23461448c)
- 태스크가 만드는 데이터는 그 태스크의 인스턴스 데이터가 갖고 다른 태스크는 바인딩으로 읽는다. 기믹의 주기 스폰 루프를 공용 태스크로 뺄 때, 바인딩이 단방향이라 태스크가 액터 멤버 목록에 덧붙일 수 없다는 점이 드러났다. (작업 기록, 날짜 미상)

## 미결
- Context Actor를 소스로 한 게임플레이 태그 바인딩을 ST 컴파일러가 거부한다는 08-23 기록이 있다. 지금 엔진에서 다시 확인하지 않았다.

## 관련
- [적 몬스터](적-몬스터.md)
- [장치와 배치물](장치와-배치물.md)
- [퀘스트 시스템](퀘스트-시스템.md)
- [초반 구간과 퀘스트](초반-구간과-퀘스트.md)
- [기획 작업 도구](기획-작업-도구.md)
- [현광](../entities/현광.md)

## 출처
- 사용자 대화로 정한 지난 결정: Claude 메모리 기록에서 옮기고 HEAD 2a3baca6a 코드로 확인 (2026-10-06 조회)
- UE 5.8 설치본 `Engine/Plugins/Runtime/StateTree`의 `StateTreeCompiler.cpp`·`StateTreeTasksStatus.h`·`StateTreeDelayTask.cpp`·`StateTreeExecutionContext.cpp` (2026-10-06 조회)
- `Source/WxGame/AI/WxStateTreeTask_LockOn.cpp` (8b331baec)
- `Source/WxGame/AI/WxStateTreeTask_MirrorAbility.cpp` (8b331baec)
- `Source/WxGame/AI/WxStateTreeTask_MirrorMovement.h` (8b331baec)
- `Source/WxGame/AI/WxAIBehaviorComponent.cpp` (8b331baec)
- `Source/WxGame/Device/` (7960789cf)
- `Source/WxGame/Device/WxDeviceComponentName.h` (d37e1dd32)
- `Source/WxGame/Quest/` (ecdda3b8e)
- `Source/WxGame/UI/Subtitle/WxStateTreeTask_PrintSubtitle.h` (05d662bc4)
- `Source/WxEditor/WxStateTreeComponentNameCustomization.h` (e9630dc2b)
- `Source/WxEditor/WxActorLocatorCustomization.h` (eda01fdf0)
