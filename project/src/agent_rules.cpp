#include "agent_rules.h"

#include <initializer_list>
#include <string>
#include <vector>

#include "conditions.h"
#include "rule.h"
#include "rules.h"

namespace nano_edr {

namespace {
    
MatchRule BuildScriptHostFromTempRule() {
    MatchRule rule("script_host_from_temp", Severity::kHigh);
    rule.AddCondition(Condition::Equals("type", {"process_start"}));
    rule.AddCondition(Condition::EndsWith("image", {"wscript.exe", "cscript.exe"}));
    rule.AddCondition(Condition::Contains("cmdline", {
        "\\appdata\\local\\temp\\", "/appdata/local/temp/",
        "\\windows\\temp\\", "/windows/temp/",
        "%temp%", "%tmp%",
    }));
    rule.AddAction("kill_process");
    return rule;
}

MatchRule BuildLolbinDownloadRule() {
    MatchRule rule("lolbin_download", Severity::kHigh);
    rule.AddCondition(Condition::Equals("type", {"process_start"}));
    rule.AddCondition(Condition::EndsWith("image", {"certutil.exe", "bitsadmin.exe"}));
    rule.AddCondition(Condition::Contains("cmdline", {"urlcache", "transfer", "http:", "https:"}));
    rule.AddAction("kill_process");
    return rule;
}

MatchRule BuildHiddenPowershellRule() {
    MatchRule rule("hidden_powershell", Severity::kMedium);
    rule.AddCondition(Condition::Equals("type", {"process_start"}));
    rule.AddCondition(Condition::EndsWith("image", {"powershell.exe", "pwsh.exe"}));
    rule.AddCondition(Condition::Contains("cmdline", {
        "-windowstyle hidden", "-encodedcommand",
        "-w hidden", "-enc",
    }));
    rule.AddAction("kill_process");
    return rule;
}

MatchRule BuildAutostartWritePathRule() {
    MatchRule rule("autostart_write", Severity::kHigh);
    rule.AddCondition(Condition::Equals("type", {"file_create", "file_write"}));
    rule.AddCondition(Condition::Contains("path", {
        "\\start menu\\programs\\startup\\",
        "/start menu/programs/startup/",
    }));
    rule.AddAction("quarantine_file");
    return rule;
}

MatchRule BuildAutostartWriteToRule() {
    MatchRule rule("autostart_write", Severity::kHigh);
    rule.AddCondition(Condition::Equals("type", {"file_move"}));
    rule.AddCondition(Condition::Contains("to", {
        "\\start menu\\programs\\startup\\",
        "/start menu/programs/startup/",
    }));
    rule.AddAction("quarantine_file");
    return rule;
}

MatchRule BuildRansomExtensionPathRule() {
    MatchRule rule("ransom_extension", Severity::kCritical);
    rule.AddCondition(Condition::Equals("type", {"file_create", "file_write"}));
    rule.AddCondition(Condition::EndsWith("path", {".locked"}));
    rule.AddAction("quarantine_file");
    return rule;
}

MatchRule BuildRansomExtensionToRule() {
    MatchRule rule("ransom_extension", Severity::kCritical);
    rule.AddCondition(Condition::Equals("type", {"file_move"}));
    rule.AddCondition(Condition::EndsWith("to", {".locked"}));
    rule.AddAction("quarantine_file");
    return rule;
}

}  // namespace

std::vector<MatchRule> BuildAgentRules() {
    std::vector<MatchRule> rules;
    rules.reserve(7);
    rules.push_back(BuildScriptHostFromTempRule());
    rules.push_back(BuildLolbinDownloadRule());
    rules.push_back(BuildHiddenPowershellRule());
    rules.push_back(BuildAutostartWritePathRule());
    rules.push_back(BuildAutostartWriteToRule());
    rules.push_back(BuildRansomExtensionPathRule());
    rules.push_back(BuildRansomExtensionToRule());
    return rules;
}

}  // namespace nano_edr
