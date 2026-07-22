#include "main_window.hpp"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("RamMQTT");
    QApplication::setOrganizationName("RamAI");

    MainWindow window;
    window.show();

    return app.exec();
}