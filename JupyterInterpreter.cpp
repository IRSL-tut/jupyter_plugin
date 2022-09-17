#include "JupyterInterpreter.h"
#include <xeus/xhelper.hpp>

#include "irsl_debug.h"

namespace nl = nlohmann;

namespace cnoid
{
    nl::json JupyterInterpreter::execute_request_impl(int execution_counter, // Typically the cell number
                                                      const std::string& code, // Code to execute
                                                      bool silent, bool store_history,
                                                      nl::json user_expressions,
                                                      bool allow_stdin)
    {
        DEBUG_STREAM(" > ec: " << execution_counter << ", silent: " << silent << ", store:" << store_history << ", stdin:" << allow_stdin);
        DEBUG_STREAM(" > " << code);

        if(!impl) { // error??
            return xeus::create_successful_reply();
        }
        bool res;

        if(code.size() > 0 && code[0] == '%') {
            res = execute_choreonoid(execution_counter, code.substr(1), silent, store_history,
                                     user_expressions, allow_stdin);
        } else {
            res = execute_python(execution_counter, code, silent, store_history,
                                 user_expressions, allow_stdin);
        }

        if (res) {
            return xeus::create_successful_reply();
        }
        return xeus::create_successful_reply(); // [todo]
    }

    void JupyterInterpreter::configure_impl()
    {
        DEBUG_PRINT();
    }

    nl::json JupyterInterpreter::complete_request_impl(const std::string& code,
                                                       int cursor_pos)
    {
        DEBUG_STREAM(" " << cursor_pos << " >>" << code);
        return xeus::create_complete_reply({}, cursor_pos, cursor_pos);
#if 0
        if (code[0] == 'H')
        {
            return xeus::create_complete_reply({"Hello", "Hey", "Howdy"}, 5, cursor_pos);
        }
        // No completion result
        else
        {
            return xeus::create_complete_reply({}, cursor_pos, cursor_pos);
        }
#endif
    }

    nl::json JupyterInterpreter::inspect_request_impl(const std::string& code,
                                                      int cursor_pos,
                                                      int detail_level)
    {
        DEBUG_STREAM(" " << detail_level << "/" << cursor_pos << " >>" << code);
        return xeus::create_inspect_reply();
#if 0
        if (code.compare("print") == 0)
        {
            return xeus::create_inspect_reply(true,
                                              {"text/plain", "Print objects to the text stream file, [...]"});
        }
        else
        {
            return xeus::create_inspect_reply();
        }
#endif
    }

    nl::json JupyterInterpreter::is_complete_request_impl(const std::string& /*code*/)
    {
        return xeus::create_is_complete_reply("complete");
    }

    nl::json JupyterInterpreter::kernel_info_request_impl()
    {
        return xeus::create_info_reply("",
                                       "Choreonoid",
                                       "0.1.0",
                                       "python",
                                       "3.7",
                                       "text/x-python",
                                       ".py");
    }
    void JupyterInterpreter::shutdown_request_impl()
    {
        DEBUG_PRINT();
    }

    bool JupyterInterpreter::execute_python(int execution_counter,
                                            const std::string& code,
                                            bool silent, bool store_history,
                                            nl::json &user_expressions,
                                            bool allow_stdin)
    {
        impl->putCommand(code);// enter?

        nl::json pub_data;
        std::string str = impl->out_strm.str();
        DEBUG_STREAM("[" << str << "]");
        if (str.size() > 0) {
            pub_data["text/plain"] = str;
            publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
        }
        // ???
        std::string err_str = impl->err_strm.str();
        if(err_str.size() > 0) {
            std::vector<std::string> tb;
            tb.push_back(err_str);
            publish_execution_error("Error", "001", tb);
        }
        return true;
    }
    bool JupyterInterpreter::execute_choreonoid(int execution_counter,
                                                const std::string& code,
                                                bool silent, bool store_history,
                                                nl::json &user_expressions,
                                                bool allow_stdin)
    {
        return true;
    }
}
