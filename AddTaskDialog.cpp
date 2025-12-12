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
    formLayout->addRow(tr("Описание:"), m_detailsEdit);
    formLayout->addRow(tr("Статус:"), m_statusComboBox);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
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
    // .trimmed() убирает пробелы по краям и проверяет, не осталась ли строка пустой
    if (m_descriptionEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка валидации"), tr("Поле 'Задание' не может быть пустым."));
        m_descriptionEdit->setFocus();
        return;
    }
    QDialog::accept();
}