// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "MetaHumanParametricGenerator.h"
#include "EditorBatchGenerationSubsystem.generated.h"

// Forward declarations
class UMetaHumanCharacter;

/**
 * Generation State Machine
 */
UENUM(BlueprintType)
enum class EBatchGenState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Preparing UMETA(DisplayName = "Preparing Character"),
	WaitingForRig UMETA(DisplayName = "Waiting for AutoRig"),
	Assembling UMETA(DisplayName = "Assembling Character"),
	Complete UMETA(DisplayName = "Complete"),
	Error UMETA(DisplayName = "Error")
};

/**
 * Editor Batch Generation Subsystem
 *
 * This subsystem runs in the editor and manages automatic MetaHuman character generation
 * with randomized parameters. Unlike actors, editor subsystems work without running the game.
 *
 * Uses FTSTicker (editor timer) to periodically check AutoRig status and advance the state machine.
 */
UCLASS()
class METAHUMANPARAMETRICPLUGIN_API UEditorBatchGenerationSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ============================================================================
	// Public API
	// ============================================================================

	/**
	 * Start the batch generation process
	 * @param bLoopMode - If true, continuously generate characters
	 * @param OutputPath - Where to save generated characters
	 * @param QualityLevel - Quality level for character assembly
	 * @param CheckInterval - How often to check AutoRig status (seconds)
	 * @param LoopDelay - Delay between characters in loop mode (seconds)
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|BatchGen")
	void StartBatchGeneration(
		bool bLoopMode = true,
		FString OutputPath = TEXT("/Game/MetaHumans"),
		EMetaHumanQualityLevel QualityLevel = EMetaHumanQualityLevel::Cinematic,
		float CheckInterval = 2.0f,
		float LoopDelay = 5.0f);

	/**
	 * Stop the current batch generation process
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|BatchGen")
	void StopBatchGeneration();

	/**
	 * Get current state as string
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MetaHuman|BatchGen")
	FString GetCurrentStateString() const;

	/**
	 * Check if batch generation is currently running
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MetaHuman|BatchGen")
	bool IsRunning() const { return CurrentState != EBatchGenState::Idle && CurrentState != EBatchGenState::Error; }

	/**
	 * Get status information
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|BatchGen")
	void GetStatusInfo(EBatchGenState& OutState, FString& OutCharacterName, int32& OutGeneratedCount) const;

private:
	// ============================================================================
	// State Machine Implementation
	// ============================================================================

	/**
	 * Ticker callback - called periodically to update state machine
	 * This is how we "tick" in the editor without running the game
	 */
	bool TickStateMachine(float DeltaTime);

	void TransitionToState(EBatchGenState NewState);

	// State handlers
	void HandleIdleState();
	void HandlePreparingState();
	void HandleWaitingForRigState();
	void HandleAssemblingState();
	void HandleCompleteState();
	void HandleErrorState();

	// ============================================================================
	// Random Parameter Generation
	// ============================================================================

	void GenerateRandomCharacterConfigs(
		FMetaHumanBodyParametricConfig& OutBodyConfig,
		FMetaHumanAppearanceConfig& OutAppearanceConfig,
		FString& OutCharacterName);

	// ============================================================================
	// Internal State
	// ============================================================================

	/** Current generation state */
	EBatchGenState CurrentState = EBatchGenState::Idle;

	/** Reference to the character being generated */
	TWeakObjectPtr<UMetaHumanCharacter> GeneratedCharacter;

	/** Current character name */
	FString CurrentCharacterName;

	/** Last error message */
	FString LastErrorMessage;

	/** Number of characters generated */
	int32 GeneratedCount = 0;

	/** Configuration */
	bool bLoopGenerationEnabled = false;
	FString OutputPathConfig;
	EMetaHumanQualityLevel QualityLevelConfig = EMetaHumanQualityLevel::Cinematic;
	float CheckIntervalConfig = 2.0f;
	float LoopDelayConfig = 5.0f;

	/** Loop delay timer */
	float LoopDelayTimer = 0.0f;

	/** Ticker handle for the state machine update */
	FTSTicker::FDelegateHandle TickerHandle;

	/** Flag to indicate state should be processed */
	bool bShouldProcessState = true;
};
