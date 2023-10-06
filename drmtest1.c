/*
 * Author: Leon.He
 * e-mail: 343005384@qq.com
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct buffer_object
{
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t handle;
    uint32_t size;
    uint8_t *vaddr;
    uint32_t fb_id;
};

struct buffer_object buf;

static int modeset_create_fb(int fd, struct buffer_object *bo)
{
    struct drm_mode_create_dumb create = {};
    struct drm_mode_map_dumb map = {};

    create.width = bo->width;
    create.height = bo->height;
    create.bpp = 32;
    drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);

    bo->pitch = create.pitch;
    bo->size = create.size;
    bo->handle = create.handle;
    drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch,
                 bo->handle, &bo->fb_id);

    map.handle = create.handle;
    drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);

    bo->vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, map.offset);

    memset(bo->vaddr, 0xFF, bo->size);

    return 0;
}

static void modeset_destroy_fb(int fd, struct buffer_object *bo)
{
    struct drm_mode_destroy_dumb destroy = {};

    drmModeRmFB(fd, bo->fb_id);

    munmap(bo->vaddr, bo->size);

    destroy.handle = bo->handle;
    drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

static int modeset_find_crtc(int fd, drmModeRes *res, drmModeConnector *conn)
{
    drmModeEncoder *enc;
    unsigned int i, j;

    /* iterate all encoders of this connector */
    for (i = 0; i < conn->count_encoders; ++i) {
        enc = drmModeGetEncoder(fd, conn->encoders[i]);
        if (!enc) {
            /* cannot retrieve encoder, ignoring... */
            continue;
        }

        /* iterate all global CRTCs */
        for (j = 0; j < res->count_crtcs; ++j) {
            /* check whether this CRTC works with the encoder */
            if (!(enc->possible_crtcs & (1 << j)))
                continue;

            drmModeFreeEncoder(enc);
            return res->crtcs[j];

        }

        drmModeFreeEncoder(enc);
    }

    /* cannot find a suitable CRTC */
    return -ENOENT;
}

int main(int argc, char **argv)
{
    int fd;
    drmModeConnector *conn;
    drmModeRes *res;
    drmModePlaneRes *plane_res;
    uint32_t conn_id;
    uint32_t crtc_id;
    int i = 0;
    int ret = 0;

    if (drmAvailable()) {
        printf("drmAvailable ok\n");
    } else {
        printf("drmAvailable failed\n");
        return -1;
    }

    fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

    res = drmModeGetResources(fd);

    for (i = 0; i < res->count_connectors; i++) {
        conn = drmModeGetConnector(fd, res->connectors[i]);
        if (conn->connection == DRM_MODE_CONNECTED) {
            conn_id = res->connectors[i];
            break;
        }
        drmModeFreeConnector(conn);
    }
    if (i == res->count_connectors) {
        printf("No connected connectors\n");
        return -1;
    }

    crtc_id = modeset_find_crtc(fd, res, conn);
    if (crtc_id < 0) {
        printf("Find crtc failed.\n");
        return -1;
    }

    ret = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if (ret) {
        printf("failed to set client cap\n");
        return -1;
    }
    plane_res = drmModeGetPlaneResources(fd);
    printf("count_planes %d\n", plane_res->count_planes);
    for (i = 0; i < plane_res->count_planes; i++) {
        printf("plane %d, crtc id: %d\n", plane_res->planes->)
    }


    buf.width = conn->modes[0].hdisplay;
    buf.height = conn->modes[0].vdisplay;

    modeset_create_fb(fd, &buf);

    drmModeSetCrtc(fd, crtc_id, buf.fb_id,
                   0, 0, &conn_id, 1, &conn->modes[0]);

    getchar();

    modeset_destroy_fb(fd, &buf);

    drmModeFreeConnector(conn);
    drmModeFreeResources(res);

    close(fd);

    return 0;
}