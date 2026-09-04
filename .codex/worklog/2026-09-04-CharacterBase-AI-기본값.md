# CharacterBase AI 기본값

## 계획

- `AWxCharacterBase` 생성자에서 `AWxAIController`와 `PlacedInWorldOrSpawned`를 기본 AI 정책으로 설정한다.
- `AWxPlayerCharacter` 생성자에서는 AIControllerClass를 비우고 AutoPossessAI를 Disabled로 설정해 플레이어의 명시적 opt-out을 보장한다.
- 기존 `AWxEnemyCharacter`와 `AWxMinion` 생성자의 중복 AI 기본값 설정을 제거한다.
- UE 5.8 `WxEditor` Win64 Development 빌드로 전체 컴파일을 검증한다.

## 완료

- `AWxCharacterBase` 생성자에서 `AIControllerClass`를 `AWxAIController`로, `AutoPossessAI`를 `PlacedInWorldOrSpawned`로 설정했다.
- `AWxPlayerCharacter` 생성자에서 `AIControllerClass`를 비우고 `AutoPossessAI`를 `Disabled`로 설정해 플레이어의 AIController 생성을 차단했다.
- `AWxEnemyCharacter`와 `AWxMinion`의 중복 AI 설정 및 불필요한 include를 제거했다.
- AI 기본값 설정 지점이 CharacterBase와 PlayerCharacter opt-out 두 곳뿐임을 확인했다.
- UE 5.8 `WxEditor` Win64 Development 빌드 성공: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_214721_495_12092.log`.
