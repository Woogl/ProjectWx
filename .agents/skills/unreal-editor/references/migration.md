# 에셋 이전: 개명·리다이렉트·재저장

C++ 쪽 이름이나 타입을 바꾸면 그것을 저장한 에셋 값이 **경고 없이** 기본값이 되거나 사라진다. 그 상태로 저장하면 디스크에서도 영구히 잃는다.
값이 비어 보이면 저장하지 말고 먼저 리다이렉트를 의심한다. 디스크의 `.uasset`을 `grep -a`로 훑으면 옛 이름과 값이 그대로 보인다.

## 개명

- **USTRUCT·UENUM·UCLASS 개명**: 같은 커밋에 `Config/DefaultEngine.ini`의 `[CoreRedirects]`에 `+StructRedirects=(OldName="/Script/모듈.옛이름",NewName="/Script/모듈.새이름")`(클래스는 `ClassRedirects`, enum은 `EnumRedirects`)를 넣고 에디터를 재시작한다.
- **프로퍼티 개명**: `PropertyRedirects`로 값은 살려도 StateTree 바인딩 경로는 살지 않는다. 바인딩은 다시 건다.
- **네이티브 태그 개명**(`WxGameplayTags`의 문자열 변경): 그 태그를 든 에셋에서 값이 조용히 사라진다.
  1. `Config/DefaultGameplayTags.ini`를 임시로 만들고 `[/Script/GameplayTags.GameplayTagsSettings]` 아래 `+GameplayTagRedirects=(OldTagName="옛것",NewTagName="새것")`를 넣는다.
  2. 빌드 후 아래 재저장 커맨드릿으로 대상 에셋을 재저장한다.
  3. 옛 문자열이 사라졌는지 `grep -rl --binary-files=text`로 확인한다.
  4. ini를 지운다(이 프로젝트는 태그 ini를 쓰지 않는다). 리다이렉트 없이 커맨드릿을 한 번 더 돌려 값이 살아남는지 본다.
- **컴포넌트 서브오브젝트 개명**: 파생 BP의 CDO에 옛 이름 export가 남아 로드할 때마다 유령 서브오브젝트가 생기고 스폰 때 이름 매칭이 깨진다. 재저장·재컴파일로는 안 풀리고 `VisibleAnywhere`라 `set_properties`도 거절된다. 에디터를 닫고 export map에서 그 export의 ObjectName FName 인덱스를 새 이름 인덱스로 4바이트 패치한다. 컴포넌트 클래스를 통째로 지운 경우는 다르다 — 패키지를 더티로 만들어 재저장하면 남은 참조가 지워진다.
- **배열 원소 타입 변경**(struct↔object 등): 로더가 기존 원소를 버리고 길이만 남긴다. CoreRedirect로도 못 잇는다. 코드 변경 전에 `get_properties`로 값을 JSON으로 떠 두고 → 코드 변경·빌드 → 에디터 재시작(레이아웃 변경은 Live Coding 불가) → `set_properties`로 다시 쓰고 → 개수·참조를 재조회로 맞춘다.

## 네이티브 액터 클래스 삭제

BP의 네이티브 부모 클래스를 지우거나 재부모화하면 그 BP의 배치 인스턴스가 월드 파티션에서 통째로 사라진다(`LogWorldPartition: Warning: Invalid actor native class`).
외부 액터 패키지의 디스크립터가 저장 당시의 네이티브 클래스 경로를 들고 있고, 그 클래스가 없으면 로드 자체가 막혀 재저장도 할 수 없다.

1. 지운 클래스를 빈 UCLASS 껍데기로 임시 부활시켜 빌드한다(헤더 하나에 몰아도 되고 .cpp는 없어도 된다).
2. 레벨을 열어 액터가 로드되면 건드려 더티로 만들고 액터를 저장한다.
3. 액터 `.uasset`을 `grep -ao "/Script/[A-Za-z]*\.[A-Za-z]*"`로 훑어 옛 경로가 사라졌는지 확인한다.
4. 껍데기를 지우고 다시 빌드한다.

다시 배치하는 대안은 ActorGuid가 바뀌어 거기서 파생되는 SaveId가 끊기므로 쓰지 않는다.

## 리다이렉트 제거

- `.agents/scripts/Check-Redirects.ps1`이 `[CoreRedirects]`의 각 항목을 아직 옛 이름으로 참조하는 패키지가 있는지 점검한다(기본은 읽기 전용). `-Interactive`는 재저장과 제거를 각각 확인받아 진행하며, 사용자는 `BatchFiles/CheckRedirects.bat`으로 같은 일을 한다.
- `UnrealEditor-Cmd -run=PkgInfo -imports`는 리다이렉트를 적용한 뒤의 임포트를 보여 줘 판정에 쓸 수 없다. 스크립트는 원본 임포트 테이블을 직접 읽는다.

## 재저장 커맨드릿

- 일반 패키지: `UnrealEditor-Cmd.exe <uproject> -run=ResavePackages -unattended -nopause -nosplash -SCCProvider=None -PACKAGE=/Game/...`. 소스 컨트롤을 끄지 않으면 git 경고로 종료 코드 1이 된다.
  - `-PACKAGE=`는 패키지마다 따로 준다(`A+B+C`는 한 이름으로 취급된다). `-PackageFolder=... -MapsOnly`는 맵만 고르지 않고 폴더 전체를 재저장한다.
  - Git Bash에서는 MSYS 경로 변환이 `/Game/...`을 망가뜨린다. PowerShell에서 돌리거나 `MSYS_NO_PATHCONV=1`을 붙인다.
- 외부 액터: `ResavePackages`는 조용히 건너뛴다. `-run=WorldPartitionBuilderCommandlet <맵> -Builder=WorldPartitionResaveActorsBuilder -ActorClassName=<클래스 경로>`를 쓴다. 이 빌더는 불러오지 못한 액터 패키지를 **삭제**하니 실행 전후 파일 목록을 비교한다.
- 에디터가 떠 있어도 커맨드릿은 별도 프로세스라 돌릴 수 있지만, 에디터가 연 파일은 `git restore`로 되돌리지 못한다.
