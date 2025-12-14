#include "AddTaskDialog.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>

AddTaskDialog::AddTaskDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Добавить новую задачу"));
    m_descriptionEdit = new QLineEdit(this);
    m_detailsEdit = new QTextEdit(this);
    m_statusComboBox = new QComboBox(this);

    // Show a light placeholder to indicate default name when left empty
    m_descriptionEdit->setPlaceholderText(tr("Новая задача"));

    // Загрузить статусы из БД
    QSqlQuery query;
    if (query.exec("SELECT name FROM STATUS")) {
        while (query.next())
            m_statusComboBox->addItem(query.value(0).toString());
    } else {
        qWarning() << "Failed to query STATUS list:" << query.lastError().text();
    }

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(tr("Задание:"), m_descriptionEdit);
    // Делаем поле описания более высоким для удобного ввода
    m_detailsEdit->setMinimumHeight(140);
    formLayout->addRow(tr("Описание:"), m_detailsEdit);
    formLayout->addRow(tr("Статус:"), m_statusComboBox);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    // Немного увеличим диалог по умолчанию, чтобы поля были комфортнее
    setMinimumWidth(560);
    setMinimumHeight(320);

    // По умолчанию выбираем статус "Запланировано", если он есть
    int idx = m_statusComboBox->findText(QStringLiteral("Запланировано"));
    if (idx != -1)
        m_statusComboBox->setCurrentIndex(idx);
}

QString AddTaskDialog::getTaskDescription() const
{
    return m_descriptionEdit->text();
}

QString AddTaskDialog::getTaskDetails() const
{
    return m_detailsEdit->toPlainText();
}

QString AddTaskDialog::getSelectedStatus() const
{
    return m_statusComboBox->currentText();
}

void AddTaskDialog::setTaskData(const QString &description, const QString &details, const QString &status)
{
    // Устанавливаем текст в поле ввода
    m_descriptionEdit->setText(description);
    m_detailsEdit->setPlainText(details);

    // Находим и устанавливаем нужный статус в выпадающем списке
    int index = m_statusComboBox->findText(status);
    if (index != -1)
    { // -1 означает, что текст не найден
        m_statusComboBox->setCurrentIndex(index);
    }
}

void AddTaskDialog::accept()
{
    // If the description is empty, fall back to a sensible default rather than blocking the user.
    QString desc = m_descriptionEdit->text().trimmed();
    if (desc.isEmpty()) {
        desc = tr("Новая задача");
        m_descriptionEdit->setText(desc);
    }
    QDialog::accept();
}