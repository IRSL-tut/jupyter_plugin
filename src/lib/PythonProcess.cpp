//
#include "PythonProcess.h"

// thread
#include <thread>

////
#pragma push_macro("slots")
#undef slots

// xeus
#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <xeus/xinterpreter.hpp>
#include <xeus/xhelper.hpp>

#include <xeus-zmq/xserver_zmq_split.hpp>
#include <xeus-zmq/xcontrol_default_runner.hpp>
#include <xeus-zmq/xshell_default_runner.hpp>
#include <xeus-zmq/xzmq_context.hpp>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <xeus-python/xinterpreter.hpp>
#include <xeus-python/xinterpreter_raw.hpp>
#include <xeus-python/xdebugger.hpp>
#include <xeus-python/xtraceback.hpp>
#include <xeus-python/xpaths.hpp>
#include <xeus-python/xeus_python_config.hpp>
#include <xeus-python/xutils.hpp>

#include "cnoid_interpreter.hpp"
#include "non_blocking_runner.hpp"

// HOTFIX
#include <dlfcn.h>
#pragma pop_macro("slots")
////

// for shutdown
#include <QCoreApplication>

#include <QTimer>

//#define IRSL_DEBUG
#include "irsl_debug.h"
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

namespace cnoid {

class PythonProcess::Impl
{
public:
    Impl(PythonProcess *_self);

public:
    PythonProcess *self;

    bool use_jupyter;
    std::string connection_file;

    void *python; //// HOT-FIX
    std::unique_ptr<pybind11::scoped_interpreter> py_interpreter;

    using kernel_ptr = std::unique_ptr<xeus::xkernel>;
    kernel_ptr kernel;

    using interpreter_ptr = std::unique_ptr<xeus::xinterpreter>;
    cnoid_interpreter *interpreter;

    xeus::non_blocking_runner* p_runner;

#ifdef USE_BLOCKING
    QTimer timer;
#else
    Runner *qrunner;
#endif
};

}

using namespace cnoid;

PythonProcess::Impl::Impl(PythonProcess *_self) : self(_self), use_jupyter(false), python(nullptr), interpreter(nullptr),
                                                  p_runner(nullptr)
#ifdef USE_BLOCKING
                                                , timer(_self)
#else
                                                , qrunner(nullptr)
#endif
{
}

void PythonProcess::proc()
{
    if (!!(impl->p_runner)) {
        bool res = impl->p_runner->proc();
        if (!res) {
            //std::cout << "runner->proc(false) pid: " << this->getpid() << ", tid: " << this->gettid() << std::endl;
        }
    }
}

bool PythonProcess::blocking_poll()
{
    if (!!(impl->p_runner)) {
        bool res = impl->p_runner->blocking_poll();
        if (!res) {
            //std::cout << "runner->proc(false) pid: " << this->getpid() << ", tid: " << this->gettid() << std::endl;
        }
        return res;
    }
    return false;
}

bool PythonProcess::initialize(const std::string &connection_file)
{
    DEBUG_PRINT();

    impl = new Impl(this);

    impl->use_jupyter = true;
    impl->connection_file = connection_file;

    return setupPython();
}

bool PythonProcess::finalize()
{
    DEBUG_PRINT();
    dlclose(impl->python);
    return true;
}

void PythonProcess::shutdown_impl()
{
    DEBUG_PRINT();
    INFO_STREAM(" shutdown_impl");
    impl->kernel->get_server().stop();
    INFO_STREAM(" shutdown_impl::stopped");

    QCoreApplication::quit(); // [TODO] exit choreonoid, is it OK?
}

