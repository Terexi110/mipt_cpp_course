#include "os_source.h"

#include <print>
#include <stdexcept>
#include <string>

#include "event.h"
#include "event_source.h"
#include "os.h"

namespace nano_edr {

namespace {

Event MakeEventFromOs(const os_event* ev) {
    if (ev == nullptr) {
        throw std::invalid_argument("os_event is null");
    }
    EventParts parts;
    parts.ts = std::to_string(ev->ts);
    parts.type = ev->type ? ev->type : "";
    parts.pid = (ev->pid == 0) ? "" : std::to_string(ev->pid);
    for (size_t i = 0; i < ev->field_count; ++i) {
        parts.fields.push_back(Field{ev->fields[i].key, ev->fields[i].value});
    }
    return Event(parts);
}

}  // namespace

OsSource::OsSource(const std::string& config_path) : handle_(config_path) {}

void OsSource::Trampoline(const os_event* ev, void* ctx) noexcept {
    if (ctx == nullptr) {
        return;
    }
    auto* handler = static_cast<IEventHandler*>(ctx);
    try {
        Event event = MakeEventFromOs(ev);
        handler->HandleEvent(event);
    } catch (const std::exception& e) {
        std::print(stderr, "ошибка обработки события: {}\n", e.what());
        handler->OnBroken();
    } catch (...) {
        std::print(stderr, "неизвестная ошибка\n");
        handler->OnBroken();
    }
}

void OsSource::Run(IEventHandler* handler) {
    handle_.Subscribe(&OsSource::Trampoline, handler);
    handle_.Start();

    os_status status = OS_OK;
    while ((status = handle_.Wait(1000)) == OS_TIMEOUT) {
        if (handler != nullptr) {
            handler->OnTick();
        }
    }
    if (status != OS_OK) {
        throw std::runtime_error("ошибка источника: " + std::string(os_status_str(status)));
    }
}

}  // namespace nano_edr