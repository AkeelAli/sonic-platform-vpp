VPP_PLATFORM_MODULE_VERSION = 1.0.0
VPP_PLATFORM_MODULE = platform-modules-vpp_$(VPP_PLATFORM_MODULE_VERSION)_amd64.deb
VPP_PLATFORM_MODULE_REL_PATH = sonic-platform-modules-vpp
$(VPP_PLATFORM_MODULE)_SRC_PATH = $(PLATFORM_PATH)/$(VPP_PLATFORM_MODULE_REL_PATH)
SONIC_DPKG_DEBS += $(VPP_PLATFORM_MODULE)