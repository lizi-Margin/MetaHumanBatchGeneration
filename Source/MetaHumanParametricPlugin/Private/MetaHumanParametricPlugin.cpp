// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetaHumanParametricPlugin.h"
#include "MetaHumanParametricGenerator.h"
#include "MetaHumanBlueprintExporter.h"
#include "EditorBatchGenerationSubsystem.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "FMetaHumanParametricPluginModule"

// Define static member variables for two-step workflow
UMetaHumanCharacter* FMetaHumanParametricPluginModule::LastGeneratedCharacter = nullptr;
FString FMetaHumanParametricPluginModule::LastOutputPath = TEXT("");
EMetaHumanQualityLevel FMetaHumanParametricPluginModule::LastQualityLevel = EMetaHumanQualityLevel::Cinematic;

void FMetaHumanParametricPluginModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("MetaHumanParametricPlugin module has been loaded"));

	// Register menu extensions
	RegisterMenuExtensions();
}

void FMetaHumanParametricPluginModule::ShutdownModule()
{
	UToolMenus::UnregisterOwner(this);

	UE_LOG(LogTemp, Log, TEXT("MetaHumanParametricPlugin module has been unloaded"));
}

void FMetaHumanParametricPluginModule::RegisterMenuExtensions()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateStatic(&FMetaHumanParametricPluginModule::AddToolbarExtension)
	);
}

void FMetaHumanParametricPluginModule::AddToolbarExtension()
{
	UE_LOG(LogTemp, Warning, TEXT("AddToolbarExtension called"));
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
	if (!Menu)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to extend LevelEditor.LevelEditorToolBar.User menu"));
		return;
	}

	FToolMenuSection& Section = Menu->FindOrAddSection("MetaHumanGenerator");
	Section.Label = LOCTEXT("MetaHumanGenerator", "MetaHuman Generator");

	// Add Two-Step Workflow submenu (New!)
	Section.AddSubMenu(
		"MetaHumanTwoStep",
		LOCTEXT("MetaHumanTwoStepLabel", "Two-Step Workflow (Recommended)"),
		LOCTEXT("MetaHumanTwoStepTooltip", "Non-blocking character generation using two-step approach"),
		FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
		{
			FToolMenuSection& TwoStepSection = SubMenu->AddSection("TwoStep", LOCTEXT("TwoStep", "Two-Step Generation"));

			// Step 1: Prepare & Rig
			TwoStepSection.AddMenuEntry(
				"Step1PrepareRig",
				LOCTEXT("Step1PrepareRigLabel", "Step 1: Prepare & Rig Character"),
				LOCTEXT("Step1PrepareRigTooltip", "Create character, configure, and start AutoRig (returns immediately)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnStep1PrepareAndRig))
			);

			// Check Status
			TwoStepSection.AddMenuEntry(
				"CheckStatus",
				LOCTEXT("CheckStatusLabel", "Check Rigging Status"),
				LOCTEXT("CheckStatusTooltip", "Check if AutoRig is complete"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnCheckRiggingStatus))
			);

			// Step 2: Assemble
			TwoStepSection.AddMenuEntry(
				"Step2Assemble",
				LOCTEXT("Step2AssembleLabel", "Step 2: Assemble Character"),
				LOCTEXT("Step2AssembleTooltip", "Assemble character after AutoRig completes"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnStep2Assemble))
			);

			// Add separator
			TwoStepSection.AddSeparator("ExportSeparator");

			// Export with Animation BP
			TwoStepSection.AddMenuEntry(
				"ExportWithAnimBP",
				LOCTEXT("ExportWithAnimBPLabel", "Export Mesh & Create Preview BP"),
				LOCTEXT("ExportWithAnimBPTooltip", "Export skeletal mesh and create preview Blueprint with animation BP (after Step 2)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnExportWithAnimBP))
			);
		}),
		false,
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details")
	);

	// Add Authentication submenu
	Section.AddSubMenu(
		"MetaHumanAuthentication",
		LOCTEXT("MetaHumanAuthenticationLabel", "Cloud Authentication"),
		LOCTEXT("MetaHumanAuthenticationTooltip", "Test and debug MetaHuman cloud services authentication"),
		FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
		{
			FToolMenuSection& AuthSection = SubMenu->AddSection("Authentication", LOCTEXT("Authentication", "Authentication Tools"));

			// Check Authentication Status
			AuthSection.AddMenuEntry(
				"CheckAuth",
				LOCTEXT("CheckAuthLabel", "Check Login Status"),
				LOCTEXT("CheckAuthTooltip", "Check if currently logged into MetaHuman cloud services"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnCheckAuthentication))
			);

			// Login to Cloud Services
			AuthSection.AddMenuEntry(
				"LoginAuth",
				LOCTEXT("LoginAuthLabel", "Login to Cloud Services"),
				LOCTEXT("LoginAuthTooltip", "Attempt to login to MetaHuman cloud services (may open browser)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnLoginToCloudServices))
			);

			// Test Full Authentication Flow
			AuthSection.AddMenuEntry(
				"TestAuth",
				LOCTEXT("TestAuthLabel", "Test Full Authentication"),
				LOCTEXT("TestAuthTooltip", "Run complete authentication test (check + login if needed)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnTestAuthentication))
			);
		}),
		false,
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Lock")
	);

	// Add Batch Generation submenu
	Section.AddSubMenu(
		"MetaHumanBatchGen",
		LOCTEXT("MetaHumanBatchGenLabel", "Batch Generation (Random)"),
		LOCTEXT("MetaHumanBatchGenTooltip", "Automatic batch character generation with random parameters"),
		FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
		{
			FToolMenuSection& BatchSection = SubMenu->AddSection("BatchGeneration", LOCTEXT("BatchGeneration", "Batch Generation"));

			// Start Batch Generation
			BatchSection.AddMenuEntry(
				"StartBatch",
				LOCTEXT("StartBatchLabel", "Start Batch Generation"),
				LOCTEXT("StartBatchTooltip", "Start generating random characters in the background"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnStartBatchGeneration))
			);

			// Check Batch Status
			BatchSection.AddMenuEntry(
				"CheckBatchStatus",
				LOCTEXT("CheckBatchStatusLabel", "Check Status"),
				LOCTEXT("CheckBatchStatusTooltip", "Check current batch generation progress"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnCheckBatchStatus))
			);

			// Stop Batch Generation
			BatchSection.AddMenuEntry(
				"StopBatch",
				LOCTEXT("StopBatchLabel", "Stop Batch Generation"),
				LOCTEXT("StopBatchTooltip", "Stop the current batch generation process"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnStopBatchGeneration))
			);
		}),
		false,
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Outliner")
	);
}

