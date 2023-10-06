cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_SYSTEM_NAME Linux)
set(tools /home/ice/works/rk3588/buildroot/output/rockchip_rk3588/host)
set(CMAKE_C_COMPILER ${tools}/bin/aarch64-linux-gcc)

include_directories(/home/ice/works/rk3588/buildroot/output/rockchip_rk3588/host/aarch64-buildroot-linux-gnu/sysroot/usr/include/libdrm)
link_libraries(drm)

project(
    drmtest
    VERSION 1.0
    LANGUAGES C)

add_executable(drmtest1 drmtest1.c)
add_executable(drmtest2 drmtest2.c)
add_executable(drmdevice drmdevice.c)
add_executable(modeset-plane-test modeset-plane-test.c)




