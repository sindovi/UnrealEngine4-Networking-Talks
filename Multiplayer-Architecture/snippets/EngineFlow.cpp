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
			GEngine->Start();
	while (!GIsRequestingExit) { EngineTick(); }
	~EngineLoopCleanupGuard() { EngineExit(); }
	return ErrorLevel;