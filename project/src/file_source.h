#ifndef NANO_EDR_FILE_SOURCES_H
#define NANO_EDR_FILE_SOURCES_H

#include <string>
#include "event_source.h"

namespace nano_edr {

class FileSource : public IEventSource {
 public:
    explicit FileSource(const std::string& path);

    void Run(IEventHandler* agent);

 private:
    std::string path_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_FILE_SOURCES_H