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
#include <QStandardPaths>
#include <QDir>

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

    // Узнаем ID статуса "Сделано" один раз при запуске
    m_statusDoneId = -1; // Значение по умолчанию, если не найдем
    QSqlQuery statusQuery(m_db);
    statusQuery.prepare("SELECT id FROM STATUS WHERE name = :name");
    statusQuery.bindValue(":name", tr("Сделано"));
    if (statusQuery.exec() && statusQuery.next())
    {
        m_statusDoneId = statusQuery.value(0).toInt();
    }
    else
    {
        qWarning() << "CRITICAL: Could not find 'Сделано' status ID! Date logic will fail.";
    }

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
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers); // Запрет изменений по double-click RMB.

    // Подгоняем ширину столбцов под содержимое
    tableView->resizeColumnsToContents();

    // --- Настройка контекстного меню (ПКМ) ---
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableView, &QTableView::customContextMenuRequested, this, &MainWindow::onCustomContextMenu);

    // --- Панель инструментов (ToolBar) ---
    m_addTaskAction = new QAction(tr("&Добавить"), this);
    m_addTaskAction->setStatusTip(tr("Добавить новую задачу"));
    connect(m_addTaskAction, &QAction::triggered, this, &MainWindow::onAddTask);

    m_editTaskAction = new QAction(tr("&Редактировать"), this);
    m_editTaskAction->setStatusTip(tr("Редактировать выбранную задачу"));
    connect(m_editTaskAction, &QAction::triggered, this, &MainWindow::onEditTask);

    m_mainToolBar = new QToolBar(tr("Инструменты"), this);
    // Запрещаем скрывать панель через контекстное меню, чтобы не сбивать пользователя
    m_mainToolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_mainToolBar->setMovable(false); // Закрепляем панель
    m_mainToolBar->addAction(m_addTaskAction);
    m_mainToolBar->addAction(m_editTaskAction); // Добавляем кнопку на панель

    addToolBar(Qt::TopToolBarArea, m_mainToolBar);

    statusBar()->showMessage(tr("Готов к работе"));
}

MainWindow::~MainWindow()
{
    if (m_db.isOpen())
    {
        m_db.close();
    }
}

void MainWindow::initDB()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    // 1. Находим "правильное" место для хранения данных
    // (Это будет C:/Users/ТвоёИмя/AppData/Local/SelfImprovementApp)
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    // 2. Убеждаемся, что эта папка существует
    QDir dir(dataPath);
    if (!dir.exists())
    {
        dir.mkpath("."); // Создаем её, если нет
    }

    // 3. Устанавливаем полный путь к БД
    QString dbPath = dataPath + "/tracker.db";
    qDebug() << "Database path set to:" << dbPath;

    m_db.setDatabaseName(dbPath);
    if (!m_db.open())
    {
        qCritical() << "Database connection failed:" << m_db.lastError().text();
        QMessageBox::critical(this, tr("Ошибка БД"), tr("Не удалось подключиться к базе данных."));
    }
    else
    {
        qDebug() << "Database connected successfully.";

        QSqlQuery query(m_db);
        // Включаем поддержку внешних ключей (для SQLite это важно)
        query.exec("PRAGMA foreign_keys = ON;");

        // 1. Создаем таблицу статусов (справочник)
        QString createStatusTable = "CREATE TABLE IF NOT EXISTS STATUS ("
                                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                    "name TEXT NOT NULL UNIQUE);";
        if (!query.exec(createStatusTable))
        {
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
        if (!query.exec(createTaskTable))
        {
            qCritical() << "Failed to create 'TASK' table:" << query.lastError();
        }

        // 3. Заполняем справочник начальными значениями, если он пуст
        if (query.exec("SELECT COUNT(*) FROM STATUS") && query.next() && query.value(0).toInt() == 0)
        {
            qDebug() << "Populating 'STATUS' table with default values...";
            query.exec("INSERT INTO STATUS (name) VALUES ('В процессе'), ('Сделано'), ('Отменено');");
        }
    }
}

void MainWindow::onAddTask()
{
    AddTaskDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        QString description = dialog.getTaskDescription();
        QString statusName = dialog.getSelectedStatus();
        QString creationTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

        // 1. Получаем ID статуса (эта логика остается)
        QSqlQuery statusQuery(m_db);
        statusQuery.prepare("SELECT id FROM STATUS WHERE name = :name");
        statusQuery.bindValue(":name", statusName);
        int statusId = 1; // "В процессе" по умолчанию
        if (statusQuery.exec() && statusQuery.next())
        {
            statusId = statusQuery.value(0).toInt();
        }

        // 2. Вставляем данные через прямой SQL-запрос (вместо insertRecord)
        QSqlQuery insertQuery(m_db);
        insertQuery.prepare("INSERT INTO TASK (description, creation_dt, status_id, is_deleted) "
                            "VALUES (:desc, :created, :status_id, 0)");

        insertQuery.bindValue(":desc", description);
        insertQuery.bindValue(":created", creationTime);
        insertQuery.bindValue(":status_id", statusId);

        if (insertQuery.exec())
        {
            // 3. Просто обновляем модель, чтобы она увидела новую запись
            m_model->select();
            statusBar()->showMessage(tr("Задача успешно добавлена!"));
        }
        else
        {
            qCritical() << "Failed to insert new task:" << insertQuery.lastError().text();
            QMessageBox::critical(this, tr("Ошибка"),
                                  tr("Не удалось добавить задачу в базу данных."));
        }
    }
}

