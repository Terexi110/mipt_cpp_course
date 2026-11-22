#include <algorithm>
#include <cctype>
#include <cstddef>
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detection.h"
#include "entity_model.h"
#include "event.h"
#include "fields.h"
#include "os_handle.h"
#include "response.h"

namespace nano_edr {

namespace {

bool StartsWithIgnoreCase(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() &&
           std::ranges::equal(text.substr(0, prefix.size()), prefix,
                              [](unsigned char a, unsigned char b) {
                                  return std::tolower(a) == std::tolower(b);
                              });
}

bool IsSystemPath(const std::string& path) {
    return StartsWithIgnoreCase(path, "C:\\Windows\\") ||
           StartsWithIgnoreCase(path, "C:\\Program Files");
}

}  // namespace

ResponsePlanner::ResponsePlanner(const EntityModel* model,
                                 std::string quarantine_dir)
    : model_(model), quarantine_dir_(std::move(quarantine_dir)) {}

ResponsePlan ResponsePlanner::Plan(const Detection& detection) const {
    ResponsePlan plan;
    const std::vector<std::string> tree = model_->GetProcessTree(detection.pid);

    auto add_quarantine = [&](const std::string& path, const char* why) {
        if (path.empty() || IsSystemPath(path)) {
            return;
        }
        Action action;
        action.kind = ActionKind::kQuarantineFile;
        action.path = path;
        action.to = QuarantinePath(path, detection.pid);
        if (action.to.empty()) {
            return;
        }
        action.reason = detection.rule + why;
        plan.Add(std::move(action));
    };

    auto add_delete = [&](const std::string& path, std::string reason) {
        if (path.empty() || IsSystemPath(path)) {
            return;
        }
        Action action;
        action.kind = ActionKind::kDeleteFile;
        action.path = path;
        action.reason = std::move(reason);
        plan.Add(std::move(action));
    };

    for (const std::string& name : detection.actions) {
        if (name == "kill_process") {
            for (std::size_t i = tree.size(); i > 0; --i) {
                const std::string& pid = tree[i - 1];
                Action action;
                action.kind = ActionKind::kKillProcess;
                action.pid = pid;
                action.expected_start = model_->FindProcess(pid)->start;
                action.reason = detection.rule;
                if (pid != detection.pid) {
                    action.reason += ", потомок " + detection.pid;
                }
                plan.Add(std::move(action));
            }
        } else if (name == "quarantine_source") {
            const FileRecord* source = model_->FindSpawnSource(detection.pid);
            if (source != nullptr) {
                add_quarantine(source->path, ", источник запуска");
            }
        } else if (name == "quarantine_image") {
            const ProcessRecord* process = model_->FindProcess(detection.pid);
            if (process != nullptr) {
                add_quarantine(process->image, ", образ процесса");
            }
        } else if (name == "delete_dropped") {
            for (const std::string& pid : tree) {
                for (const std::string& path : model_->GetFilesCreatedBy(pid, true)) {
                    add_delete(path, detection.rule + ", принесён " + pid);
                }
            }
        } else if (name == "delete_evidence") {
            for (const auto& event : detection.evidence) {
                const std::string* path = FindField(
                    *event, event->event_type() == EventType::kFileMove ? "to" : "path");
                if (path != nullptr) {
                    add_delete(*path, detection.rule + ", файл из детекта");
                }
            }
        }
    }
    return plan;
}

std::string ResponsePlanner::QuarantinePath(const std::string& path,
                                            const std::string& pid) const {
    if (quarantine_dir_.empty()) {
        return "";
    }
    const std::size_t slash = path.find_last_of('\\');
    const std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
    return quarantine_dir_ + "\\" + pid + "-" + name;
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
