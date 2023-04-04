#include "JupyterInterpreter.h"
#include <xeus/xhelper.hpp>
#include <QString>
#include <xtl/xbase64.hpp>
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
        bool res = false;

        if(code.size() > 0 && code[0] == '%') {
            res = execute_choreonoid(execution_counter, code.substr(1), silent, store_history,
                                     user_expressions, allow_stdin);
        } else if(code.size() > 0 && code[0] == '?') {
            python::object pobj_ = impl->findObject(code.substr(1));
            if(pobj_.ptr() != NULL) {
                python::dict pdic_ = impl->inspector.attr("_get_info")(pobj_);
                nl::json pub_;
                pub_["text/plain"] = pdic_["text/plain"].cast<std::string>();
                publish_execution_result(execution_counter, std::move(pub_), nl::json::object());
            }
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
        python::gil_scoped_acquire lock;
        std::vector<std::string> matches;
        int cursor_start = cursor_pos;
        std::string sub_code = code.substr(0, cursor_pos);
        python::list completions = impl->jedi_Interpreter(sub_code, python::make_tuple(python::globals())).attr("complete")();

        if (python::len(completions) != 0) {
            cursor_start -= python::len(completions[0].attr("name_with_symbols")) - python::len(completions[0].attr("complete"));
            for (python::handle completion : completions) {
                matches.push_back(completion.attr("name_with_symbols").cast<std::string>());
            }
        }
        nl::json res;
        res["cursor_start"] = cursor_start;
        res["cursor_end"] = cursor_pos;
        res["matches"] = matches;
        res["metadata"] = nl::json::object();
        res["status"] = "ok";
        return res;
    }

    nl::json JupyterInterpreter::inspect_request_impl(const std::string& code,
                                                      int cursor_pos,
                                                      int detail_level)
    {
        python::gil_scoped_acquire lock;

        python::str token_ = impl->token_at_cursor(code, cursor_pos);
        //DEBUG_STREAM(" " << detail_level << "/" << cursor_pos << " [" << code << "](" <<
        //             code.size() << ") : " << token_);

        python::object pobj_ = impl->findObject(token_);
        if(pobj_.ptr() == NULL) { // fail to find object
            //DEBUG_STREAM(" fail");
            return xeus::create_inspect_reply();
        } else {
            python::dict pdic_ = impl->inspector.attr("_get_info")(pobj_);
            nl::json res;
            for(auto it = pdic_.begin(); it != pdic_.end(); it++) {
                //DEBUG_STREAM(" " << it->first.cast<std::string>());
                //DEBUG_STREAM(" " << it->second.cast<std::string>());
                res[it->first.cast<std::string>()] = it->second.cast<std::string>();
            }
            return xeus::create_inspect_reply(true, res);
        }
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
        if (code == "display") {
            QString qmsg(code.c_str());
            Q_EMIT impl->sendComRequest(qmsg);
            DEBUG_SIMPLE(" display : " << impl->data.size());
            nl::json pub_data;
            pub_data["image/png"] = impl->data.toBase64().data();
            publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
            return true;
        }
        return false;
    }
}
