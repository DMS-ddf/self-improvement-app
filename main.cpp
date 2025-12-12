#include <QApplication>
#include "MainWindow.h"

// Подключаем WinAPI только на Windows для управления консольной кодировкой
#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
    // Устанавливаем кодировку консоли (опционально, для корректного вывода в терминал)
    SetConsoleOutputCP(1251);
#endif

    // Создаём Qt-приложение и главное окно
    QApplication app(argc, argv);
    MainWindow window;
    window.show();

    return app.exec();
}