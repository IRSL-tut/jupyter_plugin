#include <QApplication>
#include <QSurfaceFormat>

#include <QWidget>
#include <QOpenGLWidget>

#include <cnoid/GLSceneRenderer>
#include <cnoid/GLSLSceneRenderer>
#include <cnoid/Format>

#include "PythonProcess.h"

#include <iostream>
#include <string>

class MySceneWidget : public QOpenGLWidget
{
public:
    cnoid::GLSceneRenderer* renderer;
    cnoid::GLSLSceneRenderer* glslRenderer;

    cnoid::SgGroupPtr root;
    cnoid::SgGroupPtr scene;

    GLuint prevDefaultFramebufferObject;
    bool isRendering;
    bool needToClearGLOnFrameBufferChange;
public:
    MySceneWidget(QWidget *q) : QOpenGLWidget(q) {
        root = cnoid::SgGroupPtr(new cnoid::SgGroup());
        std::cerr << "mw 0" << std::endl;
        renderer = cnoid::GLSceneRenderer::create(root);
        glslRenderer = dynamic_cast<cnoid::GLSLSceneRenderer*>(renderer);
        if(!glslRenderer){
            exit(1);
        }
        ////
        std::cerr << "mw 1" << std::endl;
        renderer->setOutputStream(std::cerr);
        renderer->enableUnusedResourceCheck(true);

        renderer->setBackgroundColor(cnoid::Vector3f(0., 0., 1.));
        scene = renderer->scene();

        renderer->setCurrentCamera(1);
    }
    virtual void initializeGL() override;
    virtual void resizeGL(int width, int height) override;
    virtual void paintGL() override;
};

void MySceneWidget::initializeGL()
{
    std::cerr << "initializeGL 0" << std::endl;
    renderer->setDefaultFramebufferObject(defaultFramebufferObject());
    if(renderer->initializeGL()){
        std::cerr << "initializeGL 1" << std::endl;
        if(glslRenderer){
            auto& vendor = glslRenderer->glVendor();
            if(vendor.find("NVIDIA Corporation") != std::string::npos){
                needToClearGLOnFrameBufferChange = true;
            }
        }
    } else {
        std::cerr << "OpenGL initialization failed." << std::endl;
        // This view shoulbe be disabled when the glew initialization is failed.
    }
}

void MySceneWidget::resizeGL(int width, int height)
{
    std::cerr << "num cameras " << renderer->numCameras() << std::endl;
    std::cerr << "resizeGL 0" << std::endl;
    renderer->updateViewportInformation(0, 0, width, height);
}

void MySceneWidget::paintGL()
{
    std::cerr << "paintGL 0" << std::endl;
    auto newFramebuffer = defaultFramebufferObject();
    if(newFramebuffer != prevDefaultFramebufferObject){
        /**
           For NVIDIA GPUs, GLSLSceneRenderer may not be able to render properly
           when the placement or some other configurations of QOpenGLWidget used
           with the renderer change. To avoid the problem, the OpenGL resources
           used in the renderer should be cleared when the changes occur, and the
           resources should be recreated in the new configurations. This is done
           by the following code. The configuration changes can be detected by
           checking the ID of the default frame buffer object.
           
           \todo The view layout change in loading a project should be done before
           loading any items to avoid unnecessary re-initializations of the OpenGL
           resources to reduce the overhead.
        */
        if(needToClearGLOnFrameBufferChange && prevDefaultFramebufferObject > 0) {
            std::cerr << "paintGL clear" << std::endl;
            renderer->clearGL();
            std::cerr << "The OpenGL resources of {0} has been cleared" << std::endl;
        }
        // The default FBO must be updated after the clearGL function
        renderer->setDefaultFramebufferObject(newFramebuffer);

        prevDefaultFramebufferObject = newFramebuffer;
    }
#if 0
    if(needToUpdateViewportInformation){
        renderer->updateViewportInformation();
        needToUpdateViewportInformation = false;
    }
    bool isLightweightViewChangeActive = false;
    if(isLightweightViewChangeEnabled){
        isLightweightViewChangeActive = isCameraPositionInteractivelyChanged;
    }
    isCameraPositionInteractivelyChanged = false;
    renderer->setBoundingBoxRenderingForLightweightRenderingGroupEnabled(
        isLightweightViewChangeActive);
    if(isLightweightViewChangeActive){
        timerToRenderNormallyAfterInteractiveCameraPositionChange.start(
            isDraggingView() ? 0 : 400);
    } else if(timerToRenderNormallyAfterInteractiveCameraPositionChange.isActive()){
        timerToRenderNormallyAfterInteractiveCameraPositionChange.stop();
    }
#endif
    std::cerr << "paintGL render" << std::endl;
    isRendering = true;
    renderer->render();
    isRendering = false;
#if 0
    if(fpsTimer.isActive()){
        renderFps();
    }
#endif
}

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

    cnoid::PythonProcess p("");

    std::cerr << "widget" << std::endl;
    //QWidget ww;
    MySceneWidget ww(nullptr);
    ww.resize( QSize(800, 600) );

    std::cerr << "show" << std::endl;
    ww.show();

    std::cerr << "exec" << std::endl;
    return a.exec();
}
