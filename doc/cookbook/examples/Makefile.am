include $(top_srcdir)/build/autotools/Makefile.am.silent

NULL =

noinst_PROGRAMS = \
	actors-composite-main 			\
	animations-complex			\
	animations-looping-animator		\
	animations-looping-implicit		\
	animations-looping-state		\
	animations-moving-animator		\
	animations-moving-implicit		\
	animations-moving-state			\
	animations-path			\
	animations-path-circle			\
	animations-path-easing			\
	animations-reuse			\
	animations-rotating			\
	animations-scaling			\
	animations-scaling-zoom			\
	text-shadow				\
	textures-reflection			\
	textures-split-go			\
	textures-sub-texture			\
	layouts-bind-constraint-allocation	\
	layouts-bind-constraint-overlay		\
	layouts-bind-constraint-stage		\
	layouts-box 				\
	layouts-box-menu 			\
	layouts-box-property-effects 		\
	layouts-stacking 			\
	layouts-stacking-diff-sized-actors	\
	events-mouse-scroll 			\
	events-pointer-motion			\
	events-pointer-motion-crossing		\
	events-pointer-motion-stacked		\
	events-pointer-motion-scribbler		\
	textures-crossfade			\
	textures-crossfade-cogl			\
	textures-crossfade-slideshow		\
	script-ui				\
	script-signals				\
	events-buttons				\
	events-buttons-click			\
	events-buttons-lasso			\
	$(NULL)

INCLUDES = \
	-I$(top_srcdir)/ \
	-I$(top_srcdir)/clutter \
	-I$(top_srcdir)/clutter/cogl \
	-I$(top_srcdir)/clutter/cogl/pango \
	-I$(top_builddir)/clutter \
	-I$(top_builddir)/clutter/cogl \
	$(NULL)

LDADD = $(top_builddir)/clutter/libclutter-@CLUTTER_SONAME_INFIX@-@CLUTTER_API_VERSION@.la

AM_CFLAGS = \
        $(CLUTTER_CFLAGS) \
        $(MAINTAINER_CFLAGS) \
        -DG_DISABLE_SINGLE_INCLUDES \
	-DCOGL_DISABLE_DEPRECATED \
	-DCLUTTER_DISABLE_DEPRECATED \
        -DTESTS_DATA_DIR=\""$(top_srcdir)/tests/data/"\"

AM_LDFLAGS = $(CLUTTER_LIBS) -export-dynamic

actors_composite_main_SOURCES              = cb-button.c cb-button.h actors-composite-main.c
animations_complex_SOURCES                 = animations-complex.c
animations_looping_animator_SOURCES        = animations-looping-animator.c
animations_looping_implicit_SOURCES        = animations-looping-implicit.c
animations_looping_state_SOURCES           = animations-looping-state.c
animations_moving_animator_SOURCES         = animations-moving-animator.c
animations_moving_implicit_SOURCES         = animations-moving-implicit.c
animations_moving_state_SOURCES            = animations-moving-state.c
animations_path_SOURCES                    = animations-path.c
animations_path_circle_SOURCES             = animations-path-circle.c
animations_path_easing_SOURCES             = animations-path-easing.c
animations_reuse_SOURCES                   = animations-reuse.c
animations_rotating_SOURCES                = animations-rotating.c
animations_scaling_SOURCES                 = animations-scaling.c
animations_scaling_zoom_SOURCES            = animations-scaling-zoom.c
text_shadow_SOURCES                        = text-shadow.c
textures_reflection_SOURCES                = textures-reflection.c
textures_split_go_SOURCES                  = textures-split-go.c
textures_sub_texture_SOURCES               = textures-sub-texture.c
layouts_bind_constraint_allocation_SOURCES = layouts-bind-constraint-allocation.c
layouts_bind_constraint_overlay_SOURCES    = layouts-bind-constraint-overlay.c
layouts_bind_constraint_stage_SOURCES      = layouts-bind-constraint-stage.c
layouts_box_SOURCES                        = layouts-box.c
layouts_box_menu_SOURCES                   = layouts-box-menu.c
layouts_box_property_effects_SOURCES       = layouts-box-property-effects.c
layouts_stacking_SOURCES                   = layouts-stacking.c
layouts_stacking_diff_sized_actors_SOURCES = layouts-stacking-diff-sized-actors.c
events_mouse_scroll_SOURCES                = events-mouse-scroll.c
events_pointer_motion_SOURCES              = events-pointer-motion.c
events_pointer_motion_crossing_SOURCES     = events-pointer-motion-crossing.c
events_pointer_motion_stacked_SOURCES      = events-pointer-motion-stacked.c
events_pointer_motion_scribbler_SOURCES    = events-pointer-motion-scribbler.c
textures_crossfade_SOURCES                 = textures-crossfade.c
textures_crossfade_cogl_SOURCES            = textures-crossfade-cogl.c
textures_crossfade_slideshow_SOURCES       = textures-crossfade-slideshow.c
script_ui_SOURCES                          = script-ui.c
script_signals_SOURCES                     = script-signals.c
events_buttons_SOURCES                     = events-buttons.c
events_buttons_click_SOURCES               = events-buttons-click.c
events_buttons_lasso_SOURCES               = events-buttons-lasso.c

-include $(top_srcdir)/build/autotools/Makefile.am.gitignore
