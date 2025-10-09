#ifndef CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H
#define CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H

#include <QObject>
#include <QThread>

#include "nlohmann/json.hpp"
#include <xeus/xinterpreter.hpp>

#include <sys/types.h>

namespace nl = nlohmann;

namespace cnoid {

class PythonProcess : public QObject
{
    Q_OBJECT;
public:
    PythonProcess(const std::string &connection_file)
    {
        initialize(connection_file);
    };

    bool initialize(const std::string &connection_file);
    bool finalize();
    void shutdown_impl();
    //
    void proc();
    bool blocking_poll();

public Q_SLOTS:
    void procRequest();

private:
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

};
#endif
