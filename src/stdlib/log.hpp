#pragma once

#include "stdlib/dictionary.hpp"

#include <string>
#include <vector>

namespace tx_generated
{

void tx_log_event(const std::string& level, const std::string& message,
                  const tx_dict& fields,
                  const std::vector<std::string>& secret_keys);
void tx_log_set_context(const std::string& task_id,
                        const std::string& thread_id);

} // namespace tx_generated
