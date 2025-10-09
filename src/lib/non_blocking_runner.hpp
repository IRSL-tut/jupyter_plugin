#ifndef non_blocking_runner_HPP
#define non_blocking_runner_HPP

#include <xeus-zmq/xshell_runner.hpp>

namespace xeus
{
    class non_blocking_runner final : public xshell_runner
    {
    public:

        non_blocking_runner() = default;
        ~non_blocking_runner() override = default;

        bool blocking_poll();
        bool proc();

    private:

        void run_impl() override;
    };

    bool non_blocking_runner::blocking_poll() {
        auto chan = poll_channels();
        return true;
    }

    bool non_blocking_runner::proc()
    {
        auto chan = poll_channels(0);
        if (auto msg = read_shell(chan))
        {
            notify_shell_listener(std::move(msg.value()));
        }
        else if (auto msg = read_controller(chan))
        {
            std::string val = std::move(msg.value());
            if (val == "stop")
            {
                send_controller(std::move(val));
                return false;
            }
            else
            {
                std::string rep = notify_internal_listener(std::move(val));
                send_controller(std::move(rep));
            }
        }
        return true;
    }

    void non_blocking_runner::run_impl()
    {
    }
}


#endif
