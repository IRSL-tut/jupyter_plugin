//
#include <QWidget>
#include <QOpenGLWidget>

#include <cnoid/GLSceneRenderer>
#include <cnoid/GLSLSceneRenderer>
#include <cnoid/Format>

#include <iostream>

class MySceneWidget : public QOpenGLWidget
{
public:
    static MySceneWidget *instance();
    static MySceneWidget *create(QWidget *obj);

    cnoid::GLSceneRenderer* renderer;
    cnoid::GLSLSceneRenderer* glslRenderer;

    cnoid::SgGroupPtr root;
    cnoid::SgGroupPtr scene;

    GLuint prevDefaultFramebufferObject;
    bool isRendering;
    bool needToClearGLOnFrameBufferChange;

public:
    virtual void initializeGL() override;
    virtual void resizeGL(int width, int height) override;
    virtual void paintGL() override;

private:
    MySceneWidget(QWidget *q);
};
