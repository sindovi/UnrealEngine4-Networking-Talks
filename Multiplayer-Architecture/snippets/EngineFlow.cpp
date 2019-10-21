int32 GuardedMain(const TCHAR* CmdLine)
	int32 ErrorLevel = EnginePreInit(CmdLine);
	ErrorLevel = EngineInit();
	while (!GIsRequestingExit) { EngineTick(); }
	return ErrorLevel;