// ============================================================================
// Authentication Menu Command Callbacks
// ============================================================================

void FMetaHumanParametricPluginModule::OnCheckAuthentication()
{
	UE_LOG(LogTemp, Warning, TEXT("=== Checking MetaHuman Cloud Authentication Status ==="));

	FNotificationInfo Info(LOCTEXT("CheckingAuth", "Checking MetaHuman Cloud Authentication..."));
	Info.ExpireDuration = 2.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// Use async check to avoid blocking the UI
	UMetaHumanParametricGenerator::CheckCloudServicesLoginAsync([](bool bLoggedIn)
	{
		if (bLoggedIn)
		{
			UE_LOG(LogTemp, Log, TEXT("✓ User is logged in to MetaHuman cloud services"));
			UE_LOG(LogTemp, Log, TEXT("  Cloud operations (AutoRig, texture download) are available"));

			FNotificationInfo SuccessInfo(LOCTEXT("AuthCheckSuccess", "✓ Logged In - Cloud services available"));
			SuccessInfo.ExpireDuration = 5.0f;
			FSlateNotificationManager::Get().AddNotification(SuccessInfo);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("✗ User is NOT logged in to MetaHuman cloud services"));
			UE_LOG(LogTemp, Warning, TEXT("  Please login via: Cloud Authentication > Login to Cloud Services"));

			FNotificationInfo WarningInfo(LOCTEXT("AuthCheckFailed", "✗ Not Logged In - Use 'Login to Cloud Services' menu"));
			WarningInfo.ExpireDuration = 7.0f;
			FSlateNotificationManager::Get().AddNotification(WarningInfo);
		}
	});
}

