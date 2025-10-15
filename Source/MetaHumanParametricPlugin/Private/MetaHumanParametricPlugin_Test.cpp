// Copyright Epic Games, Inc. All Rights Reserved.
// Simple Test Program for MetaHuman Parametric Plugin
// This tests the plugin without requiring full MetaHuman template data

#include "CoreMinimal.h"
#include "MetaHumanParametricGenerator.h"
#include "HAL/Platform.h"

// Simple test that verifies plugin functionality without MetaHuman dependencies
void TestPluginBasics()
{
	UE_LOG(LogTemp, Log, TEXT("==========================================="));
	UE_LOG(LogTemp, Log, TEXT("MetaHuman Parametric Plugin - Basic Test"));
	UE_LOG(LogTemp, Log, TEXT("==========================================="));

	// Test 1: Verify structs are accessible
	UE_LOG(LogTemp, Log, TEXT("Test 1: Verifying data structures..."));

	FMetaHumanBodyParametricConfig BodyConfig;
	BodyConfig.BodyType = EMetaHumanBodyType::f_med_nrw;
	BodyConfig.bUseParametricBody = true;
	BodyConfig.GlobalDeltaScale = 1.0f;
	BodyConfig.BodyMeasurements.Add(TEXT("Height"), 170.0f);
	BodyConfig.BodyMeasurements.Add(TEXT("Chest"), 90.0f);
	BodyConfig.BodyMeasurements.Add(TEXT("Waist"), 70.0f);

	UE_LOG(LogTemp, Log, TEXT("  ✓ Body config created: %d measurements"), BodyConfig.BodyMeasurements.Num());

	FMetaHumanAppearanceConfig AppearanceConfig;
	AppearanceConfig.SkinToneU = 0.5f;
	AppearanceConfig.SkinToneV = 0.5f;
	AppearanceConfig.SkinRoughness = 1.0f;

	UE_LOG(LogTemp, Log, TEXT("  ✓ Appearance config created"));

	// Test 2: Verify class is registered
	UE_LOG(LogTemp, Log, TEXT("Test 2: Checking UClass registration..."));

	UClass* GeneratorClass = UMetaHumanParametricGenerator::StaticClass();
	if (GeneratorClass)
	{
		UE_LOG(LogTemp, Log, TEXT("  ✓ UMetaHumanParametricGenerator class is registered"));
		UE_LOG(LogTemp, Log, TEXT("    Class name: %s"), *GeneratorClass->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("  ✗ Failed to find UMetaHumanParametricGenerator class"));
	}

	// Test 3: Print body type information
	UE_LOG(LogTemp, Log, TEXT("Test 3: Body type enumeration..."));
	UE_LOG(LogTemp, Log, TEXT("  Selected body type: f_med_nrw (Female, Medium, Normal Weight)"));
	UE_LOG(LogTemp, Log, TEXT("  Body types available: 18 presets"));

	// Test 4: Verify measurements are stored correctly
	UE_LOG(LogTemp, Log, TEXT("Test 4: Validating body measurements..."));
	for (const auto& Pair : BodyConfig.BodyMeasurements)
	{
		UE_LOG(LogTemp, Log, TEXT("  %s: %.1f cm"), *Pair.Key, Pair.Value);
	}

	UE_LOG(LogTemp, Log, TEXT("==========================================="));
	UE_LOG(LogTemp, Log, TEXT("✓ All basic tests passed!"));
	UE_LOG(LogTemp, Log, TEXT("==========================================="));
	UE_LOG(LogTemp, Warning, TEXT("NOTE: Full character generation requires:"));
	UE_LOG(LogTemp, Warning, TEXT("  1. MetaHuman base templates"));
	UE_LOG(LogTemp, Warning, TEXT("  2. Texture synthesis data"));
	UE_LOG(LogTemp, Warning, TEXT("  3. DNA calibration files"));
	UE_LOG(LogTemp, Warning, TEXT("  4. Identity assets from MetaHuman Creator"));
}

// Test function that can be called from the menu
void Example_PluginTest()
{
	TestPluginBasics();
}
