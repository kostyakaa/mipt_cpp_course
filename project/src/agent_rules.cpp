#include "agent_rules.h"

#include "fields.h"

namespace nano_edr {
namespace {

bool ImageIs(const Event& event, const std::string& filename) {
    if (!IsProcessStart(event)) {
        return false;
    }

    const std::string image = NormalizePath(GetRequiredField(event, "image"));
    const std::string normalized_filename = NormalizePath(filename);

    return image == normalized_filename || image.ends_with("\\" + normalized_filename);
}

const std::string* DestinationPath(const Event& event) {
    if (event.type == "file_create" || event.type == "file_write") {
        return FindField(event, "path");
    }

    if (event.type == "file_move") {
        return FindField(event, "to");
    }

    return nullptr;
}

bool ScriptHostFromTemp(const Event& event) {
    if (!ImageIs(event, "wscript.exe") && !ImageIs(event, "cscript.exe")) {
        return false;
    }

    const std::string* command_line = FindField(event, "cmdline");
    if (command_line == nullptr) {
        return false;
    }

    const std::string normalized = NormalizePath(*command_line);

    return normalized.contains("\\appdata\\local\\temp\\") || normalized.contains("\\windows\\temp\\");
}

bool LolbinDownload(const Event& event) {
    if (!ImageIs(event, "certutil.exe") && !ImageIs(event, "bitsadmin.exe")) {
        return false;
    }

    return CommandLineContains(event, "urlcache") || CommandLineContains(event, "transfer") || CommandLineContains(event, "http:") || CommandLineContains(event, "https:");
}

bool HiddenPowershell(const Event& event) {
    if (!ImageIs(event, "powershell.exe") && !ImageIs(event, "pwsh.exe")) {
        return false;
    }

    return CommandLineContains(event, "-w hidden") || CommandLineContains(event, "-windowstyle hidden") || CommandLineContains(event, "-enc") || CommandLineContains(event, "-encodedcommand");
}

bool AutostartWrite(const Event& event) {
    const std::string* path = DestinationPath(event);
    if (path == nullptr) {
        return false;
    }

    return NormalizePath(*path).contains("\\start menu\\programs\\startup\\");
}

bool RansomExtension(const Event& event) {
    const std::string* path = DestinationPath(event);
    if (path == nullptr) {
        return false;
    }

    return NormalizePath(*path).ends_with(".locked");
}

constexpr Rule kRules[] = {
    {
        "script_host_from_temp",
        ScriptHostFromTemp,
        Severity::kHigh,
    },
    {
        "lolbin_download",
        LolbinDownload,
        Severity::kHigh,
    },
    {
        "hidden_powershell",
        HiddenPowershell,
        Severity::kMedium,
    },
    {
        "autostart_write",
        AutostartWrite,
        Severity::kHigh,
    },
    {
        "ransom_extension",
        RansomExtension,
        Severity::kCritical,
    },
};

}  // namespace

const Rule* AgentRules() {
    return kRules;
}

size_t AgentRuleCount() {
    return sizeof(kRules) / sizeof(kRules[0]);
}

}  // namespace nano_edr
