#include "MainWindow.h"
#include "AddTaskDialog.h"

#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QStatusBar>
#include <QApplication>
#include <QTableView>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlRelationalTableModel>
#include <QSqlRelationalDelegate>
#include <QToolBar>
#include <QMessageBox>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Базовая настройка окна
    resize(800, 600);
    setWindowTitle("Self Improvement Tracker v0.3");

    tableView = new QTableView(this);
    setCentralWidget(tableView);

    // --- Меню "Файл" ---
    QAction *exitAction = new QAction(tr("В&ыход"), this);
    exitAction->setShortcut(tr("Ctrl+Q"));
    exitAction->setStatusTip(tr("Выйти из приложения"));
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    QMenu *fileMenu = menuBar()->addMenu(tr("&Файл"));
    fileMenu->addAction(exitAction);

    // Инициализация соединения с БД
    initDB();

    // --- Настройка модели данных (RelationalTableModel) ---
    // Используем реляционную модель, чтобы автоматически подтягивать текстовые статусы вместо ID
    m_model = new QSqlRelationalTableModel(this, m_db);
    m_model->setTable("TASK");

    // Устанавливаем связь: столбец 4 ('status_id') в таблице TASK
    // ссылается на столбец 'id' в таблице STATUS, а отображать нужно поле 'name'.
    m_model->setRelation(4, QSqlRelation("STATUS", "id", "name"));

    // Устанавливаем понятные заголовки столбцов
    m_model->setHeaderData(1, Qt::Horizontal, tr("Задание"));
    m_model->setHeaderData(2, Qt::Horizontal, tr("Дата создания"));
    m_model->setHeaderData(3, Qt::Horizontal, tr("Дата выполнения"));
    m_model->setHeaderData(4, Qt::Horizontal, tr("Статус"));

    // Фильтр Soft Delete: показываем только не удаленные задачи.
    // Важно установить ДО вызова select().
    m_model->setFilter("is_deleted = 0");

    // Загружаем данные из базы в модель
    m_model->select();

    // Применяем модель к таблице
    tableView->setModel(m_model);
    // Делегат нужен, чтобы таблица знала, как отрисовывать реляционные данные (выпадающие списки при редактировании)
    tableView->setItemDelegate(new QSqlRelationalDelegate(tableView));

    // --- Настройка внешнего вида таблицы ---
    // Скрываем технические столбцы: ID (0) и флаг удаления (5)
    tableView->hideColumn(0);
    tableView->hideColumn(5);

    // Настраиваем поведение выделения: только одна строка целиком.
    // Это упрощает логику удаления и выглядит лучше.
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Подгоняем ширину столбцов под содержимое
    tableView->resizeColumnsToContents();

    // --- Настройка контекстного меню (ПКМ) ---
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableView, &QTableView::customContextMenuRequested, this, &MainWindow::onCustomContextMenu);

    // --- Панель инструментов (ToolBar) ---
    m_addTaskAction = new QAction(tr("&Добавить"), this);
    m_addTaskAction->setStatusTip(tr("Добавить новую задачу"));
    connect(m_addTaskAction, &QAction::triggered, this, &MainWindow::onAddTask);

    m_mainToolBar = new QToolBar(tr("Инструменты"), this);
    // Запрещаем скрывать панель через контекстное меню, чтобы не сбивать пользователя
    m_mainToolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_mainToolBar->setMovable(false); // Закрепляем панель
    m_mainToolBar->addAction(m_addTaskAction);

    addToolBar(Qt::TopToolBarArea, m_mainToolBar);

    statusBar()->showMessage(tr("Готов к работе"));
}

MainWindow::~MainWindow()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

void MainWindow::initDB()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    // Используем относительный путь, чтобы БД лежала рядом с исходниками, а не в папке build
    m_db.setDatabaseName("../tracker.db");

    if (!m_db.open()) {
        qCritical() << "Database connection failed:" << m_db.lastError().text();
        QMessageBox::critical(this, tr("Ошибка БД"), tr("Не удалось подключиться к базе данных."));
    } else {
        qDebug() << "Database connected successfully.";

        QSqlQuery query(m_db);
        // Включаем поддержку внешних ключей (для SQLite это важно)
        query.exec("PRAGMA foreign_keys = ON;");

        // 1. Создаем таблицу статусов (справочник)
        QString createStatusTable = "CREATE TABLE IF NOT EXISTS STATUS ("
                                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                    "name TEXT NOT NULL UNIQUE);";
        if (!query.exec(createStatusTable)) {
            qCritical() << "Failed to create 'STATUS' table:" << query.lastError();
        }

        // 2. Создаем основную таблицу задач
        QString createTaskTable = "CREATE TABLE IF NOT EXISTS TASK ("
                                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                  "description TEXT NOT NULL, "
                                  "creation_dt TEXT, "
                                  "completion_dt TEXT, "
                                  "status_id INTEGER, "
                                  "is_deleted INTEGER DEFAULT 0, "
                                  "FOREIGN KEY(status_id) REFERENCES STATUS(id));";
        if (!query.exec(createTaskTable)) {
             qCritical() << "Failed to create 'TASK' table:" << query.lastError();
        }

        // 3. Заполняем справочник начальными значениями, если он пуст
        if (query.exec("SELECT COUNT(*) FROM STATUS") && query.next() && query.value(0).toInt() == 0) {
            qDebug() << "Populating 'STATUS' table with default values...";
            query.exec("INSERT INTO STATUS (name) VALUES ('В процессе'), ('Сделано'), ('Отменено');");
        }
    }
}

