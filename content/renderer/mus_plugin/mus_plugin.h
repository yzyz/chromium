#ifndef CONTENT_RENDERER_MUS_PLUGIN_MUS_PLUGIN_H_
#define CONTENT_RENDERER_MUS_PLUGIN_MUS_PLUGIN_H_

#include "third_party/WebKit/public/web/WebPlugin.h"

namespace content {

class MusPlugin : public blink::WebPlugin {
 public:
  bool Initialize(blink::WebPluginContainer* container) override;
  void Destroy() override;
  blink::WebPluginContainer* Container() const override;
  void UpdateAllLifecyclePhases() override;
  void Paint(blink::WebCanvas* canvas, const blink::WebRect& rect) override;
  void UpdateGeometry(const blink::WebRect& window_rect,
                      const blink::WebRect& clip_rect,
                      const blink::WebRect& unobscured_rect,
                      bool is_visible) override;
  void UpdateFocus(bool focused, blink::WebFocusType focus_type) override;
  void UpdateVisibility(bool visible) override;
  blink::WebInputEventResult HandleInputEvent(
      const blink::WebCoalescedInputEvent& event,
      blink::WebCursorInfo& cursor_info) override;
  void DidReceiveResponse(const blink::WebURLResponse& response) override;
  void DidReceiveData(const char* data, int data_length) override;
  void DidFinishLoading() override;
  void DidFailLoading(const blink::WebURLError& error) override;
  MusPlugin();

 private:
  blink::WebPluginContainer* container_;
};

} // namespace content

#endif // CONTENT_RENDERER_MUS_PLUGIN_MUS_PLUGIN_H_
