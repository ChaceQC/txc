#pragma once

#include <any>
#include <memory>

namespace tx_generated
{

using gc_visit = void (*)(const std::any&, void*);
using gc_trace = void (*)(const void*, gc_visit, void*);
using gc_clear = void (*)(void*);
using gc_finalize = bool (*)(const std::shared_ptr<void>&);

void register_gc_node(const std::shared_ptr<void>& object, gc_trace trace,
                      gc_clear clear, gc_finalize finalize = nullptr);
void note_gc_allocation() noexcept;
void collect_cycles();
void gc_safepoint();

} // namespace tx_generated

extern "C" int txrt_gc_safepoint() noexcept;
