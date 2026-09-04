# AICharacter 역할 컴포넌트 확장

## 계획

- 현재 소환 관리 책임을 가진 `UWxMinionComponent`를 `UWxMinionManagerComponent`로 이름 변경하고, 클래스·프로퍼티 리다이렉트 및 기존 기본 서브오브젝트 이름을 유지해 직렬화 호환성을 보존한다.
- 공통 AI 폰 기반인 `AWxAICharacter`를 추가하고 AI 컨트롤러·Behavior Tree·전투 대상 판정 책임을 `AWxCharacterBase`와 기존 적/소환수 호환 클래스 사이에서 이동한다.
- `UWxEnemyComponent`를 추가해 적 네임플레이트, 처형 상호작용, 보상, 스포너 문맥을 소유하게 하고 `AWxEnemyCharacter`는 기존 에셋과 API를 위한 호환 서브클래스로 유지한다.
- 새로운 역할 컴포넌트 `UWxMinionComponent`를 `WxGame`에 추가해 주인·팀 관계를 표현하고, 기존 `AWxMinion`을 `AWxAICharacter` 기반 호환 클래스로 유지한다.
- `UWxBossComponent`와 보스 ViewModel이 `AWxEnemyCharacter` 대신 `AWxAICharacter`를 관찰하도록 변경하고, `IsEngaged` 계약을 유지한다.
- 각 소유권 이동 뒤 소스 참조와 `WxEditor` Development 빌드를 확인한다. 기존 블루프린트의 리패런팅·저장과 구형 클래스 삭제는 사용자의 에디터 마이그레이션 이후 별도 축소 단계로 남긴다.

## 완료

- `UWxMinionComponent`의 기존 소환 생성·목록·명령 구현을 `UWxMinionManagerComponent`로 이름 변경하고 클래스·프로퍼티 리다이렉트를 추가했다. 캐릭터의 기존 기본 서브오브젝트 이름은 직렬화 호환을 위해 유지했다.
- `AWxAICharacter`를 추가해 AIController 설정, Behavior Tree, 전투 대상 판정, 액터 단위 Spawnable·Interactable 라우팅을 맡겼다.
- `AWxEnemyCharacter`와 `AWxMinion`을 `AWxAICharacter` 기반 호환 클래스로 전환했다. 기존 적 BP 설정은 `PreInitializeComponents`에서 새 `UWxEnemyComponent`로 복사한다.
- `UWxEnemyComponent`에 적 네임플레이트, 피니셔 판정·이벤트, 보상, 스포너 문맥을 이관했다. 팀 소속은 컴포넌트와 분리된 상태로 유지했다.
- 새 `UWxMinionComponent`를 추가해 소환된 AI의 `Master`를 복제하고 Spawn `Instigator`에서 관계와 팀을 초기화하도록 했다. AIController는 역할 컴포넌트를 우선하고 구형 Instigator 경로를 폴백으로 사용한다.
- `UWxBossComponent`와 보스 ViewModel의 소유자 계약을 `AWxAICharacter`로 변경하고 `IsEngaged` 상태 계약을 유지했다.
- 구형 액터 클래스와 블루프린트 에셋은 삭제·수정하지 않았다. 에디터 리패런팅과 에셋 저장 이후에만 구형 클래스·호환 프로퍼티·리다이렉트를 제거할 수 있다.
- UE 5.8 `WxEditor` Win64 Development 빌드 성공: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_212152_922_12340.log`.
- `git diff --check`에서 공백 오류가 없음을 확인했다(기존 줄바꿈 변환 경고만 출력).
- 후속 설계 결정으로 `AWxAICharacter`는 제거되었다. 현재 구조는 `2026-09-04-CharacterBase-네이티브-컴포넌트-조합.md`를 따른다.
