#include "PythonProcess.h"
#include "JupyterInterpreter.h"
// xeus
#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>
#if defined(USE_XEUS3) || defined(USE_XEUS5)
#include <xeus-zmq/xserver_zmq.hpp>
#if defined(USE_XEUS5)
#include <xeus-zmq/xzmq_context.hpp>
#endif
#else
#include <xeus/xserver_zmq.hpp>
#endif
#include <xeus/xinput.hpp>
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

//
#include "pybind11/eval.h"

//#define IRSL_DEBUG
#include "irsl_debug.h"

using namespace cnoid;

#ifdef USE_OLD_OPTION
namespace po = boost::program_options;
#endif
namespace filesystem = cnoid::stdx::filesystem;

namespace {
class PythonConsoleOut2
{
public:
    PythonProcess* console;
    void setConsole(PythonProcess* _console);
    void write(std::string const& text);
    void flush();
};
class PythonConsoleErr2
{
public:
    PythonProcess* console;
    void setConsole(PythonProcess* _console);
    void write(std::string const& text);
    void flush();
};
class PythonConsoleIn2
{
public:
    PythonProcess* console;
    void setConsole(PythonProcess* _console);
    python::object readline();
};
}

namespace {
void PythonConsoleOut2::setConsole(PythonProcess* _console)
{
    console = _console;
}
void PythonConsoleOut2::write(std::string const& text)
{
    console->out_strm << text;
}
void PythonConsoleOut2::flush() {}
void PythonConsoleErr2::setConsole(PythonProcess* _console)
{
    console = _console;
}
void PythonConsoleErr2::write(std::string const& text)
{
    console->err_strm << text;
}
void PythonConsoleErr2::flush() {}
void PythonConsoleIn2::setConsole(PythonProcess* _console)
{
    console = _console;
}
python::object PythonConsoleIn2::readline()
{
    //return python::str(console->getInputFromConsoleIn());
    return python::str();
}
std::string cpp_input(const std::string &prompt)
{
    return xeus::blocking_input_request(prompt, false);
}
}

////
#ifdef USE_OLD_OPTION
void PythonProcess::onSigOptionsParsed(po::variables_map& variables)
{
    DEBUG_PRINT();
    if(variables.count("jupyter-connection")) {
        connection_file = variables["jupyter-connection"].as<std::string>();
        DEBUG_STREAM(" jupyter-connection:" << connection_file);
        bool res = setupPython();
        std::thread th_interp(&PythonProcess::interpreterThread, this);
        th_interp.detach();
    }
}
#else
void PythonProcess::onSigOptionsParsed(OptionManager *_om)
{
    DEBUG_PRINT();
    if(_om->count("--jupyter-connection")) {
        auto op = _om->get_option("--jupyter-connection");
        connection_file = op->as<std::string>();
        DEBUG_STREAM(" jupyter-connection:" << connection_file);
        bool res = setupPython();
        std::thread th_interp(&PythonProcess::interpreterThread, this);
        th_interp.detach();
    }
}
#endif

