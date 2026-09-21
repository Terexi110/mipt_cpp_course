#include "fields.h"
#include <stdexcept>

using namespace nano_edr;

const std::string* FindField(const Event& event, const std::string& key){
    for (auto& field : event.fields) {
        if (field.key == key) {
            return &field.value;
        }
    }
    return nullptr;
}

const std::string& GetRequiredField(const Event& event, const std::string& key){
    const std::string* value = ::FindField(event, key);
    if (!value) {
        throw std::invalid_argument("обязательное поле отсутствует: " + key);
    }
    return *value;
}

bool GetIntField(const Event& event, const std::string& key, uint64_t* out);

uint64_t GetIntField(const Event& event, const std::string& key,
                     uint64_t fallback);

bool IsProcessStart(const Event& event);
bool IsFileWrite(const Event& event);
bool IsNetConnect(const Event& event);

bool PathEndsWith(const Event& event, const std::string& suffix);

bool CommandLineContains(const Event& event, const std::string& needle);

std::string NormalizePath(const std::string& path);