#include "sources.h"

#include <fstream>
#include <stdexcept>

#include "agent.h"
#include "os.h"
#include "parse.h"

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

FileSource::FileSource(const std::string& path) : path_(path) {}

void FileSource::Run(Agent* agent) {
    std::ifstream log(path_);
    if (!log) {
        throw std::runtime_error("не удалось открыть журнал: " + path_);
    }

    std::string line;
    while (std::getline(log, line)) {
        if (IsBlankOrComment(line)) {
            continue;
        }
        EventParts parts;
        if (!ParseEventParts(line, &parts)) {
            continue;
        }
        try {
            Event event(parts);
            agent->HandleEvent(event);
        } catch (const std::exception& e) {
        }
    }
}

}  // namespace nano_edr