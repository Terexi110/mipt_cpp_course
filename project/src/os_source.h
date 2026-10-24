#ifndef NANO_EDR_OS_SOURCES_H
#define NANO_EDR_OS_SOURCES_H

#include <string>

#include "os_handle.h"
#include "event_source.h"

namespace nano_edr {

class OsSource : public IEventSource {
 public:
    explicit OsSource(const std::string& config_path);

    void Run(IEventHandler* agent);

    static void Trampoline(const os_event* ev, void* ctx) noexcept;

 private:
    OsHandle handle_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_OS_SOURCES_H