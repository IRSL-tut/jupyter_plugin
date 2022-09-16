#include "JupyterPlugin.h"
#include "PythonProcess.h"
#include <fmt/format.h>
//#define IRSL_DEBUG
#include "irsl_debug.h"

using namespace cnoid;

namespace {
JupyterPlugin* instance_ = nullptr;
}

JupyterPlugin* JupyterPlugin::instance()
{
    return instance_;
}
JupyterPlugin::JupyterPlugin()
    : Plugin("Jupyter")
{
    require("Python");
    instance_ = this;
    impl = new PythonProcess(this);
}
JupyterPlugin::~JupyterPlugin()
{
    delete impl;
}
bool JupyterPlugin::initialize()
{
    DEBUG_PRINT();
    return impl->initialize();
}
bool JupyterPlugin::finalize()
{
    DEBUG_PRINT();
    instance_ = nullptr;
    return impl->finalize();
}
const char* JupyterPlugin::description() const
{
    static std::string text =
        fmt::format("Jupyter Plugin Version {}\n", CNOID_FULL_VERSION_STRING) +
        "\n" +
        "Copyrigh (c) 2022 IRSL-tut Development Team.\n"
        "\n" +
        MITLicenseText() +
        "\n"  ;

    return text.c_str();
}

CNOID_IMPLEMENT_PLUGIN_ENTRY(JupyterPlugin);
