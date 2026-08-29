# 장치 입력 토글 대상 보정

## 계획

- `FWxStateTreeTask_EnablePlayerInput`이 장치의 `InteractingCharacter`를 기준으로 입력 대상을 찾도록 한다.
- 상호작용 캐릭터를 소유한 로컬 컨트롤러와 그 폰이 일치할 때만 입력을 토글해, 원격 플레이어 및 무관한 로컬 플레이어는 건드리지 않는다.
- 기존의 입력 해제 기록·복구 흐름은 유지한다.
- `WxEditor` Development 빌드로 컴파일을 확인한다.

## 완료

- `AWxDevice::GetInteractingCharacter()`로 상호작용 당사자를 확인하고, 그 캐릭터를 실제로 소유한 로컬 `APlayerController`만 입력 토글 대상으로 삼도록 변경했다.
- 원격 플레이어 상태, 데디케이티드 서버, 당사자가 아닌 로컬 플레이어는 노옵 처리한다. 입력을 끈 대상의 기록과 `ExitState` 복구 흐름은 유지했다.
- `WxEditor Win64 Development` 빌드 성공을 확인했다.
