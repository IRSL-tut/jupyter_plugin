#ifndef CNOID_JUPYTER_PLUGIN_BAR_H
#define CNOID_JUPYTER_PLUGIN_BAR_H

#include <cnoid/ToolBar>

#include "exportdecl.h"

namespace cnoid {

class CNOID_EXPORT JupyterBar : public ToolBar
{
public:
    static JupyterBar* instance();
    virtual ~JupyterBar();

    bool addUserButton(const std::string &name);
    bool addUserToggleButton(const std::string &name);
    long getUserButton(const std::string &name);
    bool getUserButtonState(const std::string &name);
private:
    JupyterBar();

    class Impl;
    Impl* impl;
};

}
#endif
