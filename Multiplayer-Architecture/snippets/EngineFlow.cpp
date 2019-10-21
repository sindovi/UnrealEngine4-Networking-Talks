FEngineLoop	GEngineLoop;
int32 GuardedMain(const TCHAR* CmdLine)
	int32 ErrorLevel = EnginePreInit(CmdLine);
		FEngineLoop::PreInit(CmdLine);
			// Paths, CVars, Modules, Plugins, Platform, Slate, Time, Threads, Memory, Physics, Renderer etc.
	ErrorLevel = EngineInit();
		FEngineLoop::Init();
	while (!GIsRequestingExit) { EngineTick(); }
	~EngineLoopCleanupGuard() { EngineExit(); }
	return ErrorLevel;