bool PythonProcess::setupPython()
{
    std::string ver(Py_GetVersion());
    std::cout << "ver: " << ver << std::endl;

    PyStatus status;
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    const std::wstring pname(L"choreonoid");
    const std::wstring phome(L"/tmp");
    config.program_name = const_cast<wchar_t*>(pname.c_str());
    config.home = const_cast<wchar_t*>(phome.c_str());
    int argc = 0;
    char **argv = nullptr;
    PyConfig_SetBytesArgv(&config, argc, argv);

    using history_manager_ptr = std::unique_ptr<xeus::xhistory_manager>;
    history_manager_ptr hist = xeus::make_in_memory_history_manager();

    nl::json debugger_config;
    debugger_config["python"] = "choreonoid";

    //impl->python = dlopen("/usr/lib/x86_64-linux-gnu/libpython3.8.so", RTLD_NOW | RTLD_GLOBAL);
    impl->py_interpreter.reset(new pybind11::scoped_interpreter());

    if (!impl->connection_file.empty()) {
        std::unique_ptr<xeus::xcontext> context = xeus::make_zmq_context();
        Impl::interpreter_ptr interpreter_(new cnoid_interpreter());
        impl->interpreter = dynamic_cast<cnoid_interpreter *>(interpreter_.get());
        impl->interpreter->process = this;
        xeus::xconfiguration config = xeus::load_configuration(impl->connection_file);
        impl->kernel = Impl::kernel_ptr(new xeus::xkernel(config,
                                                          xeus::get_user_name(),
                                                          std::move(context),
                                                          std::move(interpreter_),
                                                          [this] ( xeus::xcontext& context,
                                                                   const xeus::xconfiguration& config,
                                                                   nl::json::error_handler_t eh ) {
                                                              std::unique_ptr<xeus::xshell_runner> runner = std::make_unique<xeus::non_blocking_runner>();
                                                              this->impl->p_runner = dynamic_cast<xeus::non_blocking_runner *>(runner.get());
                                                              return xeus::make_xserver_shell(
                                                                  context,
                                                                  config,
                                                                  eh,
                                                                  std::make_unique<xeus::xcontrol_default_runner>(),
                                                                  std::move(runner));
                                                          },
                                                          std::move(hist),
                                                          xeus::make_console_logger(xeus::xlogger::full,
                                                                                    xeus::make_file_logger(xeus::xlogger::full, "/tmp/xeus.log")), // require export XEUS_LOG=1
                                                          xpyt::make_python_debugger,
                                                          debugger_config));
        impl->kernel->start();
#ifdef USE_BLOCKING
        // timered non_blocking poll
        impl->timer.setInterval(0);
        connect(&(impl->timer), &QTimer::timeout, this, &PythonProcess::proc);
        impl->timer.start();
#else
        // non-blocking using thread
        impl->qrunner = new Runner(impl->self);
        connect(impl->qrunner, &Runner::sendRequest,
                this, &PythonProcess::procRequest, Qt::BlockingQueuedConnection);
        impl->qrunner->start();
#endif
    } else {
        std::unique_ptr<xeus::xcontext> context = xeus::make_zmq_context();
        Impl::interpreter_ptr interpreter_(new cnoid_interpreter());
        impl->interpreter = dynamic_cast<cnoid_interpreter *>(interpreter_.get());
        impl->kernel = Impl::kernel_ptr(new xeus::xkernel(xeus::get_user_name(),
                                                          std::move(context),
                                                          std::move(interpreter_),
                                                          xeus::make_xserver_shell_main,
                                                          std::move(hist),
                                                          xeus::make_file_logger(xeus::xlogger::full, "/tmp/xeus.log"),
                                                          xpyt::make_python_debugger,
                                                          debugger_config));

        const auto& config = impl->kernel->get_config();
        std::cout <<
            "Starting xeus-python kernel...\n\n"
            "If you want to connect to this kernel from an other client, just copy"
            " and paste the following content inside of a `kernel.json` file. And then run for example:\n\n"
            "# jupyter console --existing kernel.json\n\n"
            "kernel.json\n```\n{\n"
            "    \"transport\": \"" + config.m_transport + "\",\n"
            "    \"ip\": \"" + config.m_ip + "\",\n"
            "    \"control_port\": " + config.m_control_port + ",\n"
            "    \"shell_port\": " + config.m_shell_port + ",\n"
            "    \"stdin_port\": " + config.m_stdin_port + ",\n"
            "    \"iopub_port\": " + config.m_iopub_port + ",\n"
            "    \"hb_port\": " + config.m_hb_port + ",\n"
            "    \"signature_scheme\": \"" + config.m_signature_scheme + "\",\n"
            "    \"key\": \"" + config.m_key + "\"\n"
            "}\n```"
            << std::endl;
        {
            std::ofstream ofs("/tmp/kernel.json");
            ofs <<
            "{\n"
            "    \"transport\": \"" + config.m_transport + "\",\n"
            "    \"ip\": \"" + config.m_ip + "\",\n"
            "    \"control_port\": " + config.m_control_port + ",\n"
            "    \"shell_port\": " + config.m_shell_port + ",\n"
            "    \"stdin_port\": " + config.m_stdin_port + ",\n"
            "    \"iopub_port\": " + config.m_iopub_port + ",\n"
            "    \"hb_port\": " + config.m_hb_port + ",\n"
            "    \"signature_scheme\": \"" + config.m_signature_scheme + "\",\n"
            "    \"key\": \"" + config.m_key + "\"\n"
            "}\n" << std::endl;
            ofs.close();
        }
        //std::cout << "Started in Kernel pid: " << this->getpid() << ", tid: " << this->gettid() << std::endl;
        impl->kernel->start();
    }

    return true;
}

void PythonProcess::procRequest()
{
    proc();
}

#ifndef USE_BLOCKING
class Runner::Impl {
public:
    Impl() { };
public:
    PythonProcess *p_proc;
};

Runner::Runner(PythonProcess *pp) : QThread()
{
    impl = new Impl();
    impl->p_proc = pp;
}

void Runner::run()
{
    while(true) {
        if (!!impl->p_proc) {
            if (impl->p_proc->blocking_poll()) {
                this->sendRequest();
            }
        }
    }
}
#endif
