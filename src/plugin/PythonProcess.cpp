#include "PythonProcess.h"
#include "JupyterInterpreter.h"
// xeus
#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <xeus/xserver_zmq.hpp>
// cnoid
#include <cnoid/UTF8>
#include <cnoid/stdx/filesystem>
// thread
#include <thread>
// split
#include <boost/algorithm/string.hpp>
// command for choreonoid
#include <QBuffer>
#include <cnoid/SceneView>
#include <cnoid/SceneWidget>

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
        DEBUG_STREAM(" jupyter-connection:" << connection_file);

        std::thread th_interp(&PythonProcess::interpreterThread, this);
        th_interp.detach();
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

    mainModule = PythonPlugin::instance()->mainModule();
    globalNamespace = PythonPlugin::instance()->globalNamespace();
    interpreter = python::module::import("code").attr("InteractiveConsole")(globalNamespace);

    pybind11::object consoleOutClass =
        pybind11::class_<PythonConsoleOut>(mainModule, "PythonConsoleOut2")
        .def(pybind11::init<>())
        .def("write", &PythonConsoleOut::write);
    consoleOut = consoleOutClass();
    PythonConsoleOut& consoleOut_ = consoleOut.cast<PythonConsoleOut&>();
    consoleOut_.setConsole(this);

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

    sys = PythonPlugin::instance()->sysModule();
    python::object inspector_class = python::module::import("IPython").attr("core").attr("oinspect").attr("Inspector");
    inspector = inspector_class();
    //
    token_at_cursor = python::module::import("IPython.utils.tokenutil").attr("token_at_cursor");
    //token_at_cursor = python::module::import("IPython").attr("utils").attr("tokenutil").attr("token_at_cursor");
    jedi_Interpreter = python::module::import("jedi").attr("Interpreter");
#if 0
    python::object keyword = python::module::import("keyword");
    pybind11::list kwlist = pybind11::cast<pybind11::list>(keyword.attr("kwlist"));
    for(size_t i = 0; i < pybind11::len(kwlist); ++i){
        keywords.push_back(pybind11::cast<string>(kwlist[i]));
    }
#endif

    connect(this, &PythonProcess::sendComRequest,
            this, &PythonProcess::procComRequest,
            Qt::BlockingQueuedConnection);  //Qt::DirectConnection);

    return true;
}
bool PythonProcess::finalize()
{
    DEBUG_PRINT();
    return true;
}
bool PythonProcess::putCommand(const std::string &_com)
{
    bool ret;
    python::gil_scoped_acquire lock;
    orgStdout = sys.attr("stdout");
    orgStderr = sys.attr("stderr");
    orgStdin  = sys.attr("stdin");
    out_strm.str("");
    out_strm.clear();
    err_strm.str("");
    err_strm.clear();
    sys.attr("stdout") = consoleOut;
    sys.attr("stderr") = consoleErr;
    sys.attr("stdin")  = consoleIn;

    DEBUG_STREAM(" push: " << _com);
    if(interpreter.attr("push")(_com).cast<bool>()) {
        // Enter scope
        DEBUG_STREAM("...");
        ret = false; // not complete

    } else  {
        // Finish command
        DEBUG_STREAM(">>>");
        ret = true; // complete
    }
    if(PyErr_Occurred()){
        PyErr_Print();
        PyErr_Clear();
    }
    sys.attr("stdout") = orgStdout;
    sys.attr("stderr") = orgStderr;
    sys.attr("stdin")  = orgStdin;

    return ret;
}
void PythonProcess::interpreterThread()
{
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
void PythonProcess::procComRequest(const QString &com)
{
    // [TODO] commander
    // choreonoidProcessCommandr(this, com);
    // void choreonoidProcessCommandr(PythonProcess *, const Qstring &);
    if(com == "display") {
        //SceneView *sv = SceneView::instance();
        //std::vector<SceneView*> lst = SceneView::instances();
        SceneWidget *sw = SceneView::instance()->sceneWidget();
        QImage im = sw->getImage();
        // debug
        //im.save("/tmp/hoge.png");
        data.clear();
        QBuffer buf(&data);
        buf.open(QIODevice::WriteOnly);
        im.save(&buf, "png");
        buf.close();
    }
}
#if 0
static bool getMemberNames(python::object& moduleObject, std::vector<std::string> &retNames)
{
    PyObject* pPyObject = moduleObject.ptr();
    if(pPyObject == NULL) {
        return false;
    }
    pybind11::handle h( PyObject_Dir(pPyObject) );
    pybind11::list memberNames = h.cast<pybind11::list>();
    for(size_t i=0; i < python::len(memberNames); ++i ) {
        if(!strstr(std::string(memberNames[i].cast<std::string>()).c_str(), "__" )) {
            retNames.push_back(std::string(memberNames[i].cast<std::string>()));
        }
    }
    return true;
}
static python::object getMemberObjectOld(std::vector<std::string>& moduleNames, python::object& parentObject)
{
    if(moduleNames.size() == 0) {
        return parentObject;
    } else {
        std::string moduleName = moduleNames.front();
        moduleNames.erase(moduleNames.begin());
        std::vector<std::string> memberNames;
        getMemberNames(parentObject, memberNames);

        if(std::find(memberNames.begin(), memberNames.end(), moduleName) == memberNames.end()) {
            //DEBUG_STREAM(" failed : " << moduleName);
            return python::object();
        } else {
            python::object childObject = parentObject.attr(moduleName.c_str());
            //DEBUG_STREAM(" suc : " << moduleName << " / " << childObject.is_none());
            return getMemberObjectOld(moduleNames, childObject);
        }
    }
}
#endif
inline static python::object getNamedMemberObject(python::object& _moduleObject, std::string &_targetName)
{
    PyObject* pPyObject = _moduleObject.ptr();
    if(pPyObject == NULL) {
        return python::object();
    }
    pybind11::handle h( PyObject_Dir(pPyObject) );
    pybind11::list memberNames = h.cast<pybind11::list>();
    for (size_t i=0; i < python::len(memberNames); ++i ) {
        std::string nm_ = memberNames[i].cast<std::string>();
        if(_targetName == nm_) {
            DEBUG_STREAM(" find : " << _targetName << " / " << nm_);
            return  _moduleObject.attr(_targetName.c_str());
        }
    }
    return python::object();
}
inline static python::object getMemberObject(std::vector<std::string>& moduleNames, python::object& parentObject)
{
    if(moduleNames.size() == 0) {
        return parentObject;
    } else {
        std::string moduleName = moduleNames.front();
        moduleNames.erase(moduleNames.begin());
        python::object obj_ = getNamedMemberObject(parentObject, moduleName);
        if(obj_.ptr() != NULL) {
            return getMemberObject(moduleNames, obj_);
        }
        return obj_;
    }
}
python::object PythonProcess::findObject(const std::string &obj_name)
{
    std::vector<std::string> dottedStrings;
    boost::split(dottedStrings, obj_name, boost::is_any_of("."));
    python::object res = getMemberObject(dottedStrings, mainModule);
    if(res.ptr() == NULL) {
        DEBUG_STREAM(" " << obj_name << " : NULL");
        dottedStrings.resize(0);
        boost::split(dottedStrings, obj_name, boost::is_any_of("."));
        python::object mod_ = mainModule.attr("__builtins__");
        res = getMemberObject(dottedStrings, mod_);
    }
    return res;
}
