#include <windows.h>
#include "MainWindow.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    MainWindow::StartMainWindow(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    return 0;
}


