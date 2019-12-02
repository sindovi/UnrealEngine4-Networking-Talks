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
						GObjTransientPkg->AddToRoot(); // Prevent from GC
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
	FEngineLoop::Init() // TODO: 3rd pass
	{
		GConfig->GetString(TEXT("/Script/Engine.Engine"), TEXT("GameEngine"), GameEngineClassName, GEngineIni);
		EngineClass = StaticLoadClass(UGameEngine::StaticClass(), nullptr, *GameEngineClassName);
		GEngine = NewObject<UEngine>(GetTransientPackage(), EngineClass);
		GEngine->ParseCommandline();
		FEngineLoop::InitTime();
			FApp::SetCurrentTime(FPlatformTime::Seconds());
			MaxFrameCounter = 0;
			MaxTickTime = 0;
			TotalTickTime = 0;
			LastFrameCycles = FPlatformTime::Cycles();
		GEngine->Init(this);
			UGameEngine::Init(InEngineLoop);
				UEngine::Init(InEngineLoop);
					FURL::StaticInit();
					FLinkerLoad::StaticInit(UTexture2D::StaticClass());
					EngineSubsystemCollection.Initialize();
						TArray<UClass*> SubsystemClasses;
						GetDerivedClasses(UEngineSubsystem, SubsystemClasses, /* bRecursive= */ true);
						for (UClass* SubsystemClass : SubsystemClasses)
							AddAndInitializeSubsystem(SubsystemClass);
								const USubsystem* CDO = SubsystemClass->GetDefaultObject<USubsystem>();
								USubsystem*& Subsystem = SubsystemMap.Add(SubsystemClass);
								Subsystem = NewObject<USubsystem>(Outer, SubsystemClass);
								Subsystem->InternalOwningSubsystem = this;
								Subsystem->Initialize(*this);
					InitializeRunningAverageDeltaTime();
					AddToRoot(); // Prevent from GC
					FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddStatic(UEngine::PreGarbageCollect);
					LoadObject<UClass>(UEngine::StaticClass()->GetOuter(), *UEngine::StaticClass()->GetName(), NULL, LOAD_Quiet|LOAD_NoWarn, NULL);
					LoadConfig(); // [/Script/Engine.Engine] from Engine.ini
					SetConfiguredProcessLimits();
						FPlatformProcess::SetProcessLimits(EProcessResource::VirtualMemory, static_cast<uint64>(VirtualMemoryLimitInKB) * 1024)
					InitializeObjectReferences();
						DefaultPhysMaterial = LoadObject<UPhysicalMaterial>(NULL, *DefaultPhysMaterialName.ToString(), NULL, LOAD_None, NULL);
						LoadEngineClass<ULocalPlayer>(LocalPlayerClassName, LocalPlayerClass);
						LoadEngineClass<AWorldSettings>(WorldSettingsClassName, WorldSettingsClass);
						LoadEngineClass<ALevelScriptActor>(LevelScriptActorClassName, LevelScriptActorClass);
						// ...
						UClass* SingletonClass = LoadClass<UObject>(nullptr, *GameSingletonClassName.ToString());
						UObject* GameSingleton = NewObject<UObject>(this, SingletonClass);
					const UGeneralProjectSettings& ProjectSettings = *GetDefault<UGeneralProjectSettings>();
					FNetworkVersion::SetProjectVersion(*ProjectSettings.ProjectVersion);
					OnTravelFailure().AddUObject(this, &UEngine::HandleTravelFailure);
					OnNetworkFailure().AddUObject(this, &UEngine::HandleNetworkFailure);
					OnNetworkLagStateChanged().AddUObject(this, &UEngine::HandleNetworkLagStateChanged);
					FEngineAnalytics::Initialize();
					FModuleManager::Get().LoadModule("ImageWriteQueue");
					FModuleManager::Get().LoadModuleChecked("StreamingPauseRendering");
					FModuleManager::Get().LoadModule("LevelSequence");
					EngineStats.Add(FEngineStatFuncs(TEXT("STAT_NamedEvents"), ...);
					// ...
				GetGameUserSettings()->LoadSettings();
				GetGameUserSettings()->ApplyNonResolutionSettings();
				FSoftClassPath GameInstanceClassName = GetDefault<UGameMapsSettings>()->GameInstanceClass;
				UClass* GameInstanceClass = LoadObject<UClass>(NULL, *GameInstanceClassName.ToString());
				GameInstance = NewObject<UGameInstance>(this, GameInstanceClass);
				GameInstance->InitializeStandalone();
					WorldContext = &GetEngine()->CreateNewWorldContext(EWorldType::Game);
						FWorldContext* NewWorldContext = new FWorldContext;
						WorldList.Add(NewWorldContext); // UEngine::GetWorldContexts()
						NewWorldContext->WorldType = WorldType;
						NewWorldContext->ContextHandle = FName(*FString::Printf(TEXT("Context_%d"), NextWorldContextHandle++));
						return *NewWorldContext;
					WorldContext->OwningGameInstance = this;
					UWorld* DummyWorld = UWorld::CreateWorld(EWorldType::Game, false);
					DummyWorld->SetGameInstance(this);
					WorldContext->SetCurrentWorld(DummyWorld);
					UGameInstance::Init();
						UGameInstance::ReceiveInit(); // BP Event
						OnlineSession = NewObject<UOnlineSession>(this, GetOnlineSessionClass());
						OnlineSession->RegisterOnlineDelegates();
							OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete);
							// ...
						FNetDelegates::OnReceivedNetworkEncryptionToken.BindUObject(this, &ThisClass::ReceivedNetworkEncryptionToken);
						FNetDelegates::OnReceivedNetworkEncryptionAck.BindUObject(this, &ThisClass::ReceivedNetworkEncryptionAck);
						SubsystemCollection.Initialize();
							TArray<UClass*> SubsystemClasses;
							GetDerivedClasses(UGameInstanceSubsystem, SubsystemClasses, /* bRecursive= */ true);
							for (UClass* SubsystemClass : SubsystemClasses)
								AddAndInitializeSubsystem(SubsystemClass);
									const USubsystem* CDO = SubsystemClass->GetDefaultObject<USubsystem>();
									USubsystem*& Subsystem = SubsystemMap.Add(SubsystemClass);
									Subsystem = NewObject<USubsystem>(Outer, SubsystemClass);
									Subsystem->InternalOwningSubsystem = this;
									Subsystem->Initialize(*this);
				bIsInitialized = true;
		UEngine::OnPostEngineInit.Broadcast();
		FCoreDelegates::OnPostEngineInit.Broadcast();
		if (FPlatformProcess::SupportsMultithreading())
			SessionService = FModuleManager::LoadModuleChecked<ISessionServicesModule>("SessionServices").GetSessionService();
			SessionService->Start();
			EngineService = new FEngineService();
		IProjectManager::Get().LoadModulesForProject(ELoadingPhase::PostEngineInit);
		IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PostEngineInit);
		GEngine->Start();
			GameInstance->StartGameInstance();
				DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);
				const UGameMapsSettings* GameMapsSettings = GetDefault<UGameMapsSettings>();
				const FString& DefaultMap = GameMapsSettings->GetGameDefaultMap();
				const FString PackageName = DefaultMap + GameMapsSettings->LocalMapOptions;
				const FURL URL(&DefaultURL, *PackageName, TRAVEL_Partial);
				EBrowseReturnVal::Type BrowseRet = GEngine->Browse(*WorldContext, URL, Error);
					return LoadMap(WorldContext, URL, NULL, Error) ? EBrowseReturnVal::Success : EBrowseReturnVal::Failure;
						// TODO
						FCoreUObjectDelegates::PreLoadMap.Broadcast(URL.Map);
						CleanupPackagesToFullyLoad(WorldContext, FULLYLOAD_Map, WorldContext.World()->PersistentLevel->GetOutermost()->GetName());
							for (int32 MapIndex = 0; MapIndex < Context.PackagesToFullyLoad.Num(); MapIndex++)
								FFullyLoadedPackagesInfo& PackagesInfo = Context.PackagesToFullyLoad[MapIndex];
								for (int32 ObjectIndex = 0; ObjectIndex < PackagesInfo.LoadedObjects.Num(); ObjectIndex++)
									PackagesInfo.LoadedObjects[ObjectIndex]->RemoveFromRoot();
								PackagesInfo.LoadedObjects.Empty();
						CleanupPackagesToFullyLoad(WorldContext, FULLYLOAD_Game_PreLoadClass, TEXT(""));
							// ...
						CleanupPackagesToFullyLoad(WorldContext, FULLYLOAD_Game_PostLoadClass, TEXT(""));
							// ...
						CleanupPackagesToFullyLoad(WorldContext, FULLYLOAD_Mutator, TEXT(""));
							// ...
						FlushAsyncLoading();
						CancelPendingMapChange(WorldContext);
						WorldContext.SeamlessTravelHandler.CancelTravel();
						GInitRunaway();
						WorldContext.World()->BeginTearingDown();
							bIsTearingDown = true;
							FWorldDelegates::OnWorldBeginTearDown.Broadcast(this);
						UEngine::ShutdownWorldNetDriver(WorldContext.World());
							UNetDriver* NetDriver = World->GetNetDriver();
							World->SetNetDriver(NULL);
							DestroyNamedNetDriver(World, NetDriver->NetDriverName);
								for (int32 Index = 0; Index < Context.ActiveNetDrivers.Num(); Index++)
									FNamedNetDriver& NamedNetDriver = Context.ActiveNetDrivers[Index];
									UNetDriver* NetDriver = NamedNetDriver.NetDriver;
									if (NetDriver && NetDriver->NetDriverName == NetDriverName)
										NetDriver->SetWorld(NULL);
										NetDriver->Shutdown();
											if (ServerConnection) // Only client has this set
												for (UChannel* Channel : ServerConnection->OpenChannels)
													UActorChannel* ActorChannel = Cast<UActorChannel>(Channel);
													ActorChannel->CleanupReplicators();
												ServerConnection->Close(); // UNetConnection::Close()
													Channels[0]->Close(EChannelCloseReason::Destroyed);
													State = USOCK_Closed;
													UNetConnection::FlushNet();
														// send buffered packets
											for (int32 ClientIndex = 0; ClientIndex < ClientConnections.Num(); ClientIndex++)
												FNetControlMessage<NMT_Failure>::Send(ClientConnections[ClientIndex], ErrorMsg);
												ClientConnections[ClientIndex]->FlushNet(true);
													UNetConnection::FlushNet();
														// send buffered packets
											for (int32 ClientIndex = ClientConnections.Num() - 1; ClientIndex >= 0; ClientIndex--)
												APawn* Pawn = ClientConnections[ClientIndex]->PlayerController->GetPawn();
												Pawn->Destroy(true); // destroy the pawn being possessed by PlayerController
												ClientConnections[ClientIndex]->CleanUp();
													UNetConnection::Close();
														Channels[0]->Close(EChannelCloseReason::Destroyed);
														State = USOCK_Closed;
														UNetConnection::FlushNet();
															// send buffered packets
													if (Driver->ServerConnection)
														Driver->ServerConnection = NULL;
													else // Client
														Driver->RemoveClientConnection(this);
													for (int32 i = OpenChannels.Num() - 1; i >= 0; i--)
														OpenChannel->ConditionalCleanUp(true, EChannelCloseReason::Destroyed);
													KeepProcessingActorChannelBunchesMap.Empty();
													PackageMap = NULL;
													// OwningActor is usually PlayerController
													OwningActor->OnNetCleanup(this);
													OwningActor = NULL;
													PlayerController = NULL;
													CleanupDormantActorState();
													Handler.Reset(NULL);
													SetClientLoginState(EClientLoginState::CleanedUp);
													Driver = nullptr;
											RepLayoutMap.Empty();
											ReplicationChangeListMap.Empty();
											ActorChannelPool.Empty();
											ConnectionlessHandler.Reset(nullptr);
											UNetDriver::SetReplicationDriver(nullptr);
										NetDriver->LowLevelDestroy();
											UNetDriver::SetWorld(NULL);
												UNetDriver::UnregisterTickEvents(World);
													World->OnTickDispatch().Remove(TickDispatchDelegateHandle);
													World->OnPostTickDispatch().Remove(PostTickDispatchDelegateHandle);
													World->OnTickFlush().Remove(TickFlushDelegateHandle);
													World->OnPostTickFlush().Remove(PostTickFlushDelegateHandle);
												World = NULL;
												WorldPackage = NULL;
												Notify = NULL;
												GetNetworkObjectList().Reset();
										Context.ActiveNetDrivers.RemoveAtSwap(Index);
										FLevelCollection* const LevelCollection = Context.World()->FindCollectionByType(DriverType);
										if (LevelCollection->GetNetDriver() == NetDriver)
											LevelCollection->SetNetDriver(nullptr);
										if (LevelCollection->GetDemoNetDriver() == NetDriver)
											LevelCollection->SetDemoNetDriver(nullptr);
							// Take care of the demo net driver specifically (so the world can clear the DemoNetDriver property)
							World->DestroyDemoNetDriver();
							// Also disconnect any net drivers that have this set as their world, to avoid GC issues
							FWorldContext &Context = GEngine->GetWorldContextFromWorldChecked(World);
							for (int32 Index = 0; Index < Context.ActiveNetDrivers.Num(); Index++)
								NetDriver = Context.ActiveNetDrivers[Index].NetDriver;
								if (NetDriver && NetDriver->GetWorld() == World)
									DestroyNamedNetDriver(World, NetDriver->NetDriverName);
										// ...
									Index--;
						WorldContext.World()->FlushLevelStreaming(EFlushLevelStreamingType::Visibility);
						FWorldDelegates::LevelRemovedFromWorld.Broadcast(nullptr, WorldContext.World());
						// Disassociate the players from their PlayerControllers in this world.
						for(auto It = WorldContext.OwningGameInstance->GetLocalPlayerIterator(); It; ++It)
							ULocalPlayer *Player = *It;
							WorldContext.World()->DestroyActor(Player->PlayerController->GetPawn(), /* bNetForce = */ true);
							WorldContext.World()->DestroyActor(Player->PlayerController, /* bNetForce = */ true);
							Player->PlayerController = nullptr;
						for (FActorIterator ActorIt(WorldContext.World()); ActorIt; ++ActorIt)
							ActorIt->RouteEndPlay(EEndPlayReason::LevelTransition);
								AActor::EndPlay(EndPlayReason);
									ActorHasBegunPlay = EActorBeginPlayState::HasNotBegunPlay;
									AActor::ReceiveEndPlay(EndPlayReason); // BP Event
									OnEndPlay.Broadcast(this, EndPlayReason);
									for (UActorComponent* Component : Components)
										Component->EndPlay(EndPlayReason);
											UActorComponent::ReceiveEndPlay(EndPlayReason); // BP Event
								AActor::ClearComponentOverlaps();
								World->RemoveNetworkActor(this);
									ForEachNetDriver(GEngine, this, [Actor](UNetDriver* const Driver) { ... });
										Driver->RemoveNetworkActor(Actor);
											GetNetworkObjectList().Remove(Actor);
											// ...
								FNavigationSystem::OnActorUnregistered(*this);
								AActor::UninitializeComponents();
									for (UActorComponent* ActorComp : Components)
										ActorComp->UninitializeComponent();
											bHasBeenInitialized = false;
						WorldContext.World()->CleanupWorld();
							FPhysScene* CurrPhysicsScene = GetPhysicsScene();
							CurrPhysicsScene->WaitPhysScenes();
							FWorldDelegates::OnWorldCleanup.Broadcast(this, bSessionEnded, bCleanupResources);
							AISystem->CleanupWorld(bSessionEnded, bCleanupResources, NewWorld);
							SetNavigationSystem(nullptr);
							ForEachNetDriver(GEngine, this, [](UNetDriver* const Driver) { ... });
								Driver->GetNetworkObjectList().Reset();
							// Tell actors to remove their components from the scene.
							ClearWorldComponents();
							PersistentLevel->ReleaseRenderingResources();
							// ...
						GEngine->WorldDestroyed(WorldContext.World());
							WorldDestroyedEvent.Broadcast(InWorld);
						WorldContext.World()->RemoveFromRoot();
						for (auto LevelIt(WorldContext.World()->GetLevelIterator()); LevelIt; ++LevelIt)
							const ULevel* Level = *LevelIt;
							CastChecked<UWorld>(Level->GetOuter())->MarkObjectsPendingKill();
								UWorld::MarkObjectsPendingKill();
									ForEachObjectWithOuter(this, MarkObjectPendingKill, true, RF_NoFlags, EInternalObjectFlags::PendingKill);
										Object->MarkPendingKill();
											GUObjectArray.IndexToObject(InternalIndex)->SetPendingKill();
						for (ULevelStreaming* LevelStreaming : WorldContext.World()->GetStreamingLevels())
							CastChecked<UWorld>(LevelStreaming->GetLoadedLevel()->GetOuter())->MarkObjectsPendingKill();
								// ...
						WorldContext.SetCurrentWorld(nullptr);
						IStreamingManager::Get().CancelForcedResources();
						WorldContext.OwningGameInstance->PreloadContentForURL(URL);
							// Preload game mode and other content if needed here
						const FName URLMapFName = FName(*URL.Map);
						UWorld::WorldTypePreLoadMap.FindOrAdd(URLMapFName) = WorldContext.WorldType;
						UPackage* WorldPackage = LoadPackage(nullptr, *URL.Map, (WorldContext.WorldType == EWorldType::PIE ? LOAD_PackageForPIE : LOAD_None));
						UWorld::WorldTypePreLoadMap.Remove(URLMapFName);
						UWorld* NewWorld = UWorld::FindWorldInPackage(WorldPackage);
						NewWorld->PersistentLevel->HandleLegacyMapBuildData();
						NewWorld->SetGameInstance(WorldContext.OwningGameInstance);
						GWorld = NewWorld;
						WorldContext.SetCurrentWorld(NewWorld);
						WorldContext.World()->WorldType = WorldContext.WorldType;
						WorldContext.World()->AddToRoot(); // Prevent from GC
						WorldContext.World()->InitWorld();
							FWorldDelegates::OnPreWorldInitialization.Broadcast(this, IVS);
							AWorldSettings* WorldSettings = GetWorldSettings();
							UWorld::CreatePhysicsScene(WorldSettings);
								FPhysScene* NewScene = new FPhysScene(Settings);
								UWorld::SetPhysicsScene(NewScene);
									PhysicsScene->SetOwningWorld(this);
							FNavigationSystem::AddNavigationSystemToWorld(...);
							UWorld::CreateAISystem();
								IAISystemModule* AISystemModule = FModuleManager::LoadModulePtr<IAISystemModule>(AIModuleName);
								AISystem = AISystemModule->CreateAISystemInstance(this);
									return NewObject<UAISystemBase>(World, AISystemClass);
							AvoidanceManager = NewObject<UAvoidanceManager>(this, GEngine->AvoidanceManagerClass);
							UWorld::SetupParameterCollectionInstances();
							Levels.Empty(1);
							Levels.Add(PersistentLevel);
							PersistentLevel->OwningWorld = this;
							PersistentLevel->bIsVisible = true;
							FVector Gravity = FVector(0.f, 0.f, GetGravityZ());
							GetPhysicsScene()->SetUpForFrame(&Gravity);
							PhysicsCollisionHandler = NewObject<UPhysicsCollisionHandler>(this, PhysHandlerClass);
							PhysicsCollisionHandler->InitCollisionHandler();
							UWorld::ConditionallyCreateDefaultLevelCollections();
							FWorldDelegates::OnPostWorldInitialization.Broadcast(this, IVS);
							PersistentLevel->InitializeRenderingResources();
							IStreamingManager::Get().AddLevel(PersistentLevel);
							UWorld::BroadcastLevelsChanged();
								LevelsChangedEvent.Broadcast();
						WorldContext.World()->SetGameMode(URL);
							AuthorityGameMode = GetGameInstance()->CreateGameModeForURL(InURL);
								AWorldSettings* Settings = World->GetWorldSettings();
								// Start by using the default game type specified in the map's worldsettings.
								TSubclassOf<AGameModeBase> GameClass = Settings->DefaultGameMode;
								if (!GameParam.IsEmpty())
									// If there is a GameMode parameter in the URL, allow it to override the default game type
									FString const GameClassName = UGameMapsSettings::GetGameModeForName(GameParam);
									GameClass = LoadClass<AGameModeBase>(nullptr, *GameClassName);
								if (!GameClass)
									// Next try to parse the map prefix
									// ...
								if (!GameClass)
									// Fall back to game default
									// ...
								if (!GameClass)
									// Fall back to raw GameMode
									// ...
								return World->SpawnActor<AGameModeBase>(GameClass, SpawnInfo);
						WorldContext.World()->Listen(URL);
							GEngine->CreateNamedNetDriver(this, NAME_GameNetDriver, NAME_GameNetDriver);
								NetDriver = UEngine::CreateNetDriver(Engine, WorldContext, NetDriverDefinition);
									Definition = Engine->NetDriverDefinitions.FindByPredicate(FindNetDriverDefPred);
									UClass* NetDriverClass = StaticLoadClass(UNetDriver::StaticClass(), nullptr, *Definition->DriverClassName.ToString(), nullptr, LOAD_Quiet);
									ReturnVal = NewObject<UNetDriver>(GetTransientPackage(), NetDriverClass);
									ReturnVal->SetNetDriverName(ReturnVal->GetFName()); // object name as network driver name by default
									return ReturnVal;
								NetDriver->SetNetDriverName(NetDriverName); // custom network driver name
							NetDriver = GEngine->FindNamedNetDriver(this, NAME_GameNetDriver);
							NetDriver->SetWorld(this);
								World = InWorld;
								WorldPackage = InWorld->GetOutermost();
								Notify = InWorld;
								UNetDriver::RegisterTickEvents(InWorld);
									TickDispatchDelegateHandle = InWorld->OnTickDispatch().AddUObject(this, &UNetDriver::TickDispatch);
									PostTickDispatchDelegateHandle = InWorld->OnPostTickDispatch().AddUObject(this, &UNetDriver::PostTickDispatch);
									TickFlushDelegateHandle = InWorld->OnTickFlush().AddUObject(this, &UNetDriver::TickFlush);
									PostTickFlushDelegateHandle = InWorld->OnPostTickFlush().AddUObject(this, &UNetDriver::PostTickFlush);
								GetNetworkObjectList().AddInitialObjects(InWorld, this);
							FLevelCollection* const SourceCollection = FindCollectionByType(ELevelCollectionType::DynamicSourceLevels);
							SourceCollection->SetNetDriver(NetDriver);
							FLevelCollection* const StaticCollection = FindCollectionByType(ELevelCollectionType::StaticLevels);
							StaticCollection->SetNetDriver(NetDriver);
							NetDriver->InitListen(this, InURL, false, Error);
								UIpNetDriver::InitBase(false, InNotify, LocalURL, bReuseAddressAndPort, Error);
									UNetDriver::InitBase(bInitAsClient, InNotify, URL, bReuseAddressAndPort, Error);
										bool bSuccess = UNetDriver::InitConnectionClass();
											NetConnectionClass = LoadClass<UNetConnection>(NULL,*NetConnectionClassName,NULL,LOAD_None,NULL);
											return NetConnectionClass != NULL;
										Notify = InNotify;
										return bSuccess;
									Socket = CreateSocket();
									// setsockopt ...
									int32 BoundPort = SocketSubsystem->BindNextPort( Socket, *LocalAddr, MaxPortCountToTry + 1, 1 );
									return true;
								UNetDriver::InitConnectionlessHandler();
									ConnectionlessHandler = MakeUnique<PacketHandler>(&DDoS);
									ConnectionlessHandler->InitializeAddressSerializer([this](const FString& InAddress) { return GetSocketSubsystem()->GetAddressFromString(InAddress); });
									ConnectionlessHandler->NotifyAnalyticsProvider(AnalyticsProvider, AnalyticsAggregator);
									ConnectionlessHandler->Initialize(Handler::Mode::Server, MAX_PACKET_SIZE, true, nullptr, nullptr, NetDriverName);
									TSharedPtr<HandlerComponent> NewComponent = ConnectionlessHandler->AddHandler(TEXT("Engine.EngineHandlerComponentFactory(StatelessConnectHandlerComponent)"), true);
									StatelessConnectComponent = StaticCastSharedPtr<StatelessConnectHandlerComponent>(NewComponent);
									StatelessConnectComponent.Pin()->SetDriver(this);
									ConnectionlessHandler->InitializeComponents();
										// ...
										for (TSharedPtr<HandlerComponent>& Component : HandlerComponents)
											Component->Initialize();
											Component->NotifyAnalyticsProvider();
								LocalURL.Port = LocalAddr->GetPort();
						LoadPackagesFully(WorldContext.World(), FULLYLOAD_Map, WorldContext.World()->PersistentLevel->GetOutermost()->GetName());
							// ...
						WorldContext.World()->FlushLevelStreaming(EFlushLevelStreamingType::Visibility);
						WorldContext.World()->CreateAISystem();
							const FName AIModuleName = UAISystemBase::GetAISystemModuleName();
							IAISystemModule* AISystemModule = FModuleManager::LoadModulePtr<IAISystemModule>(AIModuleName);
							AISystem = AISystemModule->CreateAISystemInstance(this);
								FSoftClassPath AISystemClassName = UAISystemBase::GetAISystemClassName();
								AISystemClassName = WorldSettings->GetAISystemClassName();
								TSubclassOf<UAISystemBase> AISystemClass = LoadClass<UAISystemBase>(nullptr, *AISystemClassName.ToString(), nullptr, LOAD_None, nullptr);
								AISystemInstance = NewObject<UAISystemBase>(World, AISystemClass);
								return AISystemInstance;
							return AISystem;
						WorldContext.World()->InitializeActorsForPlay(URL);
							UpdateWorldComponents(bRerunConstructionScript /* = false */ , true);
							for (int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++)
								ULevel* const Level = Levels[LevelIndex];
								Level->InitializeNetworkActors();
									// Kill non relevant client actors and set net roles correctly
									for (int32 ActorIndex = 0; ActorIndex < Actors.Num(); ActorIndex++)
										AActor* Actor = Actors[ActorIndex];
										if (!Actor->IsActorInitialized())
											if (Actor->bNetLoadOnClient)
												Actor->bNetStartup = true;
										if (!bIsServer)
											if (!Actor->bNetLoadOnClient)
												Actor->Destroy(true);
											else
												Actor->ExchangeNetRoles(true);
							GEngine->SpawnServerActors(this);
								TArray<FString> FullServerActors;
								// A configurable list of actors that are automatically spawned
								// upon server startup (just prior to InitGame)
								FullServerActors.Append(ServerActors);
								// Runtime-modified list of server actors, allowing plugins to use serveractors,
								// without permanently adding them to config files
								FullServerActors.Append(RuntimeServerActors);
								for (int32 i = 0; i < FullServerActors.Num(); i++)
									AActor* Actor = World->SpawnActor( HelperClass );
							AuthorityGameMode->InitGame(FPaths::GetBaseFilename(InURL.Map), Options, Error);
								GameSession = World->SpawnActor<AGameSession>(GetGameSessionClass(), SpawnInfo);
								GameSession->InitOptions(Options);
								FGameModeEvents::GameModeInitializedEvent.Broadcast(this);
								GameSession->RegisterServer();
								if (GameStateClass == nullptr)
									// GameStateClass is not set, falling back to AGameState.
									GameStateClass = AGameState::StaticClass();
								FGameDelegates::Get().GetPreCommitMapChangeDelegate().AddUObject(this, &AGameMode::PreCommitMapChange);
								FGameDelegates::Get().GetPostCommitMapChangeDelegate().AddUObject(this, &AGameMode::PostCommitMapChange);
								FGameDelegates::Get().GetHandleDisconnectDelegate().AddUObject(this, &AGameMode::HandleDisconnect);
							for (int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++)
								ULevel* const Level = Levels[LevelIndex];
								Level->RouteActorInitialize();
									for (int32 Index = 0; Index < Actors.Num(); ++Index)
										AActor* const Actor = Actors[Index];
										Actor->PreInitializeComponents();
											AGameModeBase::PreInitializeComponents();
												if (GameStateClass == nullptr)
													// Fallback to default GameState if none was specified.
													GameStateClass = AGameStateBase::StaticClass();
												GameState = World->SpawnActor<AGameStateBase>(GameStateClass, SpawnInfo);
												World->SetGameState(GameState);
												GameState->AuthorityGameMode = this;
												World->NetworkManager = World->SpawnActor<AGameNetworkManager>(WorldSettings->GameNetworkManagerClass, SpawnInfo);
												AGameModeBase::InitGameState();
													GameState->GameModeClass = GetClass();
													GameState->ReceivedGameModeClass();
													GameState->SpectatorClass = SpectatorClass;
													GameState->ReceivedSpectatorClass();
											ALevelScriptActor::PreInitializeComponents();
												InputComponent = NewObject<UInputComponent>(this);
												InputComponent->RegisterComponent();
												UInputDelegateBinding::BindInputDelegates(GetClass(), InputComponent);
											APawn::PreInitializeComponents();
												Instigator = this;
											// ...
										Actor->InitializeComponents();
										Actor->PostInitializeComponents();
											AActor::PostInitializeComponents();
												bActorInitialized = true;
												FNavigationSystem::OnActorRegistered(*this);
												AActor::UpdateAllReplicatedComponents();
													ReplicatedComponents.Reset();
													for (UActorComponent* Component : OwnedComponents)
														if (Component->GetIsReplicated())
															ReplicatedComponents.Add(Component);
											AGameStateBase::PostInitializeComponents();
												World->SetGameState(this);
												// ...
												for (TActorIterator<APlayerState> It(World); It; ++It)
													AGameStateBase::AddPlayerState(*It);
														PlayerArray.AddUnique(PlayerState);
											AController::PostInitializeComponents();
												GetWorld()->AddController(this);
												RootComponent->SetWorldRotation(GetControlRotation());
											APlayerController::PostInitializeComponents();
												AController::InitPlayerState();
													TSubclassOf<APlayerState> PlayerStateClassToSpawn = GameMode->PlayerStateClass;
													PlayerState = World->SpawnActor<APlayerState>(PlayerStateClassToSpawn, SpawnInfo);
													// ...
												SpawnPlayerCameraManager();
												ResetCameraMode();
												AddCheats();
												bPlayerIsWaiting = true;
												StateName = NAME_Spectating;
											APlayerState::PostInitializeComponents();
												GameStateBase->AddPlayerState(this);
												AController* OwningController = Cast<AController>(GetOwner());
												bIsABot = (Cast<APlayerController>(OwningController) == nullptr);
											// ...
										Actor->DispatchBeginPlay();
											AActor::BeginPlay();
												AActor::RegisterAllActorTickFunctions(/* bRegister = */ true, /* bDoComponents = */ false);
													AActor::RegisterActorTickFunctions(bRegister /* = true */);
														if (bRegister && PrimaryActorTick.bCanEverTick)
															PrimaryActorTick.Target = this;
															PrimaryActorTick.SetTickFunctionEnable(PrimaryActorTick.bStartWithTickEnabled || PrimaryActorTick.IsTickFunctionEnabled());
															PrimaryActorTick.RegisterTickFunction(GetLevel());
														else if (PrimaryActorTick.IsTickFunctionRegistered())
															PrimaryActorTick.UnRegisterTickFunction();
												ActorHasBegunPlay = EActorBeginPlayState::BeginningPlay;
												for (UActorComponent* Component : Components)
													Component->RegisterAllComponentTickFunctions(/* bRegister = */ true);
														UActorComponent::RegisterComponentTickFunctions(bRegister /* = true */);
															if (bRegister && SetupActorComponentTickFunction(&PrimaryComponentTick))
																PrimaryComponentTick.Target = this;
															else if (PrimaryComponentTick.IsTickFunctionRegistered())
																PrimaryComponentTick.UnRegisterTickFunction();
													Component->BeginPlay();
														UActorComponent::ReceiveBeginPlay(); // BP Event
														bHasBegunPlay = true;
												AActor::ReceiveBeginPlay(); // BP Event
												ActorHasBegunPlay = EActorBeginPlayState::HasBegunPlay;
											UWorld::BeginPlay();
												GetAuthGameMode()->StartPlay();
												GetAISystem()->StartPlay();
											AGameMode::StartPlay();
												// Don't call super, this class handles begin play/match start itself
												if (MatchState == MatchState::EnteringMap)
													AGameMode::SetMatchState(MatchState::WaitingToStart);
												if (MatchState == MatchState::WaitingToStart && AGameMode::ReadyToStartMatch())
													AGameMode::StartMatch();
														AGameMode::SetMatchState(MatchState::InProgress);
											AGameModeBase::StartPlay();
												GameState->HandleBeginPlay();
											UAISystem::StartPlay();
												PerceptionSystem->StartPlay();
													for (UAISense* Sense : Senses)
														UAIPerceptionSystem::RegisterAllPawnsAsSourcesForSense(SenseID);
													World->GetTimerManager().SetTimer(AgeStimuliTimerHandle, this, &UAIPerceptionSystem::AgeStimuli, PerceptionAgingRate, /*inbLoop=*/true);
											// ...
							for (int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++)
								ULevel* Level = Levels[LevelIndex];
								Level->SortActorList();
									OwningWorld->AddNetworkActor(WorldSettings);
									for (AActor* Actor : Actors)
										if (IsNetActor(Actor))
											NewNetActors.Add(Actor);
											OwningWorld->AddNetworkActor(Actor);
										else
											NewActors.Add(Actor);
									NewActors.Append(MoveTemp(NewNetActors));
									Actors = MoveTemp(NewActors);
							OnActorsInitialized.Broadcast(OnActorInitParams);
							FWorldDelegates::OnWorldInitializedActors.Broadcast(OnActorInitParams);
							NavigationSystem->OnInitializeActors();
							AISystem->InitializeActorsForPlay(bResetTime);
							for (int32 LevelIndex = 0; LevelIndex < Levels.Num(); LevelIndex++)
								ULevel* Level = Levels[LevelIndex];
								IStreamingManager::Get().AddLevel(Level);
						FNavigationSystem::AddNavigationSystemToWorld(*WorldContext.World(), FNavigationSystemRunMode::GameMode);
							NavigationSystemConfig = WorldSettings->GetNavigationSystemConfig();
							UNavigationSystemBase* NavSysInstance = NavigationSystemConfig->CreateAndConfigureNavigationSystem(WorldOwner);
							WorldOwner.SetNavigationSystem(NavSysInstance);
							WorldOwner.GetNavigationSystem()->InitializeForWorld(WorldOwner, RunMode);
						// Spawn play actors for all active local players
						for(auto It = WorldContext.OwningGameInstance->GetLocalPlayerIterator(); It; ++It)
							(*It)->SpawnPlayActor(URL.ToString(1),Error2,WorldContext.World());
								// ...
						IStreamingManager::Get().NotifyLevelChange();
						WorldContext.World()->BeginPlay();
							// See `AActor::BeginPlay()` breakdown above
						PostLoadMapCaller.Broadcast(WorldContext.World());
							FCoreUObjectDelegates::PostLoadMapWithWorld.Broadcast(World);
						WorldContext.World()->bWorldWasLoadedThisTick = true;
						WorldContext.OwningGameInstance->LoadComplete(StopTime - StartTime, *URL.Map);
						return true;
				UGameInstance::OnStart();
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