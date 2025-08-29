#include "cnoid_interpreter.hpp"

#include "nlohmann/json.hpp"

#include "xeus/xinterpreter.hpp"
#include "xeus/xsystem.hpp"

#include "pybind11/pybind11.h"
#include "pybind11/functional.h"
#include "pybind11_json/pybind11_json.hpp"

#include "xeus-python/xinterpreter.hpp"
#include "xeus-python/xeus_python_config.hpp"
#include "xeus-python/xtraceback.hpp"
#include "xeus-python/xutils.hpp"

using namespace cnoid;
using namespace pybind11::literals; // for ""_a

cnoid_interpreter::cnoid_interpreter(bool r_o_e, bool r_d_e) : xpyt::interpreter(r_o_e, r_d_e)
{
    m_release_gil_at_startup = false; // using PythonPlugin
}

cnoid_interpreter::~cnoid_interpreter()
{
}

py::object& cnoid_interpreter::python_shell()
{
    return this->m_ipython_shell;
}

nl::json cnoid_interpreter::kernel_info_request_impl()
{
    nl::json result;
    result["implementation"] = "choreonoid(xeus-python)";
    result["implementation_version"] = XPYT_VERSION;

    /* The jupyter-console banner for xeus-python is the following:
       __  _____ _   _ ___
       \ \/ / _ \ | | / __|
       >  <  __/ |_| \__ \
       /_/\_\___|\__,_|___/

       xeus-python: a Jupyter lernel for Python
    */
    /*
_________               .__    .___
\_   ___ \  ____   ____ |__| __| _/
/    \  \/ /    \ /  _ \|  |/ __ |
\     \___|   |  (  <_> )  / /_/ |
 \______  /___|  /\____/|__\____ |
        \/     \/               \/
     */
    std::string banner = ""
    "    _________               .__    .___\n"
    "    \\_  ____ \\  ____   ____ |__| __| _/\n"
    "    /   \\   \\/ /    \\ /  _ \\|  |/ __ | \n"
    "    \\   \\_____|  ||  (  <_> )  / /_/ | \n"
    "     \\_____  /|__||  /\\____/|__\\____ | \n"
    "           \\/      \\/               \\/ \n"
    "\n"
    "  choreonoid(xeus-python): a Jupyter kernel for Choreonoid(Python)\n"
    "  Python ";
    banner.append(PY_VERSION);
    banner.append("\n# start exec(open('/choreonoid_ws/install/share/irsl_choreonoid/sample/irsl_import.py').read())");

    result["banner"] = banner;
    result["debugger"] = true;

    result["language_info"]["name"] = "python";
    result["language_info"]["version"] = PY_VERSION;
    result["language_info"]["mimetype"] = "text/x-python";
    result["language_info"]["file_extension"] = ".py";
#if 0
    result["help_links"] = nl::json::array();
    result["help_links"][0] = nl::json::object({
            {"text", "Xeus-Python Reference"},
            {"url", "https://xeus-python.readthedocs.io"}
        });
#endif
    result["status"] = "ok";
    return result;
}

nl::json cnoid_interpreter::is_complete_request_impl(const std::string& code)
{
    py::gil_scoped_acquire acquire;
    nl::json kernel_res;

    py::object transformer_manager = py::getattr(m_ipython_shell, "input_transformer_manager", py::none());
    if (transformer_manager.is_none())
    {
        transformer_manager = m_ipython_shell.attr("input_splitter");
    }

    py::list result = transformer_manager.attr("check_complete")(code);
    auto status = result[0].cast<std::string>();

    kernel_res["status"] = status;
#if 0
    if (status.compare("incomplete") == 0)
    {
        kernel_res["indent"] = std::string(result[1].cast<std::size_t>(), ' ');
    }
#else
    // disable auto indent
    kernel_res["indent"] = "";
#endif
    return kernel_res;
}

void cnoid_interpreter::shutdown_request_impl()
{
    if (!!process) {
        process->shutdown_impl();
    }
}
