#include "agent_rules.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "combinators.h"
#include "conditions.h"
#include "entity_model.h"
#include "event.h"
#include "make_rule.h"
#include "rule.h"
#include "rules.h"

namespace nano_edr {

namespace {

std::unique_ptr<AnyOf> BuildTempPathCondition() {
    auto temp = std::make_unique<AnyOf>();
    temp->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));
    return temp;
}

std::unique_ptr<MatchRule> BuildScriptHostFromTempRule() {
    auto rule = std::make_unique<MatchRule>("script_host_from_temp", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));

    auto hosts = std::make_unique<AnyOf>();
    hosts->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    hosts->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    rule->AddCondition(std::move(hosts));

    auto temp = BuildTempPathCondition();
    rule->AddCondition(std::move(temp));

    rule->AddAction("kill_process");
    rule->AddAction("quarantine_source");
    rule->AddAction("delete_dropped");
    return rule;
}

std::unique_ptr<MatchRule> BuildLolbinDownloadRule() {
    auto rule = std::make_unique<MatchRule>("lolbin_download", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));

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
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));

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
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite, EventType::kFileMove}));

    auto paths = std::make_unique<AnyOf>();
    paths->Add(std::make_unique<FieldContains>("path", "\\start menu\\programs\\startup\\"));
    paths->Add(std::make_unique<FieldContains>("path", "/start menu/programs/startup/"));
    paths->Add(std::make_unique<FieldContains>("to", "\\start menu\\programs\\startup\\"));
    paths->Add(std::make_unique<FieldContains>("to", "/start menu/programs/startup/"));
    rule->AddCondition(std::move(paths));

    rule->AddAction("delete_evidence");
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

    rule->AddAction("kill_process");
    rule->AddAction("quarantine_image");
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

    auto temp = BuildTempPathCondition();
    first->Add(std::move(temp));

    beacon->SetFirst(std::move(first));

    beacon->SetSecond(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kNetConnect}));

    beacon->AddAction("kill_process");
    return beacon;
}

std::unique_ptr<ThresholdRule<128>> BuildThresholdRule() {
    auto mass_write = MakeRule<ThresholdRule<128>>(
        "ransomware_mass_write", Severity::kCritical,
        20,
        10000,
        "pid");
    mass_write->AddAction("kill_process");
    mass_write->AddAction("quarantine_image");
    mass_write->SetCondition(MakeCondition(AllOfStatic(
        EventTypeIs({EventType::kFileCreate, EventType::kFileWrite,
                     EventType::kFileMove}),
        AnyOfStatic(FieldContains("path", "\\Documents\\"),
                    FieldContains("path", "\\Desktop\\"),
                    FieldContains("to", "\\Documents\\"),
                    FieldContains("to", "\\Desktop\\")))));
    return mass_write;
}

std::unique_ptr<SequenceRule> BuildShellSpawnsScriptHostRule(const EntityModel* model) {
    auto shell = MakeRule<SequenceRule>("shell_spawns_script_host",
                                        Severity::kHigh, 30000);
    shell->SetFirst(MakeCondition(AllOfStatic(
        EventTypeIs({EventType::kProcessStart}),
        AnyOfStatic(FieldEndsWith("image", "cmd.exe"),
                    FieldEndsWith("image", "powershell.exe"),
                    FieldEndsWith("image", "pwsh.exe")))));

    shell->SetSecond(MakeCondition(AllOfStatic(
        EventTypeIs({EventType::kProcessStart}),
        AnyOfStatic(FieldEndsWith("image", "wscript.exe"),
                    FieldEndsWith("image", "cscript.exe")))));

    shell->set_child_of_prev(true);
    shell->SetModel(model);
    shell->AddAction("kill_process");
    return shell;
}

std::unique_ptr<LambdaRule> BuildSpawnedFromDroppedFileRule(const EntityModel* model) {
    auto dropped = MakeRule<LambdaRule>(
        "spawned_from_dropped_file", Severity::kHigh,
        [model](const Event& event) {
            if (event.event_type() != EventType::kProcessStart) {
                return false;
            }
            const FileRecord* source = model->FindSpawnSource(event.pid());
            return source != nullptr && !source->writer_pid.empty() && source->writer_pid != event.pid();
        });
    dropped->AddAction("kill_process");
    dropped->AddAction("quarantine_source");
    dropped->AddAction("delete_dropped");
    return dropped;
}

}  // namespace

RuleEngine BuildAgentRules(const EntityModel* model) {
    RuleEngine engine;
    engine.AddRule(BuildScriptHostFromTempRule());
    engine.AddRule(BuildLolbinDownloadRule());
    engine.AddRule(BuildHiddenPowershellRule());
    engine.AddRule(BuildAutostartWritePathRule());
    engine.AddRule(BuildRansomExtensionPathRule());
    engine.AddRule(BuildScriptHostBeaconRule());
    engine.AddRule(BuildThresholdRule());
    engine.AddRule(BuildShellSpawnsScriptHostRule(model));
    engine.AddRule(BuildSpawnedFromDroppedFileRule(model));
    return engine;
}

}  // namespace nano_edr