bool PythonProcess::initialize()
{
    DEBUG_PRINT();
#ifdef USE_OLD_OPTION
    OptionManager& om = self->optionManager();
    om.addOption("jupyter-connection", po::value<std::string>(), "connection file for jupyter");
    om.sigOptionsParsed(1).connect(
        [this](po::variables_map& _v) { onSigOptionsParsed(_v); } );
#else
    auto om = OptionManager::instance();
    om->add_option("--jupyter-connection", "connection file for jupyter");
    om->sigOptionsParsed(1).connect(
        [this](OptionManager *_om) { onSigOptionsParsed(_om); } );
#endif

    return true;
}
bool PythonProcess::setupPython()
{
    python::gil_scoped_acquire lock;

    mainModule = PythonPlugin::instance()->mainModule();
    globalNamespace = PythonPlugin::instance()->globalNamespace();

    try { interpreter = python::module::import("code").attr("InteractiveConsole")(globalNamespace);
    } catch (...) { /* ignore the exception on windows. this module is loaded already. */
        DEBUG_STREAM(" interpreter loading ERR");
    }

    pybind11::object consoleOutClass =
        pybind11::class_<PythonConsoleOut2>(mainModule, "PythonConsoleOut2")
        .def(pybind11::init<>())
        .def("write", &PythonConsoleOut2::write)
        .def("flush", &PythonConsoleOut2::flush);
    consoleOut = consoleOutClass();
    PythonConsoleOut2& consoleOut_ = consoleOut.cast<PythonConsoleOut2&>();
    consoleOut_.setConsole(this);

    pybind11::object consoleErrClass =
        pybind11::class_<PythonConsoleErr2>(mainModule, "PythonConsoleErr2")
        .def(pybind11::init<>())
        .def("write", &PythonConsoleErr2::write)
        .def("flush", &PythonConsoleErr2::flush);
    consoleErr = consoleErrClass();
    PythonConsoleErr2& consoleErr_ = consoleErr.cast<PythonConsoleErr2&>();
    consoleErr_.setConsole(this);

    python::object consoleInClass =
        pybind11::class_<PythonConsoleIn2>(mainModule, "PythonConsoleIn2")
        .def(pybind11::init<>())
        .def("readline", &PythonConsoleIn2::readline);
    consoleIn = consoleInClass();
    PythonConsoleIn2 consoleIn_ = consoleIn.cast<PythonConsoleIn2&>();
    consoleIn_.setConsole(this);

    sys = PythonPlugin::instance()->sysModule();
    python::object inspector_class = python::module::import("IPython").attr("core").attr("oinspect").attr("Inspector");
    inspector = inspector_class();
    //
    python::object transformer_class = python::module::import("IPython").attr("core").attr("inputtransformer2").attr("TransformerManager");
    transformer = transformer_class();
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

    ast_mod = python::module::import("ast");
    ast_interactive =  ast_mod.attr("Interactive");
    builtins = pybind11::module::import("builtins");
    bltin_compile = builtins.attr("compile");

    builtins.attr("input") = pybind11::cpp_function(&cpp_input, pybind11::arg("prompt") = "");

    connect(this, &PythonProcess::sendComRequest,
            this, &PythonProcess::procComRequest,
            Qt::BlockingQueuedConnection);  //Qt::DirectConnection);
    connect(this, &PythonProcess::sendPyRequest,
            this, &PythonProcess::procPyRequest,
            Qt::BlockingQueuedConnection);  //Qt::DirectConnection);

    return true;
}
bool PythonProcess::finalize()
{
    DEBUG_PRINT();
    return true;
}
//// exec version
static inline void exec(python::object &_code)
{
    python::exec("exec(_code_, _scope_, _scope_)", python::globals(), python::dict(python::arg("_code_") = _code, python::arg("_scope_") = python::globals()));
}
bool PythonProcess::putCommand(const std::string &_com)
{
    bool ret = true;
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

    DEBUG_STREAM(" exec: " << _com);
    ////
    try {
        std::string code_copy(_com);
        const std::string filename("<input>");

        // Parse code to AST
        python::object code_ast  = ast_mod.attr("parse")(code_copy, "<input>", "exec");
        python::list expressions = code_ast.attr("body");

        python::object last_stmt = expressions[ python::len(expressions) - 1 ];
        if (python::isinstance(last_stmt, ast_mod.attr("Expr"))) {
            code_ast.attr("body").attr("pop")();
            python::list interactive_nodes;
            interactive_nodes.append(last_stmt);

            python::object interactive_ast = ast_interactive(interactive_nodes);
            python::object compiled_code   = bltin_compile(code_ast, filename, "exec");

            python::object compiled_interactive_code = bltin_compile(interactive_ast, filename, "single");
            exec(compiled_code);
            exec(compiled_interactive_code);
        } else {
            python::object compiled_code = bltin_compile(code_ast, filename, "exec");
            exec(compiled_code);
        }
        //kernel_res["status"] = "ok";
        //kernel_res["user_expressions"] = nl::json::object();
        //kernel_res["payload"] = nl::json::array();
    } catch (python::error_already_set& e) {
        // error
        // TODO
        DEBUG_STREAM(" error: " << e.what());
        python::print(e.what(), python::arg("file") = consoleErr);
        ret = false;
    }
    sys.attr("stdout") = orgStdout;
    sys.attr("stderr") = orgStderr;
    sys.attr("stdin")  = orgStdin;

    return ret;
}
#if 0
//// interpreter version
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
#endif
void PythonProcess::interpreterThread()
{
    if(connection_file.size() == 0) return;

    xeus::xconfiguration config = xeus::load_configuration(connection_file);
#if defined(USE_XEUS5)
    std::unique_ptr<xeus::xcontext> context = xeus::make_zmq_context();
#else
    auto context = xeus::make_context<zmq::context_t>();
#endif
    // Create interpreter instance
    using interpreter_ptr = std::unique_ptr<cnoid::JupyterInterpreter>;
    interpreter_ptr interpreter = interpreter_ptr(new cnoid::JupyterInterpreter());
    interpreter->impl = this;
    // Create kernel instance and start it
#if defined(USE_XEUS5)
    xeus::xkernel kernel(config,
                         xeus::get_user_name(),
                         std::move(context),
                         std::move(interpreter),
                         xeus::make_xserver_default);
#else
    xeus::xkernel kernel(config,
                         xeus::get_user_name(),
                         std::move(context),
                         std::move(interpreter),
                         xeus::make_xserver_zmq);
#endif
    kernel.start();
}
void PythonProcess::procPyRequest(const std::string &line)
{
    this->is_complete = this->putCommand(line);
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
