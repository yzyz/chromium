#include "content/renderer/mus_plugin/mus_plugin.h"

#include <map>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "cc/blink/web_layer_impl.h"
#include "cc/layers/surface_layer.h"
#include "cc/layers/solid_color_layer.h"
#include "cc/surfaces/stub_surface_reference_factory.h"
#include "components/viz/common/surfaces/local_surface_id_allocator.h"
#include "content/renderer/mus/renderer_window_tree_client.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_widget.h"
#include "content/common/render_widget_window_tree_client_factory.mojom.h"
#include "content/common/mus_plugin_window_tree_client_factory.mojom.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPlugin.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"

namespace content {

void EmbedCallback(bool result) {
  if (!result)
    LOG(ERROR) << "embed failed";
}

namespace {
typedef std::map<ui::Id, MusPlugin*> WindowIdMap;
base::LazyInstance<WindowIdMap>::Leaky g_window_id_map =
    LAZY_INSTANCE_INITIALIZER;
} // namespace

// static
MusPlugin* MusPlugin::Get(ui::Id window_id) {
  auto it = g_window_id_map.Get().find(window_id);
  if (it != g_window_id_map.Get().end())
    return it->second;
  return nullptr;
}

void MusPlugin::SetSurfaceInfo(const viz::SurfaceInfo& surface_info) {
  LOG(ERROR) << "SetSurfaceInfo";
  LOG(ERROR) << "id: " << surface_info.id();
  LOG(ERROR) << "device_scale_factor: " << surface_info.device_scale_factor();
  LOG(ERROR) << "width: " << surface_info.size_in_pixels().width();
  LOG(ERROR) << "height: " << surface_info.size_in_pixels().height();
  LOG(ERROR) << "is valid: " << surface_info.is_valid();
  scoped_refptr<cc::SurfaceLayer> surface_layer =
      cc::SurfaceLayer::Create(new cc::StubSurfaceReferenceFactory());
  surface_layer->SetIsDrawable(true);
  surface_layer->SetPrimarySurfaceInfo(surface_info);
  surface_layer->SetFallbackSurfaceInfo(surface_info);
  surface_layer->SetBounds(surface_info.size_in_pixels());
  LOG(ERROR) << "layer width: " << surface_layer->bounds().width();
  LOG(ERROR) << "layer height: " << surface_layer->bounds().height();

  scoped_refptr<cc::SolidColorLayer> solid_color_layer =
      cc::SolidColorLayer::Create();
  solid_color_layer->SetBackgroundColor(0xFFFF0000);

  //cc_blink::WebLayerImpl* web_layer = new cc_blink::WebLayerImpl(surface_layer);
  cc_blink::WebLayerImpl* web_layer =
      new cc_blink::WebLayerImpl(solid_color_layer);
  LOG(ERROR) << "weblayer width: " << web_layer->Bounds().width;
  LOG(ERROR) << "weblayer height: " << web_layer->Bounds().height;
  LOG(ERROR) << "DrawsContent: " << web_layer->DrawsContent();
  LOG(ERROR) << "Opacity: " << web_layer->Opacity();
  container_->SetWebLayer(web_layer);
}

bool MusPlugin::Initialize(blink::WebPluginContainer* container) {
  LOG(ERROR) << "Initialize";
  container_ = container;

  blink::WebDocument document = container->GetDocument();
  blink::WebFrame* web_frame = document.GetFrame();
  RenderFrameImpl* render_frame = RenderFrameImpl::FromWebFrame(web_frame);
  RenderWidget* render_widget = render_frame->GetRenderWidget();
  int routing_id = render_widget->routing_id();

  RendererWindowTreeClient* client = RendererWindowTreeClient::Get(routing_id);
  ui::mojom::WindowTree* tree = client->GetWindowTree();

  if (!tree) {
    LOG(ERROR) << "tree does not exist";
    return false;
  }

  g_window_id_map.Get().insert(std::make_pair(17, this));
  tree->NewWindow(20, 17, base::nullopt);
  tree->AddWindow(21, client->root_window_id(), 17);
  viz::LocalSurfaceIdAllocator allocator;
  tree->SetWindowBounds(22, 17, gfx::Rect(10, 20, 0, 0), allocator.GenerateId());
  tree->SetWindowVisibility(23, 17, true);

  mojom::MusPluginWindowTreeClientFactoryPtr factory;
  ChildThread::Get()->GetConnector()->BindInterface(
      mojom::kBrowserServiceName, mojo::MakeRequest(&factory));

  // create a connection from renderer to browser
  // create a WindowTreeClient
  // browser launches desired mus app, gives it WindowTreeClientRequest
  // gives WindowTreeClientPtr to window server

  ui::mojom::WindowTreeClientPtr window_tree_client;
  factory->CreateWindowTreeClientForMusPlugin(
      routing_id, mojo::MakeRequest(&window_tree_client));
  tree->Embed(17, std::move(window_tree_client),
              ui::mojom::kEmbedFlagEmbedderInterceptsEvents,
              base::Bind(&EmbedCallback));
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