void FMetaHumanParametricPluginModule::OnLoginToCloudServices()
{
	UE_LOG(LogTemp, Warning, TEXT("=== Attempting to Login to MetaHuman Cloud Services ==="));

	FNotificationInfo Info(LOCTEXT("LoggingIn", "Logging into MetaHuman Cloud Services... (May open browser)"));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// First check if already logged in
	UMetaHumanParametricGenerator::CheckCloudServicesLoginAsync([](bool bLoggedIn)
	{
		if (bLoggedIn)
		{
			UE_LOG(LogTemp, Log, TEXT("✓ Already logged in - no action needed"));

			FNotificationInfo AlreadyLoggedInfo(LOCTEXT("AlreadyLoggedIn", "✓ Already Logged In - No action needed"));
			AlreadyLoggedInfo.ExpireDuration = 5.0f;
			FSlateNotificationManager::Get().AddNotification(AlreadyLoggedInfo);
		}
		else
		{
			// Not logged in, attempt login
			UE_LOG(LogTemp, Log, TEXT("Not logged in - attempting login..."));
			UE_LOG(LogTemp, Log, TEXT("  Note: A browser window may open for Epic Games login"));

			FNotificationInfo LoginAttemptInfo(LOCTEXT("LoginAttempt", "Attempting login... Check browser if window opens"));
			LoginAttemptInfo.ExpireDuration = 5.0f;
			FSlateNotificationManager::Get().AddNotification(LoginAttemptInfo);

			UMetaHumanParametricGenerator::LoginToCloudServicesAsync(
				[]()
				{
					// Login succeeded
					UE_LOG(LogTemp, Log, TEXT("✓ Successfully logged in to MetaHuman cloud services"));
					UE_LOG(LogTemp, Log, TEXT("  Cloud operations are now available"));

					FNotificationInfo SuccessInfo(LOCTEXT("LoginSuccess", "✓ Login Succeeded - Cloud services available"));
					SuccessInfo.ExpireDuration = 5.0f;
					FSlateNotificationManager::Get().AddNotification(SuccessInfo);
				},
				[]()
				{
					// Login failed
					UE_LOG(LogTemp, Error, TEXT("✗ Failed to login to MetaHuman cloud services"));
					UE_LOG(LogTemp, Error, TEXT("  Possible causes:"));
					UE_LOG(LogTemp, Error, TEXT("  - Browser login window was not completed"));
					UE_LOG(LogTemp, Error, TEXT("  - Network connectivity issues"));
					UE_LOG(LogTemp, Error, TEXT("  - MetaHuman cloud services unavailable"));
					UE_LOG(LogTemp, Error, TEXT("  - EOS (Epic Online Services) configuration missing"));

					FNotificationInfo ErrorInfo(LOCTEXT("LoginFailed", "✗ Login Failed - Check Output Log for details"));
					ErrorInfo.ExpireDuration = 7.0f;
					FSlateNotificationManager::Get().AddNotification(ErrorInfo);
				}
			);
		}
	});
}

void FMetaHumanParametricPluginModule::OnTestAuthentication()
{
	UE_LOG(LogTemp, Warning, TEXT("=== Running Full MetaHuman Authentication Test ==="));

	FNotificationInfo Info(LOCTEXT("TestingAuth", "Testing MetaHuman Authentication... Check Output Log"));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// Use the built-in test function
	UMetaHumanParametricGenerator::TestCloudAuthentication();

	// The TestCloudAuthentication function will handle its own logging and notifications
	// via the async callbacks, so we just show a completion message
	FNotificationInfo CompletedInfo(LOCTEXT("AuthTestStarted", "Authentication test started - watch Output Log for results"));
	CompletedInfo.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(CompletedInfo);
}

// ============================================================================
// Two-Step Workflow Callbacks
// ============================================================================

