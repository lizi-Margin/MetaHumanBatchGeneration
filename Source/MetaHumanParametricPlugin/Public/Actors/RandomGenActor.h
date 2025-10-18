// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MetaHumanParametricGenerator.h"
#include "RandomGenActor.generated.h"

// Forward declarations
class UMetaHumanCharacter;

/**
 * Generation State Machine
 */
UENUM(BlueprintType)
enum class ERandomGenState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Preparing UMETA(DisplayName = "Preparing Character"),
	WaitingForRig UMETA(DisplayName = "Waiting for AutoRig"),
	Assembling UMETA(DisplayName = "Assembling Character"),
	Complete UMETA(DisplayName = "Complete"),
	Error UMETA(DisplayName = "Error")
};

/**
 * Random MetaHuman Generator Actor
 *
 * This actor automatically generates MetaHuman characters with randomized parameters.
 * It uses a simple tick-based state machine to monitor AutoRig progress and
 * automatically advance through the two-step generation workflow.
 *
 * Usage:
 * 1. Place actor in level
 * 2. Set OutputPath and QualityLevel if desired
 * 3. Set bAutoStartOnBeginPlay to true, or call StartGeneration() manually
 * 4. Actor will automatically generate a random character and save it
 */
UCLASS(Blueprintable)
class METAHUMANPARAMETRICPLUGIN_API ARandomGenActor : public AActor
{
	GENERATED_BODY()

public:
	ARandomGenActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	// ============================================================================
	// Configuration Properties
	// ============================================================================

	/** Whether to automatically start generation on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bAutoStartOnBeginPlay = false;

	/** Output path for generated characters (e.g., "/Game/MetaHumans") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FString OutputPath = TEXT("/Game/MetaHumans");

	/** Quality level for character assembly */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	EMetaHumanQualityLevel QualityLevel = EMetaHumanQualityLevel::Cinematic;

	/** Tick interval for checking rig status (in seconds, 0 = every frame) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (UIMin = "0.0", UIMax = "5.0"))
	float TickInterval = 1.0f;

	/** Whether to destroy the actor after successful generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bDestroyAfterComplete = false;

	/** Whether to generate another character after completion (loop mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bLoopGeneration = false;

	/** Delay before starting next generation in loop mode (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (UIMin = "0.0", UIMax = "60.0"))
	float LoopDelay = 5.0f;



	// ============================================================================
	// Public API
	// ============================================================================

	/**
	 * Start the random character generation process
	 * Can be called from Blueprint or C++
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|RandomGen")
	void StartGeneration();

	/**
	 * Stop the current generation process
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|RandomGen")
	void StopGeneration();

	/**
	 * Get current state as string
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MetaHuman|RandomGen")
	FString GetCurrentStateString() const;

	/**
	 * Get the generated character (only valid after completion)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MetaHuman|RandomGen")
	UMetaHumanCharacter* GetGeneratedCharacter() const { return GeneratedCharacter.Get(); }

	// ============================================================================
	// Status Properties (Read-Only)
	// ============================================================================

	/** Current generation state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	ERandomGenState CurrentState = ERandomGenState::Idle;

	/** Name of the character being generated */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	FString CurrentCharacterName;

	/** Last error message (if any) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	FString LastErrorMessage;

	/** Number of characters generated (in loop mode) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	int32 GeneratedCount = 0;

private:
	// ============================================================================
	// State Machine Implementation
	// ============================================================================

	void UpdateStateMachine(float DeltaTime);
	void TransitionToState(ERandomGenState NewState);

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

	FMetaHumanBodyParametricConfig GenerateRandomBodyConfig();
	FMetaHumanAppearanceConfig GenerateRandomAppearanceConfig();
	FString GenerateUniqueCharacterName();

	// ============================================================================
	// Internal State
	// ============================================================================

	/** Reference to the character being generated (weak pointer to avoid hard reference) */
	TWeakObjectPtr<UMetaHumanCharacter> GeneratedCharacter;

	/** Accumulated delta time for tick interval */
	float AccumulatedDeltaTime = 0.0f;

	/** Timer for loop delay */
	float LoopDelayTimer = 0.0f;

	/** Flag to indicate if we should process state this frame */
	bool bShouldProcessState = false;
};
