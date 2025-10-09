#include <QApplication>
#include <QMainWindow>

#include "PythonProcess.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    QMainWindow window;
    //OpenGLWidget *glWidget = new OpenGLWidget(&window);
    //window.setCentralWidget(glWidget);
    window.resize(800, 600);
    window.show();

    cnoid::PythonProcess p("");

    return a.exec();
}
