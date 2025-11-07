#include "AddTaskDialog.h"

#include <QLineEdit>
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

    // Создаем поля для ввода данных
    m_descriptionEdit = new QLineEdit(this);
    m_statusComboBox = new QComboBox(this);

    // Загружаем список доступных статусов из БД в выпадающий список
    QSqlQuery query("SELECT name FROM STATUS");
    while (query.next())
    {
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

void AddTaskDialog::setTaskData(const QString &description, const QString &status)
{
    // Устанавливаем текст в поле ввода
    m_descriptionEdit->setText(description);

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
    if (m_descriptionEdit->text().trimmed().isEmpty()) 
    {
        // 1. Если поле пустое — показываем ошибку
        QMessageBox::warning(this, tr("Ошибка валидации"),
                             tr("Поле 'Задание' не может быть пустым."));
        
        // 2. И ГЛАВНОЕ: НЕ вызываем QDialog::accept(),
        //    чтобы диалог НЕ закрылся.
    } 
    else 
    {
        // 3. Если всё в порядке — вызываем базовый метод,
        //    который закроет диалог и вернет QDialog::Accepted.
        QDialog::accept();
    }
}