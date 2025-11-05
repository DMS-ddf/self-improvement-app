#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>

// Прямые объявления (forward declarations) нужны, чтобы ускорить компиляцию.
// Вместо подключения тяжелых хедеров, мы просто говорим компилятору, что такие классы существуют.
class QTableView;
class QSqlRelationalTableModel;
class QToolBar;
class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT // Макрос обязателен для любого класса, использующего сигналы и слоты Qt

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Слот для обработки нажатия на кнопку "Добавить" в тулбаре
    void onAddTask();

    // Слоты для работы с контекстным меню таблицы
    void onCustomContextMenu(const QPoint &pos); // Вызывается при клике ПКМ
    void onDeleteSoft();                         // "Мягкое" удаление (в корзину)
    void onDeleteHard();                         // Полное удаление из базы

private:
    // Метод для инициализации подключения к базе данных и создания таблиц
    void initDB();

    // Основные элементы интерфейса и данных
    QTableView *tableView;             // Виджет таблицы
    QSqlDatabase m_db;                 // Объект подключения к БД
    QSqlRelationalTableModel *m_model; // Модель данных с поддержкой связей между таблицами

    // Элементы панели инструментов
    QToolBar *m_mainToolBar;
    QAction *m_addTaskAction;
};

#endif // MAINWINDOW_H