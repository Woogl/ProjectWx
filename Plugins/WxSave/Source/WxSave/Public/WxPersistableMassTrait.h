// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityElementTypes.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "MassEntityTraitBase.h"
#include "WxPersistableMassTrait.generated.h"

class UMassEntityConfigAsset;

/** Mass 엔티티를 영속화 대상으로 표시하고 원본 EntityConfig를 보관한다. */
USTRUCT()
struct WXSAVE_API FWxPersistableEntityConfigFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UMassEntityConfigAsset> EntityConfig = nullptr;
};

template<>
struct TMassFragmentTraits<FWxPersistableEntityConfigFragment> final
{
	enum
	{
		AuthorAcceptsItsNotTriviallyCopyable = true
	};
};

UCLASS(meta = (DisplayName = "Wx Persistable Entity Config"))
class WXSAVE_API UWxPersistableEntityConfigTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
