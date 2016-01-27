AM_CFLAGS = \
	$(TDM_DRM_CFLAGS) \
	-I$(top_srcdir)/src

libtdm_drm_la_LTLIBRARIES = libtdm-drm.la
libtdm_drm_ladir = $(TDM_MODULE_PATH)
libtdm_drm_la_LDFLAGS = -module -avoid-version
libtdm_drm_la_LIBADD = $(TDM_DRM_LIBS) -ldl

libtdm_drm_la_SOURCES = \
	fbdev_simple.c

