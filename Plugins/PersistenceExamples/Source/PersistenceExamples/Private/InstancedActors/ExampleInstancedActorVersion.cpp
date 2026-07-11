// Copyright Epic Games, Inc. All Rights Reserved.

#include "InstancedActors/ExampleInstancedActorVersion.h"
#include "Serialization/CustomVersion.h"

// Hardcoded GUID for my savable feature
const FGuid FExampleInstancedActorVersion::GUID(0x2F8B6D14, 0xA47C42E9, 0x91D5C30B, 0x6E1A4F87);

// Registers the feature GUID and its latest version numer
static FCustomVersionRegistration GRegisterExampleInstancedActorVersion(
	FExampleInstancedActorVersion::GUID,
	FExampleInstancedActorVersion::LatestVersion,
	TEXT("ExampleInstancedActorVersion"));
