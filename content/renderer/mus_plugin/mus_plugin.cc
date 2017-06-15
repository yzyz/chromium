#include "content/renderer/mus_plugin/mus_plugin.h"

#include "third_party/WebKit/public/web/WebPlugin.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"

namespace content {

bool MusPlugin::Initialize(blink::WebPluginContainer* container) {
  LOG(ERROR) << "Initialize";
  container_ = container;
  return true;
}

void MusPlugin::Destroy() {
  LOG(ERROR) << "Destroy";
}

blink::WebPluginContainer* MusPlugin::Container() const {
  return container_;
}

void MusPlugin::UpdateAllLifecyclePhases() {
  LOG(ERROR) << "UpdateAllLifecyclePhases";
}

void MusPlugin::Paint(blink::WebCanvas* canvas, const blink::WebRect& rect) {
  LOG(ERROR) << "Paint";
}

void MusPlugin::UpdateGeometry(const blink::WebRect& window_rect,
                    const blink::WebRect& clip_rect,
                    const blink::WebRect& unobscured_rect,
                    bool is_visible) {
  LOG(ERROR) << "UpdateGeometry";
}

void MusPlugin::UpdateFocus(bool focused, blink::WebFocusType focus_type) {
  LOG(ERROR) << "UpdateFocus";
}

void MusPlugin::UpdateVisibility(bool visible) {
  LOG(ERROR) << "UpdateVisibility";
}

blink::WebInputEventResult MusPlugin::HandleInputEvent(
    const blink::WebCoalescedInputEvent& event,
    blink::WebCursorInfo& cursor_info) {
  LOG(ERROR) << "HandleInputEvent";
  return blink::WebInputEventResult::kNotHandled;
}

void MusPlugin::DidReceiveResponse(const blink::WebURLResponse& response) {
  LOG(ERROR) << "DidReceiveResponse";
}

void MusPlugin::DidReceiveData(const char* data, int data_length) {
  LOG(ERROR) << "DidReceiveData";
}

void MusPlugin::DidFinishLoading() {
  LOG(ERROR) << "DidReceiveLoading";
}

void MusPlugin::DidFailLoading(const blink::WebURLError& error) {
  LOG(ERROR) << "DidReceiveLoading";
}

MusPlugin::MusPlugin() {
  LOG(ERROR) << "Created mus plugin";
}

} // namespace content
