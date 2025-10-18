// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/RandomGenActor.h"
#include "MetaHumanCharacter.h"
#include "MetaHumanBodyType.h"
#include "Misc/DateTime.h"

ARandomGenActor::ARandomGenActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f; // We'll handle interval manually
}

void ARandomGenActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("RandomGenActor: BeginPlay - Auto start: %s"),
		bAutoStartOnBeginPlay ? TEXT("Yes") : TEXT("No"));

	if (bAutoStartOnBeginPlay)
	{
		StartGeneration();
	}
}

void ARandomGenActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Accumulate delta time for tick interval
	AccumulatedDeltaTime += DeltaTime;

	// Check if we should process state this frame
	if (TickInterval > 0.0f)
	{
		if (AccumulatedDeltaTime >= TickInterval)
		{
			bShouldProcessState = true;
			AccumulatedDeltaTime = 0.0f;
		}
		else
		{
			bShouldProcessState = false;
		}
	}
	else
	{
		// TickInterval is 0, process every frame
		bShouldProcessState = true;
	}

	// Update state machine
	if (bShouldProcessState)
	{
		UpdateStateMachine(DeltaTime);
	}

	// Handle loop delay timer
	if (CurrentState == ERandomGenState::Complete && bLoopGeneration && LoopDelayTimer > 0.0f)
	{
		LoopDelayTimer -= DeltaTime;
		if (LoopDelayTimer <= 0.0f)
		{
			UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Loop delay finished, starting new generation"));
			StartGeneration();
		}
	}
}

// ============================================================================
// Public API
// ============================================================================

