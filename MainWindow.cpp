#include "MainWindow.h"
#include "AddTaskDialog.h"

#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QStatusBar>
#include <QApplication>
#include <QTableView>
#include <QHeaderView>
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
        m_model->setRelation(MainWindow::COL_STATUS, QSqlRelation("STATUS", "id", "name"));

    // Устанавливаем понятные заголовки столбцов
        m_model->setHeaderData(MainWindow::COL_DESC, Qt::Horizontal, tr("Задание"));
        m_model->setHeaderData(MainWindow::COL_DETAILS, Qt::Horizontal, tr("Описание"));
        m_model->setHeaderData(MainWindow::COL_CREATION_DT, Qt::Horizontal, tr("Дата создания"));
        m_model->setHeaderData(MainWindow::COL_COMPLETION_DT, Qt::Horizontal, tr("Дата выполнения"));
        m_model->setHeaderData(MainWindow::COL_STATUS, Qt::Horizontal, tr("Статус"));

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
        tableView->hideColumn(MainWindow::COL_ID);
        tableView->hideColumn(MainWindow::COL_IS_DELETED);

    // Настраиваем поведение выделения: только одна строка целиком.
    // Это упрощает логику удаления и выглядит лучше.
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers); // Запрет изменений по double-click RMB.

    // Подгоняем ширину столбцов: делаем столбец с описанием растягивающимся,
    // остальные подгоняем под содержимое — это улучшает читаемость при изменении ширины окна.
    QHeaderView *header = tableView->horizontalHeader();
    header->setSectionResizeMode(MainWindow::COL_DESC, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(MainWindow::COL_DETAILS, QHeaderView::Stretch);
    header->setSectionResizeMode(MainWindow::COL_CREATION_DT, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(MainWindow::COL_COMPLETION_DT, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(MainWindow::COL_STATUS, QHeaderView::ResizeToContents);

    // --- Настройка контекстного меню (ПКМ) ---
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableView, &QTableView::customContextMenuRequested, this, &MainWindow::onCustomContextMenu);
    connect(tableView, &QTableView::doubleClicked, this, &MainWindow::onTableDoubleClicked);

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
           dir.mkpath(dataPath); // Создаем её, если нет
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
              qCritical() << "Failed to create 'STATUS' table:" << query.lastError().text();
        }

        // 2. Создаем основную таблицу задач
        QString createTaskTable = "CREATE TABLE IF NOT EXISTS TASK ("
                                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                  "description TEXT NOT NULL, "
                                  "details TEXT, "
                                  "creation_dt TEXT, "
                                  "completion_dt TEXT, "
                                  "status_id INTEGER, "
                                  "is_deleted INTEGER DEFAULT 0, "
                                  "FOREIGN KEY(status_id) REFERENCES STATUS(id));";
        if (!query.exec(createTaskTable))
        {
              qCritical() << "Failed to create 'TASK' table:" << query.lastError().text();
        }

        // Ensure `details` column exists AND is in the expected position.
        // Older DBs may lack the column or have it appended at the end (SQLite ALTER TABLE adds columns at the end).
        QSqlQuery pragmaCheck(m_db);
        QStringList cols;
        if (pragmaCheck.exec("PRAGMA table_info('TASK');")) {
            while (pragmaCheck.next()) {
                cols << pragmaCheck.value("name").toString();
            }
        }

        int idxDetails = cols.indexOf("details");
        if (idxDetails == -1) {
            // Column missing: add it (simple ALTER is fine)
            qDebug() << "Adding 'details' column to existing TASK table...";
            if (!query.exec("ALTER TABLE TASK ADD COLUMN details TEXT;")) {
                qCritical() << "Failed to add 'details' column:" << query.lastError().text();
            }
        } else if (idxDetails != 2) {
            // Column exists but in wrong position — rebuild table with desired column order
            qDebug() << "Rebuilding TASK table to normalize column order...";
            QString createNew = "CREATE TABLE IF NOT EXISTS TASK_new ("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                "description TEXT NOT NULL, "
                                "details TEXT, "
                                "creation_dt TEXT, "
                                "completion_dt TEXT, "
                                "status_id INTEGER, "
                                "is_deleted INTEGER DEFAULT 0, "
                                "FOREIGN KEY(status_id) REFERENCES STATUS(id));";
            if (!query.exec("BEGIN TRANSACTION;")) qWarning() << query.lastError().text();
            if (!query.exec(createNew)) {
                qCritical() << "Failed to create TASK_new:" << query.lastError().text();
            } else {
                // Copy data by column names to preserve values regardless of physical order
                QString copySql = "INSERT INTO TASK_new (id, description, details, creation_dt, completion_dt, status_id, is_deleted) "
                                  "SELECT id, description, details, creation_dt, completion_dt, status_id, is_deleted FROM TASK;";
                if (!query.exec(copySql)) {
                    qCritical() << "Failed to copy data to TASK_new:" << query.lastError().text();
                } else {
                    if (!query.exec("DROP TABLE TASK;")) {
                        qCritical() << "Failed to drop old TASK table:" << query.lastError().text();
                    } else if (!query.exec("ALTER TABLE TASK_new RENAME TO TASK;")) {
                        qCritical() << "Failed to rename TASK_new to TASK:" << query.lastError().text();
                    }
                }
            }
            if (!query.exec("COMMIT;")) qWarning() << query.lastError().text();
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
        QString details = dialog.getTaskDetails();
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
        insertQuery.prepare("INSERT INTO TASK (description, details, creation_dt, status_id, is_deleted) "
                    "VALUES (:desc, :details, :created, :status_id, 0)");

        insertQuery.bindValue(":desc", description);
        insertQuery.bindValue(":details", details);
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
    if (m_model->setData(m_model->index(row, MainWindow::COL_IS_DELETED), 1))
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
    // Нам нужны: ID (0), Описание (1), Подробное описание (2) и Имя Статуса (5)
    int taskId = m_model->data(m_model->index(row, MainWindow::COL_ID)).toInt();
    QString currentDesc = m_model->data(m_model->index(row, MainWindow::COL_DESC)).toString();
    QString currentDetails = m_model->data(m_model->index(row, MainWindow::COL_DETAILS)).toString();
    QString currentStatus = m_model->data(m_model->index(row, MainWindow::COL_STATUS)).toString(); // QSqlRelationalTableModel крутой и отдает нам текст!

    // 3. Создаем диалог и заполняем его данными
    AddTaskDialog dialog(this);
    dialog.setWindowTitle(tr("Редактировать задачу")); // Меняем заголовок
    dialog.setTaskData(currentDesc, currentDetails, currentStatus);

    // 4. Запускаем диалог и ждем, пока пользователь нажмет "ОК"
    if (dialog.exec() == QDialog::Accepted)
    {
        // 5. Получаем НОВЫЕ данные из диалога
        QString newDesc = dialog.getTaskDescription();
        QString newDetails = dialog.getTaskDetails();
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
                    "details = :details, "
                    "status_id = :status_id, "
                    "completion_dt = :completion_dt "
                    "WHERE id = :id");

        updateQuery.bindValue(":desc", newDesc);
        updateQuery.bindValue(":details", newDetails);
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

void MainWindow::onTableDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    int row = index.row();

    int taskId = m_model->data(m_model->index(row, MainWindow::COL_ID)).toInt();
    QString currentDesc = m_model->data(m_model->index(row, MainWindow::COL_DESC)).toString();
    QString currentDetails = m_model->data(m_model->index(row, MainWindow::COL_DETAILS)).toString();
    QString currentStatus = m_model->data(m_model->index(row, MainWindow::COL_STATUS)).toString();

    AddTaskDialog dialog(this);
    dialog.setWindowTitle(tr("Просмотр / редактирование задачи"));
    dialog.setTaskData(currentDesc, currentDetails, currentStatus);

    if (dialog.exec() == QDialog::Accepted)
    {
        QString newDesc = dialog.getTaskDescription();
        QString newDetails = dialog.getTaskDetails();
        QString newStatusName = dialog.getSelectedStatus();

        QSqlQuery statusQuery(m_db);
        statusQuery.prepare("SELECT id FROM STATUS WHERE name = :name");
        statusQuery.bindValue(":name", newStatusName);
        int newStatusId = 1;
        if (statusQuery.exec() && statusQuery.next())
            newStatusId = statusQuery.value(0).toInt();

        QVariant completionDt;
        if (newStatusId == m_statusDoneId)
            completionDt = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

        QSqlQuery updateQuery(m_db);
        updateQuery.prepare("UPDATE TASK SET description = :desc, details = :details, status_id = :status_id, completion_dt = :completion_dt WHERE id = :id");
        updateQuery.bindValue(":desc", newDesc);
        updateQuery.bindValue(":details", newDetails);
        updateQuery.bindValue(":status_id", newStatusId);
        updateQuery.bindValue(":completion_dt", completionDt);
        updateQuery.bindValue(":id", taskId);

        if (updateQuery.exec()) {
            m_model->select();
            statusBar()->showMessage(tr("Задача обновлена"));
        } else {
            qCritical() << "Failed to update task (double-click):" << updateQuery.lastError().text();
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось сохранить изменения."));
        }
    }
}