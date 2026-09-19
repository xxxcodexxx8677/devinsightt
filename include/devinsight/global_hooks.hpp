#ifndef DEVINSIGHT_GLOBAL_HOOKS_HPP
#define DEVINSIGHT_GLOBAL_HOOKS_HPP

#include "devinsight/memory_tracker.hpp"

namespace devinsight {

// Enable or disable interception of global operator new / delete
void set_global_hooks_enabled(bool enabled);
bool is_global_hooks_enabled();

} // namespace devinsight

#endif // DEVINSIGHT_GLOBAL_HOOKS_HPP
