---
name: pull
description: 원격 저장소 최신버전 가져오기
user-invocable: true
allowed-tools: Bash
---

# 원격 저장소 최신버전 가져오기

원격 저장소의 최신 내용을 현재 브랜치에 가져오라.
충돌은 자동 병합을 우선하고, 자동 병합이 불가능한 파일은 사용자에게 선택을 맡긴다.

## 절차

### 1단계: 현재 상태 확인

`git status`로 로컬 변경 사항을 확인하라.

### 2단계: 로컬 변경 사항 임시 저장

커밋되지 않은 변경 사항이 있으면 `git stash -u`로 자동 임시 저장하라. 사용자에게 묻지 말고 바로 진행하라.
`git stash -u`가 "No local changes to save"를 출력하면 실제로 저장된 것이 없으므로 4단계 복원은 건너뛴다. (저장 여부는 stash 출력이나 `git stash list`로 확인한다.)

### 3단계: Pull 실행

`git pull --no-rebase --no-edit`로 원격 저장소의 최신 내용을 가져오라. (머지를 강제해 아래 ours/theirs 기준을 고정하고, 머지 커밋 메시지 에디터가 떠서 멈추는 것을 막는다.)
- 추적 대상(upstream)이 없어 실패하면 `git pull --no-rebase --no-edit origin <브랜치명>`으로 원격·브랜치를 명시해 가져오라.
- 자동 병합이 가능하면 그대로 진행한다.
- 자동 병합이 불가능한 충돌 파일이 있으면, 파일별로 사용자에게 원격 버전(theirs)과 로컬 버전(ours) 중 어느 쪽을 적용할지 선택하게 하라. 선택에 따라 `git checkout --theirs` 또는 `git checkout --ours`를 적용한 뒤 `git add`로 충돌을 해결하고 `git commit --no-edit`으로 병합을 완료하라.

### 4단계: Stash 복원

2단계에서 stash를 사용했다면 `git stash pop`으로 복원하라.
- 자동 병합이 불가능한 충돌 파일이 있으면, 파일별로 사용자에게 현재 버전(ours)과 stash 버전(theirs) 중 어느 쪽을 적용할지 선택하게 하라. 선택에 따라 `git checkout --ours` 또는 `git checkout --theirs`를 적용한 뒤 `git add`로 충돌을 해결하고 `git stash drop`으로 stash를 제거하라.

### 5단계: 결과 보고

다음을 요약하여 보고하라:
- 가져온 커밋 수
- 변경된 파일 목록
- 충돌 해결 결과 (해당되는 경우)
- stash 복원 결과 (해당되는 경우)
