# MirrorAbility 제외 태그 정확 일치

## 계획

- 승인한 대로 ExcludedAbilities를 TArray<FGameplayTagContainer>로 변경한다.
- 배열의 어느 컨테이너든 어빌리티 AssetTags와 정확히 같은 태그가 하나라도 있으면 HasAnyExact로 제외한다. 부모 태그는 매칭하지 않는다.
- 자동 실행과 정리 및 사용자 수정 코드는 보존한다. UE 5.8 WxEditor Development 빌드와 diff 검사를 수행한다.

## 완료

- ExcludedAbilities를 TArray<FGameplayTagContainer>로 변경하고 GameplayTagContainer.h를 명시적으로 포함했다.
- 각 항목을 GetAssetTags().HasAnyExact(Tags)로 비교한다. 정확한 태그 하나만 일치해도 제외하며 부모·자식 태그만 일치하거나 목록이 비어 있으면 제외하지 않는다.
- 자동 실행·정리 및 사용자 수정 서식·설명은 보존했다. WxCore와 에셋은 수정하지 않았다.
- UE 5.8 WxEditor Win64 Development 빌드 성공(exit 0). git diff --check에서 기존 사용자 수정 부분의 공백 2곳(27, 179행)을 확인했다. 이번 변경부에는 공백 오류가 없으며 기존 서식은 보존했다. 실제 에디터 설정·플레이 검증은 미실시.
- 로그: C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-18_205017_207_23368.log


