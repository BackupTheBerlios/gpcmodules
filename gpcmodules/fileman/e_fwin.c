#include <e.h>
#include "e_fwin_common.h"
#include "e_fwin_dlg.h"
#include "e_mod_main.h"
#include "e_toolbar.h"

/* Protos */
static E_Fwin *_e_fwin_new(E_Container *con, const char *dev, const char *path);
static void _e_fwin_free(E_Fwin *fwin);
static void _e_fwin_cb_delete(E_Win *win);
static void _e_fwin_cb_resize(E_Win *win);
static void _e_fwin_cb_path_change(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_cb_go_up(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_config_set(E_Fwin *fwin);
static void _e_fwin_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_fwin_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
static void _e_fwin_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
static void _e_fwin_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
static void _e_fwin_pan_scroll_update(E_Fwin *fwin);
static void _e_fwin_window_title_set(E_Fwin *fwin);
static void _e_fwin_window_icon_set(E_Fwin *fwin, E_Fm2_Icon_Info *ici);
static void _e_fwin_cb_changed(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_cb_deleted(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_cb_selected(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_cb_sel_changed(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_open(E_Fwin *fwin);
static const char *_e_fwin_custom_path_eval(E_Fwin *fwin, Efreet_Desktop *desk, const char *prev_path, const char *key);
static int _e_fwin_zone_move_resize(void *data, int type, void *event);
static void _e_fwin_zone_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_fwin_menu_extend_cb_start(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info);
static void _e_fwin_menu_extend_cb_end(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info);
static void _e_fwin_menu_cb_open(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fwin_menu_cb_open_with(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fwin_menu_cb_parent(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fwin_menu_cb_desktop_edit(void *data, E_Menu *m, E_Menu_Item *mi);

static Evas_List *fwins = NULL;

EAPI int
e_fwin_init(void) 
{
   return 1;
}

EAPI int
e_fwin_shutdown(void) 
{
   Evas_List *l = NULL;
   
   l = fwins;
   fwins = NULL;
   while (l) 
     {
	e_object_del(E_OBJECT(l->data));
	l = evas_list_remove_list(l, l);
     }
   return 1;
}

EAPI void
e_fwin_new(E_Container *con, const char *dev, const char *path) 
{
   E_Fwin *fwin;
   
   fwin = _e_fwin_new(con, dev, path);
}

EAPI void
e_fwin_zone_new(E_Zone *zone, const char *dev, const char *path) 
{
   E_Fwin *fwin;
   Evas_Object *o;
   
   fwin = E_OBJECT_ALLOC(E_Fwin, E_FWIN_TYPE, _e_fwin_free);
   if (!fwin) return;
   fwin->zone = zone;
   fwin->zone_handler = ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE,
						_e_fwin_zone_move_resize, fwin);
   evas_object_event_callback_add(zone->bg_event_object, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_fwin_zone_cb_mouse_down, fwin);
   fwins = evas_list_append(fwins, fwin);
   
   o = e_fm2_add(zone->container->bg_evas);
   fwin->o_fm = o;
   _e_fwin_config_set(fwin);
   e_fm2_custom_theme_content_set(o, "desktop");
   evas_object_smart_callback_add(o, "dir_changed", _e_fwin_cb_changed, fwin);
   evas_object_smart_callback_add(o, "dir_deleted", _e_fwin_cb_deleted, fwin);
   evas_object_smart_callback_add(o, "selected", _e_fwin_cb_selected, fwin);
   evas_object_smart_callback_add(o, "selection_change", 
				  _e_fwin_cb_sel_changed, fwin);
   
   e_fm2_icon_menu_start_extend_callback_set(o, _e_fwin_menu_extend_cb_start, fwin);
   e_fm2_icon_menu_end_extend_callback_set(o, _e_fwin_menu_extend_cb_end, fwin);
   
   e_fm2_underlay_hide(o);
   evas_object_show(o);
   
   o = e_scrollframe_add(zone->container->bg_evas);
   ecore_x_icccm_state_set(zone->container->bg_win, ECORE_X_WINDOW_STATE_HINT_NORMAL);
   e_drop_xdnd_register_set(zone->container->bg_win, 1);
   e_scrollframe_custom_theme_set(o, "base/theme/fileman", 
				  "e/fileman/desktop/scrollframe");
   evas_object_data_set(fwin->o_fm, "fwin", fwin);
   e_scrollframe_extern_pan_set(o, fwin->o_fm, _e_fwin_pan_set, _e_fwin_pan_get,
				_e_fwin_pan_max_get, _e_fwin_pan_child_size_get);
   evas_object_propagate_events_set(fwin->o_fm, 0);
   fwin->o_scroll = o;
   evas_object_move(o, fwin->zone->x, fwin->zone->y);
   evas_object_resize(o, fwin->zone->w, fwin->zone->h);
   evas_object_show(o);
   
   e_fm2_window_object_set(fwin->o_fm, E_OBJECT(fwin->zone));
   evas_object_focus_set(fwin->o_fm, 1);
   e_fm2_path_set(fwin->o_fm, dev, path);
}

EAPI void
e_fwin_all_unsel(void *data) 
{
   E_Fwin *fwin;
   
   fwin = data;
   E_OBJECT_CHECK(fwin);
   E_OBJECT_TYPE_CHECK(fwin, E_FWIN_TYPE);
   e_fm2_all_unsel(fwin->o_fm);
}

EAPI void
e_fwin_zone_shutdown(E_Zone *zone) 
{
   Evas_List *l = NULL;
   
   for (l = fwins; l; l = l->next) 
     {
	E_Fwin *fwin;
	
	fwin = l->data;
	if (!fwin) continue;
	if (fwin->zone != zone) continue;
	e_object_del(E_OBJECT(fwin));
	fwin = NULL;
     }
}

EAPI void
e_fwin_reload_all(void) 
{
   Evas_List *l;
   
   for (l = fwins; l; l = l->next) 
     {
	E_Fwin *fwin;
	
	fwin = l->data;
	if (!fwin) continue;
	if (fwin->zone)
	  e_fwin_zone_shutdown(fwin->zone);
	else
	  {
	     _e_fwin_config_set(fwin);
	     e_fm2_refresh(fwin->o_fm);
	     _e_fwin_window_title_set(fwin);
	  }
     }
   
   for (l = e_manager_list(); l; l = l->next) 
     {
	E_Manager *man;
	Evas_List *ll = NULL;
	
	man = l->data;
	if (!man) continue;
	for (ll = man->containers; ll; ll = ll->next) 
	  {
	     E_Container *con;
	     Evas_List *lll = NULL;
	     
	     con = ll->data;
	     if (!con) continue;
	     for (lll = con->zones; lll; lll = lll->next) 
	       {
		  E_Zone *zone;
		  
		  zone = lll->data;
		  if (!zone) continue;
		  if ((zone->container->num == 0) && (zone->num == 0) && 
		      (fileman_config->view.show_desktop_icons))
		    e_fwin_zone_new(zone, "desktop", "/");
		  else 
		    {
		       char buf[256];
		       
		       if (fileman_config->view.show_desktop_icons) 
			 {
			    snprintf(buf, sizeof(buf), "%i", 
				     (zone->container->num + zone->num));
			    e_fwin_zone_new(zone, "desktop", buf);
			 }
		    }
	       }
	  }
     }
   
}

/* Private Functions */
static E_Fwin *
_e_fwin_new(E_Container *con, const char *dev, const char *path) 
{
   E_Fwin *fwin;
   Evas *evas;
   Evas_Object *o;
   char buf[4096];

   fwin = E_OBJECT_ALLOC(E_Fwin, E_FWIN_TYPE, _e_fwin_free);
   if (!fwin) return NULL;
   
   fwin->win = e_win_new(con);
   if (!fwin->win) 
     {
	E_FREE(fwin);
	return NULL;
     }
   e_win_delete_callback_set(fwin->win, _e_fwin_cb_delete);
   e_win_resize_callback_set(fwin->win, _e_fwin_cb_resize);
   fwin->win->data = fwin;
   
   evas = e_win_evas_get(fwin->win);
   
   o = edje_object_add(evas);
   fwin->o_bg = o;
   e_theme_edje_object_set(o, "base/theme/fileman", 
			   "e/fileman/default/window/main");
   evas_object_show(o);
   
   o = e_toolbar_add(evas);
   fwin->o_tb = o;
   evas_object_move(o, 0, 0);
   evas_object_resize(o, fwin->win->w, 32);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "path_changed", 
				  _e_fwin_cb_path_change, fwin);
   evas_object_smart_callback_add(o, "go_up", _e_fwin_cb_go_up, fwin);
   
   o = e_fm2_add(evas);
   fwin->o_fm = o;
   _e_fwin_config_set(fwin);
   evas_object_smart_callback_add(o, "dir_changed", _e_fwin_cb_changed, fwin);
   evas_object_smart_callback_add(o, "dir_deleted", _e_fwin_cb_deleted, fwin);
   evas_object_smart_callback_add(o, "selected", _e_fwin_cb_selected, fwin);
   evas_object_smart_callback_add(o, "selection_change", 
				  _e_fwin_cb_sel_changed, fwin);
   
   e_fm2_icon_menu_start_extend_callback_set(o, _e_fwin_menu_extend_cb_start, fwin);
   e_fm2_icon_menu_end_extend_callback_set(o, _e_fwin_menu_extend_cb_end, fwin);   
   evas_object_show(o);
   
   o = e_scrollframe_add(evas);
   e_scrollframe_custom_theme_set(o, "base/theme/fileman", 
				  "e/fileman/default/scrollframe");
   evas_object_data_set(fwin->o_fm, "fwin", fwin);
   e_scrollframe_extern_pan_set(o, fwin->o_fm, _e_fwin_pan_set, _e_fwin_pan_get,
				_e_fwin_pan_max_get, _e_fwin_pan_child_size_get);
   evas_object_propagate_events_set(fwin->o_fm, 0);
   fwin->o_scroll = o;
   evas_object_show(o);
   
   o = edje_object_add(evas);
   edje_object_part_swallow(e_scrollframe_edje_object_get(fwin->o_scroll), 
			    "e.swallow.overlay", o);
   evas_object_pass_events_set(o, 1);
   fwin->o_over = o;
   
   e_fm2_window_object_set(fwin->o_fm, E_OBJECT(fwin->win));
   evas_object_focus_set(fwin->o_fm, 1);
   e_fm2_path_set(fwin->o_fm, dev, path);
   
   snprintf(buf, sizeof(buf), "_fwin::/%s", e_fm2_real_path_get(fwin->o_fm));
   e_win_name_class_set(fwin->win, "E", buf);
   
   _e_fwin_window_title_set(fwin);
   
   e_win_size_min_set(fwin->win, 24, 24);
   e_win_resize(fwin->win, 280, 200);
   e_win_show(fwin->win);
   if (fwin->win->evas_win)
     e_drop_xdnd_register_set(fwin->win->evas_win, 1);
   
   if (fwin->win->border) 
     e_win_border_icon_set(fwin->win, "enlightenment/fileman");

   fwins = evas_list_append(fwins, fwin);
   return fwin;
}

static void 
_e_fwin_free(E_Fwin *fwin) 
{
   if (!fwin) return;
   fwins = evas_list_remove(fwins, fwin);

   if (!fwin->zone)
     e_drop_xdnd_register_set(fwin->win->evas_win, 0);
   
   if (fwin->win) e_object_del(E_OBJECT(fwin->win));
   if (fwin->o_tb) 
     {
	evas_object_smart_callback_del(fwin->o_tb, "path_changed",
				       _e_fwin_cb_path_change);
	evas_object_smart_callback_del(fwin->o_tb, "go_up",
				       _e_fwin_cb_go_up);
	evas_object_del(fwin->o_tb);
     }
   if (fwin->o_fm) 
     {
	evas_object_smart_callback_del(fwin->o_fm, "dir_changed", 
				       _e_fwin_cb_changed);
	evas_object_smart_callback_del(fwin->o_fm, "dir_deleted", 
				       _e_fwin_cb_deleted);
	evas_object_smart_callback_del(fwin->o_fm, "selected", 
				       _e_fwin_cb_selected);
	evas_object_smart_callback_del(fwin->o_fm, "selection_change", 
				       _e_fwin_cb_sel_changed);
	evas_object_del(fwin->o_fm);
     }
   
   if (fwin->zone) 
     {
	evas_object_event_callback_del(fwin->zone->bg_event_object,
				       EVAS_CALLBACK_MOUSE_DOWN,
				       _e_fwin_zone_cb_mouse_down);
	e_drop_xdnd_register_set(fwin->zone->container->bg_win, 0);
     }
   
   if (fwin->o_scroll) evas_object_del(fwin->o_scroll);
   if (fwin->zone_handler) ecore_event_handler_del(fwin->zone_handler);

   if (fwin->bg_file) evas_stringshare_del(fwin->bg_file);
   if (fwin->over_file) evas_stringshare_del(fwin->over_file);
   if (fwin->scroll_file) evas_stringshare_del(fwin->scroll_file);
   if (fwin->theme_file) evas_stringshare_del(fwin->theme_file);
   E_FREE(fwin);
}

static void 
_e_fwin_cb_delete(E_Win *win) 
{
   E_Fwin *fwin;
   
   if (!win) return;
   fwin = win->data;
   e_object_del(E_OBJECT(fwin));
}

static void 
_e_fwin_cb_resize(E_Win *win) 
{
   E_Fwin *fwin;
   
   if (!win) return;
   fwin = win->data;
   if (fwin->o_bg) 
     {
	if (fwin->win)
	  evas_object_resize(fwin->o_bg, fwin->win->w, fwin->win->h);
     }
   if (fwin->o_tb) 
     {
	evas_object_move(fwin->o_tb, 0, 0);
	evas_object_resize(fwin->o_tb, fwin->win->w, 32);
     }
   if (fwin->win) 
     {
	evas_object_move(fwin->o_scroll, 0, 32);
	evas_object_resize(fwin->o_scroll, fwin->win->w, (fwin->win->h - 32));
     }
   else if (fwin->zone)
     evas_object_resize(fwin->o_scroll, fwin->zone->w, fwin->zone->h);
}

static void 
_e_fwin_cb_path_change(void *data, Evas_Object *obj, void *event_info) 
{
   E_Fwin *fwin;
   const char *path, *dev, *p;
   
   fwin = data;
   if (!fwin) return;
   path = e_toolbar_path_get(fwin->o_tb);
   e_fm2_path_get(fwin->o_fm, &dev, &p);
   e_fm2_path_set(fwin->o_fm, path, p);
}

static void 
_e_fwin_cb_go_up(void *data, Evas_Object *obj, void *event_info) 
{
   E_Fwin *fwin;
   char *p, *t;

   fwin = data;
   if (!fwin) return;
   t = strdup(e_fm2_real_path_get(fwin->o_fm));
   p = strrchr(t, '/');
   if (p) 
     {
	*p = 0;
	if (strlen(t) <= 0) t = "/";
	e_fm2_path_set(fwin->o_fm, NULL, t);
     }
}

static void 
_e_fwin_config_set(E_Fwin *fwin) 
{
   E_Fm2_Config fmc;
   
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.icon.icon.w = fileman_config->icon.icon.w;
   fmc.icon.icon.h = fileman_config->icon.icon.h;
   fmc.icon.fixed.w = 0;
   fmc.icon.fixed.h = 0;
   fmc.view.selector = 0;
   fmc.view.single_click = fileman_config->view.single_click;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.extension.show = fileman_config->icon.extension.show;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = fileman_config->list.sort.dirs.first;
   fmc.list.sort.dirs.last = fileman_config->list.sort.dirs.last;
   fmc.selection.single = 0;
   fmc.selection.windows_modifiers = 0;
   if (!fwin->zone) 
     {
	fmc.view.mode = fileman_config->view.mode;
	fmc.view.open_dirs_in_place = fileman_config->view.open_dirs_in_place;
     }
   else 
     {
	fmc.view.mode = E_FM2_VIEW_MODE_CUSTOM_ICONS;
	fmc.view.open_dirs_in_place = 0;
	fmc.view.fit_custom_pos = 1;
     }
   e_fm2_config_set(fwin->o_fm, &fmc);
}

static void 
_e_fwin_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y) 
{
   E_Fwin *fwin;
   
   fwin = evas_object_data_get(obj, "fwin");
   e_fm2_pan_set(obj, x, y);
   if (x > fwin->fm_pan.mx) x = fwin->fm_pan.mx;
   if (y > fwin->fm_pan.my) y = fwin->fm_pan.my;
   if (x < 0) x = 0;
   if (y < 0) y = 0;
   fwin->fm_pan.x = x;
   fwin->fm_pan.y = y;
   _e_fwin_pan_scroll_update(fwin);   
}

static void 
_e_fwin_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y) 
{
   E_Fwin *fwin;
   
   fwin = evas_object_data_get(obj, "fwin");
   e_fm2_pan_get(obj, x, y);
   fwin->fm_pan.x = *x;
   fwin->fm_pan.y = *y;   
}

static void 
_e_fwin_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y) 
{
   E_Fwin *fwin;
   
   fwin = evas_object_data_get(obj, "fwin");
   e_fm2_pan_max_get(obj, x, y);
   fwin->fm_pan.mx = *x;
   fwin->fm_pan.my = *y;
   _e_fwin_pan_scroll_update(fwin);   
}

static void 
_e_fwin_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h) 
{
   E_Fwin *fwin;
   
   fwin = evas_object_data_get(obj, "fwin");
   e_fm2_pan_child_size_get(obj, w, h);
   fwin->fm_pan.w = *w;
   fwin->fm_pan.h = *h;
   _e_fwin_pan_scroll_update(fwin);   
}

static void 
_e_fwin_pan_scroll_update(E_Fwin *fwin) 
{
   Edje_Message_Int_Set *msg;
 
   if ((fwin->fm_pan.x == fwin->fm_pan_last.x) &&
       (fwin->fm_pan.y == fwin->fm_pan_last.y) &&
       (fwin->fm_pan.mx == fwin->fm_pan_last.mx) &&
       (fwin->fm_pan.my == fwin->fm_pan_last.my) &&
       (fwin->fm_pan.w == fwin->fm_pan_last.w) &&
       (fwin->fm_pan.h == fwin->fm_pan_last.h)) return;

   msg = alloca(sizeof(Edje_Message_Int_Set) -
		sizeof(int) + (6 * sizeof(int)));
   msg->count = 6;
   msg->val[0] = fwin->fm_pan.x;
   msg->val[1] = fwin->fm_pan.y;
   msg->val[2] = fwin->fm_pan.mx;
   msg->val[3] = fwin->fm_pan.my;
   msg->val[4] = fwin->fm_pan.w;
   msg->val[5] = fwin->fm_pan.h;

   if (fwin->o_under)
     edje_object_message_send(fwin->o_under, EDJE_MESSAGE_INT_SET, 1, msg);
   if (fwin->o_over)
     edje_object_message_send(fwin->o_over, EDJE_MESSAGE_INT_SET, 1, msg);
   if (fwin->o_scroll)
     edje_object_message_send(e_scrollframe_edje_object_get(fwin->o_scroll), 
			      EDJE_MESSAGE_INT_SET, 1, msg);
   fwin->fm_pan_last.x = fwin->fm_pan.x;
   fwin->fm_pan_last.y = fwin->fm_pan.y;
   fwin->fm_pan_last.mx = fwin->fm_pan.mx;
   fwin->fm_pan_last.my = fwin->fm_pan.my;
   fwin->fm_pan_last.w = fwin->fm_pan.w;
   fwin->fm_pan_last.h = fwin->fm_pan.h;   
}

static void 
_e_fwin_window_title_set(E_Fwin *fwin) 
{
   char buf[4096];
   const char *file;
   char *p, *s;
   
   if (!fwin) return;
   if (fwin->zone) return;
   if (!fwin->win) return;
   
   if (fileman_config->view.show_full_path) 
     file = e_fm2_real_path_get(fwin->o_fm);
   else
     file = ecore_file_file_get(e_fm2_real_path_get(fwin->o_fm));
   
   if (!file) return;
   snprintf(buf, sizeof(buf), "%s", file);
   e_win_title_set(fwin->win, buf);
   if (fwin->o_tb) 
     {
	e_toolbar_path_set(fwin->o_tb, e_fm2_real_path_get(fwin->o_fm));

	s = strdup(e_fm2_real_path_get(fwin->o_fm));
	p = strrchr(s, '/');
	if (p) 
	  {
	     if ((s[0] == '/') && (strlen(s) == 1)) 
	       e_toolbar_button_enable(fwin->o_tb, 0);
	     else
	       e_toolbar_button_enable(fwin->o_tb, 1);
	  }
	free(s);
	
	//	if (e_fm2_has_parent_get(fwin->o_fm))
//	  e_toolbar_button_enable(fwin->o_tb, 1);
//	else
//	  e_toolbar_button_enable(fwin->o_tb, 0);
     }
}

static void 
_e_fwin_window_icon_set(E_Fwin *fwin, E_Fm2_Icon_Info *ici) 
{
   Evas *evas;
   Evas_Object *icon;
   const char *itype = NULL, *file = NULL, *group = NULL;
   
   if (!fwin) return;
   if (!fwin->win) return;
   if (!fwin->win->border) return;
   evas = evas_object_evas_get(fwin->o_fm);
   icon = e_fm2_icon_get(evas, ici->ic, NULL, NULL, 0, &itype);
   if (!icon) return;
   if (!strcmp(evas_object_type_get(icon), "edje")) 
     {
	edje_object_file_get(icon, &file, &group);
	if (file) e_win_border_icon_set(fwin->win, file);
	if (group) e_win_border_icon_key_set(fwin->win, group);
     }
   else 
     {
	file = e_icon_file_get(icon);
	if (file) e_win_border_icon_set(fwin->win, file);
     }
   evas_object_del(icon);
}

static void 
_e_fwin_cb_changed(void *data, Evas_Object *obj, void *event_info) 
{
   E_Fwin *fwin;
   Efreet_Desktop *desk;
   char buf[4096];
   
   fwin = data;
   E_OBJECT_CHECK(fwin);
   E_OBJECT_TYPE_CHECK(fwin, E_FWIN_TYPE);
   snprintf(buf, sizeof(buf), "%s/.directory.desktop", 
	    e_fm2_real_path_get(fwin->o_fm));
   desk = efreet_desktop_new(buf);
   if (desk) 
     {
	fwin->bg_file = _e_fwin_custom_path_eval(fwin, desk, fwin->bg_file, 
						 "X-Enlightenment-Directory-Wallpaper");
	fwin->over_file = _e_fwin_custom_path_eval(fwin, desk, fwin->over_file,
						   "X-Enlightenment-Directory-Overlay");
	fwin->scroll_file = _e_fwin_custom_path_eval(fwin, desk, fwin->scroll_file,
						     "X-Enlightenment-Directory-Scrollframe");
	fwin->theme_file = _e_fwin_custom_path_eval(fwin, desk, fwin->theme_file,
						    "X-Enlightenment-Directory-Theme");
	efreet_desktop_free(desk);
     }
   if (fwin->o_under) 
     {
	evas_object_hide(fwin->o_under);
	edje_object_file_set(fwin->o_under, NULL, NULL);
	if (fwin->bg_file) 
	  edje_object_file_set(fwin->o_under, fwin->bg_file, 
			       "e/desktop/backround");
	evas_object_show(fwin->o_under);
     }
   if (fwin->o_over) 
     {
	evas_object_hide(fwin->o_over);
	edje_object_file_set(fwin->o_over, NULL, NULL);
	if (fwin->over_file)
	  edje_object_file_set(fwin->o_over, fwin->over_file, 
			       "e/desktop/background");
	evas_object_show(fwin->o_over);
     }
   if (fwin->o_scroll) 
     {
	if ((fwin->scroll_file) && 
	    (e_util_edje_collection_exists(fwin->scroll_file, 
					   "e/fileman/default/scrollframe")))
	  {
	     e_scrollframe_custom_edje_file_set(fwin->o_scroll, 
						(char *)fwin->scroll_file,
						"e/fileman/default/scrollframe");
	  }
	else 
	  {
	     if (fwin->zone)
	       e_scrollframe_custom_theme_set(fwin->o_scroll, "base/theme/fileman",
					      "e/fileman/desktop/scrollframe");
	     else
	       e_scrollframe_custom_theme_set(fwin->o_scroll, "base/theme/fileman",
					      "e/fileman/default/scrollframe");
	  }
	e_scrollframe_child_pos_set(fwin->o_scroll, 0, 0);
     }
   if ((fwin->theme_file) && (ecore_file_exists(fwin->theme_file))) 
     e_fm2_custom_theme_set(obj, fwin->theme_file);
   else
     e_fm2_custom_theme_set(obj, NULL);
   
   if (fwin->zone) return;
   _e_fwin_window_title_set(fwin);
}

static void 
_e_fwin_cb_deleted(void *data, Evas_Object *obj, void *event_info) 
{
   E_Fwin *fwin;
   
   fwin = data;
   E_OBJECT_CHECK(fwin);
   E_OBJECT_TYPE_CHECK(fwin, E_FWIN_TYPE);
   e_object_del(E_OBJECT(fwin));
}

static void 
_e_fwin_cb_selected(void *data, Evas_Object *obj, void *event_info) 
{
   E_Fwin *fwin;
   
   fwin = data;
   E_OBJECT_CHECK(fwin);
   E_OBJECT_TYPE_CHECK(fwin, E_FWIN_TYPE);
   _e_fwin_open(fwin);
}

static void 
_e_fwin_cb_sel_changed(void *data, Evas_Object *obj, void *event_info) 
{
   E_Fwin *fwin;
   Evas_List *l = NULL;
   
   fwin = data;
   E_OBJECT_CHECK(fwin);
   E_OBJECT_TYPE_CHECK(fwin, E_FWIN_TYPE);
   for (l = fwins; l; l = l->next) 
     {
	if (l->data != fwin)
	  e_fwin_all_unsel(l->data);
     }
}

static void
_e_fwin_open(E_Fwin *fwin) 
{
   Evas_List *sel = NULL, *l = NULL;

   sel = e_fm2_selected_list_get(fwin->o_fm);
   if (!sel) return;
   for (l = sel; l; l = l->next) 
     {
	E_Fm2_Icon_Info *ici;
	E_Fwin *fwin2 = NULL;
	char buf[4096];
	
	ici = l->data;
	if (!ici) continue;
	if ((ici->link) && (ici->mount)) 
	  {
	     if (!fileman_config->view.open_dirs_in_place || fwin->zone) 
	       {
		  if (fwin->win)
		    fwin2 = _e_fwin_new(fwin->win->container, ici->link, "/");
		  else if (fwin->zone)
		    fwin2 = _e_fwin_new(fwin->zone->container, ici->link, "/");
	       }
	     else 
	       {
		  e_fm2_path_set(fwin->o_fm, ici->link, "/");
		  _e_fwin_window_title_set(fwin);
		  _e_fwin_window_icon_set(fwin, ici);
	       }
	  }
	else if ((ici->link) && (ici->removable)) 
	  {
	     snprintf(buf, sizeof(buf), "removable:%s", ici->link);
	     if (!fileman_config->view.open_dirs_in_place || fwin->zone) 
	       {
		  if (fwin->win)
		    fwin2 = _e_fwin_new(fwin->win->container, buf, "/");
		  else if (fwin->zone)
		    fwin2 = _e_fwin_new(fwin->zone->container, buf, "/");
	       }
	     else 
	       {
		  e_fm2_path_set(fwin->o_fm, buf, "/");
		  _e_fwin_window_title_set(fwin);
		  _e_fwin_window_icon_set(fwin, ici);
	       }
	  }
	else if (ici->real_link) 
	  {
	     if (S_ISDIR(ici->statinfo.st_mode)) 
	       {
		  if (!fileman_config->view.open_dirs_in_place || fwin->zone) 
		    {
		       if (fwin->win)
			 fwin2 = _e_fwin_new(fwin->win->container, NULL, ici->real_link);
		       else if (fwin->zone)
			 fwin2 = _e_fwin_new(fwin->zone->container, NULL, ici->real_link);
		    }
		  else 
		    {
		       e_fm2_path_set(fwin->o_fm, NULL, ici->real_link);
		       _e_fwin_window_title_set(fwin);
		       _e_fwin_window_icon_set(fwin, ici);
		    }
	       }
	     else 
	       e_fwin_open_dialog(fwin, sel);
	  }
	else 
	  {
	     snprintf(buf, sizeof(buf), "%s/%s", 
		      e_fm2_real_path_get(fwin->o_fm), ici->file);
	     if (S_ISDIR(ici->statinfo.st_mode)) 
	       {
		  if (!fileman_config->view.open_dirs_in_place || fwin->zone) 
		    {
		       if (fwin->win)
			 fwin2 = _e_fwin_new(fwin->win->container, NULL, buf);
		       else if (fwin->zone)
			 fwin2 = _e_fwin_new(fwin->zone->container, NULL, buf);
		    }
		  else 
		    {
		       e_fm2_path_set(fwin->o_fm, NULL, buf);
		       _e_fwin_window_title_set(fwin);
		       _e_fwin_window_icon_set(fwin, ici);
		    }
	       }
	     else 
	       e_fwin_open_dialog(fwin, sel);
	  }
	if (fwin2) 
	  {
	     _e_fwin_window_icon_set(fwin2, ici);
	     fwin2 = NULL;
	  }
     }

   evas_list_free(sel);
}

static const char *
_e_fwin_custom_path_eval(E_Fwin *fwin, Efreet_Desktop *desk, const char *prev_path, const char *key) 
{
   char buf[4096];
   const char *res, *ret = NULL;

   E_OBJECT_CHECK(fwin);
   res = ecore_hash_get(desk->x, key);
   if (prev_path) evas_stringshare_del(prev_path);
   if (!res) return;
   if (res[0] == '/')
     ret = evas_stringshare_add(res);
   else
     {
	snprintf(buf, sizeof(buf), "%s/%s", 
		 e_fm2_real_path_get(fwin->o_fm), res);
	ret = evas_stringshare_add(buf);
     }
   return ret;
}

static int
_e_fwin_zone_move_resize(void *data, int type, void *event) 
{
   E_Event_Zone_Move_Resize *ev;
   E_Fwin *fwin;
   
   if (type != E_EVENT_ZONE_MOVE_RESIZE) return 1;
   fwin = data;
   ev = event;
   if (!fwin) return 1;
   if (fwin->o_bg) 
     {
	evas_object_move(fwin->o_bg, ev->zone->x, ev->zone->y);
	evas_object_resize(fwin->o_bg, ev->zone->w, ev->zone->h);
     }
   if (fwin->o_scroll) 
     {
	evas_object_move(fwin->o_scroll, ev->zone->x, ev->zone->y);
	evas_object_resize(fwin->o_scroll, ev->zone->w, ev->zone->h);
     }
   return 1;
}

static void 
_e_fwin_zone_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info) 
{
   E_Fwin *fwin;
   
   fwin = data;
   E_OBJECT_CHECK(fwin);
   E_OBJECT_TYPE_CHECK(fwin, E_FWIN_TYPE);
   e_fwin_all_unsel(fwin);
}

static void 
_e_fwin_menu_extend_cb_start(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info) 
{
   E_Fwin *fwin;
   E_Menu_Item *mi;
   const char *file;
   
   fwin = data;
   E_OBJECT_CHECK(fwin);
   E_OBJECT_TYPE_CHECK(fwin, E_FWIN_TYPE);
   file = e_theme_edje_file_get("base/theme/fileman", 
				"e/fileman/default/button/open");
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Open"));
   e_menu_item_icon_edje_set(mi, file, "e/fileman/default/button/open");
   e_menu_item_callback_set(mi, _e_fwin_menu_cb_open, fwin);
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Open with..."));
   e_menu_item_icon_edje_set(mi, file, "e/fileman/default/button/open");
   e_menu_item_callback_set(mi, _e_fwin_menu_cb_open_with, fwin);
}

static void 
_e_fwin_menu_extend_cb_end(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info) 
{
   E_Fwin *fwin;
   E_Menu_Item *mi;
   const char *file, *path;
   
   fwin = data;
   E_OBJECT_CHECK(fwin);
   E_OBJECT_TYPE_CHECK(fwin, E_FWIN_TYPE);
   if (e_fm2_has_parent_get(obj)) 
     {
	file = e_theme_edje_file_get("base/theme/fileman", 
				     "e/fileman/default/button/parent");

	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);

	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Go to Parent Directory"));
	e_menu_item_icon_edje_set(mi, file, "e/fileman/default/button/parent");
	e_menu_item_callback_set(mi, _e_fwin_menu_cb_parent, fwin);
     }
   if ((info) && (info->mime)) 
     {
	path = e_fm2_real_path_get(info->fm);
	if (!path) return;
	if (strstr(path, "fileman/favorites")) return;
	if (!strcmp(info->mime, "application/x-desktop")) 
	  {
	     file = e_theme_edje_file_get("base/theme/fileman", 
					  "e/fileman/default/button/properties");

	     mi = e_menu_item_new(m);
	     e_menu_item_separator_set(mi, 1);
	     
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Edit Desktop File"));
	     e_menu_item_icon_edje_set(mi, file, 
				       "e/fileman/default/button/properties");
	     e_menu_item_callback_set(mi, _e_fwin_menu_cb_desktop_edit, info);
	  }
     }
}

static void 
_e_fwin_menu_cb_open(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Fwin *fwin;
   Evas_List *sel;
   
   fwin = data;
   E_OBJECT_CHECK(fwin);
   sel = e_fm2_selected_list_get(fwin->o_fm);
   if (!sel) return;
   e_fwin_open_dialog(fwin, sel);
   evas_list_free(sel);
}

static void 
_e_fwin_menu_cb_open_with(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Fwin *fwin;
   Evas_List *sel;
   
   fwin = data;
   E_OBJECT_CHECK(fwin);
   sel = e_fm2_selected_list_get(fwin->o_fm);
   if (!sel) return;
   e_fwin_open_dialog(fwin, sel);
   evas_list_free(sel);
}

static void 
_e_fwin_menu_cb_parent(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   e_fm2_parent_go(data);
}

static void 
_e_fwin_menu_cb_desktop_edit(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Fm2_Icon_Info *info;
   Efreet_Desktop *desk;
   const char *path;
   char buf[4096];
   
   info = data;
   if ((!info) || (!info->file)) return;
   path = e_fm2_real_path_get(info->fm);
   if (!path) return;
   if (strstr(path, "fileman/favorites")) return;
   
   snprintf(buf, sizeof(buf), "%s/%s", path, info->file);
   desk = efreet_desktop_get(buf);
   if (!desk) return;
   e_desktop_edit(e_container_current_get(e_manager_current_get()), desk);
}


