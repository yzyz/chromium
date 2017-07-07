#include "content/browser/mus_plugin/mus_plugin_window_tree_client_factory.h"

#include "content/common/mus_plugin_window_tree_client_factory.mojom.h"
#include "content/public/common/service_manager_connection.h"
#include "mash/public/interfaces/launchable.mojom.h"
#include "mash/quick_launch/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

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

  mash::mojom::LaunchablePtr launchable;
  service_manager::Connector* connector =
      ServiceManagerConnection::GetForProcess()->GetConnector();
  connector->BindInterface(mash::quick_launch::mojom::kServiceName, &launchable);
  launchable->Launch(mash::mojom::kWindow,
                     mash::mojom::LaunchMode::MAKE_NEW,
                     std::move(request));
}

MusPluginWindowTreeClientFactoryImpl::MusPluginWindowTreeClientFactoryImpl(
    mojom::MusPluginWindowTreeClientFactoryRequest request)
    : binding_(this, std::move(request)) {
}

MusPluginWindowTreeClientFactoryImpl::~MusPluginWindowTreeClientFactoryImpl() {
}

} // namespace content
