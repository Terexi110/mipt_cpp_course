#include "rules.h"

namespace nano_edr {

const char* SeverityName(Severity severity){
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

size_t CheckRules(const Event& event, const Rule* rules, size_t rule_count){
    return 0;    
}


}

