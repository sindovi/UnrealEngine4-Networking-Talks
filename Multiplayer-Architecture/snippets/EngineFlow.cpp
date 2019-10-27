FEngineLoop GEngineLoop;
int32 GuardedMain(const TCHAR* CmdLine)
{
	int32 ErrorLevel = EnginePreInit(CmdLine)
	{
		FEngineLoop::PreInit(CmdLine);
			// Paths, CVars, Modules, Plugins, Platform, Slate, Threads, Memory, Physics, Renderer etc.
	}
	ErrorLevel = EngineInit()
	{
		FEngineLoop::Init();
			GConfig->GetString(TEXT("/Script/Engine.Engine"), TEXT("GameEngine"), GameEngineClassName, GEngineIni);
			EngineClass = StaticLoadClass(UGameEngine::StaticClass(), nullptr, *GameEngineClassName);
			GEngine = NewObject<UEngine>(GetTransientPackage(), EngineClass);
			GEngine->Init(this)
			{
				UGameEngine::Init(InEngineLoop);
					UEngine::Init(InEngineLoop);
						EngineSubsystemCollection.Initialize();
						OnTravelFailure().AddUObject(this, &UEngine::HandleTravelFailure);
						OnNetworkFailure().AddUObject(this, &UEngine::HandleNetworkFailure);
					FSoftClassPath GameInstanceClassName = GetDefault<UGameMapsSettings>()->GameInstanceClass;
					UClass* GameInstanceClass = LoadObject<UClass>(NULL, *GameInstanceClassName.ToString());
					GameInstance = NewObject<UGameInstance>(this, GameInstanceClass);
					GameInstance->InitializeStandalone()
					{
						WorldContext = &GetEngine()->CreateNewWorldContext(EWorldType::Game);
						WorldContext->OwningGameInstance = this;
						UWorld* DummyWorld = UWorld::CreateWorld(EWorldType::Game, false);
						DummyWorld->SetGameInstance(this);
						WorldContext->SetCurrentWorld(DummyWorld);
						UGameInstance::Init()
						{
							UGameInstance::ReceiveInit(); // BP Event Function
							UClass* SpawnClass = GetOnlineSessionClass();
							OnlineSession = NewObject<UOnlineSession>(this, SpawnClass);
							OnlineSession->RegisterOnlineDelegates();
							SubsystemCollection.Initialize();
						}
					}
			}
			GEngine->Start()
			{
				UGameEngine::Start();
					GameInstance->StartGameInstance()
					{
						const UGameMapsSettings* GameMapsSettings = GetDefault<UGameMapsSettings>();
						const FString& DefaultMap = GameMapsSettings->GetGameDefaultMap();
						FString PackageName = DefaultMap + GameMapsSettings->LocalMapOptions;
						FURL URL(&DefaultURL, *PackageName, TRAVEL_Partial);
						BrowseRet = Engine->Browse(*WorldContext, URL, Error)
							return LoadMap(WorldContext, URL, NULL, Error) ? EBrowseReturnVal::Success : EBrowseReturnVal::Failure;
								if (WorldContext.World())
									WorldContext.World()->BeginTearingDown();
									ShutdownWorldNetDriver(WorldContext.World());
									for (auto It = WorldContext.OwningGameInstance->GetLocalPlayerIterator(); It; ++It)
										ULocalPlayer *Player = *It;
										if (Player->PlayerController && Player->PlayerController->GetWorld() == WorldContext.World())
											if (Player->PlayerController->GetPawn())
												WorldContext.World()->DestroyActor(Player->PlayerController->GetPawn(), true);
											WorldContext.World()->DestroyActor(Player->PlayerController, true);
											Player->PlayerController = nullptr;
									for (FActorIterator ActorIt(WorldContext.World()); ActorIt; ++ActorIt)
										ActorIt->RouteEndPlay(EEndPlayReason::LevelTransition);
									WorldContext.World()->CleanupWorld();
									for (auto LevelIt(WorldContext.World()->GetLevelIterator()); LevelIt; ++LevelIt)
										const ULevel* Level = *LevelIt;
										CastChecked<UWorld>(Level->GetOuter())->MarkObjectsPendingKill();
									WorldContext.SetCurrentWorld(nullptr);
								UPackage* WorldPackage = LoadPackage(nullptr, *URL.Map, LOAD_None);
								UWorld* NewWorld = UWorld::FindWorldInPackage(WorldPackage);
								NewWorld->SetGameInstance(WorldContext.OwningGameInstance);
								GWorld = NewWorld;
								WorldContext.SetCurrentWorld(NewWorld);
								WorldContext.World()->AddToRoot();
								WorldContext.World()->InitWorld();
									Levels.Empty(1);
									Levels.Add(PersistentLevel);
									PersistentLevel->OwningWorld = this;
									PersistentLevel->bIsVisible = true;
									bIsWorldInitialized = true;
									BroadcastLevelsChanged();
								WorldContext.World()->SetGameMode(URL);
									if (IsServer() && !AuthorityGameMode)
										AuthorityGameMode = GetGameInstance()->CreateGameModeForURL(InURL);
											UGameInstance::CreateGameModeForURL(InURL);
												AWorldSettings* Settings = World->GetWorldSettings();
												TSubclassOf<AGameModeBase> GameClass = Settings->DefaultGameMode;
												if (!GameParam.IsEmpty())
													// If there is a GameMode parameter in the URL, allow it to override the default game type
													FString const GameClassName = UGameMapsSettings::GetGameModeForName(GameParam);
													GameClass = LoadClass<AGameModeBase>(nullptr, *GameClassName);
												if (!GameClass)
													// Next try to parse the map prefix
												if (!GameClass)
													// Fall back to game default
												if (!GameClass)
													// Fall back to raw GameMode
												return World->SpawnActor<AGameModeBase>(GameClass, SpawnInfo);
								WorldContext.World()->Listen(URL);
									NetDriver = GEngine->CreateNamedNetDriver(this, NAME_GameNetDriver, NAME_GameNetDriver);
									NetDriver->SetWorld(this);
										UNetDriver::RegisterTickEvents(InWorld);
											TickDispatchDelegateHandle = InWorld->OnTickDispatch().AddUObject(this, &UNetDriver::TickDispatch);
											PostTickDispatchDelegateHandle = InWorld->OnPostTickDispatch().AddUObject(this, &UNetDriver::PostTickDispatch);
											TickFlushDelegateHandle = InWorld->OnTickFlush().AddUObject(this, &UNetDriver::TickFlush);
											PostTickFlushDelegateHandle = InWorld->OnPostTickFlush().AddUObject(this, &UNetDriver::PostTickFlush);
										GetNetworkObjectList().AddInitialObjects(InWorld, this);
											for (FActorIterator Iter(World); Iter; ++Iter)
												if (ULevel::IsNetActor(Actor))
													FindOrAdd(Actor, NetDriver);
									NetDriver->InitListen(this, InURL, false, Error);
								WorldContext.World()->InitializeActorsForPlay(URL)
								{
									for (int32 LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++)
										ULevel* const Level = Levels[LevelIndex];
										Level->InitializeNetworkActors();
											for (int32 ActorIndex = 0; ActorIndex < Actors.Num(); ActorIndex++)
												if (!Actor->IsActorInitialized())
													if (Actor->bNetLoadOnClient)
														Actor->bNetStartup = true;
													if (!bIsServer)
														if (!Actor->bNetLoadOnClient)
															Actor->Destroy(true);
														else
															Actor->ExchangeNetRoles(true);
									if (CurNetMode == NM_ListenServer || CurNetMode == NM_DedicatedServer)
										GEngine->SpawnServerActors(this);
											UEngine::SpawnServerActors(UWorld* World);
												TArray<FString> FullServerActors;
												// A configurable list of actors that are automatically spawned
												// upon server startup (just prior to InitGame)
												FullServerActors.Append(ServerActors);
												// Runtime-modified list of server actors, allowing plugins to use serveractors,
												// without permanently adding them to config files
												FullServerActors.Append(RuntimeServerActors);
												for (int32 i=0; i < FullServerActors.Num(); i++)
													AActor* Actor = World->SpawnActor( HelperClass );
									if (AuthorityGameMode && !AuthorityGameMode->IsActorInitialized())
										AuthorityGameMode->InitGame(FPaths::GetBaseFilename(InURL.Map), Options, Error);
											AGameMode::InitGame(MapName, Options, ErrorMessage);
												AGameModeBase::InitGame(MapName, Options, ErrorMessage);
													GameSession = World->SpawnActor<AGameSession>(GetGameSessionClass(), SpawnInfo);
													GameSession->InitOptions(Options);
													FGameModeEvents::GameModeInitializedEvent.Broadcast(this);
													GameSession->RegisterServer();
												if (GameStateClass == nullptr)
													// GameStateClass is not set, falling back to AGameState.
													GameStateClass = AGameState::StaticClass();
												FGameDelegates::Get().GetPendingConnectionLostDelegate().AddUObject(this, &AGameMode::NotifyPendingConnectionLost);
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
														GameState = World->SpawnActor<AGameStateBase>(GameStateClass, SpawnInfo);
														World->SetGameState(GameState);
														GameState->AuthorityGameMode = this;
														AGameModeBase::InitGameState();
															GameState->GameModeClass = GetClass();
															GameState->ReceivedGameModeClass();
															GameState->SpectatorClass = SpectatorClass;
															GameState->ReceivedSpectatorClass();
												Actor->InitializeComponents();
												Actor->PostInitializeComponents();
													AGameStateBase::PostInitializeComponents();
														for (TActorIterator<APlayerState> It(World); It; ++It)
															AGameStateBase::AddPlayerState(*It);
																PlayerArray.AddUnique(PlayerState);
												Actor->DispatchBeginPlay();
									OnActorsInitialized.Broadcast(OnActorInitParams);
									FWorldDelegates::OnWorldInitializedActors.Broadcast(OnActorInitParams);
								}
								WorldContext.World()->BeginPlay();
									AGameModeBase* const GameMode = GetAuthGameMode();
									GameMode->StartPlay();
										GameState->HandleBeginPlay();
								WorldContext.OwningGameInstance->LoadComplete(StopTime - StartTime, *URL.Map);
								return true;
						UGameInstance::OnStart();
					}
			}
	}
	while (!GIsRequestingExit)
	{
		EngineTick();
			FEngineLoop::Tick();
				FCoreDelegates::OnBeginFrame.Broadcast();
				GEngine->UpdateTimeAndHandleMaxTickRate();
					FPlatformProcess::SleepNoStats(WaitTime);
				GEngine->Tick(FApp::GetDeltaTime(), bIdleMode);
					for (int32 WorldIdx = 0; WorldIdx < WorldList.Num(); ++WorldIdx)
						FWorldContext &Context = WorldList[WorldIdx];
						GWorld = Context.World();
						Context.World()->Tick(LEVELTICK_All, DeltaSeconds);
							FWorldDelegates::OnWorldTickStart.Broadcast(TickType, DeltaSeconds);
							BroadcastTickDispatch(DeltaSeconds);
								// UNetDriver::TickDispatch(DeltaTime) - Network Input, Read Socket and Update Actors
							BroadcastPostTickDispatch();
							if (NetDriver && NetDriver->ServerConnection)
								TickNetClient(DeltaSeconds);
							FWorldDelegates::OnWorldPreActorTick.Broadcast(this, TickType, DeltaSeconds);
							for (AActor* LevelSequenceActor : LevelSequenceActors)
								LevelSequenceActor->Tick(DeltaSeconds);
							for (int32 i = 0; i < LevelCollections.Num(); ++i)
								TArray<ULevel*> LevelsToTick;
								for (ULevel* CollectionLevel : LevelCollections[i].GetLevels())
									if (Levels.Contains(CollectionLevel))
										LevelsToTick.Add(CollectionLevel);
								FTickTaskManagerInterface::Get().StartFrame(this, DeltaSeconds, TickType, LevelsToTick);
								RunTickGroup(TG_PrePhysics); // AActor::PrimaryActorTick.TickGroup = TG_PrePhysics;
								RunTickGroup(TG_StartPhysics);
								RunTickGroup(TG_DuringPhysics, false);
								RunTickGroup(TG_EndPhysics);
								RunTickGroup(TG_PostPhysics);
								GetTimerManager().Tick(DeltaSeconds);
								FTickableGameObject::TickObjects(this, TickType, bIsPaused, DeltaSeconds);
								RunTickGroup(TG_PostUpdateWork);
								RunTickGroup(TG_LastDemotable);
								FTickTaskManagerInterface::Get().EndFrame();
							FWorldDelegates::OnWorldPostActorTick.Broadcast(this, TickType, DeltaSeconds);
							BroadcastTickFlush(RealDeltaSeconds);
								// UNetDriver::TickFlush(DeltaSeconds) - Network Output, Process Actors and Write to Socket
							BroadcastPostTickFlush(RealDeltaSeconds);
							GEngine->ConditionalCollectGarbage();
						ConditionalCommitMapChange(Context);
					FTickableGameObject::TickObjects(nullptr, LEVELTICK_All, false, DeltaSeconds);
				GFrameCounter++;
				FTicker::GetCoreTicker().Tick(FApp::GetDeltaTime());
				FThreadManager::Get().Tick();
				GEngine->TickDeferredCommands();
				FCoreDelegates::OnEndFrame.Broadcast();
	}
	~EngineLoopCleanupGuard()
	{
		EngineExit();
			FEngineLoop::Exit();
				UGameEngine::PreExit()
				{
					for (int32 WorldIndex = 0; WorldIndex < WorldList.Num(); ++WorldIndex)
						UWorld* const World = WorldList[WorldIndex].World();
						World->BeginTearingDown();
						ShutdownWorldNetDriver(World);
						for (FActorIterator ActorIt(World); ActorIt; ++ActorIt)
							ActorIt->RouteEndPlay(EEndPlayReason::Quit);
						World->GetGameInstance()->Shutdown();
							UGameInstance::ReceiveShutdown(); // BP Event Function
							OnlineSession->ClearOnlineDelegates();
							OnlineSession = nullptr;
							SubsystemCollection.Deinitialize();
							WorldContext = nullptr;
					GEngine->PreExit();
				}
	}
	return ErrorLevel;
}