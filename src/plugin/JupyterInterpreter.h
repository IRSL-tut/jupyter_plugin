#ifndef CNOID_JUPYTER_INTERPRETER_H
#define CNOID_JUPYTER_INTERPRETER_H

#include <xeus/xinterpreter.hpp>
#include <nlohmann/json.hpp>

#include "JupyterPlugin.h"
#include "PythonProcess.h"

using xeus::xinterpreter;
namespace nl = nlohmann;

namespace cnoid
{
    class JupyterInterpreter : public xinterpreter
    {
    public:
        JupyterInterpreter() : after_is_complete(false), not_in_scope(false), is_complete_previous_pos(0)
        {   xeus::register_interpreter(this); }
        virtual ~JupyterInterpreter() = default;

        PythonProcess *impl;
    private:
        void configure_impl() override;
#if defined(USE_XEUS5)
        void execute_request_impl(send_reply_callback cb,
                                  int execution_counter,
                                  const std::string& code,
                                  xeus::execute_request_config config,
                                  nl::json user_expressions) override;
#else
        nl::json execute_request_impl(int execution_counter,
                                      const std::string& code,
                                      bool silent,
                                      bool store_history,
                                      nl::json user_expressions,
                                      bool allow_stdin) override;
#endif
        nl::json complete_request_impl(const std::string& code,
                                       int cursor_pos) override;
        nl::json inspect_request_impl(const std::string& code,
                                      int cursor_pos,
                                      int detail_level) override;
        nl::json is_complete_request_impl(const std::string& code) override;
        nl::json kernel_info_request_impl() override;
        void shutdown_request_impl() override;

      ////
        void publish_result_and_error(int exec_counter);
        bool execute_python(const std::string& code, bool &is_complete, bool in_complete);
        bool execute_python_is_complete(const std::string& code);
        bool execute_choreonoid(int execution_counter,
                                const std::string& code,
                                bool silent,
                                bool store_history,
                                nl::json &user_expressions,
                                bool allow_stdin);

        bool after_is_complete;
        bool not_in_scope;
        int is_complete_previous_pos;
        int executed_pos;
        int current_indent;

        std::ostringstream oss_out;
        std::ostringstream oss_err;
    };
}
#endif
