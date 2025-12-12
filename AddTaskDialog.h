#ifndef ADDTASKDIALOG_H
#define ADDTASKDIALOG_H

#include <QDialog>
#include <QObject>

// Forward declarations: чтобы ускорить компиляцию
class QLineEdit;
class QComboBox;
class QTextEdit;

class AddTaskDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddTaskDialog(QWidget *parent = nullptr);
    QString getTaskDescription() const;
    QString getTaskDetails() const;
    QString getSelectedStatus() const;
    void setTaskData(const QString &description, const QString &details, const QString &status);

public slots:
    void accept() override;

private:
    QLineEdit *m_descriptionEdit;
    QTextEdit *m_detailsEdit;
    QComboBox *m_statusComboBox;
};

#endif // ADDTASKDIALOG_H