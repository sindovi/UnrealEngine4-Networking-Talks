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
						OnTravelFailure().AddUObject(this, &UEngine::HandleTravelFailure);
						OnNetworkFailure().AddUObject(this, &UEngine::HandleNetworkFailure);
					FSoftClassPath GameInstanceClassName = GetDefault<UGameMapsSettings>()->GameInstanceClass;
					UClass* GameInstanceClass = LoadObject<UClass>(NULL, *GameInstanceClassName.ToString());
					GameInstance = NewObject<UGameInstance>(this, GameInstanceClass);
					GameInstance->InitializeStandalone();
						WorldContext = &GetEngine()->CreateNewWorldContext(EWorldType::Game);
						WorldContext->OwningGameInstance = this;
						UWorld* DummyWorld = UWorld::CreateWorld(EWorldType::Game, false);
						DummyWorld->SetGameInstance(this);
						WorldContext->SetCurrentWorld(DummyWorld);
						UGameInstance::Init();
							UGameInstance::ReceiveInit(); // BP Event Function
							UClass* SpawnClass = GetOnlineSessionClass();
							OnlineSession = NewObject<UOnlineSession>(this, SpawnClass);
							OnlineSession->RegisterOnlineDelegates();
							SubsystemCollection.Initialize();
			GEngine->Start();
				UGameEngine::Start();
					GameInstance->StartGameInstance();
						// TODO
	while (!GIsRequestingExit)
		EngineTick();
			FEngineLoop::Tick();
				// TODO
	~EngineLoopCleanupGuard()
		EngineExit();
			FEngineLoop::Exit();
				// TODO
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