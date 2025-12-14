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
#include <QStyledItemDelegate>
#include <QPainter>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
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
    setWindowTitle("Self Improvement Tracker v1.0.2");

    tableView = new QTableView(this);
    // We'll embed the table into a central widget that also contains pagination controls
    QWidget *central = new QWidget(this);
    QVBoxLayout *centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0,0,0,0);
    centralLayout->addWidget(tableView);

    // Pagination controls (created below)
    m_paginationWidget = new QWidget(central);
    QHBoxLayout *pLay = new QHBoxLayout(m_paginationWidget);
    pLay->setContentsMargins(6,6,6,6);
    pLay->setSpacing(8);
    m_prevPageButton = new QPushButton(tr("< Пред"), m_paginationWidget);
    m_nextPageButton = new QPushButton(tr("След >"), m_paginationWidget);
    m_pageInfoLabel = new QLabel(m_paginationWidget);
    m_pageSizeCombo = new QComboBox(m_paginationWidget);
    m_pageSizeCombo->addItems({"10","25","50","75","100"});
    m_pageSizeCombo->setCurrentIndex(0);
    pLay->addWidget(m_prevPageButton);
    pLay->addWidget(m_nextPageButton);
    // Prominent Add button placed near pagination controls for easy access
    m_addTaskButton = new QPushButton(tr("+ Добавить"), m_paginationWidget);
    m_addTaskButton->setToolTip(tr("Добавить новую задачу (Ctrl+N)"));
    m_addTaskButton->setMinimumHeight(28);
    m_addTaskButton->setStyleSheet("QPushButton{background-color:#3a86ff;color:white;border-radius:4px;padding:6px 12px;} QPushButton:hover{background-color:#2f6fe0;}");
    connect(m_addTaskButton, &QPushButton::clicked, this, &MainWindow::onAddTask);
    pLay->addWidget(m_addTaskButton);
    pLay->addStretch();
    pLay->addWidget(new QLabel(tr("Показывать по:"), m_paginationWidget));
    pLay->addWidget(m_pageSizeCombo);
    pLay->addWidget(m_pageInfoLabel);
    centralLayout->addWidget(m_paginationWidget);

    // Ensure the pagination bar stays visible and has fixed height
    m_paginationWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_pageInfoLabel->setText(tr("Стр. 1 / 1 (0)"));
    centralLayout->setStretch(0, 1); // tableView expands
    centralLayout->setStretch(1, 0); // pagination stays compact

    setCentralWidget(central);

    // Delegate to color the status column based on status text
    class StatusColorDelegate : public QStyledItemDelegate {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
            if (index.column() != MainWindow::COL_STATUS) {
                QStyledItemDelegate::paint(painter, option, index);
                return;
            }

            // If the row is selected, keep default selection rendering
            if (option.state & QStyle::State_Selected) {
                QStyledItemDelegate::paint(painter, option, index);
                return;
            }

            QString status = index.data(Qt::DisplayRole).toString();
            QColor color(Qt::white);
            if (status == QStringLiteral("Запланировано")) color = QColor(200,200,200); // gray
            else if (status == QStringLiteral("В процессе")) color = QColor(100,150,255); // blue
            else if (status == QStringLiteral("Сделано")) color = QColor(180,255,180); // green
            else if (status == QStringLiteral("Отменено")) color = QColor(255,180,180); // red
            else if (status == QStringLiteral("Отложено")) color = QColor(255,200,120); // orange

            painter->save();
            painter->fillRect(option.rect, color);
            painter->setFont(option.font);
            QFontMetrics fm(option.font);
            QString elided = fm.elidedText(status, Qt::ElideRight, option.rect.width() - 8);
            painter->setPen(Qt::black);
            painter->drawText(option.rect.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, elided);
            painter->restore();
        }
    };
    // view model for paginated display
    m_viewModel = new QSqlQueryModel(this);
    // Initialize page size from the combo box so the initial view uses the selected value (default "10").
    m_pageSize = m_pageSizeCombo->currentText().toInt();
    m_currentPage = 0;

    // connect pagination UI
    connect(m_prevPageButton, &QPushButton::clicked, this, [this]() {
        if (m_currentPage > 0) { --m_currentPage; refreshView(); }
    });
    connect(m_nextPageButton, &QPushButton::clicked, this, [this]() {
        ++m_currentPage; refreshView();
    });
    connect(m_pageSizeCombo, &QComboBox::currentTextChanged, this, [this](const QString &text){
        m_pageSize = text.toInt();
        m_currentPage = 0;
        refreshView();
    });

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
    // The table will show the paginated view model
    tableView->setModel(m_viewModel);
    // Делегат нужен, чтобы таблица знала, как отрисовывать реляционные данные (выпадающие списки при редактировании)
    // For editing operations we still keep a relational model (m_model) but visual display uses m_viewModel.
    tableView->setItemDelegate(new QSqlRelationalDelegate(tableView));
    // Create and store the custom delegate for coloring status cells so it can be
    // reapplied after model replacements in refreshView().
    m_statusDelegate = new StatusColorDelegate(tableView);
    tableView->setItemDelegateForColumn(MainWindow::COL_STATUS, m_statusDelegate);

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
    // Enable clickable sorting via header
    tableView->setSortingEnabled(true);
    m_sortColumn = -1;
    m_sortOrder = Qt::AscendingOrder;
    connect(header, &QHeaderView::sectionClicked, this, &MainWindow::onHeaderClicked);

    // --- Настройка контекстного меню (ПКМ) ---
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableView, &QTableView::customContextMenuRequested, this, &MainWindow::onCustomContextMenu);
    connect(tableView, &QTableView::doubleClicked, this, &MainWindow::onTableDoubleClicked);

    // --- Панель инструментов (ToolBar) ---
    m_addTaskAction = new QAction(tr("&Добавить"), this);
    m_addTaskAction->setStatusTip(tr("Добавить новую задачу"));
    m_addTaskAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(m_addTaskAction, &QAction::triggered, this, &MainWindow::onAddTask);

    m_editTaskAction = new QAction(tr("&Редактировать"), this);
    m_editTaskAction->setStatusTip(tr("Редактировать выбранную задачу"));
    connect(m_editTaskAction, &QAction::triggered, this, &MainWindow::onEditTask);

    m_mainToolBar = new QToolBar(tr("Инструменты"), this);
    // Пока панель инструментов пустая — скрываем её, чтобы не выглядела несуразно.
    m_mainToolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_mainToolBar->setMovable(false); // Закрепляем панель
    m_mainToolBar->hide();

    statusBar()->showMessage(tr("Готов к работе"));
    // initial fill
    refreshView();
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

        // 3. Заполняем/дополняем справочник начальными значениями.
        // Если таблица пустая — вставляем полный набор. Если непустая — добавляем недостающие значения.
        QStringList requiredStatuses = {"Запланировано", "В процессе", "Сделано", "Отложено", "Отменено"};
        if (query.exec("SELECT COUNT(*) FROM STATUS") && query.next() && query.value(0).toInt() == 0)
        {
            qDebug() << "Populating 'STATUS' table with default values...";
            for (const QString &s : requiredStatuses) {
                QSqlQuery ins(m_db);
                ins.prepare("INSERT INTO STATUS (name) VALUES (:name);");
                ins.bindValue(":name", s);
                if (!ins.exec())
                    qWarning() << "Failed to insert status" << s << ins.lastError().text();
            }
        } else {
            // Ensure specific statuses exist (add missing ones)
            for (const QString &s : requiredStatuses) {
                QSqlQuery chk(m_db);
                chk.prepare("SELECT COUNT(*) FROM STATUS WHERE name = :name");
                chk.bindValue(":name", s);
                if (chk.exec() && chk.next()) {
                    if (chk.value(0).toInt() == 0) {
                        QSqlQuery ins(m_db);
                        ins.prepare("INSERT INTO STATUS (name) VALUES (:name);");
                        ins.bindValue(":name", s);
                        if (!ins.exec())
                            qWarning() << "Failed to insert missing status" << s << ins.lastError().text();
                    }
                }
            }
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

        // 1. Получаем ID статуса по выбранному имени; если выбранный статус не найден,
        // используем 'Запланировано' как рекомендованный по умолчанию, иначе fallback = 1.
        QSqlQuery statusQuery(m_db);
        statusQuery.prepare("SELECT id FROM STATUS WHERE name = :name");
        statusQuery.bindValue(":name", statusName);
        int statusId = 0;
        if (statusQuery.exec() && statusQuery.next()) {
            statusId = statusQuery.value(0).toInt();
        } else {
            QString defaultStatusName = QStringLiteral("Запланировано");
            QSqlQuery getDefault(m_db);
            getDefault.prepare("SELECT id FROM STATUS WHERE name = :name");
            getDefault.bindValue(":name", defaultStatusName);
            if (getDefault.exec() && getDefault.next()) {
                statusId = getDefault.value(0).toInt();
            } else {
                statusId = 1; // ultimate fallback
            }
        }

        // 2. Вставляем данные через прямой SQL-запрос (вместо insertRecord)
        QSqlQuery insertQuery(m_db);
        insertQuery.prepare("INSERT INTO TASK (description, details, creation_dt, completion_dt, status_id, is_deleted) "
                    "VALUES (:desc, :details, :created, :completed, :status_id, 0)");

        insertQuery.bindValue(":desc", description);
        insertQuery.bindValue(":details", details);
        insertQuery.bindValue(":created", creationTime);

        // Если задача создаётся сразу со статусом "Сделано", ставим completion_dt = creationTime
        QVariant completionDt;
        if (m_statusDoneId != -1 && statusId == m_statusDoneId) {
            completionDt = creationTime;
        }
        insertQuery.bindValue(":completed", completionDt);
        insertQuery.bindValue(":status_id", statusId);

        if (insertQuery.exec())
        {
            // 3. Обновляем представление
            m_model->select();
            refreshView();
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

    // Удобство: при ПКМ автоматически выделяем строку под курсором,
    // чтобы действия (редактировать/удалить) применялись к этой строке.
    tableView->selectRow(index.row());

    QMenu contextMenu(this);
    QAction *actionEdit = contextMenu.addAction(tr("Редактировать"));
    QAction *actionSoftDelete = contextMenu.addAction(tr("Удалить (в корзину)"));
    QAction *actionHardDelete = contextMenu.addAction(tr("Удалить полностью"));

    // Используем connect с лямбдами или прямыми слотами
    connect(actionEdit, &QAction::triggered, this, &MainWindow::onEditTask);
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
    int taskId = m_viewModel->data(m_viewModel->index(row, 0)).toInt();

    QSqlQuery q(m_db);
    q.prepare("UPDATE TASK SET is_deleted = 1 WHERE id = :id");
    q.bindValue(":id", taskId);
    if (q.exec()) {
        refreshView();
        statusBar()->showMessage(tr("Задача перемещена в корзину"));
    } else {
        QMessageBox::warning(this, tr("Ошибка БД"), q.lastError().text());
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
                                  tr("Вы уверены, что хотите удалить эту задачу? Это действие нельзя будет отменить."),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        int row = selectedRows.first().row();
        int taskId = m_viewModel->data(m_viewModel->index(row, 0)).toInt();
        QSqlQuery q(m_db);
        q.prepare("DELETE FROM TASK WHERE id = :id");
        q.bindValue(":id", taskId);
        if (q.exec()) {
            refreshView();
            statusBar()->showMessage(tr("Задача удалена полностью"));
        } else {
            QMessageBox::warning(this, tr("Ошибка БД"), q.lastError().text());
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

    // 2. Извлекаем текущие данные из view model (пагинация)
    // Нам нужны: ID (0), Описание (1), Подробное описание (2) и Имя Статуса (5)
    int taskId = m_viewModel->data(m_viewModel->index(row, 0)).toInt();
    QString currentDesc = m_viewModel->data(m_viewModel->index(row, 1)).toString();
    QString currentDetails = m_viewModel->data(m_viewModel->index(row, 2)).toString();
    QString currentStatus = m_viewModel->data(m_viewModel->index(row, 5)).toString();

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
            m_model->select(); // Keep relational model in sync
            refreshView();
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

    int taskId = m_viewModel->data(m_viewModel->index(row, 0)).toInt();
    QString currentDesc = m_viewModel->data(m_viewModel->index(row, 1)).toString();
    QString currentDetails = m_viewModel->data(m_viewModel->index(row, 2)).toString();
    QString currentStatus = m_viewModel->data(m_viewModel->index(row, 5)).toString();

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
            refreshView();
            statusBar()->showMessage(tr("Задача обновлена"));
        } else {
            qCritical() << "Failed to update task (double-click):" << updateQuery.lastError().text();
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось сохранить изменения."));
        }
    }
}

void MainWindow::onHeaderClicked(int section)
{
    if (section < 0)
        return;

    if (m_sortColumn == section) {
        // toggle order
        m_sortOrder = (m_sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        m_sortColumn = section;
        m_sortOrder = Qt::AscendingOrder;
    }

    // Apply sort to the view by re-querying with new ORDER BY
    refreshView();

    // Update visual indicator
    QHeaderView *header = tableView->horizontalHeader();
    header->setSortIndicator(m_sortColumn, m_sortOrder);
    header->setSortIndicatorShown(true);
}

void MainWindow::refreshView()
{
    // compute total rows
    QSqlQuery countQ(m_db);
    int total = 0;
    if (countQ.exec("SELECT COUNT(*) FROM TASK WHERE is_deleted = 0")) {
        if (countQ.next()) total = countQ.value(0).toInt();
    }

    int offset = m_currentPage * m_pageSize;
    if (offset < 0) offset = 0;

    // Map sort column to DB column name
    QString orderBy = "creation_dt DESC";
    if (m_sortColumn >= 0) {
        switch (m_sortColumn) {
            case MainWindow::COL_DESC: orderBy = "description"; break;
            case MainWindow::COL_DETAILS: orderBy = "details"; break;
            case MainWindow::COL_CREATION_DT: orderBy = "creation_dt"; break;
            case MainWindow::COL_COMPLETION_DT: orderBy = "completion_dt"; break;
            case MainWindow::COL_STATUS: orderBy = "STATUS.name"; break;
            default: orderBy = "creation_dt"; break;
        }
        orderBy += (m_sortOrder == Qt::DescendingOrder) ? " DESC" : " ASC";
    }

    QString sql = QString("SELECT TASK.id, TASK.description, TASK.details, TASK.creation_dt, TASK.completion_dt, STATUS.name as status, TASK.is_deleted "
                          "FROM TASK LEFT JOIN STATUS ON TASK.status_id = STATUS.id "
                          "WHERE TASK.is_deleted = 0 "
                          "ORDER BY %1 "
                          "LIMIT %2 OFFSET %3;").arg(orderBy).arg(m_pageSize).arg(offset);

    m_viewModel->setQuery(sql, m_db);
    // set headers
    m_viewModel->setHeaderData(1, Qt::Horizontal, tr("Задание"));
    m_viewModel->setHeaderData(2, Qt::Horizontal, tr("Описание"));
    m_viewModel->setHeaderData(3, Qt::Horizontal, tr("Дата создания"));
    m_viewModel->setHeaderData(4, Qt::Horizontal, tr("Дата выполнения"));
    m_viewModel->setHeaderData(5, Qt::Horizontal, tr("Статус"));

    tableView->setModel(m_viewModel);
    // Ensure technical columns remain hidden when the view model is reset
    tableView->hideColumn(MainWindow::COL_ID);
    tableView->hideColumn(MainWindow::COL_IS_DELETED);
    // Reapply our stored status-color delegate to the status column
    tableView->setItemDelegateForColumn(MainWindow::COL_STATUS, m_statusDelegate);
    QHeaderView *header = tableView->horizontalHeader();
    header->setSectionResizeMode(MainWindow::COL_DESC, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(MainWindow::COL_DETAILS, QHeaderView::Stretch);
    header->setSectionResizeMode(MainWindow::COL_CREATION_DT, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(MainWindow::COL_COMPLETION_DT, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(MainWindow::COL_STATUS, QHeaderView::ResizeToContents);

    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    if (m_currentPage >= totalPages) m_currentPage = totalPages - 1;
    m_pageInfoLabel->setText(tr("Стр. %1 / %2 (%3)").arg(m_currentPage+1).arg(totalPages).arg(total));
    m_prevPageButton->setEnabled(m_currentPage > 0);
    m_nextPageButton->setEnabled((m_currentPage+1) < totalPages);
}