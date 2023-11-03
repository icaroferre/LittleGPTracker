#include "Application/Application.h"
#include "Adapters/DEB/System/RG351PSystem.h"
#include <string.h>
#include "Adapters/SDL/GUI/SDLGUIWindowImp.h"

int main(int argc,char *argv[]) 
{
	RG351PSystem::Boot(argc,argv) ;

	SDLCreateWindowParams params ;
	params.title="littlegptracker" ;
	params.cacheFonts_=true ;

	Application::GetInstance()->Init(params) ;

	int retval=RG351PSystem::MainLoop() ;

	return retval ;
}



void _assert() {
} ;
