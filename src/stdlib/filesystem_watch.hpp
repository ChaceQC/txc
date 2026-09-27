#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace tx_generated
{

struct fs_watcher_state;
using fs_watcher = std::shared_ptr<fs_watcher_state>;

struct watch_event_value
{
    std::string kind;
    std::string path;
};

fs_watcher fs_watch(const std::string& path, bool recursive);
watch_event_value fs_watch_next(const fs_watcher& watcher,
                                std::int64_t timeout_millis);
void fs_close_watch(const fs_watcher& watcher);

} // namespace tx_generated
