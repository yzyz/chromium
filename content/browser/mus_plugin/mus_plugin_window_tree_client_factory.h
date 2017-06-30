#ifndef CONTENT_BROWSER_MUS_PLUGIN_MUS_PLUGIN_WINDOW_TREE_CLIENT_FACTORY_H_
#define CONTENT_BROWSER_MUS_PLUGIN_MUS_PLUGIN_WINDOW_TREE_CLIENT_FACTORY_H_

#include "content/common/mus_plugin_window_tree_client_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

class MusPluginWindowTreeClientFactoryImpl
    : public mojom::MusPluginWindowTreeClientFactory {
 public:
  static void CreateNewFactory(
      mojom::MusPluginWindowTreeClientFactoryRequest request);

  void CreateWindowTreeClientForMusPlugin(
      uint32_t routing_id, ui::mojom::WindowTreeClientRequest request) override;

  MusPluginWindowTreeClientFactoryImpl(
      mojom::MusPluginWindowTreeClientFactoryRequest);
  ~MusPluginWindowTreeClientFactoryImpl() override;

 private:
  mojo::Binding<mojom::MusPluginWindowTreeClientFactory> binding_;
};

} // namespace content

#endif // CONTENT_BROWSER_MUS_PLUGIN_MUS_PLUGIN_WINDOW_TREE_CLIENT_FACTORY_H_
