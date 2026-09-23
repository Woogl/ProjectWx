// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DataTableRowFixupSettings.generated.h"

/** 사용자마다 켜고 끄는 에디터 개인 설정(Editor Preferences > Wx)이다. */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "DataTable Row Fixup"))
class UDataTableRowFixupSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDataTableRowFixupSettings();

	/** 행 이름을 바꾸면 그 행을 가리키던 FDataTableRowHandle 을 새 이름으로 고친다. 참조 에셋을 로드해 Dirty 로 만들며, 저장은 사용자가 한다. */
	UPROPERTY(Config, EditAnywhere, Category = "DataTable")
	bool bUpdateReferencesOnRowRename = true;
};
