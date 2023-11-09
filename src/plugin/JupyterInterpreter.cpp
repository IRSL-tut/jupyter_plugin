#include "JupyterInterpreter.h"
#include <xeus/xhelper.hpp>
#include <QString>
#include <QCoreApplication>
#include <xtl/xbase64.hpp>
#include "irsl_debug.h"
#include <vector>
#include <sstream>

namespace nl = nlohmann;

namespace cnoid
{
    bool split_code(std::vector<std::string> &res, const std::string &_code, const char elm = '\n')
    {
        int start_ = 0;
        int end_ = _code.find_first_of(elm);

        if(end_ == std::string::npos) return false;

        while(start_ < _code.size()){
            std::string sub(_code, start_, end_ - start_);
            res.push_back(sub);
            start_ = end_ + 1;
            end_ = _code.find_first_of(elm, start_);
            if(end_ == std::string::npos) {
                end_ = _code.size();
            }
        }
        return true;
    }
    inline int right_trim_len(const std::string &_code) // start=0, len=right_trim_len
    {
        // TODO: rewrite / find_last_not_of
        int end_pos = 0;
        for (int idx = _code.size() - 1; idx >= 0; idx--) {
            if (_code[idx] != ' ' && _code[idx] != '\t') {
                end_pos = idx + 1;
                break;
            }
        }
        return end_pos;
    }
    inline int left_trim_pos(const std::string &_code)// start=left_trim_pos
    {
        // TODO: rewrite / find_first_not_of
        int res = -1;
        for (int idx = 0; idx < _code.size(); idx++) {
            if (_code[idx] != ' ' && _code[idx] != '\t') {
                res = idx;
                break;
            }
        }
        return res;
    }
    bool right_trim(std::string &res, const std::string &_code)
    {
        int end_pos = right_trim_len(_code);
        if (end_pos == 0) return false;
        res = std::string(_code, 0, end_pos);
        return true;
    }
    bool left_trim(std::string &res, const std::string &_code)
    {
        int pos = left_trim_pos(_code);
        if (pos < 0) return false;
        res = std::string(_code, pos);
        return true;
    }
    inline void last_line(std::string &res, const std::string &_code)
    {
        // return last line without '\n'
        if (_code.empty()) {
            res.clear();
            return;
        }
        size_t start_pos = std::string::npos;
        if (_code.back() == '\n') {
            start_pos = _code.size() - 1;
        }
        size_t pos = _code.find_last_of('\n', start_pos);
        if(pos == std::string::npos) {
            res = _code;
        } else {
            res = _code.substr(pos+1);
        }
    }
    nl::json JupyterInterpreter::execute_request_impl(int execution_counter, // Typically the cell number
                                                      const std::string& code, // Code to execute
                                                      bool silent, bool store_history,
                                                      nl::json user_expressions,
                                                      bool allow_stdin)
    {
        DEBUG_STREAM(" > ec: " << execution_counter << ", silent: " << silent << ", store:" << store_history << ", stdin:" << allow_stdin);
        DEBUG_STREAM(" > uex: " << user_expressions);
        DEBUG_STREAM(" > code(" << code.size() << ")>|" << code <<"|<");

        if(!impl) { // error??
            return xeus::create_successful_reply();
        }
        bool res = false;
        std::string cur_code_(code);
        if (after_is_complete) {
            // reset is_complete
            after_is_complete = false;
            is_complete_previous_pos = 0;
            if (not_in_scope) {
                not_in_scope = false;
                DEBUG_STREAM(" exec: after is_complete, finish exec");
                publish_result_and_error(execution_counter);
                return xeus::create_successful_reply();
            } else { // in scope
                DEBUG_STREAM(" exec: after is_complete, in scope : " << executed_pos << " | " << code.size());
                // remove_first_line / execute scope with multi-line
                if(code.size() > executed_pos && code[executed_pos] == '\n') {
                    cur_code_ = code.substr(executed_pos);
                } else {
                    cur_code_ = "\n";
                }
            }
        }

        if(code.size() > 0 && code[0] == '%') {
            res = execute_choreonoid(execution_counter, code.substr(1), silent, store_history,
                                     user_expressions, allow_stdin);
        } else if(code.size() > 0 && code[0] == '?') {
            python::gil_scoped_acquire lock;

            python::object pobj_ = impl->findObject(code.substr(1));
            if(pobj_.ptr() != NULL) {
                python::dict pdic_ = impl->inspector.attr("_get_info")(pobj_);
                nl::json pub_;
                pub_["text/plain"] = pdic_["text/plain"].cast<std::string>();
#if 0
                for(auto it = pdic_.begin(); it != pdic_.end(); it++) {
                    DEBUG_STREAM(" " << it->first.cast<std::string>());
                    DEBUG_STREAM(" " << it->second.cast<std::string>());
                    pub_[it->first.cast<std::string>()] = it->second.cast<std::string>();
                }
#endif
                publish_execution_result(execution_counter, std::move(pub_), nl::json::object());
            }
        } else {
            bool dummy;
            res = execute_python(cur_code_, dummy, false);
            publish_result_and_error(execution_counter);
        }

        if (res) {
            return xeus::create_successful_reply();
        }
        return xeus::create_successful_reply(); // [todo]
#if 0
        nl::json create_error_reply(const std::string& ename = std::string(),
                                    const std::string& evalue = std::string(),
                                    const nl::json& trace_back = nl::json::array());
#endif
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
#if 0
        nl::json create_complete_reply(const nl::json& matches,
                                       const int& cursor_start,
                                       const int& cursor_end,
                                       const nl::json& metadata = nl::json::object());
#endif

    }

