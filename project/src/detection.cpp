#include "detection.h"

#include <format>
#include <string>

namespace nano_edr {

std::string ToString(const Detection& detection) {
    std::string result = std::format("[DETECT] {}  {}  ts={} pid={}",
                       SeverityName(detection.severity),
                       detection.rule,
                       detection.ts.ms,
                       detection.pid);
    if (!detection.chain.empty()) {
        result += std::format(" chain={}", detection.chain);
    }
    return result;
}

}  // namespace nano_edr
