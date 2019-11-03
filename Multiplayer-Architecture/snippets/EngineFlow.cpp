// IS_PROGRAM 0
// WITH_ENGINE 1
// WITH_COREUOBJECT 1
// BUILD_EMBEDDED_APP 0
// WITH_APPLICATION_CORE 0
// FPlatformProperties::IsProgram() == false
// FPlatformProperties::RequiresCookedData() == true

FEngineLoop GEngineLoop;
int32 GuardedMain(const TCHAR* CmdLine)
	FEngineLoop::PreInit(CmdLine)
	{
		FPlatformProcess::SetCurrentWorkingDirectoryToBaseDir();
		FCommandLine::Set(CmdLine);
		GError = FPlatformOutputDevices::GetError();
		GWarn = FPlatformOutputDevices::GetFeedbackContext();
		IFileManager::Get().ProcessCommandLineOptions();
		GGameThreadId = FPlatformTLS::GetCurrentThreadId();
		GIsGameThreadIdInitialized = true;
		FPlatformProcess::SetThreadAffinityMask(FPlatformAffinity::GetMainGameMask());
		FPlatformProcess::SetupGameThread();
		FMath::RandInit(Seed1);
		FMath::SRandInit(Seed2);
		FPaths::SetProjectFilePath(ProjectFilePath);
		FPlatformProcess::AddDllDirectory(*ProjectBinariesDirectory);
		FModuleManager::Get().SetGameBinariesDirectory(*ProjectBinariesDirectory);
		FTaskGraphInterface::Startup(FPlatformMisc::NumberOfCores());
		FTaskGraphInterface::Get().AttachToThread(ENamedThreads::GameThread);
		FEngineLoop::LoadCoreModules();
		InitializeRenderingCVarsCaching();
		if (FPlatformProcess::SupportsMultithreading())
			int StackSize = 128;
			GThreadPool = FQueuedThreadPool::Allocate();
			int32 NumThreadsInThreadPool = FPlatformMisc::NumberOfWorkerThreadsToSpawn();
			if (FPlatformProperties::IsServerOnly())
				NumThreadsInThreadPool = 1;
			GThreadPool->Create(NumThreadsInThreadPool, StackSize * 1024, TPri_SlightlyBelowNormal);
			GBackgroundPriorityThreadPool = FQueuedThreadPool::Allocate();
			int32 NumThreadsInThreadPool = 2;
			if (FPlatformProperties::IsServerOnly())
				NumThreadsInThreadPool = 1;
			GBackgroundPriorityThreadPool->Create(NumThreadsInThreadPool, 128 * 1024, TPri_Lowest);
		FEngineLoop::LoadPreInitModules();
		AppLifetimeEventCapture::Init();
		AppInit();
		FPlatformFileManager::Get().InitializeNewAsyncIO();
		if (FPlatformProcess::SupportsMultithreading())
			GIOThreadPool = FQueuedThreadPool::Allocate();
			int32 NumThreadsInThreadPool = FPlatformMisc::NumberOfIOWorkerThreadsToSpawn();
			if (FPlatformProperties::IsServerOnly())
				NumThreadsInThreadPool = 2;
			GIOThreadPool->Create(NumThreadsInThreadPool, 96 * 1024, TPri_AboveNormal);
		GSystemSettings.Initialize(bHasEditorToken);
		ApplyCVarSettingsFromIni(TEXT("/Script/Engine.RendererSettings"), *GEngineIni, ECVF_SetByProjectSetting);
		ApplyCVarSettingsFromIni(TEXT("/Script/Engine.RendererOverrideSettings"), *GEngineIni, ECVF_SetByProjectSetting);
		ApplyCVarSettingsFromIni(TEXT("/Script/Engine.StreamingSettings"), *GEngineIni, ECVF_SetByProjectSetting);
		ApplyCVarSettingsFromIni(TEXT("/Script/Engine.GarbageCollectionSettings"), *GEngineIni, ECVF_SetByProjectSetting);
		ApplyCVarSettingsFromIni(TEXT("/Script/Engine.NetworkSettings"), *GEngineIni, ECVF_SetByProjectSetting);
		FConfigCacheIni::LoadConsoleVariablesFromINI();
		FPlatformMisc::PlatformInit();
		FPlatformMemory::Init();
		FPlatformMisc::CommandLineCommands();
		IPlatformFeaturesModule::Get();
		InitGamePhys();
		InitEngineTextLocalization();
		UStringTable::InitializeEngineBridge();
		if (!IsRunningDedicatedServer())
			FPlatformSplash::Show();
		if (!IsRunningDedicatedServer())
			FSlateApplication::Create();
		else // IsRunningDedicatedServer() == true
			EKeys::Initialize();
			FCoreStyle::ResetToDefault();
		FShaderParametersMetadata::InitializeAllGlobalStructs();
		RHIInit(bHasEditorToken); // Render Hardware Interface
		RenderUtilsInit();
		FShaderCodeLibrary::InitForRuntime(GMaxRHIShaderPlatform);
		FShaderPipelineCache::Initialize(GMaxRHIShaderPlatform);
		GetRendererModule();
		InitializeShaderTypes();
		InitGameTextLocalization();
		FPackageName::RegisterShortPackageNamesForUObjectModules();
		ProcessNewlyLoadedUObjects();
		UMaterialInterface::InitDefaultMaterials();
		UMaterialInterface::AssertDefaultMaterialsExist();
		UMaterialInterface::AssertDefaultMaterialsPostLoaded();
		IStreamingManager::Get();
		FModuleManager::Get().StartProcessingNewlyLoadedObjects();
		bool bDisableDisregardForGC = GUseDisregardForGCOnDedicatedServers == 0;
		if (bDisableDisregardForGC)
			GUObjectArray.DisableDisregardForGC();
		FEngineLoop::LoadStartupCoreModules();
		IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PreLoadingScreen);
		IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PreLoadingScreen);
		FPlatformApplicationMisc::PostInit();
		FCoreUObjectDelegates::PreGarbageCollectConditionalBeginDestroy.AddStatic(StartRenderCommandFenceBundler);
		FCoreUObjectDelegates::PostGarbageCollectConditionalBeginDestroy.AddStatic(StopRenderCommandFenceBundler);
		FEngineLoop::LoadStartupModules();
		if (GUObjectArray.IsOpenForDisregardForGC())
			GUObjectArray.CloseDisregardForGC();
		IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PostEngineInit);
		IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PostEngineInit);
		NotifyRegistrationComplete();
	}
	FEngineLoop::Init()
	{
		// TODO
	}
	FEngineLoop::Tick()
	{
		// TODO
	}
	FEngineLoop::Exit()
	{
		// TODO
	}
	return ErrorLevel;