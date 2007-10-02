#include "e.h"
#include "e_mod_main.h"
#include "e_toolbar.h"

typedef struct _E_Toolbar_Smart_Data E_Toolbar_Smart_Data;

struct _E_Toolbar_Smart_Data 
{
   Evas_Object *o_base;
   Evas_Object *o_btn;
   Evas_Object *o_entry;
   int x, y, w, h;
   float valign;
   char *path;
};

static void _e_toolbar_smart_add(Evas_Object *obj);
static void _e_toolbar_smart_del(Evas_Object *obj);
static void _e_toolbar_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_toolbar_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_toolbar_smart_show(Evas_Object *obj);
static void _e_toolbar_smart_hide(Evas_Object *obj);
static void _e_toolbar_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_toolbar_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_toolbar_clip_unset(Evas_Object *obj);
static void _e_toolbar_key_down_cb(void *data, Evas_Object *obj, void *event_info);
static void _e_toolbar_btn_cb(void *data, void *data2);

static Evas_Smart *_e_toolbar_smart = NULL;
static int _e_toolbar_smart_use = 0;

EAPI Evas_Object *
e_toolbar_add(Evas *evas) 
{
   if (!_e_toolbar_smart) 
     {
	static const Evas_Smart_Class sc = 
	  {
	     "e_toolbar",
	       EVAS_SMART_CLASS_VERSION,
	       _e_toolbar_smart_add,
	       _e_toolbar_smart_del,
	       _e_toolbar_smart_move,
	       _e_toolbar_smart_resize,
	       _e_toolbar_smart_show,
	       _e_toolbar_smart_hide,
	       _e_toolbar_color_set,
	       _e_toolbar_clip_set,
	       _e_toolbar_clip_unset,
	       NULL
	  };
	_e_toolbar_smart = evas_smart_class_new(&sc);
	_e_toolbar_smart_use = 0;
     }
   _e_toolbar_smart_use++;
   return evas_object_smart_add(evas, _e_toolbar_smart);
}

EAPI void
e_toolbar_path_set(Evas_Object *obj, const char *path) 
{
   E_Toolbar_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj))) return;
   e_widget_entry_text_set(sd->o_entry, strdup(path));
//   evas_object_smart_callback_call(obj, "path_changed", NULL);
}

EAPI const char *
e_toolbar_path_get(Evas_Object *obj) 
{
   E_Toolbar_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj))) return NULL;
   return e_widget_entry_text_get(sd->o_entry);
}

EAPI void
e_toolbar_button_enable(Evas_Object *obj, int enable) 
{
   E_Toolbar_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj))) return;
   e_widget_disabled_set(sd->o_btn, !(enable));
}

/* Private Functions */
static void 
_e_toolbar_smart_add(Evas_Object *obj) 
{
   Evas *evas;
   E_Toolbar_Smart_Data *sd;
   Evas_Object *o;
   char buf[4096];
   
   if ((!obj) || !(evas = evas_object_evas_get(obj))) return;
   
   sd = malloc(sizeof(E_Toolbar_Smart_Data));
   if (!sd) return;
   sd->valign = 0.0;
   sd->path = NULL;
   
   evas_object_smart_data_set(obj, sd);

   snprintf(buf, sizeof(buf), "%s/fileman.edj", e_module_dir_get(conf_module));
   
   o = edje_object_add(evas);
   sd->o_base = o;
   if (!e_theme_edje_object_set(o, "base/theme/modules/fileman", 
				"modules/fileman/toolbar"))
     edje_object_file_set(o, buf, "modules/fileman/toolbar");
   evas_object_smart_member_add(o, obj);
   edje_object_size_min_calc(o, &sd->w, &sd->h);

   o = e_widget_button_add(evas, _("Up"), "widget/up_dir", 
			   _e_toolbar_btn_cb, obj, NULL);
   sd->o_btn = o;
   edje_object_part_swallow(sd->o_base, "e.swallow.button", o);
   evas_object_show(o);
   
   edje_object_part_text_set(sd->o_base, "e.text.location", _("Location:"));

   o = e_widget_entry_add(evas, &(sd->path));
   sd->o_entry = o;
   edje_object_part_swallow(sd->o_base, "e.swallow.entry", o);
   evas_object_show(o);
   
   evas_object_smart_callback_add(sd->o_entry, "key_down", 
				  _e_toolbar_key_down_cb, obj);
}

static void 
_e_toolbar_smart_del(Evas_Object *obj) 
{
   E_Toolbar_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj))) return;

   evas_object_smart_callback_del(sd->o_entry, "key_down", 
				  _e_toolbar_key_down_cb);
   
   /* Del Evas Objects */
   evas_object_del(sd->o_btn);
   evas_object_del(sd->o_entry);
   evas_object_del(sd->o_base);
   free(sd);
}

static void 
_e_toolbar_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y) 
{
   E_Toolbar_Smart_Data *sd;
   Evas_Coord px, py;
   Evas_Coord nx, ny;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj))) return;
   
   evas_object_geometry_get(obj, &px, &py, NULL, NULL);
   evas_object_geometry_get(sd->o_base, &nx, &ny, NULL, NULL);
   evas_object_move(sd->o_base, nx + (x - px), ny + (y - py));
}

static void 
_e_toolbar_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h) 
{
   E_Toolbar_Smart_Data *sd;
   Evas_Coord x, y;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj))) return;
   
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   evas_object_move(sd->o_base, x, y + ((h - sd->h) * sd->valign));
   evas_object_resize(sd->o_base, w, h);
   sd->w = w;
   sd->h = h;
}

static void 
_e_toolbar_smart_show(Evas_Object *obj) 
{
   E_Toolbar_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj))) return;

   evas_object_show(sd->o_base);
}

static void 
_e_toolbar_smart_hide(Evas_Object *obj) 
{
   E_Toolbar_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj))) return;

   evas_object_hide(sd->o_base);
}

static void 
_e_toolbar_color_set(Evas_Object *obj, int r, int g, int b, int a) 
{
   E_Toolbar_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj))) return;

   evas_object_color_set(sd->o_base, r, g, b, a);
}

static void 
_e_toolbar_clip_set(Evas_Object *obj, Evas_Object *clip) 
{
   E_Toolbar_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj))) return;
   
   evas_object_clip_set(sd->o_base, clip);
}

static void 
_e_toolbar_clip_unset(Evas_Object *obj) 
{
   E_Toolbar_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj))) return;
   
   evas_object_clip_unset(sd->o_base);
}

static void 
_e_toolbar_key_down_cb(void *data, Evas_Object *obj, void *event_info) 
{
   Evas_Event_Key_Down *ev;
   Evas_Object *object;
   E_Toolbar_Smart_Data *sd;
   
   object = data;
   if ((!object) || !(sd = evas_object_smart_data_get(object))) return;
   
   ev = event_info;
   if (!strcmp(ev->keyname, "Return")) 
     evas_object_smart_callback_call(object, "path_changed", NULL);
}

static void 
_e_toolbar_btn_cb(void *data, void *data2) 
{
   Evas_Object *obj;
   
   obj = data;
   evas_object_smart_callback_call(obj, "go_up", NULL);
}
