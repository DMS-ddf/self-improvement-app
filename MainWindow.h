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
class QSqlQueryModel;
class QPushButton;
class QLabel;
class QComboBox;
class QWidget;
class QStyledItemDelegate;

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
    void onHeaderClicked(int section);

private:
    // Инициализация и подготовка БД
    void initDB();
    void refreshView();

    // Виджеты и модель
    QTableView *tableView;
    QSqlDatabase m_db;
    QSqlRelationalTableModel *m_model;
    QSqlQueryModel *m_viewModel;

    // Панель инструментов
    QToolBar *m_mainToolBar;
    QAction *m_addTaskAction;
    QAction *m_editTaskAction;

    // Pagination UI and state
    QWidget *m_paginationWidget;
    QPushButton *m_prevPageButton;
    QPushButton *m_nextPageButton;
    QLabel *m_pageInfoLabel;
    QComboBox *m_pageSizeCombo;
    int m_pageSize;
    int m_currentPage;

    QStyledItemDelegate *m_statusDelegate;
    QPushButton *m_addTaskButton;

    int m_statusDoneId; // ID статуса "Сделано"
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
};

#endif // MAINWINDOW_H