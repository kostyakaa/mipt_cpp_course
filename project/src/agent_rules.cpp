#include "agent_rules.h"

#include <memory>
#include <utility>
#include <vector>

#include "conditions.h"
#include "event.h"
#include "rule.h"
#include "rules.h"

namespace {

std::unique_ptr<nano_edr::ICondition> MakeScriptHostCondition() {
    auto hosts = std::make_unique<nano_edr::AnyOf>();
    hosts->Add(std::make_unique<nano_edr::FieldEndsWith>("image",
                                                         "wscript.exe"));
    hosts->Add(std::make_unique<nano_edr::FieldEndsWith>("image",
                                                         "cscript.exe"));
    return hosts;
}

std::unique_ptr<nano_edr::ICondition> MakeTempPathCondition() {
    auto temp = std::make_unique<nano_edr::AnyOf>();
    temp->Add(std::make_unique<nano_edr::FieldContains>(
        "cmdline", "\\appdata\\local\\temp\\"));
    temp->Add(std::make_unique<nano_edr::FieldContains>(
        "cmdline", "/appdata/local/temp/"));
    temp->Add(std::make_unique<nano_edr::FieldContains>(
        "cmdline", "\\windows\\temp\\"));
    temp->Add(std::make_unique<nano_edr::FieldContains>(
        "cmdline", "/windows/temp/"));
    temp->Add(
        std::make_unique<nano_edr::FieldContains>("cmdline", "%temp%"));
    temp->Add(
        std::make_unique<nano_edr::FieldContains>("cmdline", "%tmp%"));
    return temp;
}

}  // namespace

namespace nano_edr {

RuleEngine BuildAgentRules() {
    RuleEngine engine;

    auto script_host =
        std::make_unique<MatchRule>("script_host_from_temp", Severity::kHigh);
    script_host->AddCondition(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kProcessStart}));
    script_host->AddCondition(MakeScriptHostCondition());
    script_host->AddCondition(MakeTempPathCondition());
    script_host->AddAction("kill_process");
    engine.AddRule(std::move(script_host));

    auto lolbin =
        std::make_unique<MatchRule>("lolbin_download", Severity::kHigh);
    lolbin->AddCondition(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kProcessStart}));
    auto lolbin_images = std::make_unique<AnyOf>();
    lolbin_images->Add(
        std::make_unique<FieldEndsWith>("image", "certutil.exe"));
    lolbin_images->Add(
        std::make_unique<FieldEndsWith>("image", "bitsadmin.exe"));
    lolbin->AddCondition(std::move(lolbin_images));
    auto download_arguments = std::make_unique<AnyOf>();
    download_arguments->Add(
        std::make_unique<FieldContains>("cmdline", "urlcache"));
    download_arguments->Add(
        std::make_unique<FieldContains>("cmdline", "transfer"));
    download_arguments->Add(
        std::make_unique<FieldContains>("cmdline", "http:"));
    download_arguments->Add(
        std::make_unique<FieldContains>("cmdline", "https:"));
    lolbin->AddCondition(std::move(download_arguments));
    lolbin->AddAction("kill_process");
    engine.AddRule(std::move(lolbin));

    auto hidden =
        std::make_unique<MatchRule>("hidden_powershell", Severity::kMedium);
    hidden->AddCondition(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kProcessStart}));
    auto powershell_images = std::make_unique<AnyOf>();
    powershell_images->Add(
        std::make_unique<FieldEndsWith>("image", "powershell.exe"));
    powershell_images->Add(
        std::make_unique<FieldEndsWith>("image", "pwsh.exe"));
    hidden->AddCondition(std::move(powershell_images));
    auto hidden_arguments = std::make_unique<AnyOf>();
    hidden_arguments->Add(
        std::make_unique<FieldContains>("cmdline", "-w hidden"));
    hidden_arguments->Add(std::make_unique<FieldContains>(
        "cmdline", "-windowstyle hidden"));
    hidden_arguments->Add(
        std::make_unique<FieldContains>("cmdline", "-enc"));
    hidden_arguments->Add(
        std::make_unique<FieldContains>("cmdline", "-encodedcommand"));
    hidden->AddCondition(std::move(hidden_arguments));
    hidden->AddAction("kill_process");
    engine.AddRule(std::move(hidden));

    auto autostart =
        std::make_unique<MatchRule>("autostart_write", Severity::kHigh);
    autostart->AddCondition(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite,
                               EventType::kFileMove}));
    auto startup_paths = std::make_unique<AnyOf>();
    startup_paths->Add(std::make_unique<FieldContains>(
        "path", "\\start menu\\programs\\startup\\"));
    startup_paths->Add(std::make_unique<FieldContains>(
        "path", "/start menu/programs/startup/"));
    startup_paths->Add(std::make_unique<FieldContains>(
        "to", "\\start menu\\programs\\startup\\"));
    startup_paths->Add(std::make_unique<FieldContains>(
        "to", "/start menu/programs/startup/"));
    autostart->AddCondition(std::move(startup_paths));
    autostart->AddAction("quarantine_file");
    engine.AddRule(std::move(autostart));

    auto ransom =
        std::make_unique<MatchRule>("ransom_extension", Severity::kCritical);
    ransom->AddCondition(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite,
                               EventType::kFileMove}));
    auto locked_paths = std::make_unique<AnyOf>();
    locked_paths->Add(
        std::make_unique<FieldEndsWith>("path", ".locked"));
    locked_paths->Add(std::make_unique<FieldEndsWith>("to", ".locked"));
    ransom->AddCondition(std::move(locked_paths));
    ransom->AddAction("quarantine_file");
    engine.AddRule(std::move(ransom));

    auto beacon = std::make_unique<SequenceRule>(
        "script_host_beacon", Severity::kCritical, 10000);
    auto beacon_start = std::make_unique<AllOf>();
    beacon_start->Add(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kProcessStart}));
    beacon_start->Add(MakeScriptHostCondition());
    beacon_start->Add(MakeTempPathCondition());
    beacon->SetFirst(std::move(beacon_start));
    beacon->SetSecond(std::make_unique<EventTypeIs>(
        std::vector<EventType>{EventType::kNetConnect}));
    beacon->AddAction("kill_process");
    engine.AddRule(std::move(beacon));

    return engine;
}

}  // namespace nano_edr
