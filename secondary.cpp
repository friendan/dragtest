#include <windows.h>
#include "SlaveWindow.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SlaveWindow::StartMainWindow(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    return 0;
}