void FMetaHumanParametricPluginModule::OnStep1PrepareAndRig()
{
	UE_LOG(LogTemp, Warning, TEXT("=== Two-Step Workflow: Step 1 - Prepare & Rig ==="));

	FNotificationInfo Info(LOCTEXT("Step1Starting", "Step 1: Preparing and starting AutoRig..."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// Create a slender female character as example
	FMetaHumanBodyParametricConfig BodyConfig;
	BodyConfig.BodyType = EMetaHumanBodyType::f_med_nrw;
	BodyConfig.GlobalDeltaScale = 1.0f;
	BodyConfig.bUseParametricBody = true;
	BodyConfig.BodyMeasurements.Add(TEXT("Height"), 168.0f);
	BodyConfig.BodyMeasurements.Add(TEXT("Waist"), 62.0f);
	BodyConfig.BodyMeasurements.Add(TEXT("Chest"), 85.0f);
	BodyConfig.QualityLevel = EMetaHumanQualityLevel::Cinematic;

	FMetaHumanAppearanceConfig AppearanceConfig;

	// Generate unique character name with timestamp to avoid conflicts
	FDateTime Now = FDateTime::Now();
	FString CharacterName = FString::Printf(TEXT("TwoStepTest_%02d%02d_%02d%02d%02d"),
		Now.GetMonth(), Now.GetDay(),
		Now.GetHour(), Now.GetMinute(), Now.GetSecond());
	FString OutputPath = TEXT("/Game/MetaHumans");

	UE_LOG(LogTemp, Log, TEXT("Creating character: %s"), *CharacterName);

	UMetaHumanCharacter* Character = nullptr;
	bool bSuccess = UMetaHumanParametricGenerator::PrepareAndRigCharacter(
		CharacterName,
		OutputPath,
		BodyConfig,
		AppearanceConfig,
		Character
	);

	if (bSuccess && Character)
	{
		// Store for Step 2
		LastGeneratedCharacter = Character;
		LastOutputPath = OutputPath;
		LastQualityLevel = BodyConfig.QualityLevel;

		UE_LOG(LogTemp, Log, TEXT("✓ Step 1 Complete - AutoRig is running in background"));
		UE_LOG(LogTemp, Log, TEXT("Character stored: %s"), *Character->GetName());

		FNotificationInfo SuccessInfo(LOCTEXT("Step1Complete", "✓ Step 1 Complete! AutoRig running in background. Use 'Check Status' to monitor."));
		SuccessInfo.ExpireDuration = 7.0f;
		FSlateNotificationManager::Get().AddNotification(SuccessInfo);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("✗ Step 1 Failed!"));

		FNotificationInfo ErrorInfo(LOCTEXT("Step1Failed", "✓ Step 1 Failed - Check Output Log"));
		ErrorInfo.ExpireDuration = 5.0f;
		FSlateNotificationManager::Get().AddNotification(ErrorInfo);
	}
}

void FMetaHumanParametricPluginModule::OnCheckRiggingStatus()
{
	if (!LastGeneratedCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("No character from Step 1 - please run Step 1 first"));

		FNotificationInfo WarningInfo(LOCTEXT("NoCharacter", "No character found - please run Step 1 first"));
		WarningInfo.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(WarningInfo);
		return;
	}

	FString Status = UMetaHumanParametricGenerator::GetRiggingStatusString(LastGeneratedCharacter);

	UE_LOG(LogTemp, Log, TEXT("=== Rigging Status ==="));
	UE_LOG(LogTemp, Log, TEXT("Character: %s"), *LastGeneratedCharacter->GetName());
	UE_LOG(LogTemp, Log, TEXT("Status: %s"), *Status);

	FString NotificationText = FString::Printf(TEXT("Status: %s"), *Status);
	FNotificationInfo StatusInfo(FText::FromString(NotificationText));
	StatusInfo.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(StatusInfo);
}

void FMetaHumanParametricPluginModule::OnStep2Assemble()
{
	if (!LastGeneratedCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("No character from Step 1 - please run Step 1 first"));

		FNotificationInfo ErrorInfo(LOCTEXT("NoCharacterForStep2", "No character found - please run Step 1 first"));
		ErrorInfo.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(ErrorInfo);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("=== Two-Step Workflow: Step 2 - Assemble ==="));

	FNotificationInfo Info(LOCTEXT("Step2Starting", "Step 2: Assembling character..."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	bool bSuccess = UMetaHumanParametricGenerator::AssembleCharacter(
		LastGeneratedCharacter,
		LastOutputPath,
		LastQualityLevel
	);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("✓ Step 2 Complete - Character fully generated!"));

		FNotificationInfo SuccessInfo(LOCTEXT("Step2Complete", "✓ Step 2 Complete! Character is ready in /Game/MetaHumans/"));
		SuccessInfo.ExpireDuration = 7.0f;
		FSlateNotificationManager::Get().AddNotification(SuccessInfo);

	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("✗ Step 2 Failed! Check if AutoRig is complete."));

		FNotificationInfo ErrorInfo(LOCTEXT("Step2Failed", "✗ Step 2 Failed - Is AutoRig complete? Check status first!"));
		ErrorInfo.ExpireDuration = 7.0f;
		FSlateNotificationManager::Get().AddNotification(ErrorInfo);
	}
}

