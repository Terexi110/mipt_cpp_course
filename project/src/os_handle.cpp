#include "os_handle.h"

#include <print>
#include <stdexcept>

#include "os.h"

namespace nano_edr {

OsHandle::OsHandle(const std::string& config_path) {
    os_status status = os_init(config_path.c_str(), &handle_);
    if (status != OS_OK) {
        throw std::runtime_error("не удалось инициализировать " + config_path + ": " + os_status_str(status));
    }
}

OsHandle::~OsHandle() {
    os_free(handle_);
}

void OsHandle::Subscribe(os_event_cb callback, void* context) {
    os_status status = os_event_subscribe(handle_, callback, context);
    if (status != OS_OK) {
        throw std::runtime_error(std::string("не удалось подписать событие: ") + os_status_str(status));
    }
}

void OsHandle::Start() {
    os_status status = os_start(handle_);
    if (status != OS_OK) {
        throw std::runtime_error(std::string("не удалось запустить поток событий: ") + os_status_str(status));
    }
}

void OsHandle::Stop() {
    os_stop(handle_);
}

os_status OsHandle::Wait(uint32_t timeout_ms) {
    os_status status = os_wait(handle_, timeout_ms);
    return status;
}

}  // namespace nano_edr