void MainWindow::onCustomContextMenu(const QPoint &pos)
{
    // Проверяем, что клик был именно по ячейке с данными
    QModelIndex index = tableView->indexAt(pos);
    if (!index.isValid())
        return;

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
    if (selectedRows.isEmpty())
        return;

    int row = selectedRows.first().row();

    // Soft Delete: просто меняем флаг is_deleted на 1 (столбец с индексом 5)
    if (m_model->setData(m_model->index(row, 5), 1))
    {
        if (m_model->submitAll())
        {
            // Обязательно обновляем модель, чтобы фильтр "is_deleted = 0" скрыл строку
            m_model->select();
            statusBar()->showMessage(tr("Задача перемещена в корзину"));
        }
        else
        {
            QMessageBox::warning(this, tr("Ошибка БД"), m_model->lastError().text());
        }
    }
}

void MainWindow::onDeleteHard()
{
    QModelIndexList selectedRows = tableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return;

    // Запрашиваем подтверждение, так как действие необратимо
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Подтверждение"),
                                  tr("Вы уверены, что хотите удалить эту задачу НАВСЕГДА?"),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        // Hard Delete: физическое удаление строки из БД
        m_model->removeRow(selectedRows.first().row());

        if (m_model->submitAll())
        {
            m_model->select(); // Обновляем, чтобы убрать возможные пустые строки
            statusBar()->showMessage(tr("Задача удалена полностью"));
        }
        else
        {
            QMessageBox::warning(this, tr("Ошибка БД"), m_model->lastError().text());
            m_model->revertAll(); // Если не вышло, отменяем изменения в модели
        }
    }
}

void MainWindow::onEditTask()
{
    // 1. Проверяем, выбрана ли строка
    QModelIndexList selectedRows = tableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
    {
        QMessageBox::information(this, tr("Редактирование"), tr("Пожалуйста, выберите задачу для редактирования."));
        return;
    }

    int row = selectedRows.first().row();

    // 2. Извлекаем текущие данные из модели
    // Нам нужны: ID (0), Описание (1) и Имя Статуса (4)
    int taskId = m_model->data(m_model->index(row, 0)).toInt();
    QString currentDesc = m_model->data(m_model->index(row, 1)).toString();
    QString currentStatus = m_model->data(m_model->index(row, 4)).toString(); // QSqlRelationalTableModel крутой и отдает нам текст!

    // 3. Создаем диалог и заполняем его данными
    AddTaskDialog dialog(this);
    dialog.setWindowTitle(tr("Редактировать задачу")); // Меняем заголовок
    dialog.setTaskData(currentDesc, currentStatus);

    // 4. Запускаем диалог и ждем, пока пользователь нажмет "ОК"
    if (dialog.exec() == QDialog::Accepted)
    {
        // 5. Получаем НОВЫЕ данные из диалога
        QString newDesc = dialog.getTaskDescription();
        QString newStatusName = dialog.getSelectedStatus();

        // 6. Получаем ID нового статуса
        QSqlQuery statusQuery(m_db);
        statusQuery.prepare("SELECT id FROM STATUS WHERE name = :name");
        statusQuery.bindValue(":name", newStatusName);
        int newStatusId = 1; // "В процессе" по умолчанию
        if (statusQuery.exec() && statusQuery.next())
        {
            newStatusId = statusQuery.value(0).toInt();
        }

        // 7. *** ВОТ ОНА! ЧИСТАЯ ЛОГИКА ДАТЫ ***
        QVariant completionDt; // По умолчанию NULL
        if (newStatusId == m_statusDoneId)
        {
            // Если статус "Сделано", ставим текущую дату
            completionDt = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        }

        // 8. Подготавливаем и выполняем прямой SQL-запрос UPDATE
        QSqlQuery updateQuery(m_db);
        updateQuery.prepare("UPDATE TASK SET "
                            "description = :desc, "
                            "status_id = :status_id, "
                            "completion_dt = :completion_dt "
                            "WHERE id = :id");

        updateQuery.bindValue(":desc", newDesc);
        updateQuery.bindValue(":status_id", newStatusId);
        updateQuery.bindValue(":completion_dt", completionDt);
        updateQuery.bindValue(":id", taskId);

        if (updateQuery.exec())
        {
            m_model->select(); // Обновляем таблицу, чтобы увидеть изменения
            statusBar()->showMessage(tr("Задача успешно обновлена!"));
        }
        else
        {
            qCritical() << "Failed to update task:" << updateQuery.lastError().text();
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось обновить задачу."));
        }
    }
}