#include <QApplication>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Создаем виджет в "куче" с помощью указателя и 'new'
    QWidget *window = new QWidget();

    // Теперь мы работаем с указателем, поэтому используем '->' вместо '.'
    window->resize(400, 300);
    window->setWindowTitle("Self Improvement Tracker v1.0");
    window->show();

    // app.exec() запускает цикл, и окно будет жить, пока он работает
    return app.exec();
}