// ============================================================================
// Batch Generation Callbacks
// ============================================================================

void FMetaHumanParametricPluginModule::OnStartBatchGeneration()
{
	UE_LOG(LogTemp, Warning, TEXT("=== Starting Batch Generation ==="));

	// Get the batch generation subsystem
	UEditorBatchGenerationSubsystem* BatchSubsystem = GEditor->GetEditorSubsystem<UEditorBatchGenerationSubsystem>();
	if (!BatchSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get EditorBatchGenerationSubsystem!"));
		FNotificationInfo ErrorInfo(LOCTEXT("NoSubsystem", "Failed to get batch generation subsystem"));
		ErrorInfo.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(ErrorInfo);
		return;
	}

	// Check if already running
	if (BatchSubsystem->IsRunning())
	{
		UE_LOG(LogTemp, Warning, TEXT("Batch generation already running!"));
		FNotificationInfo WarningInfo(LOCTEXT("AlreadyRunning", "Batch generation already running. Stop it first."));
		WarningInfo.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(WarningInfo);
		return;
	}

	// Start batch generation
	BatchSubsystem->StartBatchGeneration(
		true,                                    // bLoopMode
		TEXT("/Game/MetaHumans"),               // OutputPath
		EMetaHumanQualityLevel::Cinematic,      // QualityLevel
		2.0f,                                    // CheckInterval (check every 2 seconds)
		5.0f                                     // LoopDelay (5 seconds between characters)
	);

	UE_LOG(LogTemp, Log, TEXT("✓ Batch generation started"));
	UE_LOG(LogTemp, Log, TEXT("  Output: /Game/MetaHumans"));
	UE_LOG(LogTemp, Log, TEXT("  Loop Mode: Enabled"));

	FNotificationInfo SuccessInfo(LOCTEXT("BatchStarted", "✓ Batch Generation Started - Characters will generate automatically in loop mode"));
	SuccessInfo.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(SuccessInfo);
}

void FMetaHumanParametricPluginModule::OnStopBatchGeneration()
{
	UE_LOG(LogTemp, Warning, TEXT("=== Stopping Batch Generation ==="));

	// Get the batch generation subsystem
	UEditorBatchGenerationSubsystem* BatchSubsystem = GEditor->GetEditorSubsystem<UEditorBatchGenerationSubsystem>();
	if (!BatchSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get EditorBatchGenerationSubsystem!"));
		return;
	}

	if (!BatchSubsystem->IsRunning())
	{
		UE_LOG(LogTemp, Warning, TEXT("No batch generation running"));
		FNotificationInfo WarningInfo(LOCTEXT("NoBatchRunning", "No batch generation is running"));
		WarningInfo.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(WarningInfo);
		return;
	}

	// Stop the subsystem
	BatchSubsystem->StopBatchGeneration();

	UE_LOG(LogTemp, Log, TEXT("✓ Batch generation stopped"));

	FNotificationInfo SuccessInfo(LOCTEXT("BatchStopped", "✓ Batch Generation Stopped"));
	SuccessInfo.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(SuccessInfo);
}

