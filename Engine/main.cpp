#include "AllHeader.h"
#include "AstranEditorUI.h"

/*
extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
*/

//Teena - TODO: Fix the fucking engine

void Stealth()
{
	HWND Stealth;
	AllocConsole();
	Stealth = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(Stealth, 0);
}

int main(void)
{
	//Stealth();
	//HWND hWnd = GetConsoleWindow();
	//ShowWindow(hWnd, SW_HIDE);

	AstranEditorUI editorUI;
	editorUI.StartupModule();

	editorUI.ImGuiRender();
	
	editorUI.ShutdownModule();
	return 0;
}
