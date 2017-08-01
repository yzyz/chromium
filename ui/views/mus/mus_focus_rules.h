#ifndef UI_VIEWS_MUS_MUS_FOCUS_RULES_H_
#define UI_VIEWS_MUS_MUS_FOCUS_RULES_H_

#include "ui/wm/core/base_focus_rules.h"

namespace views {

class MusFocusRules : public wm::BaseFocusRules {
 private:
  bool SupportsChildActivation(aura::Window* window) const override;
};

} // namespace views

#endif // UI_VIEWS_MUS_MUS_FOCUS_RULES_H_
