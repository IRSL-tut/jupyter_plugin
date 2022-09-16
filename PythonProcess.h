#ifndef CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H
#define CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H

#include <cnoid/PythonPlugin>
#include <cnoid/OptionManager> //option
#include <sstream>

namespace cnoid {

class JupyterPlugin;

class PythonProcess
{
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

    void onSigOptionsParsed(boost::program_options::variables_map& variables);
    bool initialize();
    bool finalize();
    void putCommand(const std::string &_com);

    std::ostringstream out_strm;
    std::ostringstream err_strm;
    void interpreterThread();
};
}

#endif
