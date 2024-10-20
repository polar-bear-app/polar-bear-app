//
// Created by Nguyễn Hồng Phát on 12/10/24.
//
#include <string>
#include <unistd.h>
#include <android/log.h>
#include <sys/socket.h>
#include <assert.h>

#include "wayland/src/wayland-server.h"

#define LOG_TAG "PolarBearCompositorImplementations"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static void
surface_attach(struct wl_client *client,
               struct wl_resource *resource,
               struct wl_resource *buffer_resource, int32_t sx, int32_t sy) {
    if (buffer_resource) {
        auto buffer = wl_shm_buffer_get(buffer_resource);
        if (buffer == NULL) {
            wl_client_post_no_memory(client);
            return;
        }
    }

    if (wl_resource_get_version(resource) >= WL_SURFACE_OFFSET_SINCE_VERSION) {
        if (sx != 0 || sy != 0) {
            wl_resource_post_error(resource,
                                   WL_SURFACE_ERROR_INVALID_OFFSET,
                                   "Can't attach with an offset");
            return;
        }
    }
}

// static void
// surface_damage(struct wl_client *client,
//                struct wl_resource *resource,
//                int32_t x, int32_t y, int32_t width, int32_t height)
//{
//     struct weston_surface *surface = wl_resource_get_user_data(resource);
//
//     if (width <= 0 || height <= 0)
//         return;
//
//     pixman_region32_union_rect(&surface->pending.damage_surface,
//                                &surface->pending.damage_surface,
//                                x, y, width, height);
// }
// static void
// surface_commit(struct wl_client *client, struct wl_resource *resource)
//{
//     struct weston_surface *surface = wl_resource_get_user_data(resource);
//     struct weston_subsurface *sub = weston_surface_to_subsurface(surface);
//     enum weston_surface_status status;
//
//     if (!weston_surface_is_pending_viewport_source_valid(surface)) {
//         assert(surface->viewport_resource);
//
//         wl_resource_post_error(surface->viewport_resource,
//                                WP_VIEWPORT_ERROR_OUT_OF_BUFFER,
//                                "wl_surface@%d has viewport source outside buffer",
//                                wl_resource_get_id(resource));
//         return;
//     }
//
//     if (!weston_surface_is_pending_viewport_dst_size_int(surface)) {
//         assert(surface->viewport_resource);
//
//         wl_resource_post_error(surface->viewport_resource,
//                                WP_VIEWPORT_ERROR_BAD_SIZE,
//                                "wl_surface@%d viewport dst size not integer",
//                                wl_resource_get_id(resource));
//         return;
//     }
//
//     if (surface->pending.acquire_fence_fd >= 0) {
//         assert(surface->synchronization_resource);
//
//         if (!surface->pending.buffer) {
//             fd_clear(&surface->pending.acquire_fence_fd);
//             wl_resource_post_error(surface->synchronization_resource,
//                                    ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_NO_BUFFER,
//                                    "wl_surface@%"PRIu32" no buffer for synchronization",
//                                    wl_resource_get_id(resource));
//             return;
//         }
//
//         if (surface->pending.buffer->type == WESTON_BUFFER_SHM) {
//             fd_clear(&surface->pending.acquire_fence_fd);
//             wl_resource_post_error(surface->synchronization_resource,
//                                    ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_UNSUPPORTED_BUFFER,
//                                    "wl_surface@%"PRIu32" unsupported buffer for synchronization",
//                                    wl_resource_get_id(resource));
//             return;
//         }
//     }
//
//     if (surface->pending.buffer_release_ref.buffer_release &&
//         !surface->pending.buffer) {
//         assert(surface->synchronization_resource);
//
//         wl_resource_post_error(surface->synchronization_resource,
//                                ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_NO_BUFFER,
//                                "wl_surface@%"PRIu32" no buffer for synchronization",
//                                wl_resource_get_id(resource));
//         return;
//     }
//
//     if (sub) {
//         status = weston_subsurface_commit(sub);
//     } else {
//         status = WESTON_SURFACE_CLEAN;
//         wl_list_for_each(sub, &surface->subsurface_list, parent_link) {
//             if (sub->surface != surface)
//                 status |= weston_subsurface_parent_commit(sub, 0);
//         }
//         status |= weston_surface_commit(surface);
//     }
//
//     if (status & WESTON_SURFACE_DIRTY_SUBSURFACE_CONFIG)
//         surface->compositor->view_list_needs_rebuild = true;
// }


static const struct wl_surface_interface surface_interface = {
        nullptr,        // surface_destroy
        surface_attach, // surface_attach
        nullptr,        // surface_damage
        nullptr,        // surface_frame
        nullptr,        // surface_set_opaque_region
        nullptr,        // surface_set_input_region
        nullptr,        // surface_commit
        nullptr,        // surface_set_buffer_transform
        nullptr,        // surface_set_buffer_scale
        nullptr,        // surface_damage_buffer
        nullptr,        // surface_offset
};

