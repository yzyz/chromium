#include "content/browser/mus_plugin/mus_plugin_window_tree_client_factory.h"

#include "content/common/mus_plugin_window_tree_client_factory.mojom.h"

namespace content {

// static
void MusPluginWindowTreeClientFactoryImpl::CreateNewFactory(
    mojom::MusPluginWindowTreeClientFactoryRequest request) {
  new MusPluginWindowTreeClientFactoryImpl(std::move(request));
  LOG(ERROR) << "WE HAVE CREATED A NEW MusPluginWindowTreeClientFactoryImpl";
}

void MusPluginWindowTreeClientFactoryImpl::CreateWindowTreeClientForMusPlugin(
    uint32_t routing_id, ui::mojom::WindowTreeClientRequest request) {
  LOG(ERROR) << "CreateWindowTreeClientForMusPlugin";
}

MusPluginWindowTreeClientFactoryImpl::MusPluginWindowTreeClientFactoryImpl(
    mojom::MusPluginWindowTreeClientFactoryRequest request)
    : binding_(this, std::move(request)) {
}

MusPluginWindowTreeClientFactoryImpl::~MusPluginWindowTreeClientFactoryImpl() {
}

} // namespace content
