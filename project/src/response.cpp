#include "response.h"

#include <cctype>
#include <print>
#include <utility>

#include "fields.h"

namespace {

bool IsSystemPath(const std::string& path) {
    const std::string windows = "C:\\Windows\\";
    const std::string programs = "C:\\Program Files";
    for (const std::string* prefix : {&windows, &programs}) {
        if (path.size() >= prefix->size()) {
            bool matches = true;
            for (std::size_t index = 0; index < prefix->size(); ++index) {
                const unsigned char left = static_cast<unsigned char>(path[index]);
                const unsigned char right = static_cast<unsigned char>((*prefix)[index]);
                if (std::tolower(left) != std::tolower(right)) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                return true;
            }
        }
    }
    return false;
}

void AddFileAction(nano_edr::ResponsePlan* plan, nano_edr::Action action) {
    if (action.path.empty() || IsSystemPath(action.path)) {
        return;
    }
    if (action.kind == nano_edr::ActionKind::kQuarantineFile && action.to.empty()) {
        return;
    }
    plan->Add(std::move(action));
}

}  // namespace

namespace nano_edr {

ResponsePlanner::ResponsePlanner(const EntityModel* model,
                                 std::string quarantine_dir)
    : model_(model), quarantine_dir_(std::move(quarantine_dir)) {}

std::string ResponsePlanner::QuarantinePath(const std::string& path,
                                            const std::string& pid) const {
    if (quarantine_dir_.empty()) {
        return "";
    }
    const std::size_t separator = path.find_last_of('\\');
    const std::size_t name_begin = separator == std::string::npos ? 0 : separator + 1;
    std::string result = quarantine_dir_;
    if (result.back() != '\\') {
        result += '\\';
    }
    result += pid;
    result += '-';
    result.append(path, name_begin, std::string::npos);
    return result;
}

ResponsePlan ResponsePlanner::Plan(const Detection& detection) const {
    ResponsePlan plan;
    if (model_ == nullptr) {
        return plan;
    }
    const std::vector<std::string> tree = model_->GetProcessTree(detection.pid);
    for (const std::string& name : detection.actions) {
        if (name == "kill_process") {
            for (std::size_t index = tree.size(); index > 0; --index) {
                const std::string& pid = tree[index - 1];
                Action action;
                action.pid = pid;
                action.expected_start = model_->FindProcess(pid)->start;
                action.reason = detection.rule;
                if (pid != detection.pid) {
                    action.reason += ", потомок " + detection.pid;
                }
                plan.Add(std::move(action));
            }
        } else if (name == "quarantine_source" || name == "quarantine_image") {
            const std::string* path = nullptr;
            if (name == "quarantine_source") {
                const FileRecord* source = model_->FindSpawnSource(detection.pid);
                if (source != nullptr) {
                    path = &source->path;
                }
            } else {
                const ProcessRecord* process = model_->FindProcess(detection.pid);
                if (process != nullptr) {
                    path = &process->image;
                }
            }
            if (path != nullptr) {
                Action action;
                action.kind = ActionKind::kQuarantineFile;
                action.path = *path;
                action.to = QuarantinePath(*path, detection.pid);
                action.reason = detection.rule + (name == "quarantine_source"
                                                      ? ", источник запуска"
                                                      : ", образ процесса");
                AddFileAction(&plan, std::move(action));
            }
        } else if (name == "delete_dropped") {
            for (const std::string& pid : tree) {
                for (const std::string& path : model_->GetFilesCreatedBy(pid, true)) {
                    Action action;
                    action.kind = ActionKind::kDeleteFile;
                    action.path = path;
                    action.reason = detection.rule + ", принесён " + pid;
                    AddFileAction(&plan, std::move(action));
                }
            }
        } else if (name == "delete_evidence") {
            for (const std::unique_ptr<Event>& event : detection.evidence) {
                const std::string* path = FindField(*event,
                                                    event->event_type() == EventType::kFileMove ? "to" : "path");
                if (path == nullptr) {
                    continue;
                }
                Action action;
                action.kind = ActionKind::kDeleteFile;
                action.path = *path;
                action.reason = detection.rule + ", файл из детекта";
                AddFileAction(&plan, std::move(action));
            }
        }
    }
    return plan;
}

Responder::Responder(OsHandle* os, bool dry_run) : os_(os), dry_run_(dry_run) {}

void Responder::Execute(const ResponsePlan& plan) {
    for (const Action& action : plan.actions()) {
        ++stats_.planned;
        bool repeat = false;
        for (const std::string& target : done_) {
            if (target == action.target()) {
                repeat = true;
                break;
            }
        }
        if (repeat) {
            ++stats_.skipped_repeat;
            continue;
        }
        done_.push_back(action.target());
        if (dry_run_ || os_ == nullptr) {
            std::print("[PLAN] {}\n", ToString(action));
            continue;
        }
        switch (action.kind) {
            case ActionKind::kKillProcess:
                Kill(action);
                break;
            case ActionKind::kQuarantineFile:
                MoveToQuarantine(action);
                break;
            case ActionKind::kDeleteFile:
                Delete(action);
                break;
        }
    }
}

}  // namespace nano_edr
