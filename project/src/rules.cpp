#include "rules.h"

#include <print>

namespace nano_edr {

const char* SeverityName(Severity severity) {
    switch (severity) {
        case Severity::kLow:
            return "low";
        case Severity::kMedium:
            return "medium";
        case Severity::kHigh:
            return "high";
        case Severity::kCritical:
            return "critical";
        default:
            return "?";
    }
}

size_t CheckRules(const Event& event, const Rule* rules, size_t rule_count) {
    if (!rules || rule_count == 0) {
        return 0;
    }
    size_t detect_count = 0;
    for (size_t i = 0; i < rule_count; ++i) {
        const Rule& rule = rules[i];
        if (rule.check && rule.check(event)) {
            std::print("[DETECT] {}  {}  ts={} pid={}\n",
                       SeverityName(rule.severity), rule.id, event.ts, event.pid);
            ++detect_count;
        }
    }
    return detect_count;
}

}  // namespace nano_edr
