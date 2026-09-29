#pragma once

#include "stdlib/dictionary.hpp"

#include <string>
#include <cstdint>
#include <vector>

namespace tx_generated
{

void tx_log_event(const std::string& level, const std::string& message,
                  const tx_dict& fields,
                  const std::vector<std::string>& secret_keys);
void tx_log_set_context(const std::string& task_id,
                        const std::string& thread_id);
bool tx_log_enabled(const std::string& level);
void tx_log_set_level(const std::string& level);
void tx_log_set_file(const std::string& path, std::int64_t max_bytes, std::int64_t backups);
void tx_log_set_stderr();
void tx_log_flush();
void tx_log_write(const std::string& record);
void tx_log_set_secret_keys(std::vector<std::string> keys);
std::vector<std::string> tx_log_secret_keys();

} // namespace tx_generated
