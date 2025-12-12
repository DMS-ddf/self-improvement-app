#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QSqlDatabase>

// Forward declarations — ускоряют компиляцию.
class QTableView;
class QSqlRelationalTableModel;
class QToolBar;
class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Именованные индексы столбцов таблицы
    enum Column {
        COL_ID = 0,
        COL_DESC = 1,
        COL_DETAILS = 2,
        COL_CREATION_DT = 3,
        COL_COMPLETION_DT = 4,
        COL_STATUS = 5,
        COL_IS_DELETED = 6
    };

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Слоты: добавление, контекстное меню, удаление, редактирование
    void onAddTask();
    void onCustomContextMenu(const QPoint &pos);
    void onDeleteSoft();
    void onDeleteHard();
    void onEditTask();
    void onTableDoubleClicked(const QModelIndex &index);

private:
    // Инициализация и подготовка БД
    void initDB();

    // Виджеты и модель
    QTableView *tableView;
    QSqlDatabase m_db;
    QSqlRelationalTableModel *m_model;

    // Панель инструментов
    QToolBar *m_mainToolBar;
    QAction *m_addTaskAction;
    QAction *m_editTaskAction;

    int m_statusDoneId; // ID статуса "Сделано"
};

#endif // MAINWINDOW_H