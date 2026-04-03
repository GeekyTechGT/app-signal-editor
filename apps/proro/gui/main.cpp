#include <QApplication>
#include <QMainWindow>
#include <QLabel>

// TODO: replace with actual main window
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("MyProject — proro");
    window.setCentralWidget(new QLabel("TODO: implement main window"));
    window.resize(800, 600);
    window.show();

    return app.exec();
}
