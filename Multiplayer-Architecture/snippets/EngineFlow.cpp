FEngineLoop	GEngineLoop;
int32 GuardedMain(const TCHAR* CmdLine)
	int32 ErrorLevel = EnginePreInit(CmdLine);
		FEngineLoop::PreInit(CmdLine);
			// Paths, CVars, Modules, Plugins, Platform, Slate, Threads, Memory, Physics, Renderer etc.
	ErrorLevel = EngineInit();
		FEngineLoop::Init();
			GConfig->GetString(TEXT("/Script/Engine.Engine"), TEXT("GameEngine"), GameEngineClassName, GEngineIni);
			EngineClass = StaticLoadClass(UGameEngine::StaticClass(), nullptr, *GameEngineClassName);
			GEngine = NewObject<UEngine>(GetTransientPackage(), EngineClass);
			GEngine->Init(this);
				UGameEngine::Init(InEngineLoop);
					UEngine::Init(InEngineLoop);
						EngineSubsystemCollection.Initialize();
							for (const auto& SubsystemClasses : DynamicSystemModuleMap)
								for (const auto& SubsystemClass : SubsystemClasses.Value)
									AddAndInitializeSubsystem(SubsystemClass);
										Subsystem = NewObject<USubsystem>(Outer, SubsystemClass);
										Subsystem->Initialize(*this);
						OnTravelFailure().AddUObject(this, &UEngine::HandleTravelFailure);
						OnNetworkFailure().AddUObject(this, &UEngine::HandleNetworkFailure);
					GNetworkProfiler.EnableTracking(true);
					GetGameUserSettings()->LoadSettings();
					GetGameUserSettings()->ApplyNonResolutionSettings();
					FSoftClassPath GameInstanceClassName = GetDefault<UGameMapsSettings>()->GameInstanceClass;
					UClass* GameInstanceClass = LoadObject<UClass>(NULL, *GameInstanceClassName.ToString());
					GameInstance = NewObject<UGameInstance>(this, GameInstanceClass);
					GameInstance->InitializeStandalone();
					UGameViewportClient* ViewportClient = NewObject<UGameViewportClient>(this, GameViewportClientClass);
					ViewportClient->Init(*GameInstance->GetWorldContext(), GameInstance);
					GameInstance->GetWorldContext()->GameViewport = ViewportClient;
			GEngine->Start();
				UGameEngine::Start();
					GameInstance->StartGameInstance();
	while (!GIsRequestingExit)
		EngineTick();
			FEngineLoop::Tick();
	~EngineLoopCleanupGuard()
		EngineExit();
			FEngineLoop::Exit();
				FlushAsyncLoading();
				IStreamingManager::Get().BlockTillAllRequestsFinished();
				GEngine->PreExit();
				FSlateApplication::Shutdown();
				AppPreExit();
				TermGamePhys();
				StopRenderingThread();
				FTaskGraphInterface::Shutdown();
				IStreamingManager::Shutdown();
	return ErrorLevel;