    nl::json JupyterInterpreter::inspect_request_impl(const std::string& code,
                                                      int cursor_pos,
                                                      int detail_level)
    {
        python::gil_scoped_acquire lock;

        python::str token_ = impl->token_at_cursor(code, cursor_pos);
        //DEBUG_STREAM(" " << detail_level << "/" << cursor_pos << " [" << code << "](" <<
        //             code.size() << ") : " << token_);
        DEBUG_STREAM(" token : " << token_);
        python::object pobj_ = impl->findObject(token_);
        if(pobj_.ptr() == NULL) { // fail to find object
            DEBUG_STREAM(" fail");
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

    nl::json JupyterInterpreter::is_complete_request_impl(const std::string& code)
    {
        // should be implement for console
        DEBUG_STREAM(" is_cmoplete [" << after_is_complete << "](" << code.size() << "/" << is_complete_previous_pos << "): " << code);
        if(code.size() > 0 && (code[0] == '?' || code[0] == '%')) {
            return xeus::create_is_complete_reply("complete");
        }
        {
            python::gil_scoped_acquire lock;
            python::tuple ret;
            ret = impl->transformer.attr("check_complete")(code);
            DEBUG_STREAM(" tuple_size: " << ret.size());
            if (ret.size() > 1) {
                std::string res_ = ret[0].cast<std::string>();
                int pos_ = -1;
                if (ret[1].is_none()) {
                    DEBUG_STREAM(" NONE");
                } else {
                    pos_ = ret[1].cast<int>();
                }
                DEBUG_STREAM(" res: \'" << res_ << "\', pos: "  << pos_);
                if (pos_ >= 0) {
                    // TODO: indent
                    return xeus::create_is_complete_reply(res_);
                } else {
                    return xeus::create_is_complete_reply(res_);
                }
            } else {
                ERROR_STREAM(" :ipython transformer failed: " << ret.size());
            }
        }
        bool new_enter = !after_is_complete;
        after_is_complete = true;
        int cur_pos = code.size();
        if (cur_pos <= is_complete_previous_pos) {
            is_complete_previous_pos = 0;
            after_is_complete = false;
        }

        if (code[is_complete_previous_pos] == '\n') {
            if (code.size() == is_complete_previous_pos) {
                // to execute_impl
                not_in_scope = false;
                return xeus::create_is_complete_reply("complete");
            }
            std::string new_code_ = code.substr(is_complete_previous_pos+1);
            DEBUG_STREAM("new_code(" << left_trim_pos(new_code_) << "):" << new_code_);
            if(left_trim_pos(new_code_) < 0) {
                // to execute_impl
                not_in_scope = false;
                return xeus::create_is_complete_reply("complete");
            }
        }

        if (new_enter) {
            not_in_scope = false;
            current_indent = -1; // []
            DEBUG_STREAM(" new_enter : " << code);
            std::string lline_;
            int len = 0;
            last_line(lline_, code);
            if (lline_.size() != code.size()) {
                if (right_trim_len(lline_) == 0) {
                    len = lline_.size();
                }
            }
            bool ret;
            if (len != 0) {
                std::string tmp = code.substr(0, code.size()-len);
                tmp += "\n";
                ret = execute_python_is_complete(tmp);
            } else {
                ret = execute_python_is_complete(code);
            }
            executed_pos = code.size();
            if (ret) { // finish execution
                not_in_scope = true;
                return xeus::create_is_complete_reply("complete");
            }
        }
        {   // check indent
            std::string new_code_;
            last_line(new_code_, code.substr(is_complete_previous_pos));
            DEBUG_STREAM(" last_line: >>|" << new_code_ << "|<<");
            std::vector<std::string> active_code_;
            std::string tmp_code_;
            if(split_code(active_code_, new_code_, '#')) {
                tmp_code_ = active_code_[0];
            } else {
                tmp_code_ = new_code_;
            }
            int res_ = right_trim_len(tmp_code_);
            if (current_indent == -1) {
                // enter new and in-scope
                current_indent = 4;
            } else if (res_ > 0 && tmp_code_[res_-1] == ':') {
                current_indent += 4;
                DEBUG_STREAM(" add indent: " << current_indent);
            } else {
                int pos_ = left_trim_pos(new_code_);
                if (pos_ < 0) pos_ = new_code_.size();
                current_indent = pos_;
                DEBUG_STREAM(" new indent: " << current_indent);
            }
        }
        // 'complete', 'incomplete', 'invalid', 'unknown'
        //return xeus::create_is_complete_reply("complete");
        is_complete_previous_pos = cur_pos;
        std::ostringstream oss_;
        for (int i = 0; i < current_indent; i++) oss_ << " ";
        return xeus::create_is_complete_reply("incomplete", oss_.str());
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
        QCoreApplication::quit(); // [TODO] exit choreonoid, is it OK?
    }
    void JupyterInterpreter::publish_result_and_error(int exec_counter)
    {
        std::string out = oss_out.str();
        std::string err = oss_err.str();
        nl::json pub_data;
        if (out.size() > 0) {
            if (out.size() > 1 && out.back() == '\n') {
                out = out.substr(0, out.size() -1);
            }
            pub_data["text/plain"] = out;
            publish_execution_result(exec_counter, std::move(pub_data), nl::json::object());
        }
        if(err.size() > 0) {
            std::vector<std::string> tb;
            tb.push_back(err);
            publish_execution_error("Error", "001", tb);
        }
    }
    bool JupyterInterpreter::execute_python_is_complete(const std::string& code)
    {
        // code should be a line
        DEBUG_STREAM("in-comp: " << code);
        //bool res = impl->putCommand(code);// enter?
        bool res;
        execute_python(code, res, true);
        return res;
    }
    bool JupyterInterpreter::execute_python(const std::string& code, bool &is_complete, bool in_complete)
    {
        std::vector<std::string> lines_;
        DEBUG_STREAM(" code: " << code);
        if (!split_code(lines_, code)) {
            lines_.push_back(code);
        }
        DEBUG_STREAM(" lines: " << lines_.size());
        if (in_complete) {
            //
        } else if (!in_complete) {
            if (lines_.size() > 1) {
                lines_.push_back("\n");
            } else {
                std::vector<std::string> active_code_;
                std::string cur_code;
                if(split_code(active_code_, code, '#')) {
                    cur_code = active_code_[0];
                } else {
                    cur_code = code;
                }
                std::string _res;
                if (right_trim(_res, cur_code)) {
                    if (_res.back() == ':') {
                        std::vector<std::string> tb;
                        tb.push_back(_res);
                        {
                            std::ostringstream oss_;
                            for (int i = 0; i < _res.size() - 1; i++) oss_ << " ";
                            oss_ << "^";
                            tb.push_back(oss_.str());
                        }
                        tb.push_back("SyntaxError: incomplete input");
                        publish_execution_error("Error", "002", tb);
                        return false; // do not enter python interpreter
                    }
                }
            }
        }

        bool error_ = false;
        oss_out.str("");
        oss_out.clear(std::stringstream::goodbit);
        oss_err.str("");
        oss_err.clear(std::stringstream::goodbit);

        int res_counter_ = 0;
        for(int i = 0; i < lines_.size(); i++) {
            DEBUG_STREAM("in: " << lines_[i]);
            //is_complete = impl->putCommand(lines_[i]);// enter?
            impl->sendPyRequest(lines_[i]);
            is_complete = impl->is_complete;
            if(impl->out_strm.str().size() > 0) {
                if(res_counter_ != 0) {
                    oss_out << std::endl;
                }
                oss_out << impl->out_strm.str();
                res_counter_++;
            }
            if (impl->err_strm.str().size() > 0) {
                error_ = true;
                break;
            }
        }
        if(error_) oss_err << impl->err_strm.str();
        return true;
    }
#if 0
    //// direct run from another thread
    bool JupyterInterpreter::execute_python(const std::string& code, bool &is_complete, bool in_complete)
    {
        std::vector<std::string> lines_;
        DEBUG_STREAM(" code: " << code);
        if (!split_code(lines_, code)) {
            lines_.push_back(code);
        }
        DEBUG_STREAM(" lines: " << lines_.size());
        if (in_complete) {
            //
        } else if (!in_complete) {
            if (lines_.size() > 1) {
                lines_.push_back("\n");
            } else {
                std::vector<std::string> active_code_;
                std::string cur_code;
                if(split_code(active_code_, code, '#')) {
                    cur_code = active_code_[0];
                } else {
                    cur_code = code;
                }
                std::string _res;
                if (right_trim(_res, cur_code)) {
                    if (_res.back() == ':') {
                        std::vector<std::string> tb;
                        tb.push_back(_res);
                        {
                            std::ostringstream oss_;
                            for (int i = 0; i < _res.size() - 1; i++) oss_ << " ";
                            oss_ << "^";
                            tb.push_back(oss_.str());
                        }
                        tb.push_back("SyntaxError: incomplete input");
                        publish_execution_error("Error", "002", tb);
                        return false; // do not enter python interpreter
                    }
                }
            }
        }

        bool error_ = false;
        oss_out.str("");
        oss_out.clear(std::stringstream::goodbit);
        oss_err.str("");
        oss_err.clear(std::stringstream::goodbit);

        int res_counter_ = 0;
        for(int i = 0; i < lines_.size(); i++) {
            DEBUG_STREAM("in: " << lines_[i]);
            is_complete = impl->putCommand(lines_[i]);// enter?
            if(impl->out_strm.str().size() > 0) {
                if(res_counter_ != 0) {
                    oss_out << std::endl;
                }
                oss_out << impl->out_strm.str();
                res_counter_++;
            }
            if (impl->err_strm.str().size() > 0) {
                error_ = true;
                break;
            }
        }
        if(error_) oss_err << impl->err_strm.str();
#if 0
        nl::json pub_data;
        DEBUG_STREAM("[" << oss_.str() << "]");
        if (oss_.str().size() > 0) {
            pub_data["text/plain"] = oss_.str();
            publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
        }
        // ???
        if(error_) {
            std::vector<std::string> tb;
            tb.push_back(impl->err_strm.str());
            publish_execution_error("Error", "001", tb);
        }
#endif
        return true;
    }
#endif
    bool JupyterInterpreter::execute_choreonoid(int execution_counter,
                                                const std::string& code,
                                                bool silent, bool store_history,
                                                nl::json &user_expressions,
                                                bool allow_stdin)
    {
#if 0
        //// TODO
        nl::json pub_data;
        std::vector<std::string> err;
        bool ret = sendCommandForChoreonoid(impl, code, pub_data, err);
        //sendCommandForChoreonoid(PythonProcess *, const std::string &, nl::json &, std::vector<std::string> &);
        if (ret) {
            publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
        } else {
            publish_execution_error("Error", "003", err);
        }
#endif
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
