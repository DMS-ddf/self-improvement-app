#include <QApplication>
#include <QMainWindow>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow mainWindow;
    mainWindow.resize(800, 600);
    mainWindow.setWindowTitle("Self Improvement Tracker v1.0");

    QLabel *centralLabel = new QLabel("Здесь будет наше приложение!");
    centralLabel->setAlignment(Qt::AlignCenter);
    mainWindow.setCentralWidget(centralLabel);

    mainWindow.show();

    return app.exec();
}