static void
compositor_create_surface(struct wl_client *client,
                          struct wl_resource *resource, uint32_t id) {
    auto surface =
            wl_resource_create(client, &wl_surface_interface,
                               wl_resource_get_version(resource), id);
    wl_resource_set_implementation(surface, &surface_interface,
                                   surface, nullptr);
}

static void
compositor_create_region(struct wl_client *client,
                         struct wl_resource *resource, uint32_t id) {
    auto region =
            wl_resource_create(client, &wl_region_interface, 1, id);
    if (region == nullptr) {
        free(region);
        wl_resource_post_no_memory(resource);
        return;
    }
    //    wl_resource_set_implementation(region, &region_interface, region, nullptr);
}

static const struct wl_compositor_interface compositor_impl = {
        compositor_create_surface,
        compositor_create_region};

static struct wlr_shm_mapping *mapping_create(int fd, size_t size) {
//    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//    if (data == MAP_FAILED)
//    {
//        wlr_log_errno(WLR_DEBUG, "mmap failed");
//        return NULL;
//    }
//
//    struct wlr_shm_mapping *mapping = calloc(1, sizeof(*mapping));
//    if (mapping == NULL)
//    {
//        munmap(data, size);
//        return NULL;
//    }
//
//    mapping->data = data;
//    mapping->size = size;
//    return mapping;
}

static struct wlr_shm *shm_from_resource(struct wl_resource *resource);

static const struct wl_shm_interface shm_impl = {
        .create_pool = [](struct wl_client *client,
                          struct wl_resource *shm_resource, uint32_t id, int fd, int32_t size) {
            struct wlr_shm *shm = shm_from_resource(shm_resource);

            if (size <= 0) {
                wl_resource_post_error(shm_resource, WL_SHM_ERROR_INVALID_STRIDE,
                                       "Invalid size (%d)", size);
                close(fd);
                return;
            }

            struct wlr_shm_mapping *mapping = mapping_create(fd, size);
            if (mapping == NULL) {
                wl_resource_post_error(shm_resource, WL_SHM_ERROR_INVALID_FD,
                                       "Failed to create memory mapping");
                close(fd);
                return;
            }

//            struct wlr_shm_pool *pool = calloc(1, sizeof(*pool));
//            if (pool == NULL) {
//                wl_resource_post_no_memory(shm_resource);
//                mapping_drop(mapping);
//                return;
//            }
//
//            uint32_t version = wl_resource_get_version(shm_resource);
//            pool->resource =
//                    wl_resource_create(client, &wl_shm_pool_interface, version, id);
//            if (pool->resource == NULL) {
//                wl_resource_post_no_memory(shm_resource);
//                free(pool);
//                return;
//            }
//            wl_resource_set_implementation(pool->resource, &pool_impl, pool,
//                                           pool_handle_resource_destroy);
//
//            pool->mapping = mapping;
//            pool->shm = shm;
//            pool->fd = fd;
//            wl_list_init(&pool->buffers);
        },
        .release = [](struct wl_client *client,
                      struct wl_resource *resource) { wl_resource_destroy(resource); },
};

struct wlr_shm *shm_from_resource(struct wl_resource *resource) {
    assert(wl_resource_instance_of(resource, &wl_shm_interface, &shm_impl));
    return static_cast<wlr_shm *>(wl_resource_get_user_data(resource));
}

void implement(wl_display *wl_display) {
    // register globals
    wl_global_create(wl_display, &wl_shm_interface, wl_shm_interface.version,
                     nullptr, [](struct wl_client *client, void *data, uint32_t version,
                                 uint32_t id) {
                struct wl_resource *resource = wl_resource_create(client, &wl_shm_interface,
                                                                  version, id);
                if (resource == NULL) {
                    wl_client_post_no_memory(client);
                    return;
                }
                wl_resource_set_implementation(resource, &shm_impl, nullptr, NULL);

                //    for (size_t i = 0; i < shm->formats_len; i++) {
                //        wl_shm_send_format(resource, shm->formats[i]);
                //    }
            });

    wl_global_create(wl_display, &wl_compositor_interface,
                     wl_compositor_interface.version, nullptr,
                     [](struct wl_client *wl_client, void *data,
                        uint32_t version, uint32_t id) {
                         //    struct wlr_compositor *compositor = data;

                         struct wl_resource *resource =
                                 wl_resource_create(wl_client, &wl_compositor_interface, version,
                                                    id);
                         if (resource == NULL) {
                             wl_client_post_no_memory(wl_client);
                             return;
                         }
                         wl_resource_set_implementation(resource, &compositor_impl, nullptr, NULL);
                     }

    );
}