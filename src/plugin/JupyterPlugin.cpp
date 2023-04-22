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
    : Plugin("Jupyter"), impl(nullptr)
{
    require("Python");
    instance_ = this;
#ifdef EXT_BUNDLE
    impl = new PythonProcess(this);
#endif
}
JupyterPlugin::~JupyterPlugin()
{
    if (!!impl) {
        // delete impl; //
    }
}
#ifndef EXT_BUNDLE
bool JupyterPlugin::customizeApplication(AppCustomizationUtil& app)
{
    DEBUG_PRINT();
    impl = new PythonProcess(this);
    return true;
}
#endif
bool JupyterPlugin::initialize()
{
    DEBUG_PRINT();
    if(!!impl) {
        return impl->initialize();
    } else {
        return false;
    }
}
bool JupyterPlugin::finalize()
{
    DEBUG_PRINT();
    instance_ = nullptr;
    if(!!impl) {
        return impl->finalize();
    } else {
        return true;
    }
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
