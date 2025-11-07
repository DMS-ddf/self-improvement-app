#ifndef ADDTASKDIALOG_H
#define ADDTASKDIALOG_H

#include <QDialog>

// Forward declarations: объявляем классы заранее, чтобы не подключать
// тяжелые заголовочные файлы здесь. Это ускоряет компиляцию.
class QLineEdit;
class QComboBox;

class AddTaskDialog : public QDialog
{
Q_OBJECT // Макрос, необходимый для работы сигналов и слотов Qt

    public :
    // explicit запрещает неявные преобразования типов при создании диалога
    explicit AddTaskDialog(QWidget *parent = nullptr);

    // Геттеры для получения введенных пользователем данных
    QString getTaskDescription() const;
    QString getSelectedStatus() const;

    // Заполняет поля диалога данными существующей задачи
    void setTaskData(const QString &description, const QString &status);

public slots:
    // Переопределяем стандартный слот, который вызывается при нажатии "ОК"
    virtual void accept() override;

private:
    // Указатели на виджеты, которые мы создадим в .cpp файле.
    // Храним их здесь, чтобы иметь к ним доступ из методов класса.
    QLineEdit *m_descriptionEdit;
    QComboBox *m_statusComboBox;
};

#endif // ADDTASKDIALOG_H