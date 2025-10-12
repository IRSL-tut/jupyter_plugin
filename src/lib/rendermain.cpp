//

#include <QApplication>
#include <QSurfaceFormat>

#if defined(USE_MYSCENE) && USE_MYSCENE
#include "myscene.h"
#else
#include <cnoid/GLSceneRenderer>
#include <cnoid/SceneWidget>
#include <cnoid/SceneWidgetConfig>
#endif

#include "PythonProcess.h"

#include <iostream>
#include <string>

#if defined(USE_MYSCENE) && USE_MYSCENE
#else
class MyConfig : public cnoid::SceneWidgetConfig
{
public:
    MyConfig() : cnoid::SceneWidgetConfig() { };
};
#endif

int main(int argc, char **argv)
{
    cnoid::GLSceneRenderer::initializeClass();

    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    //fmt.setAlphaBufferSize(8);//
    //fmt.setDepthBufferSize(24);//
    //fmt.setStencilBufferSize(8);//
    //fmt.setRenderableType(QSurfaceFormat::OpenGL);//
    fmt.setSwapInterval(false);
    QSurfaceFormat::setDefaultFormat(fmt);

    std::cerr << "app" << std::endl;
    QApplication a(argc, argv);

    std::cerr << "widget" << std::endl;
#if defined(USE_MYSCENE) && USE_MYSCENE
    MySceneWidget *ww = MySceneWidget::create(nullptr);
#else
    cnoid::SceneWidget *ww = new cnoid::SceneWidget(nullptr);
    MyConfig cfg;
    cfg.addSceneWidget(ww, true);
#endif
    ww->resize( QSize(800, 600) );

    std::cerr << "show" << std::endl;
    ww->show();

    cnoid::PythonProcess p("");

    std::cerr << "exec" << std::endl;
    return a.exec();
}
