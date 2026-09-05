// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Item.generated.h"

/** 원본의 타입을 해석하지 않는다. 표시 데이터는 호출부 또는 파생 VM이 공급한다. */
UCLASS()
class WXUI_API UWxViewModel_Item : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 현재 DisplayName을 전달해도 Deinitialize의 초기화에 영향을 받지 않도록 이름은 값으로 받는다. */
	void Initialize(const UObject* InSourceObject, FText InDisplayName, const TSoftObjectPtr<UObject>& InIcon);
	void SetDisplayName(const FText& InDisplayName);
	void SetIcon(const TSoftObjectPtr<UObject>& InIcon);
	virtual void Deinitialize() override;

	/** 표시 중에는 원본을 유지하며 Deinitialize에서 참조를 해제한다. */
	UPROPERTY(Transient, BlueprintReadOnly, FieldNotify, Category = "Wx|UI")
	TObjectPtr<const UObject> SourceObject;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|UI")
	FText DisplayName;

	/** 소프트 참조의 이미지를 비동기 로드하여 노출한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|UI")
	TObjectPtr<UObject> Icon;

protected:
	void SetSourceObject(const UObject* InSourceObject);
	virtual void ApplyLoadedImage(FName FieldName, UObject* LoadedImage) override;
};
