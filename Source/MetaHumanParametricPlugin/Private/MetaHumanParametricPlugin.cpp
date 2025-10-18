// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetaHumanParametricPlugin.h"
#include "MetaHumanParametricGenerator.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

#define LOCTEXT_NAMESPACE "FMetaHumanParametricPluginModule"

// Forward declarations for example functions
extern void Example1_CreateSlenderFemale();
extern void Example2_CreateMuscularMale();
extern void Example3_CreateShortRoundedCharacter();
extern void Example4_BatchCreateCharacters();
extern void Example_PluginTest();

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

	Section.AddSubMenu(
		"MetaHumanExamples",
		LOCTEXT("MetaHumanExamplesLabel", "MetaHuman Examples"),
		LOCTEXT("MetaHumanExamplesTooltip", "Generate example MetaHuman characters"),
		FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
		{
			FToolMenuSection& SubSection = SubMenu->AddSection("Examples", LOCTEXT("Examples", "Character Examples"));

			// TEST: Plugin basics test
			FToolMenuEntry& EntryTest = SubSection.AddMenuEntry(
				"PluginTest",
				LOCTEXT("PluginTestLabel", "0. Run Plugin Test (Safe)"),
				LOCTEXT("PluginTestTooltip", "Test plugin functionality without MetaHuman dependencies"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnRunPluginTest))
			);

			SubSection.AddSeparator("TestSeparator");

			FToolMenuEntry& Entry1 = SubSection.AddMenuEntry(
				"SlenderFemale",
				LOCTEXT("SlenderFemaleLabel", "1. Slender Female"),
				LOCTEXT("SlenderFemaleTooltip", "Create a slender female character (168cm, 62cm waist)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnGenerateSlenderFemale))
			);

			FToolMenuEntry& Entry2 = SubSection.AddMenuEntry(
				"MuscularMale",
				LOCTEXT("MuscularMaleLabel", "2. Muscular Male"),
				LOCTEXT("MuscularMaleTooltip", "Create a muscular male character (185cm, 110cm chest)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnGenerateMuscularMale))
			);

			FToolMenuEntry& Entry3 = SubSection.AddMenuEntry(
				"ShortRounded",
				LOCTEXT("ShortRoundedLabel", "3. Short Rounded"),
				LOCTEXT("ShortRoundedTooltip", "Create a short rounded character (155cm)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnGenerateShortRounded))
			);

			// 示例 4: 批量生成
			FToolMenuEntry& Entry4 = SubSection.AddMenuEntry(
				"BatchGenerate",
				LOCTEXT("BatchGenerateLabel", "4. Batch Generate (5 Characters)"),
				LOCTEXT("BatchGenerateTooltip", "Generate 5 different character types in one go"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnBatchGenerate))
			);
		}),
		false,
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports")
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
}

void FMetaHumanParametricPluginModule::OnGenerateSlenderFemale()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Example 1: Slender Female..."));

	FNotificationInfo Info(LOCTEXT("GeneratingSlenderFemale", "Generating Slender Female Character... (Running in background)"));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// Run generation in background thread to allow blocking operations (AutoRig wait loop)
	Async(EAsyncExecution::Thread, []()
	{
		UE_LOG(LogTemp, Log, TEXT("=== Background Thread: Starting character generation ==="));
		Example1_CreateSlenderFemale();

		// Show completion notification on game thread
		AsyncTask(ENamedThreads::GameThread, []()
		{
			FNotificationInfo CompletedInfo(LOCTEXT("GenerationComplete", "Character Generation Complete! Check Output Log."));
			CompletedInfo.ExpireDuration = 5.0f;
			FSlateNotificationManager::Get().AddNotification(CompletedInfo);
			UE_LOG(LogTemp, Log, TEXT("=== Background Thread: Character generation completed ==="));
		});
	});
}

void FMetaHumanParametricPluginModule::OnGenerateMuscularMale()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Example 2: Muscular Male..."));

	FNotificationInfo Info(LOCTEXT("GeneratingMuscularMale", "Generating Muscular Male Character..."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	Example2_CreateMuscularMale();

	FNotificationInfo CompletedInfo(LOCTEXT("GenerationComplete2", "Character Generation Complete! Check Output Log."));
	CompletedInfo.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(CompletedInfo);
}

void FMetaHumanParametricPluginModule::OnGenerateShortRounded()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Example 3: Short Rounded..."));

	FNotificationInfo Info(LOCTEXT("GeneratingShortRounded", "Generating Short Rounded Character..."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	Example3_CreateShortRoundedCharacter();

	FNotificationInfo CompletedInfo(LOCTEXT("GenerationComplete3", "Character Generation Complete! Check Output Log."));
	CompletedInfo.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(CompletedInfo);
}

void FMetaHumanParametricPluginModule::OnBatchGenerate()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Example 4: Batch Generate..."));

	FNotificationInfo Info(LOCTEXT("BatchGenerating", "Batch Generating 5 Characters... This may take a while."));
	Info.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	Example4_BatchCreateCharacters();

	FNotificationInfo CompletedInfo(LOCTEXT("BatchComplete", "Batch Generation Complete! Check Output Log for results."));
	CompletedInfo.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(CompletedInfo);
}

void FMetaHumanParametricPluginModule::OnRunPluginTest()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Plugin Test..."));

	FNotificationInfo Info(LOCTEXT("RunningPluginTest", "Running Plugin Test (Safe - No MetaHuman Dependencies)"));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	Example_PluginTest();

	FNotificationInfo CompletedInfo(LOCTEXT("PluginTestComplete", "Plugin Test Complete! Check Output Log for results."));
	CompletedInfo.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(CompletedInfo);
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetaHumanParametricPluginModule, MetaHumanParametricPlugin)
