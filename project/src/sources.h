#ifndef NANO_EDR_SOURCES_H
#define NANO_EDR_SOURCES_H

#include <string>

#include "agent.h"
#include "os_handle.h"

namespace nano_edr {

class OsSource {
 public:
    explicit OsSource(const std::string& config_path);

    void Run(Agent* agent);

 private:
    OsHandle handle_;
};

class FileSource {
 public:
    explicit FileSource(const std::string& path);

    void Run(Agent* agent);

 private:
    std::string path_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_SOURCES_H