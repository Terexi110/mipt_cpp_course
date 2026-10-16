#include "os_source.h"

#include <stdexcept>

#include "agent.h"
#include "os.h"

namespace nano_edr {

OsSource::OsSource(const std::string& config_path) : handle_(config_path) {}

void OsSource::Run(Agent* agent) {
    handle_.Subscribe(Agent::Trampoline, agent);
    handle_.Start();

    os_status status = OS_OK;
    while ((status = handle_.Wait(1000)) == OS_TIMEOUT) {
    }
    if (status != OS_OK) {
        throw std::runtime_error("ошибка источника: " + std::string(os_status_str(status)));
    }
}

}  // namespace nano_edr