#include "PythonProcess.h"

#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <xeus/xserver_zmq.hpp>
#include "JupyterInterpreter.h"

#include <cnoid/UTF8>
#include <cnoid/stdx/filesystem>

//#define IRSL_DEBUG
#include "irsl_debug.h"

using namespace cnoid;

namespace po = boost::program_options;
namespace filesystem = cnoid::stdx::filesystem;

namespace {
class PythonConsoleOut
{
public:
    PythonProcess* console;
    void setConsole(PythonProcess* _console);
    void write(std::string const& text);
};
class PythonConsoleErr
{
public:
    PythonProcess* console;
    void setConsole(PythonProcess* _console);
    void write(std::string const& text);
};
class PythonConsoleIn
{
public:
    PythonProcess* console;
    void setConsole(PythonProcess* _console);
    python::object readline();
};
}

namespace {
void PythonConsoleOut::setConsole(PythonProcess* _console)
{
    console = _console;
}
void PythonConsoleOut::write(std::string const& text)
{
    console->out_strm << text;
}
void PythonConsoleErr::setConsole(PythonProcess* _console)
{
    console = _console;
}
void PythonConsoleErr::write(std::string const& text)
{
    console->err_strm << text;
}
void PythonConsoleIn::setConsole(PythonProcess* _console)
{
    console = _console;
}
python::object PythonConsoleIn::readline()
{
    //return python::str(console->getInputFromConsoleIn());
    return python::str();
}
}

////
void PythonProcess::onSigOptionsParsed(po::variables_map& variables)
{
    DEBUG_PRINT();
    if(variables.count("jupyter-connection")) {
        connection_file = variables["jupyter-connection"].as<std::string>();
    }
}
bool PythonProcess::initialize()
{
    DEBUG_PRINT();
    OptionManager& om = self->optionManager();
    om.addOption("jupyter-connection", po::value<std::string>(), "connection file for jupyter");
    om.sigOptionsParsed(1).connect(
        [this](po::variables_map& _v) { onSigOptionsParsed(_v); } );

    python::gil_scoped_acquire lock;

    DEBUG_STREAM(" a0 : " << PythonPlugin::instance());
    mainModule = PythonPlugin::instance()->mainModule();
    DEBUG_STREAM(" a0-1 " << mainModule);
    globalNamespace = PythonPlugin::instance()->globalNamespace();
    DEBUG_STREAM(" a0-2 " << globalNamespace);
    interpreter = python::module::import("code").attr("InteractiveConsole")(globalNamespace);

    DEBUG_STREAM(" a1");
    pybind11::object consoleOutClass =
        pybind11::class_<PythonConsoleOut>(mainModule, "PythonConsoleOut2")
        .def(pybind11::init<>())
        .def("write", &PythonConsoleOut::write);
    DEBUG_STREAM(" a1-1");
    consoleOut = consoleOutClass();
    DEBUG_STREAM(" a1-2");
    PythonConsoleOut& consoleOut_ = consoleOut.cast<PythonConsoleOut&>();
    DEBUG_STREAM(" a1-3");
    consoleOut_.setConsole(this);

    DEBUG_STREAM(" a1-4");
    pybind11::object consoleErrClass =
        pybind11::class_<PythonConsoleErr>(mainModule, "PythonConsoleErr2")
        .def(pybind11::init<>())
        .def("write", &PythonConsoleErr::write);
    consoleErr = consoleErrClass();
    PythonConsoleErr& consoleErr_ = consoleErr.cast<PythonConsoleErr&>();
    consoleErr_.setConsole(this);

    python::object consoleInClass =
        pybind11::class_<PythonConsoleIn>(mainModule, "PythonConsoleIn2")
        .def(pybind11::init<>())
        .def("readline", &PythonConsoleIn::readline);
    consoleIn = consoleInClass();
    PythonConsoleIn& consoleIn_ = consoleIn.cast<PythonConsoleIn&>();
    consoleIn_.setConsole(this);

    DEBUG_STREAM(" a2");
    sys = PythonPlugin::instance()->sysModule();
    python::object inspector_class = python::module::import("IPython").attr("core").attr("oinspect").attr("Inspector");
    inspector = inspector_class();

#if 0
    python::object keyword = python::module::import("keyword");
    pybind11::list kwlist = pybind11::cast<pybind11::list>(keyword.attr("kwlist"));
    for(size_t i = 0; i < pybind11::len(kwlist); ++i){
        keywords.push_back(pybind11::cast<string>(kwlist[i]));
    }
#endif

    interpreterThread();

    return true;
}
bool PythonProcess::finalize()
{
    DEBUG_PRINT();
    return true;
}
void PythonProcess::putCommand(const std::string &_com)
{
    python::gil_scoped_acquire lock;
    orgStdout = sys.attr("stdout");
    orgStderr = sys.attr("stderr");
    orgStdin  = sys.attr("stdin");
    out_strm.clear();
    err_strm.clear();
    sys.attr("stdout") = consoleOut;
    sys.attr("stderr") = consoleErr;
    sys.attr("stdin")  = consoleIn;

    if(interpreter.attr("push")(_com).cast<bool>()) {
        // ...
    } else  {
        // >>>
    }
    if(PyErr_Occurred()){
        PyErr_Print();
        PyErr_Clear();
    }
    sys.attr("stdout") = orgStdout;
    sys.attr("stderr") = orgStderr;
    sys.attr("stdin")  = orgStdin;
}
void PythonProcess::interpreterThread()
{
    //
    if(connection_file.size() == 0) return;

    xeus::xconfiguration config = xeus::load_configuration(connection_file);
    auto context = xeus::make_context<zmq::context_t>();

    // Create interpreter instance
    using interpreter_ptr = std::unique_ptr<cnoid::JupyterInterpreter>;
    interpreter_ptr interpreter = interpreter_ptr(new cnoid::JupyterInterpreter());
    interpreter->impl = this;
    // Create kernel instance and start it
    xeus::xkernel kernel(config, xeus::get_user_name(), std::move(context),
                         std::move(interpreter), xeus::make_xserver_zmq);
    kernel.start();
}
