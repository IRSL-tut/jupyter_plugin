#ifndef CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H
#define CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H

#include <QObject>
#include <QThread>
#include <cnoid/OptionManager> //option

#include "nlohmann/json.hpp"
#include "JupyterPlugin.h"
#include <xeus/xinterpreter.hpp>

#include <sys/types.h>

namespace nl = nlohmann;

namespace cnoid {

class PythonProcess : public QObject
{
    Q_OBJECT;
public:
    PythonProcess(JupyterPlugin *_self) : self(_self)
    {
        self = _self;
    }
    std::string connection_file;

#ifdef USE_OLD_OPTION
    void onSigOptionsParsed(boost::program_options::variables_map& variables);
#else
    void onSigOptionsParsed(OptionManager *_om);
#endif

    bool initialize();
    bool finalize();
    void shutdown_impl();
    //
    void proc();
    bool blocking_poll();

public Q_SLOTS:
    void procRequest();

private:
    JupyterPlugin *self;
    bool setupPython();

private:
    class Impl;
    Impl *impl;
};

#ifndef USE_BLOCKING
class Runner : public QThread
{
    Q_OBJECT;
public:
    Runner(PythonProcess *pp);
    void run() override;

Q_SIGNALS:
    void sendRequest();

private:
    class Impl;
    Impl *impl;
};

#endif

}
#endif
