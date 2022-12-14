#ifndef CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H
#define CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H
#include <QObject>
#include <cnoid/PythonPlugin>
#include <cnoid/OptionManager> //option
#include <sstream>

namespace cnoid {

class JupyterPlugin;

class PythonProcess : public QObject
{
    Q_OBJECT;
public:
    PythonProcess(JupyterPlugin *_self) : self(_self)
    { self = _self; }
    JupyterPlugin *self;
    std::string connection_file;

    python::module mainModule;
    python::object globalNamespace;
    python::object consoleOut;
    python::object consoleErr;
    python::object consoleIn;
    python::object sys;
    python::object orgStdout;
    python::object orgStderr;
    python::object orgStdin;
    python::object interpreter;
    python::object inspector;
    python::object token_at_cursor;
    python::object jedi_Interpreter;//

    void onSigOptionsParsed(boost::program_options::variables_map& variables);
    bool initialize();
    bool finalize();
    void putCommand(const std::string &_com);
    //void inspectObject(const std::string &obj_name);
    python::object findObject(const std::string &obj_name);
    void interpreterThread();

    QByteArray data;
    std::ostringstream out_strm;
    std::ostringstream err_strm;
public Q_SLOTS:
    void procComRequest(const QString &com);
Q_SIGNALS:
    void sendComRequest(const QString &msg);
};
}

#endif
