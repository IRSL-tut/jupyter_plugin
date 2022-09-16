#ifndef CNOID_JUPYTER_PLUGIN_H
#define CNOID_JUPYTER_PLUGIN_H

#include <cnoid/Plugin>

namespace cnoid {

class PythonProcess;

class JupyterPlugin : public Plugin
{
public:
    static JupyterPlugin* instance();
    JupyterPlugin();
    ~JupyterPlugin();
    virtual bool initialize() override;
    virtual bool finalize() override;
    virtual const char* description() const override;

private:
    PythonProcess *impl;
};

}

#endif