void ARandomGenActor::StartGeneration()
{
	if (CurrentState != ERandomGenState::Idle && CurrentState != ERandomGenState::Complete && CurrentState != ERandomGenState::Error)
	{
		UE_LOG(LogTemp, Warning, TEXT("RandomGenActor: Cannot start - already running (State: %s)"),
			*GetCurrentStateString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Starting random character generation"));
	TransitionToState(ERandomGenState::Preparing);
}

void ARandomGenActor::StopGeneration()
{
	UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Stopping generation"));
	TransitionToState(ERandomGenState::Idle);
	GeneratedCharacter.Reset();
	CurrentCharacterName.Empty();
}

FString ARandomGenActor::GetCurrentStateString() const
{
	switch (CurrentState)
	{
		case ERandomGenState::Idle: return TEXT("Idle");
		case ERandomGenState::Preparing: return TEXT("Preparing Character");
		case ERandomGenState::WaitingForRig: return TEXT("Waiting for AutoRig");
		case ERandomGenState::Assembling: return TEXT("Assembling Character");
		case ERandomGenState::Complete: return TEXT("Complete");
		case ERandomGenState::Error: return TEXT("Error");
		default: return TEXT("Unknown");
	}
}

// ============================================================================
// State Machine Implementation
// ============================================================================

void ARandomGenActor::UpdateStateMachine(float DeltaTime)
{
	switch (CurrentState)
	{
		case ERandomGenState::Idle:
			HandleIdleState();
			break;
		case ERandomGenState::Preparing:
			HandlePreparingState();
			break;
		case ERandomGenState::WaitingForRig:
			HandleWaitingForRigState();
			break;
		case ERandomGenState::Assembling:
			HandleAssemblingState();
			break;
		case ERandomGenState::Complete:
			HandleCompleteState();
			break;
		case ERandomGenState::Error:
			HandleErrorState();
			break;
	}
}

void ARandomGenActor::TransitionToState(ERandomGenState NewState)
{
	if (CurrentState == NewState)
		return;

	UE_LOG(LogTemp, Log, TEXT("RandomGenActor: State transition: %s -> %s"),
		*GetCurrentStateString(), *UEnum::GetValueAsString(NewState));

	CurrentState = NewState;
}

void ARandomGenActor::HandleIdleState()
{
	// Nothing to do in idle state
}

void ARandomGenActor::HandlePreparingState()
{
	UE_LOG(LogTemp, Log, TEXT("RandomGenActor: === Starting Character Preparation ==="));

	// Generate random parameters
	FMetaHumanBodyParametricConfig BodyConfig = GenerateRandomBodyConfig();
	FMetaHumanAppearanceConfig AppearanceConfig = GenerateRandomAppearanceConfig();
	CurrentCharacterName = GenerateUniqueCharacterName();

	UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Character Name: %s"), *CurrentCharacterName);
	UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Body Type: %s"), *UEnum::GetValueAsString(BodyConfig.BodyType));
	UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Output Path: %s"), *OutputPath);

	// Call Step 1: Prepare and Rig
	UMetaHumanCharacter* Character = nullptr;
	bool bSuccess = UMetaHumanParametricGenerator::PrepareAndRigCharacter(
		CurrentCharacterName,
		OutputPath,
		BodyConfig,
		AppearanceConfig,
		Character
	);

	if (bSuccess && Character)
	{
		GeneratedCharacter = Character;
		UE_LOG(LogTemp, Log, TEXT("RandomGenActor: ✓ Preparation complete, AutoRig started"));
		UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Transitioning to WaitingForRig state"));
		TransitionToState(ERandomGenState::WaitingForRig);
	}
	else
	{
		LastErrorMessage = TEXT("Failed to prepare character or start AutoRig");
		UE_LOG(LogTemp, Error, TEXT("RandomGenActor: ✗ %s"), *LastErrorMessage);
		TransitionToState(ERandomGenState::Error);
	}
}

void ARandomGenActor::HandleWaitingForRigState()
{
	if (!GeneratedCharacter.IsValid())
	{
		LastErrorMessage = TEXT("Character reference lost while waiting for rig");
		UE_LOG(LogTemp, Error, TEXT("RandomGenActor: ✗ %s"), *LastErrorMessage);
		TransitionToState(ERandomGenState::Error);
		return;
	}

	// Check rigging status
	FString RigStatus = UMetaHumanParametricGenerator::GetRiggingStatusString(GeneratedCharacter.Get());

	// Log status periodically (only when we actually check)
	if (bShouldProcessState)
	{
		UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Checking rig status... %s"), *RigStatus);
	}

	// Check if rigged
	if (RigStatus.Contains(TEXT("Rigged (Ready for assembly!)")))
	{
		UE_LOG(LogTemp, Log, TEXT("RandomGenActor: ✓ AutoRig complete! Proceeding to assembly"));
		TransitionToState(ERandomGenState::Assembling);
	}
	else if (RigStatus.Contains(TEXT("Unrigged")) && !RigStatus.Contains(TEXT("RigPending")))
	{
		// If it went back to Unrigged (not RigPending), that means it failed
		LastErrorMessage = TEXT("AutoRig failed - character is unrigged");
		UE_LOG(LogTemp, Error, TEXT("RandomGenActor: ✗ %s"), *LastErrorMessage);
		TransitionToState(ERandomGenState::Error);
	}
	// Otherwise, still waiting (RigPending)
}

void ARandomGenActor::HandleAssemblingState()
{
	if (!GeneratedCharacter.IsValid())
	{
		LastErrorMessage = TEXT("Character reference lost during assembly");
		UE_LOG(LogTemp, Error, TEXT("RandomGenActor: ✗ %s"), *LastErrorMessage);
		TransitionToState(ERandomGenState::Error);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("RandomGenActor: === Starting Character Assembly ==="));

	// Call Step 2: Assemble
	bool bSuccess = UMetaHumanParametricGenerator::AssembleCharacter(
		GeneratedCharacter.Get(),
		OutputPath,
		QualityLevel
	);

	if (bSuccess)
	{
		GeneratedCount++;
		UE_LOG(LogTemp, Log, TEXT("RandomGenActor: ✓✓✓ Character generation complete! ✓✓✓"));
		UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Character '%s' saved to %s"), *CurrentCharacterName, *OutputPath);
		UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Total characters generated: %d"), GeneratedCount);
		TransitionToState(ERandomGenState::Complete);
	}
	else
	{
		LastErrorMessage = TEXT("Failed to assemble character");
		UE_LOG(LogTemp, Error, TEXT("RandomGenActor: ✗ %s"), *LastErrorMessage);
		TransitionToState(ERandomGenState::Error);
	}
}

void ARandomGenActor::HandleCompleteState()
{
	// Log completion (only once)
	static bool bLoggedCompletion = false;
	if (!bLoggedCompletion)
	{
		UE_LOG(LogTemp, Log, TEXT("RandomGenActor: === Generation Complete ==="));

		if (bLoopGeneration)
		{
			UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Loop mode enabled - will start next character in %.1f seconds"), LoopDelay);
			LoopDelayTimer = LoopDelay;
		}
		else if (bDestroyAfterComplete)
		{
			UE_LOG(LogTemp, Log, TEXT("RandomGenActor: Destroying actor after completion"));
			Destroy();
		}

		bLoggedCompletion = true;
	}

	// Reset flag when leaving complete state
	if (CurrentState != ERandomGenState::Complete)
	{
		bLoggedCompletion = false;
	}
}

void ARandomGenActor::HandleErrorState()
{
	// Log error (only once)
	static bool bLoggedError = false;
	if (!bLoggedError)
	{
		UE_LOG(LogTemp, Error, TEXT("RandomGenActor: === Error State ==="));
		UE_LOG(LogTemp, Error, TEXT("RandomGenActor: Error: %s"), *LastErrorMessage);
		UE_LOG(LogTemp, Error, TEXT("RandomGenActor: Call StopGeneration() or StartGeneration() to retry"));
		bLoggedError = true;
	}

	// Reset flag when leaving error state
	if (CurrentState != ERandomGenState::Error)
	{
		bLoggedError = false;
	}
}

// ============================================================================
// Random Parameter Generation
// ============================================================================

FMetaHumanBodyParametricConfig ARandomGenActor::GenerateRandomBodyConfig()
{
	FMetaHumanBodyParametricConfig Config;

	// Randomly select body type
	int32 RandomBodyTypeIndex = FMath::RandRange(0, 17); // 18 body types (0-17)
	Config.BodyType = static_cast<EMetaHumanBodyType>(RandomBodyTypeIndex);

	// Global delta scale (usually keep at 1.0 for full effect)
	Config.GlobalDeltaScale = 1.0f;

	// Enable parametric body
	Config.bUseParametricBody = true;

	// Generate random body measurements
	Config.BodyMeasurements.Empty();
	Config.BodyMeasurements.Add(TEXT("Height"), FMath::FRandRange(150.0f, 195.0f));
	Config.BodyMeasurements.Add(TEXT("Chest"), FMath::FRandRange(75.0f, 120.0f));
	Config.BodyMeasurements.Add(TEXT("Waist"), FMath::FRandRange(60.0f, 100.0f));
	Config.BodyMeasurements.Add(TEXT("Hips"), FMath::FRandRange(80.0f, 120.0f));
	Config.BodyMeasurements.Add(TEXT("ShoulderWidth"), FMath::FRandRange(35.0f, 55.0f));
	Config.BodyMeasurements.Add(TEXT("ArmLength"), FMath::FRandRange(55.0f, 75.0f));
	Config.BodyMeasurements.Add(TEXT("LegLength"), FMath::FRandRange(75.0f, 105.0f));

	// Quality level
	Config.QualityLevel = QualityLevel;

	return Config;
}

FMetaHumanAppearanceConfig ARandomGenActor::GenerateRandomAppearanceConfig()
{
	FMetaHumanAppearanceConfig Config;

	// Random skin tone (UV coordinates in 0-1 range)
	Config.SkinToneU = FMath::FRandRange(0.0f, 1.0f);
	Config.SkinToneV = FMath::FRandRange(0.0f, 1.0f);

	// Skin roughness (0.5 - 1.5 is a reasonable range)
	Config.SkinRoughness = FMath::FRandRange(0.5f, 1.5f);

	// Random iris pattern (there are multiple Iris patterns, let's use a random enum value)
	// Note: You may need to adjust this based on the actual available patterns
	int32 RandomIrisPattern = FMath::RandRange(0, 10); // Assuming ~10 iris patterns
	Config.IrisPattern = static_cast<EMetaHumanCharacterEyesIrisPattern>(RandomIrisPattern);

	// Random iris color
	Config.IrisPrimaryColorU = FMath::FRandRange(0.0f, 1.0f);
	Config.IrisPrimaryColorV = FMath::FRandRange(0.0f, 1.0f);

	// Random eyelashes type
	int32 RandomEyelashType = FMath::RandRange(0, 2); // Assuming 3 types (Thin, Medium, Thick)
	Config.EyelashesType = static_cast<EMetaHumanCharacterEyelashesType>(RandomEyelashType);

	// Enable eyelash grooms (randomly)
	Config.bEnableEyelashGrooms = FMath::RandBool();

	return Config;
}

FString ARandomGenActor::GenerateUniqueCharacterName()
{
	FDateTime Now = FDateTime::Now();

	// Format: RandomChar_MMDD_HHMMSS
	FString Name = FString::Printf(TEXT("RandomChar_%02d%02d_%02d%02d%02d"),
		Now.GetMonth(), Now.GetDay(),
		Now.GetHour(), Now.GetMinute(), Now.GetSecond());

	return Name;
}
