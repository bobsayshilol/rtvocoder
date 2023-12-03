#include "Pipewire.h"

namespace pipewire {

namespace {

struct roundtrip_data {
    pw_main_loop *loop;
    int pending;
};

static void on_core_done(void *data, uint32_t id, int seq) {
    auto *rd = static_cast<roundtrip_data *>(data);

    if (id == PW_ID_CORE && seq == rd->pending) {
        pw_main_loop_quit(rd->loop);
    }
}

static pw_core_events const core_events = {
    .version = PW_VERSION_CORE_EVENTS,
    .info = nullptr,
    .done = on_core_done,
    .ping = nullptr,
    .error = nullptr,
    .remove_id = nullptr,
    .bound_id = nullptr,
    .add_mem = nullptr,
    .remove_mem = nullptr,
    .bound_props = nullptr,
};

}  // namespace

namespace {

struct RegistryEventData {
    std::vector<Device> devices;
    Direction dir;
};

static void registry_event_global(void *data, uint32_t /*id*/,
                                  uint32_t /*permissions*/, char const *type,
                                  uint32_t /*version*/, spa_dict const *props) {
    if (false) {
        spa_dict_item const *item;
        printf("%s\n", type);
        spa_dict_for_each(item, props) {
            printf("  %s: %s\n", item->key, item->value);
        }
    }

    // These seem to correspond to the sink/source names in pulse.
    if (strcmp(type, "PipeWire:Interface:Node")) {
        return;
    }

    auto *devices = static_cast<RegistryEventData *>(data);
    auto &device = devices->devices.emplace_back();

    // Extract the info we want.
    spa_dict_item const *item;
    spa_dict_for_each(item, props) {
        if (!strcmp(item->key, "node.description")) {
            device.description = item->value;
        } else if (!strcmp(item->key, "node.name")) {
            device.name = item->value;
        } else if (!strcmp(item->key, "object.serial")) {
            device.id = std::atoi(item->value);
        }
    }
}

static pw_registry_events const registry_events = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = registry_event_global,
    .global_remove = nullptr,
};

}  // namespace

std::vector<Device> fetch_devices(pw_context *context, pw_main_loop *loop,
                                  Direction dir) {
    if (dir == Direction::Out) {
        // TODO
        return {};
    }

    auto *core = pw_context_connect(context, nullptr, 0);
    auto *registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);

    RegistryEventData devices;
    devices.dir = dir;

    spa_hook registry_listener;
    spa_zero(registry_listener);
    pw_registry_add_listener(registry, &registry_listener, &registry_events,
                             &devices);

    roundtrip_data data;
    data.loop = loop;

    spa_hook core_listener;
    spa_zero(core_listener);
    pw_core_add_listener(core, &core_listener, &core_events, &data);
    data.pending = pw_core_sync(core, PW_ID_CORE, 0);

    pw_main_loop_run(loop);

    spa_hook_remove(&core_listener);
    pw_proxy_destroy((pw_proxy *)registry);
    pw_core_disconnect(core);

    return devices.devices;
}

}  // namespace pipewire
