#include "agent_rules.h"

#include <string>

#include "fields.h"

namespace nano_edr {

namespace {

bool MatchesImage(const Event& event, const std::string& name1, const std::string& name2) {
    if (!IsProcessStart(event)) {
        return false;
    }
    const std::string& image = GetRequiredField(event, "image");
    std::string norm_image = NormalizePath(image);
    return norm_image.ends_with(name1) || norm_image.ends_with(name2);
}

const std::string* GetTargetFilePath(const Event& event) {
    if (event.type == "file_move") {
        return FindField(event, "to");
    }
    if (event.type == "file_create" || event.type == "file_write") {
        return FindField(event, "path");
    }
    return nullptr;
}

}  // namespace

bool ScriptHostFromTemp(const Event& event) {
    if (!MatchesImage(event, "wscript.exe", "cscript.exe")) {
        return false;
    }
    return CommandLineContains(event, "\\appdata\\local\\temp\\") ||
           CommandLineContains(event, "\\windows\\temp\\");
}

bool LolbinDownload(const Event& event) {
    if (!MatchesImage(event, "certutil.exe", "bitsadmin.exe")) {
        return false;
    }
    return CommandLineContains(event, "urlcache") ||
           CommandLineContains(event, "transfer") ||
           CommandLineContains(event, "https:") ||
           CommandLineContains(event, "http:");
}

bool HiddenPowershell(const Event& event) {
    if (!MatchesImage(event, "powershell.exe", "pwsh.exe")) {
        return false;
    }
    return CommandLineContains(event, "-windowstyle hidden") ||
           CommandLineContains(event, "-encodedcommand") ||
           CommandLineContains(event, "-w hidden") ||
           CommandLineContains(event, "-enc");
}

bool AutostartWrite(const Event& event) {
    const std::string* path = GetTargetFilePath(event);
    if (!path) {
        return false;
    }
    std::string norm = NormalizePath(*path);
    return norm.find("\\start menu\\programs\\startup\\") != std::string::npos;
}

bool RansomExtension(const Event& event) {
    const std::string* path = GetTargetFilePath(event);
    if (!path) {
        return false;
    }
    std::string norm = NormalizePath(*path);
    return norm.ends_with(".locked");
}

constexpr Rule kRules[] = {
    {"script_host_from_temp", ScriptHostFromTemp, Severity::kHigh},
    {"lolbin_download", LolbinDownload, Severity::kHigh},
    {"hidden_powershell", HiddenPowershell, Severity::kMedium},
    {"autostart_write", AutostartWrite, Severity::kHigh},
    {"ransom_extension", RansomExtension, Severity::kCritical},
};

const Rule* AgentRules() {
    return kRules;
}

size_t AgentRuleCount() {
    return std::size(kRules);
}

}  // namespace nano_edr
