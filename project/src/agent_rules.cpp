#include "agent_rules.h"

#include <memory>
#include <string>
#include <vector>

#include "conditions.h"
#include "event.h"
#include "rule.h"
#include "rules.h"

namespace nano_edr {

namespace {


std::unique_ptr<MatchRule> BuildScriptHostFromTempRule() {
    auto rule = std::make_unique<MatchRule>("script_host_from_temp", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType> {EventType::kProcessStart}));

    auto hosts = std::make_unique<AnyOf>();
    hosts->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    hosts->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    rule->AddCondition(std::move(hosts));

    auto temp = std::make_unique<AnyOf>();
    temp->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));
    rule->AddCondition(std::move(temp));


    rule->AddAction("kill_process");
    return rule;
}

std::unique_ptr<MatchRule> BuildLolbinDownloadRule() {
    auto rule = std::make_unique<MatchRule>("lolbin_download", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType> {EventType::kProcessStart}));

    auto lolbins = std::make_unique<AnyOf>();
    lolbins->Add(std::make_unique<FieldEndsWith>("image", "certutil.exe"));
    lolbins->Add(std::make_unique<FieldEndsWith>("image", "bitsadmin.exe"));
    rule->AddCondition(std::move(lolbins));

    auto args = std::make_unique<AnyOf>();
    args->Add(std::make_unique<FieldContains>("cmdline", "urlcache"));
    args->Add(std::make_unique<FieldContains>("cmdline", "transfer"));
    args->Add(std::make_unique<FieldContains>("cmdline", "http:"));
    args->Add(std::make_unique<FieldContains>("cmdline", "https:"));
    rule->AddCondition(std::move(args));

    rule->AddAction("kill_process");
    return rule;
}

std::unique_ptr<MatchRule> BuildHiddenPowershellRule() {
    auto rule = std::make_unique<MatchRule>("hidden_powershell", Severity::kMedium);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType> {EventType::kProcessStart}));

    auto shells = std::make_unique<AnyOf>();
    shells->Add(std::make_unique<FieldEndsWith>("image", "powershell.exe"));
    shells->Add(std::make_unique<FieldEndsWith>("image", "pwsh.exe"));
    rule->AddCondition(std::move(shells));
    
    auto args = std::make_unique<AnyOf>();
    args->Add(std::make_unique<FieldContains>("cmdline", "-windowstyle hidden"));
    args->Add(std::make_unique<FieldContains>("cmdline", "-encodedcommand"));
    args->Add(std::make_unique<FieldContains>("cmdline", "-w hidden"));
    args->Add(std::make_unique<FieldContains>("cmdline", "-enc"));
    rule->AddCondition(std::move(args));

    rule->AddAction("kill_process");
    return rule;
}

std::unique_ptr<MatchRule> BuildAutostartWritePathRule() {
    auto rule = std::make_unique<MatchRule>("autostart_write", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType> 
        {EventType::kFileCreate, EventType::kFileWrite, EventType::kFileMove}));

    auto paths = std::make_unique<AnyOf>();
    paths->Add(std::make_unique<FieldContains>("path", "\\start menu\\programs\\startup\\"));
    paths->Add(std::make_unique<FieldContains>("path", "/start menu/programs/startup/"));
    paths->Add(std::make_unique<FieldContains>("to", "\\start menu\\programs\\startup\\"));
    paths->Add(std::make_unique<FieldContains>("to", "/start menu/programs/startup/"));
    rule->AddCondition(std::move(paths));

    rule->AddAction("quarantine_file");
    return rule;
}

std::unique_ptr<MatchRule> BuildRansomExtensionPathRule() {
    auto rule = std::make_unique<MatchRule>("ransom_extension", Severity::kCritical);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{
        EventType::kFileCreate, EventType::kFileWrite, EventType::kFileMove}));

    auto paths = std::make_unique<AnyOf>();
    paths->Add(std::make_unique<FieldEndsWith>("path", ".locked"));
    paths->Add(std::make_unique<FieldEndsWith>("to", ".locked"));
    rule->AddCondition(std::move(paths));

    rule->AddAction("quarantine_file");
    return rule;
}

std::unique_ptr<SequenceRule> BuildScriptHostBeaconRule() {
    auto beacon = std::make_unique<SequenceRule>("script_host_beacon", Severity::kCritical, 10000);

    auto first = std::make_unique<AllOf>();
    first->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));

    auto hosts = std::make_unique<AnyOf>();
    hosts->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    hosts->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    first->Add(std::move(hosts));

    auto temp = std::make_unique<AnyOf>();
    temp->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));
    first->Add(std::move(temp));

    beacon->SetFirst(std::move(first));

    beacon->SetSecond(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kNetConnect}));

    beacon->AddAction("kill_process");
    return beacon;
}

}  // namespace

RuleEngine BuildAgentRules() {
    RuleEngine engine;
    engine.AddRule(BuildScriptHostFromTempRule());
    engine.AddRule(BuildLolbinDownloadRule());
    engine.AddRule(BuildHiddenPowershellRule());
    engine.AddRule(BuildAutostartWritePathRule());
    engine.AddRule(BuildRansomExtensionPathRule());
    engine.AddRule(BuildScriptHostBeaconRule()); 
    return engine;
}

}  // namespace nano_edr