void FMetaHumanParametricPluginModule::OnCheckBatchStatus()
{
	UE_LOG(LogTemp, Log, TEXT("=== Checking Batch Generation Status ==="));

	// Get the batch generation subsystem
	UEditorBatchGenerationSubsystem* BatchSubsystem = GEditor->GetEditorSubsystem<UEditorBatchGenerationSubsystem>();
	if (!BatchSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get EditorBatchGenerationSubsystem!"));
		return;
	}

	if (!BatchSubsystem->IsRunning())
	{
		UE_LOG(LogTemp, Warning, TEXT("No batch generation running"));
		FNotificationInfo WarningInfo(LOCTEXT("NoBatchForStatus", "No batch generation is running"));
		WarningInfo.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(WarningInfo);
		return;
	}

	// Get status
	EBatchGenState State;
	FString CharacterName;
	int32 Count;
	BatchSubsystem->GetStatusInfo(State, CharacterName, Count);

	FString StateString = BatchSubsystem->GetCurrentStateString();

	UE_LOG(LogTemp, Log, TEXT("Current State: %s"), *StateString);
	UE_LOG(LogTemp, Log, TEXT("Current Character: %s"), CharacterName.IsEmpty() ? TEXT("None") : *CharacterName);
	UE_LOG(LogTemp, Log, TEXT("Characters Generated: %d"), Count);

	FString StatusMessage = FString::Printf(TEXT("State: %s | Count: %d | Current: %s"),
		*StateString, Count, CharacterName.IsEmpty() ? TEXT("None") : *CharacterName);

	FNotificationInfo StatusInfo(FText::FromString(StatusMessage));
	StatusInfo.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(StatusInfo);
}

// ============================================================================
// Export with Animation BP Callback
// ============================================================================

void FMetaHumanParametricPluginModule::OnExportWithAnimBP()
{
	if (!LastGeneratedCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("No character available for export - please complete Step 1 and Step 2 first"));

		FNotificationInfo ErrorInfo(LOCTEXT("NoCharacterForExport", "No character found - please complete Step 1 & 2 first"));
		ErrorInfo.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(ErrorInfo);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("=== Exporting Character with Animation BP ==="));

	FNotificationInfo Info(LOCTEXT("ExportStarting", "Exporting skeletal mesh and creating preview Blueprint..."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// Animation blueprint path - using the user specified path
	FString AnimBlueprintPath = TEXT("/Game/HumanCharacter/Mannequin/Animations/ThirdPerson_AnimBP.ThirdPerson_AnimBP");

	// Output path
	FString OutputPath = TEXT("/Game/ExportedCharacters");

	// Generate base name from character
	FString CharacterName = LastGeneratedCharacter->GetName();
	FString BaseName = CharacterName;

	UE_LOG(LogTemp, Log, TEXT("Character: %s"), *CharacterName);
	UE_LOG(LogTemp, Log, TEXT("Output Path: %s"), *OutputPath);
	UE_LOG(LogTemp, Log, TEXT("Animation BP: %s"), *AnimBlueprintPath);

	// Export character with preview BP
	USkeletalMesh* ExportedMesh = nullptr;
	UBlueprint* PreviewBP = nullptr;

	bool bSuccess = UMetaHumanBlueprintExporter::ExportCharacterWithPreviewBP(
		LastGeneratedCharacter,
		AnimBlueprintPath,
		OutputPath,
		BaseName,
		ExportedMesh,
		PreviewBP
	);

	if (bSuccess && ExportedMesh && PreviewBP)
	{
		UE_LOG(LogTemp, Log, TEXT("✓ Export Complete!"));
		UE_LOG(LogTemp, Log, TEXT("  Skeletal Mesh: %s"), *ExportedMesh->GetPathName());
		UE_LOG(LogTemp, Log, TEXT("  Preview BP: %s"), *PreviewBP->GetPathName());

		FString SuccessMessage = FString::Printf(TEXT("✓ Export Complete! Mesh & BP created in %s"), *OutputPath);
		FNotificationInfo SuccessInfo(FText::FromString(SuccessMessage));
		SuccessInfo.ExpireDuration = 7.0f;
		FSlateNotificationManager::Get().AddNotification(SuccessInfo);

	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("✗ Export Failed! Check Output Log for details."));

		FNotificationInfo ErrorInfo(LOCTEXT("ExportFailed", "✗ Export Failed - Check Output Log for details"));
		ErrorInfo.ExpireDuration = 7.0f;
		FSlateNotificationManager::Get().AddNotification(ErrorInfo);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetaHumanParametricPluginModule, MetaHumanParametricPlugin)
