#include "MainWindow.h"

#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QStatusBar>
#include <QApplication>
#include <QTableView>
#include <QDebug> 

// Реализация конструктора
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(800, 600);
    setWindowTitle("Self Improvement Tracker v1.0");

    tableView = new QTableView(this); 
    setCentralWidget(tableView);
    
    QAction *exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+Q"));
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(exitAction);

    statusBar()->showMessage(tr("Ready"));

    initDB();
}

// Реализация деструктора
MainWindow::~MainWindow()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

// Реализация метода инициализации Базы Данных
void MainWindow::initDB()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");

    m_db.setDatabaseName("tracker.db");

    if (!m_db.open()) {
        qDebug() << "Error: connection with database fail";
        statusBar()->showMessage(tr("Database connection failed!"));
    } else {
        qDebug() << "Database: connection ok";
        statusBar()->showMessage(tr("Database connection successful!"));
    }
}
