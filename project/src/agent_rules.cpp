#include "agent_rules.h"

#include <initializer_list>
#include <string>
#include <vector>

#include "conditions.h"
#include "rule.h"
#include "rules.h"

namespace nano_edr {

namespace {

MatchRule MakeRule(const std::string& id, Severity severity,
                   std::initializer_list<Condition> conditions,
                   const std::string& action) {
    MatchRule rule(id, severity);
    for (const auto& condition : conditions) {
        rule.AddCondition(condition);
    }
    if (!action.empty()) {
        rule.AddAction(action);
    }
    return rule;
}

}  // namespace

std::vector<MatchRule> BuildAgentRules() {
    return {
        MakeRule("script_host_from_temp", Severity::kHigh,
                 {
                     Condition::Equals("type", {"process_start"}),
                     Condition::EndsWith("image", {"wscript.exe", "cscript.exe"}),
                     Condition::Contains("cmdline", {
                         "\\appdata\\local\\temp\\", "/appdata/local/temp/",
                         "\\windows\\temp\\", "/windows/temp/",
                         "%temp%", "%tmp%",
                     }),
                 },
                 "kill_process"),
        MakeRule("lolbin_download", Severity::kHigh,
                 {
                     Condition::Equals("type", {"process_start"}),
                     Condition::EndsWith("image", {"certutil.exe", "bitsadmin.exe"}),
                     Condition::Contains("cmdline", {"urlcache", "transfer", "http:", "https:"}),
                 },
                 "kill_process"),
        MakeRule("hidden_powershell", Severity::kMedium,
                 {
                     Condition::Equals("type", {"process_start"}),
                     Condition::EndsWith("image", {"powershell.exe", "pwsh.exe"}),
                     Condition::Contains("cmdline", {
                         "-windowstyle hidden", "-encodedcommand",
                         "-w hidden", "-enc",
                     }),
                 },
                 "kill_process"),
        MakeRule("autostart_write", Severity::kHigh,
                 {
                     Condition::Equals("type", {"file_create", "file_write"}),
                     Condition::Contains("path", {
                         "\\start menu\\programs\\startup\\",
                         "/start menu/programs/startup/",
                     }),
                 },
                 "quarantine_file"),
        MakeRule("autostart_write", Severity::kHigh,
                 {
                     Condition::Equals("type", {"file_move"}),
                     Condition::Contains("to", {
                         "\\start menu\\programs\\startup\\",
                         "/start menu/programs/startup/",
                     }),
                 },
                 "quarantine_file"),
        MakeRule("ransom_extension", Severity::kCritical,
                 {
                     Condition::Equals("type", {"file_create", "file_write"}),
                     Condition::EndsWith("path", {".locked"}),
                 },
                 "quarantine_file"),
        MakeRule("ransom_extension", Severity::kCritical,
                 {
                     Condition::Equals("type", {"file_move"}),
                     Condition::EndsWith("to", {".locked"}),
                 },
                 "quarantine_file"),
    };
}

}  // namespace nano_edr
