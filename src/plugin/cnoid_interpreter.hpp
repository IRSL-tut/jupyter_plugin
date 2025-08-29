#include <xeus-python/xinterpreter.hpp>
#include "PythonProcess.h"

namespace cnoid
{

class cnoid_interpreter : public xpyt::interpreter
{
public:
    cnoid_interpreter(bool redirect_output_enabled=true, bool redirect_display_enabled = true);
    virtual ~cnoid_interpreter();

    PythonProcess *process;

    py::object& python_shell();

protected:
    nl::json kernel_info_request_impl() override;
    nl::json is_complete_request_impl(const std::string& code) override;
    void shutdown_request_impl() override;

};

}
