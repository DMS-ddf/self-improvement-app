#include <QApplication>
#include "MainWindow.h"

// Если компилируем под Windows, подключаем заголовок для работы с её API.
// Это нужно только для функции SetConsoleOutputCP.
#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
    // Специальный хак для Windows: устанавливаем кодировку консоли в CP1251 (кириллица).
    // Это нужно, чтобы qDebug() с русским текстом не выводил "крякозябры" в терминал VSCode.
    // На работу самого графического интерфейса это не влияет.
#ifdef _WIN32
    SetConsoleOutputCP(1251);
#endif

    // Создаем главный объект приложения Qt. Он управляет циклом событий,
    // настройками GUI и многим другим. Должен быть создан ровно один раз.
    QApplication app(argc, argv);

    // Создаем и показываем наше главное окно.
    MainWindow window;
    window.show();

    // Запускаем цикл обработки событий (Event Loop).
    // Программа будет "крутиться" внутри app.exec(), пока не будет дана команда на выход.
    return app.exec();
}