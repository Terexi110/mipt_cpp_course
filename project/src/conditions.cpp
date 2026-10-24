#include <utility>

#include "conditions.h"
#include "fields.h"
#include "text.h"

namespace nano_edr {

bool FieldEquals::Matches(const Event& event) const {
    const std::string* value = FindField(event, key_);
    return value && EqualsIgnoreCase(*value, value_);
}

bool FieldContains::Matches(const Event& event) const {
    const std::string* value = FindField(event, key_);
    return value && ContainsIgnoreCase(*value, needle_);   
}

bool FieldEndsWith::Matches(const Event& event) const {
    const std::string* value = FindField(event, key_);
    return value && EndsWithIgnoreCase(*value, suffix_);
}

bool FieldInList::Matches(const Event &event) const {
    const std::string* value = FindField(event, key_);
    if (values_.size() == 0 || !value) {
        return false;
    }
    for (const auto& item : values_) {
        if (EqualsIgnoreCase(*value, item)) {
            return true;
        }
    }
    return false;
}

bool EventTypeIs::Matches(const Event& event) const {
    for (EventType type : types_) {
        if (event.event_type() == type) {
            return true;
        }
    }
    return false;
}

void AllOf::Add(std::unique_ptr<ICondition> condition) {
    parts_.push_back(std::move(condition));
}

bool AllOf::Matches(const Event& event) const {
    if (parts_.size() == 0) {
        return true;
    }
    for (auto& condition : parts_) {
        if (!condition->Matches(event)) {
            return false;
        }
    }
    return true;
}

void AnyOf::Add(std::unique_ptr<ICondition> condition) {
    parts_.push_back(std::move(condition));
}

bool AnyOf::Matches(const Event& event) const {
    if (parts_.size() == 0) {
        return false;
    }
    for (const auto& condition : parts_) {
        if (condition->Matches(event)) {
            return true;
        }
    }
    return false;
}


}