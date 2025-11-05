#include "AddTaskDialog.h"

#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

AddTaskDialog::AddTaskDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Добавить новую задачу"));

    // Создаем поля для ввода данных
    m_descriptionEdit = new QLineEdit(this);
    m_statusComboBox = new QComboBox(this);

    // Загружаем список доступных статусов из БД в выпадающий список
    QSqlQuery query("SELECT name FROM STATUS");
    while (query.next()) {
        m_statusComboBox->addItem(query.value(0).toString());
    }

    // Используем форму для аккуратного размещения полей с подписями
    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(tr("Задание:"), m_descriptionEdit);
    formLayout->addRow(tr("Статус:"), m_statusComboBox);

    // Стандартные кнопки диалога (ОК / Отмена)
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    // Связываем нажатия кнопок с закрытием диалога (принять/отклонить)
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Основная компоновка: форма сверху, кнопки снизу
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

QString AddTaskDialog::getTaskDescription() const
{
    return m_descriptionEdit->text();
}

QString AddTaskDialog::getSelectedStatus() const
{
    return m_statusComboBox->currentText();
}