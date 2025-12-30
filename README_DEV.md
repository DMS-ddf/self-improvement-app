Self-Improvement App — Developer Notes

Краткое описание
- Небольшое Qt6/C++ приложение для отслеживания задач.
- GUI: `MainWindow` (таблица задач), диалог `AddTaskDialog`.
- Хранение: SQLite (файл `tracker.db` в каталоге AppData для пользователя).

Файловая структура (важное)
- main.cpp — точка входа, запускает `QApplication` и `MainWindow`.
- MainWindow.h / MainWindow.cpp — основное окно, модель `QSqlRelationalTableModel`, операции CRUD.
  - Введён `enum Column` (COL_ID, COL_DESC, COL_CREATION_DT, COL_COMPLETION_DT, COL_STATUS, COL_IS_DELETED).
  - `initDB()` создаёт/подключает SQLite, таблицы `STATUS` и `TASK`.
- AddTaskDialog.h / AddTaskDialog.cpp — диалог для добавления/редактирования задач.
- CMakeLists.txt — сборка проекта (Qt6), настройка `CMAKE_PREFIX_PATH` указывает путь к Qt.

Как приложение работает (в двух словах)
1. При старте `main()` создаёт `QApplication` и отображает `MainWindow`.
2. `MainWindow::initDB()` открывает SQLite (файл `%AppData%/SelfImprovementApp/tracker.db`), создаёт таблицы и заполняет справочник статусов если пуст.
3. `QSqlRelationalTableModel` загружает таблицу `TASK`, связывает столбец `status_id` со справочником `STATUS`.
4. Добавление/редактирование происходит через `AddTaskDialog`; при изменении статуса `"Сделано"` в коде ставится `completion_dt`.
5. Удаление: "мягкое" (флаг `is_deleted = 1`) и "жёсткое" (физическое удаление из БД).

Где что править быстро
- Порядок/индексы столбцов: `MainWindow::Column` в `MainWindow.h`.
- SQL-схема: в `MainWindow::initDB()` — если нужна новая колонка, добавить CREATE TABLE или ALTER.
- Статусы: таблица `STATUS` (заполняется автоматически только если пусто).

Сборка и запуск (Windows, пример с MSYS2/MinGW-w64)
1. Установите Qt (например, `Z:/Qt/6.10.0/mingw_64`) и MSYS2/mingw64 toolchain.
2. Установите CMake (и убедитесь, что `cmake` в PATH).
3. В корне проекта:

```powershell
Remove-Item -Recurse -Force .\build
mkdir build; cd build
cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER="Z:/msys/mingw64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="Z:/msys/mingw64/bin/g++.exe" -DCMAKE_PREFIX_PATH="Z:/Qt/6.10.0/mingw_64" ..
Z:\msys\mingw64\bin\mingw32-make.exe -j1
```

4. Собранный бинарник будет в `build/bin/SelfImprovementApp.exe`.

Примечания по отладке
- Если moc/автоген ругается, удалите `build/` и пересоберите полностью — это синхронизирует moc и заголовки.
- Если появляются ошибки линковки по `__imp___argc` или похожие — убедитесь, что используемый компилятор соответствует сборке Qt (MSYS2/mingw-w64 vs MSVC).
- Для локальных правок UI используйте Qt Creator (он сам настроит Qt Kit и moc).

Идеи для развития (быстрый TODO)
- Добавить поиск/фильтрацию по тексту задач.
- Поддержка экспорта/импорта (CSV).
- Улучшить обработку статусов: хранить порядок/цвет, поддерживать редактирование справочника.

Если нужно — могу дополнить README примерами кода, диаграммой потоков или более подробной инструкцией по деплою на Windows.
