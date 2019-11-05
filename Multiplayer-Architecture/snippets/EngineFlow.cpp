// IS_PROGRAM 0
// WITH_ENGINE 1
// WITH_COREUOBJECT 1
// BUILD_EMBEDDED_APP 0
// WITH_APPLICATION_CORE 0
// FPlatformProperties::IsProgram() == false
// FPlatformProperties::RequiresCookedData() == true

// 1st pass: high-level function calls in the flow
// 2nd pass: in-depth function calls breakdown
// 3rd pass: necessarity checks on every lines brokendown
// Final pass: simplification

FEngineLoop GEngineLoop;
int32 GuardedMain(const TCHAR* CmdLine)
	FEngineLoop::PreInit(CmdLine) // TODO: 3rd pass
	{
		FMemory::SetupTLSCachesOnCurrentThread(); // Thread-Local Storage
		FLowLevelMemTracker::Get().ProcessCommandLine(CmdLine); // Low-Level Memory Tracker
		FPlatformMisc::InitTaggedStorage(1024);
		FPlatformProcess::SetCurrentWorkingDirectoryToBaseDir();
		FCommandLine::Set(CmdLine);
		GError = FPlatformOutputDevices::GetError();
		GWarn = FPlatformOutputDevices::GetFeedbackContext();
		IFileManager::Get().ProcessCommandLineOptions();
		FPlatformProcess::SetThreadAffinityMask(FPlatformAffinity::GetMainGameMask());
		FMath::RandInit(Seed1);
		FMath::SRandInit(Seed2);
		FPaths::SetProjectFilePath(ProjectFilePath);
		IProjectManager::Get().LoadProjectFile(FPaths::GetProjectFilePath());
		FPlatformProcess::AddDllDirectory(*ProjectBinariesDirectory);
		FModuleManager::Get().SetGameBinariesDirectory(*ProjectBinariesDirectory);
		FTaskGraphInterface::Startup(FPlatformMisc::NumberOfCores());
			FTaskGraphImplementation(NumThreads);
				int32 MaxTaskThreads = MAX_THREADS;
				int32 NumTaskThreads = FPlatformMisc::NumberOfWorkerThreadsToSpawn();
				// ...
				for (int32 ThreadIndex = 0; ThreadIndex < NumThreads; ThreadIndex++)
					WorkerThreads[ThreadIndex].TaskGraphWorker = new FTaskThreadAnyThread(...);
					WorkerThreads[ThreadIndex].TaskGraphWorker->Setup(...);
				TaskGraphImplementationSingleton = this; // now reentrancy is ok
				// ...
		FTaskGraphInterface::Get().AttachToThread(ENamedThreads::GameThread);
		FEngineLoop::LoadCoreModules();
			FModuleManager::Get().LoadModule(TEXT("CoreUObject"));
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
			FModuleManager::Get().LoadModule(TEXT("Engine"));
			FModuleManager::Get().LoadModule(TEXT("Renderer"));
			FModuleManager::Get().LoadModule(TEXT("AnimGraphRuntime"));
			FPlatformApplicationMisc::LoadPreInitModules(); // Linux
			FModuleManager::Get().LoadModule(TEXT("Landscape"));
			FModuleManager::Get().LoadModule(TEXT("RenderCore"));
		AppLifetimeEventCapture::Init();
		FEngineLoop::AppInit();
			BeginInitTextLocalization();
			FPlatformMisc::PlatformPreInit();
				FGenericCrashContext::Initialize();
			IFileManager::Get().ProcessCommandLineOptions();
			FPageAllocator::LatchProtectedMode();
			FPlatformOutputDevices::SetupOutputDevices();
			FConfigCacheIni::InitializeConfigSystem();
				GConfig = new FConfigCacheIni(EConfigCacheType::DiskBacked);
				FConfigCacheIni::LoadGlobalIniFile(GEngineIni, TEXT("Engine"), nullptr, bDefaultEngineIniRequired);
				FConfigCacheIni::LoadGlobalIniFile(GGameIni, TEXT("Game"));
				FConfigCacheIni::LoadGlobalIniFile(GInputIni, TEXT("Input"));
				FConfigCacheIni::LoadGlobalIniFile(GScalabilityIni, TEXT("Scalability"), ScalabilityPlatformOverride);
				FConfigCacheIni::LoadGlobalIniFile(GHardwareIni, TEXT("Hardware"));
				FConfigCacheIni::LoadGlobalIniFile(GGameUserSettingsIni, TEXT("GameUserSettings"));
				GConfig->bIsReadyForUse = true;
				FCoreDelegates::ConfigReadyForUse.Broadcast();
			ProjectManager.LoadModulesForProject(ELoadingPhase::EarliestPossible);
			PluginManager.LoadModulesForEnabledPlugins(ELoadingPhase::EarliestPossible);
			FPlatformStackWalk::Init(); // FUnixPlatformStackWalk
			FLogSuppressionInterface::Get().ProcessConfigAndCommandLine();
			ProjectManager.LoadModulesForProject(ELoadingPhase::PostConfigInit);
			PluginManager.LoadModulesForEnabledPlugins(ELoadingPhase::PostConfigInit);
			if (GLogConsole && FParse::Param(FCommandLine::Get(), TEXT("LOG")))
				GLogConsole->Show(true);
			FApp::PrintStartupLogMessages();
				UE_LOG(LogInit, Log, TEXT("Build: %s"), FApp::GetBuildVersion());
				UE_LOG(LogInit, Log, TEXT("Engine Version: %s"), *FEngineVersion::Current().ToString());
				UE_LOG(LogInit, Log, TEXT("Net CL: %u"), FNetworkVersion::GetNetworkCompatibleChangelist());
				// ...
			FCoreDelegates::OnInit.Broadcast();
				InitUObject();
					FGCCSyncObject::Create();
					for (const TPair<FString,FConfigFile>& It : *GConfig)
						FCoreRedirects::ReadRedirectsFromIni(It.Key);
						FLinkerLoad::CreateActiveRedirectsMap(It.Key);
					FCoreDelegates::OnExit.AddStatic(StaticExit);
					// ...
					StaticUObjectInit();
						UObjectBaseInit();
							GConfig->GetInt(TEXT("/Script/Engine.GarbageCollectionSettings"), ...);
							GUObjectAllocator.AllocatePermanentObjectPool(SizeOfPermanentObjectPool);
							GUObjectArray.AllocateObjectPool(MaxUObjects, MaxObjectsNotConsideredByGC, ...);
							InitAsyncThread();
								FAsyncLoadingThread::Get().InitializeAsyncThread();
									// ...
							Internal::GObjInitialized = true;
							UObjectProcessRegistrants();
								// ...
						GObjTransientPkg = NewObject<UPackage>(nullptr, TEXT("/Engine/Transient"), RF_Transient);
						GObjTransientPkg->AddToRoot();
						GShouldVerifyGCAssumptions = FParse::Param(FCommandLine::Get(), TEXT("VERIFYGC"));
		FPlatformFileManager::Get().InitializeNewAsyncIO();
		if (FPlatformProcess::SupportsMultithreading())
			GIOThreadPool = FQueuedThreadPool::Allocate();
			int32 NumThreadsInThreadPool = FPlatformMisc::NumberOfIOWorkerThreadsToSpawn();
			if (FPlatformProperties::IsServerOnly())
				NumThreadsInThreadPool = 2;
			GIOThreadPool->Create(NumThreadsInThreadPool, 96 * 1024, TPri_AboveNormal);
		GSystemSettings.Initialize(bHasEditorToken);
		ApplyCVarSettingsFromIni(TEXT("/Script/Engine.RendererSettings"), *GEngineIni, ...);
		ApplyCVarSettingsFromIni(TEXT("/Script/Engine.RendererOverrideSettings"), *GEngineIni, ...);
		ApplyCVarSettingsFromIni(TEXT("/Script/Engine.StreamingSettings"), *GEngineIni, ...);
		ApplyCVarSettingsFromIni(TEXT("/Script/Engine.GarbageCollectionSettings"), *GEngineIni, ...);
		ApplyCVarSettingsFromIni(TEXT("/Script/Engine.NetworkSettings"), *GEngineIni, ...);
		FConfigCacheIni::LoadConsoleVariablesFromINI();
			ApplyCVarSettingsFromIni(TEXT("ConsoleVariables"), *GEngineIni, ...);
			IConsoleManager::Get().CallAllConsoleVariableSinks();
		FPlatformMisc::PlatformInit();
			FUnixPlatformMisc::PlatformInit();
				InstallChildExitedSignalHanlder(); // not my typo, stock UE code! :)
				UnixPlatForm_CheckIfKSMUsable();
				UnixPlatform_UpdateCacheLineSize();
				UnixPlatformStackWalk_PreloadModuleSymbolFile();
		FPlatformMemory::Init();
			FUnixPlatformMemory::Init();
				FGenericPlatformMemory::Init();
					SetupMemoryPools();
		InitGamePhys();
			InitGamePhysCore();
				#if INCLUDE_CHAOS
					FModuleManager::Get().LoadModule("Chaos");
					FModuleManager::Get().LoadModule("ChaosSolvers");
					FModuleManager::Get().LoadModule("ChaosSolverEngine");
				#if WITH_PHYSX
					PhysDLLHelper::LoadPhysXModules(/*bLoadCookingModule=*/ false);
					GPhysXFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, *GPhysXAllocator, *ErrorCallback);
					GPhysXVisualDebugger = PxCreatePvd(*GPhysXFoundation);
					GPhysXSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *GPhysXFoundation, PScale, false, GPhysXVisualDebugger);
					FPhysxSharedData::Initialize();
					PxInitExtensions(*GPhysXSDK, GPhysXVisualDebugger);
					PxRegisterHeightFields(*GPhysXSDK);
					#if WITH_APEX
						apex::ApexSDKDesc ApexDesc;
						GApexSDK = apex::CreateApexSDK(ApexDesc, &ErrorCode);
						#if WITH_APEX_CLOTHING
						GApexModuleClothing = static_cast<apex::ModuleClothing*>(GApexSDK->createModule("Clothing"));
						GApexModuleClothing->init(*ModuleParams);
		InitEngineTextLocalization();
		if (!IsRunningDedicatedServer())
			FPlatformSplash::Show();
		if (!IsRunningDedicatedServer())
			FSlateApplication::Create();
		else // IsRunningDedicatedServer() == true, still initialize required basics
			EKeys::Initialize();
			FCoreStyle::ResetToDefault();
		FShaderParametersMetadata::InitializeAllGlobalStructs();
		RHIInit(bHasEditorToken); // Render Hardware Interface
			if (!FApp::CanEverRender())
				InitNullRHI();
					// ...
		RenderUtilsInit();
			static IConsoleVariable* DBufferVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DBuffer"));
			if (DBufferVar && DBufferVar->GetInt())
				GDBufferPlatformMask = ~0u;
			// ...
		FShaderCodeLibrary::InitForRuntime(GMaxRHIShaderPlatform);
		FShaderPipelineCache::Initialize(GMaxRHIShaderPlatform);
		InitializeShaderTypes();
		InitGameTextLocalization();
		FPackageName::RegisterShortPackageNamesForUObjectModules();
		ProcessNewlyLoadedUObjects();
			UClassRegisterAllCompiledInClasses();
			UObjectProcessRegistrants();
			UObjectLoadAllCompiledInStructs();
			UObjectLoadAllCompiledInDefaultProperties();
		UMaterialInterface::InitDefaultMaterials();
		UMaterialInterface::AssertDefaultMaterialsExist();
		UMaterialInterface::AssertDefaultMaterialsPostLoaded();
		IStreamingManager::Get();
			StreamingManagerCollection = new FStreamingManagerCollection();
				TextureStreamingManager = new FRenderAssetStreamingManager();
				AddStreamingManager(TextureStreamingManager);
				AudioStreamingManager = new FAudioStreamingManager();
				AddStreamingManager(AudioStreamingManager);
				AnimationStreamingManager = new FAnimationStreamingManager();
				AddStreamingManager(AnimationStreamingManager);
			return *StreamingManagerCollection;
		FModuleManager::Get().StartProcessingNewlyLoadedObjects();
		bool bDisableDisregardForGC = GUseDisregardForGCOnDedicatedServers == 0;
		if (bDisableDisregardForGC)
			GUObjectArray.DisableDisregardForGC();
		FEngineLoop::LoadStartupCoreModules();
			FModuleManager::Get().LoadModule(TEXT("Core"));
			FModuleManager::Get().LoadModule(TEXT("Networking"));
			FPlatformApplicationMisc::LoadStartupModules(); // Linux
			if (FPlatformProcess::SupportsMultithreading())
				FModuleManager::LoadModuleChecked<IMessagingModule>("Messaging");
			FModuleManager::Get().LoadModule(TEXT("ClothingSystemRuntime"));
			FModuleManager::Get().LoadModule(TEXT("PacketHandler"));
			FModuleManager::Get().LoadModule(TEXT("NetworkReplayStreaming"));
		IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PreLoadingScreen);
		IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PreLoadingScreen);
		FPlatformApplicationMisc::PostInit();
		PostInitRHI();
			RHIPostInit(PixelFormatByteWidth);
		FCoreUObjectDelegates::PreGarbageCollectConditionalBeginDestroy.AddStatic(StartRenderCommandFenceBundler);
		FCoreUObjectDelegates::PostGarbageCollectConditionalBeginDestroy.AddStatic(StopRenderCommandFenceBundler);
		FEngineLoop::LoadStartupModules();
			IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PreDefault);
			IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PreDefault);
			IProjectManager::Get().LoadModulesForProject(ELoadingPhase::Default);
			IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::Default);
			IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PostDefault);
			IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PostDefault);
		if (GUObjectArray.IsOpenForDisregardForGC())
			GUObjectArray.CloseDisregardForGC();
		IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PostEngineInit);
		IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PostEngineInit);
	}
	FEngineLoop::Init() // TODO: 2nd pass
	{
		GConfig->GetString(TEXT("/Script/Engine.Engine"), TEXT("GameEngine"), GameEngineClassName, GEngineIni);
		EngineClass = StaticLoadClass( UGameEngine::StaticClass(), nullptr, *GameEngineClassName);
		GEngine = NewObject<UEngine>(GetTransientPackage(), EngineClass);
		GEngine->ParseCommandline();
		FEngineLoop::InitTime();
		GEngine->Init(this);
		UEngine::OnPostEngineInit.Broadcast();
		FCoreDelegates::OnPostEngineInit.Broadcast();
		if (FPlatformProcess::SupportsMultithreading())
			SessionService = FModuleManager::LoadModuleChecked<ISessionServicesModule>("SessionServices").GetSessionService();
			SessionService->Start();
			EngineService = new FEngineService();
		IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PostEngineInit);
		IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PostEngineInit);
		GEngine->Start();
		GIsRunning = true;
		FThreadHeartBeat::Get().Start();
		FCoreDelegates::OnFEngineLoopInitComplete.Broadcast();
	}
	FEngineLoop::Tick() // TODO: 2nd pass
	{
		FLowLevelMemTracker::Get().UpdateStatsPerFrame();
		FThreadHeartBeat::Get().HeartBeat(true);
		FGameThreadHitchHeartBeat::Get().FrameStart();
		FPlatformMisc::TickHotfixables();
		FPlatformMisc::BeginNamedEventFrame();
		uint64 CurrentFrameCounter = GFrameCounter;
		IConsoleManager::Get().CallAllConsoleVariableSinks();
		FCoreDelegates::OnBeginFrame.Broadcast();
		GLog->FlushThreadedLogs();
		GEngine->UpdateTimeAndHandleMaxTickRate();
		GEngine->TickPerformanceMonitoring( FApp::GetDeltaTime() );
		ResetAsyncLoadingStats();
		GMalloc->UpdateStats();
		CalculateFPSTimings();
		FPlatformApplicationMisc::PumpMessages(true);
		FPlatformFileManager::Get().TickActivePlatformFile();
		GInputTime = FPlatformTime::Cycles64();
		GEngine->Tick(FApp::GetDeltaTime(), bIdleMode);
		GShaderCompilingManager->ProcessAsyncResults(true, false);
		GDistanceFieldAsyncQueue->ProcessAsyncTasks();
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(ConcurrentTask);
		RHITick(FApp::GetDeltaTime()); // Update RHI.
		GFrameCounter++;
		TotalTickTime += FApp::GetDeltaTime();
		FTicker::GetCoreTicker().Tick(FApp::GetDeltaTime());
		FThreadManager::Get().Tick();
		GEngine->TickDeferredCommands();
		FCoreDelegates::OnEndFrame.Broadcast();
	}
	FEngineLoop::Exit() // TODO: 2nd pass
	{
		GIsRunning = 0;
		GLogConsole = nullptr;
		FlushAsyncLoading();
		UTexture2D::CancelPendingTextureStreaming();
		delete EngineService; // shut down messaging
		EngineService = nullptr;
		SessionService->Stop();
		SessionService.Reset();
		GEngine->PreExit();
		RHIExitAndStopRHIThread(); // Render Hardware Interface
		FTaskGraphInterface::Shutdown();
		IStreamingManager::Shutdown();
		FPlatformMisc::ShutdownTaggedStorage();
	}
	return ErrorLevel;