void MainWindow::onAddTask()
{
    AddTaskDialog dialog(this);
    // Запускаем диалог модально. Ждем, пока пользователь нажмет ОК или Отмена.
    if (dialog.exec() == QDialog::Accepted) {
        QString description = dialog.getTaskDescription();
        QString statusName = dialog.getSelectedStatus();
        QString creationTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

        // Получаем ID статуса по его имени для вставки в таблицу TASK
        QSqlQuery statusQuery(m_db);
        statusQuery.prepare("SELECT id FROM STATUS WHERE name = :name");
        statusQuery.bindValue(":name", statusName);

        int statusId = 1; // Fallback значение
        if (statusQuery.exec() && statusQuery.next()) {
             statusId = statusQuery.value(0).toInt();
        } else {
             qWarning() << "Status ID not found for:" << statusName;
        }

        // Подготавливаем новую запись через модель
        QSqlRecord newRecord = m_model->record();
        newRecord.setValue("description", description);
        newRecord.setValue("creation_dt", creationTime);
        newRecord.setValue("status_id", statusId);
        // ВАЖНО: Явно указываем 0, чтобы избежать NULL значений, которые могут быть скрыты фильтром
        newRecord.setValue("is_deleted", 0);

        // Вставляем запись (-1 означает вставку в конец)
        if (m_model->insertRecord(-1, newRecord)) {
            // Применяем изменения и обновляем отображение
            m_model->submitAll();
            m_model->select();
            statusBar()->showMessage(tr("Задача успешно добавлена"));
        } else {
            QMessageBox::warning(this, tr("Ошибка"),
                                 tr("Не удалось добавить задачу: ") + m_model->lastError().text());
        }
    }
}

void MainWindow::onCustomContextMenu(const QPoint &pos)
{
    // Проверяем, что клик был именно по ячейке с данными
    QModelIndex index = tableView->indexAt(pos);
    if (!index.isValid()) return;

    QMenu contextMenu(this);
    QAction *actionSoftDelete = contextMenu.addAction(tr("Удалить (в корзину)"));
    QAction *actionHardDelete = contextMenu.addAction(tr("Удалить полностью"));

    // Используем connect с лямбдами или прямыми слотами
    connect(actionSoftDelete, &QAction::triggered, this, &MainWindow::onDeleteSoft);
    connect(actionHardDelete, &QAction::triggered, this, &MainWindow::onDeleteHard);

    // Показываем меню в глобальных координатах курсора
    contextMenu.exec(tableView->viewport()->mapToGlobal(pos));
}

void MainWindow::onDeleteSoft()
{
    // Получаем список выделенных строк (благодаря SingleSelection там будет максимум одна)
    QModelIndexList selectedRows = tableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) return;

    int row = selectedRows.first().row();

    // Soft Delete: просто меняем флаг is_deleted на 1 (столбец с индексом 5)
    if (m_model->setData(m_model->index(row, 5), 1)) {
        if (m_model->submitAll()) {
            // Обязательно обновляем модель, чтобы фильтр "is_deleted = 0" скрыл строку
            m_model->select();
            statusBar()->showMessage(tr("Задача перемещена в корзину"));
        } else {
             QMessageBox::warning(this, tr("Ошибка БД"), m_model->lastError().text());
        }
    }
}

void MainWindow::onDeleteHard()
{
    QModelIndexList selectedRows = tableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) return;

    // Запрашиваем подтверждение, так как действие необратимо
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Подтверждение"),
                                  tr("Вы уверены, что хотите удалить эту задачу НАВСЕГДА?"),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Hard Delete: физическое удаление строки из БД
        m_model->removeRow(selectedRows.first().row());

        if (m_model->submitAll()) {
            m_model->select(); // Обновляем, чтобы убрать возможные пустые строки
            statusBar()->showMessage(tr("Задача удалена полностью"));
        } else {
            QMessageBox::warning(this, tr("Ошибка БД"), m_model->lastError().text());
            m_model->revertAll(); // Если не вышло, отменяем изменения в модели
        }
    }
}