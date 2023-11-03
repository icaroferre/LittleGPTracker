
#include "RG351PSystem.h"
#define USE_SDL_AUDIO
#ifndef _NO_JACK_
#include "Adapters/Jack/Audio/JackAudio.h"
#include "Adapters/Jack/Midi/JackMidiService.h"
#include "Adapters/Jack/Client/JackClient.h"
#endif
// #ifdef USE_SDL_AUDIO
#include "Adapters/SDL/Audio/SDLAudio.h"
#include "Adapters/Dummy/Midi/DummyMidi.h"

// #else

// 	#include "Adapters/RTMidi/RTMidiService.h"
// 	#include "Adapters/RTAudio/RTAudioStub.h"
// #endif

#include "Adapters/SDL/GUI/GUIFactory.h"
#include "Adapters/SDL/GUI/SDLGUIWindowImp.h"
#include "Adapters/SDL/GUI/SDLEventManager.h"
#include "Adapters/Unix/FileSystem/UnixFileSystem.h"
#include "Adapters/SDL/Process/SDLProcess.h"

#include "Adapters/Unix/Process/UnixProcess.h"
#include "Application/Model/Config.h"
#include "System/Console/Logger.h"
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <libgen.h>
#include "Application/Model/Config.h"
#include "Application/Controllers/ControlRoom.h"
#include "Application/Commands/NodeList.h"
#include "Adapters/SDL/Timer/SDLTimer.h"

EventManager *DEBSystem::eventManager_ = NULL ;

int DEBSystem::MainLoop() {
	eventManager_->InstallMappings();
	return eventManager_->MainLoop() ;
} ;

void DEBSystem::Boot(int argc,char **argv) {

	SDL_putenv((char *)"SDL_VIDEO_X11_WMCLASS=LittleGPTracker") ;

	// Install System
	System::Install(new DEBSystem()) ;

	// Install FileSystem
	FileSystem::Install(new UnixFileSystem()) ;

  // Install aliases

	char buff[1024];
	ssize_t len = ::readlink("/proc/self/exe",buff,sizeof(buff)-1);
	if (len != -1)
	{
		buff[len] = 0;
	}
	else
	{
		strcpy(buff,".");
	}
	Path::SetAlias("bin",dirname(buff)) ;

	Path::SetAlias("root",".") ;

#ifdef _DEBUG
  Trace::GetInstance()->SetLogger(*(new StdOutLogger()));
#else
  Path logPath("bin:lgpt.log");
  FileLogger *fileLogger=new FileLogger(logPath);
  if(fileLogger->Init().Succeeded())
  {
    Trace::GetInstance()->SetLogger(*fileLogger);    
  }
#endif

  // Process arguments

	Config::GetInstance()->ProcessArguments(argc,argv) ;

	// Install GUI Factory
	I_GUIWindowFactory::Install(new GUIFactory()) ;

	// Install Timers

	TimerService::GetInstance()->Install(new SDLTimerService()) ;

// 	// See if jack available
// #ifndef _NO_JACK_
// 	if (JackClient::GetInstance()->Init()) {
// 		Trace::Debug("Audio","Found jack.. connecting to it") ;
// 		AudioSettings hints ;  // Jack doesn't care of hints for now on
// 		Audio::Install(new JackAudio(hints)) ;
// 		// MidiService::Install(new JackMidiService()) ;
// 		MidiService::Install(new DummyMidi());
// 	}
// 	else
// #endif
	//  {
		Trace::Debug("Audio","Jack not found.. using default audio driver") ;
		AudioSettings hints ;
		hints.bufferSize_= 1024 ;
		hints.preBufferCount_=8 ;
		// #ifdef USE_SDL_AUDIO
		Trace::Debug("Audio","Using SDL audio driver") ;
		printf("Using SDL audio driver\n") ;
		Audio::Install(new SDLAudio(hints)) ;
		printf("Audio installed\n") ;
		MidiService::Install(new DummyMidi());

		printf("Midi installed using dummy\n") ;
		// #else
		// 	Audio::Install(new RTAudioStub(hints)) ;
		// 	MidiService::Install(new RTMidiService()) ;
		// #endif
	// }

	// Install Threads

	SysProcessFactory::Install(new SDLProcessFactory()) ;

	if ( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0 )   {
		return;
	}
	SDL_EnableUNICODE(1);
	SDL_ShowCursor(SDL_DISABLE);

	atexit(SDL_Quit);

	eventManager_=I_GUIWindowFactory::GetInstance()->GetEventManager() ;
	eventManager_->Init() ;

	eventManager_->MapAppButton("a",APP_BUTTON_A) ;
	eventManager_->MapAppButton("s",APP_BUTTON_B) ;
	eventManager_->MapAppButton("left",APP_BUTTON_LEFT) ;
	eventManager_->MapAppButton("right",APP_BUTTON_RIGHT) ;
	eventManager_->MapAppButton("up",APP_BUTTON_UP) ;
	eventManager_->MapAppButton("down",APP_BUTTON_DOWN) ;
	eventManager_->MapAppButton("right ctrl",APP_BUTTON_L) ;
	eventManager_->MapAppButton("left ctrl",APP_BUTTON_R) ;
	eventManager_->MapAppButton("space",APP_BUTTON_START) ;
} ;

void DEBSystem::Shutdown() {
} ;

static int secbase=0 ;

unsigned long DEBSystem::GetClock() {

   struct timeval tp;

   gettimeofday(&tp, NULL);
   if (!secbase)
    {
        secbase = tp.tv_sec;
        return long(tp.tv_usec/1000.0);
     }
     return long((tp.tv_sec - secbase)*1000 + tp.tv_usec/1000.0);
}

void DEBSystem::Sleep(int millisec) {
/*	if (millisec>0)
		::Sleep(millisec) ;
*/}

void *DEBSystem::Malloc(unsigned size) {
	void *ptr=malloc(size) ;
//	Trace::Debug("alloc:%x  (%d)",ptr,size) ;
	return ptr ;
}

void DEBSystem::Free(void *ptr) {
//	Trace::Debug("free:%x",ptr) ;
	free(ptr) ;
} 

void DEBSystem::Memset(void *addr,char val,int size) {
    intptr_t intptrValue = reinterpret_cast<intptr_t>(addr);
    unsigned int ad=(unsigned int)static_cast<unsigned int>(intptrValue);
    if (((ad&0x3)==0)&&((size&0x3)==0)) { // Are we 4-byte aligned ?
        unsigned int intVal=0 ;
        for (int i=0;i<4;i++) {
             intVal=(intVal<<8)+val ;  
        }
        unsigned int *dst=(unsigned int *)addr ;
        size_t intSize=size>>2 ;

        for (unsigned int i=0;i<intSize;i++) {
            *dst++=intVal ;
        }
    } else {
        memset(addr,val,size) ;
    } ;
} ;

void *DEBSystem::Memcpy(void *s1, const void *s2, int n) {
    return memcpy(s1,s2,n) ;
} ;  

void DEBSystem::AddUserLog(const char *msg) {
	fprintf(stderr,"LOG: %s\n",msg) ;
};

void DEBSystem::PostQuitMessage() {
	SDLEventManager::GetInstance()->PostQuitMessage() ;
} ; 

unsigned int DEBSystem::GetMemoryUsage() {
	return 0 ;
} ;
