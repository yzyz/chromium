#include "ui/views/mus/mus_focus_rules.h"

namespace views {

bool MusFocusRules::SupportsChildActivation(aura::Window* window) const {
  return true;
}

} // namespace views
