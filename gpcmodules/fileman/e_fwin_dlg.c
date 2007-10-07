#include <e.h>
#include "e_fwin_common.h"
#include "e_fwin_dlg.h"
#include "e_fwin.h"

typedef struct _Fm_File Fm_File;
struct _Fm_File 
{
   const char *file;
   int ext;
   Evas_List *apps;
};

/* Private Protos */
static E_Fwin_Exec_Type _file_is_exec(E_Fm2_Icon_Info *ici);
static void _file_exec(E_Fwin *fwin, E_Fm2_Icon_Info *ici, E_Fwin_Exec_Type ext);
static void _open_dialog_new(E_Fwin *fwin, Evas_List *apps);
static void _dia_cb_open(void *data, E_Dialog *dia);
static void _dia_cb_close(void *data, E_Dialog *dia);
static void _dia_cb_free(void *data);
static void _dia_cb_selected(void *data, Evas_Object *obj, void *event_info);
static int _dia_cb_desk_sort(Efreet_Desktop *d1, Efreet_Desktop *d2);
static Evas_List *_dia_get_cats(void);
static int _dia_cb_desk_list_sort(void *data1, void *data2);

EAPI void
e_fwin_open_dialog(E_Fwin *fwin, Evas_List *sel, int show) 
{
   Evas_List *l = NULL, *mimes = NULL, *apps = NULL;
   int need_dia = 0;
   
   if (!show) 
     {
	for (l = sel; l; l = l->next) 
	  {
	     E_Fm2_Icon_Info *ici = NULL;
	     char buf[4096];
	     
	     ici = l->data;
	     if (!ici) continue;
	     if ((ici->link) && (ici->mount)) 
	       {
		  if (fwin->win)
		    e_fwin_new(fwin->win->container, ici->link, "/");
		  else if (fwin->zone)
		    e_fwin_new(fwin->zone->container, ici->link, "/");
	       }
	     else if ((ici->link) && (ici->removable))
	       {
		  snprintf(buf, sizeof(buf), "removable:%s", ici->link);
		  if (fwin->win)
		    e_fwin_new(fwin->win->container, buf, "/");
		  else if (fwin->zone)
		    e_fwin_new(fwin->zone->container, buf, "/");
	       }
	     else if (ici->real_link) 
	       {
		  if (S_ISDIR(ici->statinfo.st_mode)) 
		    {
		       if (fwin->win)
			 e_fwin_new(fwin->win->container, NULL, ici->real_link);
		       else if (fwin->zone)
			 e_fwin_new(fwin->zone->container, NULL, ici->real_link);
		    }
		  else
		    need_dia = 1;
	       }
	     else 
	       {
		  snprintf(buf, sizeof(buf), "%s/%s", 
			   e_fm2_real_path_get(fwin->o_fm), ici->file);
		  if (S_ISDIR(ici->statinfo.st_mode)) 
		    {
		       if (fwin->win)
			 e_fwin_new(fwin->win->container, NULL, buf);
		       else if (fwin->zone)
			 e_fwin_new(fwin->zone->container, NULL, buf);
		    }
		  else
		    need_dia = 1;
	       }
	  }
	if (!need_dia) return;
	need_dia = 0;
     }
   
   /* build mime type list */
   for (l = sel; l; l = l->next) 
     {
	E_Fm2_Icon_Info *ici = NULL;
	const char *f = NULL;
	     
	ici = l->data;
	if (!ici) continue;
	if (!((ici->link) && (ici->mount))) 
	  {
	     if (_file_is_exec(ici) == E_FWIN_EXEC_NONE) 
	       {
		  if (ici->link) 
		    f = e_fm_mime_filename_get(ici->link);
		  else if (ici->mime)
		    f = strdup(ici->mime);
		  if (f)
		    mimes = evas_list_append(mimes, f);
	       }
	  }
     }
   
   /* Make app list */
   if (mimes) 
     {
	for (l = mimes; l; l = l->next) 
	  {
	     Ecore_List *ml = NULL;
	     Efreet_Desktop *desk;
	     
	     ml = efreet_util_desktop_mime_list(l->data);
	     if (!ml) continue;
	     ecore_list_first_goto(ml);
	     while (desk = ecore_list_next(ml))
	       apps = evas_list_append(apps, desk);
	     ecore_list_destroy(ml);
	  }
     }
   
   if (!show) 
     {
	if (evas_list_count(mimes) <= 1) 
	  {
	     Efreet_Desktop *desk = NULL;
	     Ecore_List *files_list = NULL;
	     char pcwd[4096];
	     
	     need_dia = 1;
	     if (mimes) desk = e_exehist_mime_desktop_get(mimes->data);
	     getcwd(pcwd, sizeof(pcwd));
	     chdir(e_fm2_real_path_get(fwin->o_fm));
	     
	     files_list = ecore_list_new();
	     ecore_list_free_cb_set(files_list, free);
	     for (l = sel; l; l = l->next) 
	       {
		  E_Fm2_Icon_Info *ici = NULL;
		  
		  ici = l->data;
		  if (!ici) continue;
		  if (_file_is_exec(ici) == E_FWIN_EXEC_NONE)
		    ecore_list_append(files_list, strdup(ici->file));
	       }
	     for (l = sel; l; l = l->next) 
	       {
		  E_Fm2_Icon_Info *ici = NULL;
		  E_Fwin_Exec_Type ext;
		  
		  ici = l->data;
		  if (!ici) continue;
		  ext = _file_is_exec(ici);
		  if (ext != E_FWIN_EXEC_NONE) 
		    {
		       _file_exec(fwin, ici, ext);
		       need_dia = 0;
		    }
	       }
	     if (desk) 
	       {
		  if (fwin->win) 
		    {
		       if (e_exec(fwin->win->border->zone, desk, NULL, files_list, "fwin"))
			 need_dia = 0;
		    }
		  else if (fwin->zone) 
		    {
		       if (e_exec(fwin->zone, desk, NULL, files_list, "fwin"))
			 need_dia = 0;
		    }
	       }
	     ecore_list_destroy(files_list);
	     
	     chdir(pcwd);
	     if (!need_dia) 
	       {
		  if (apps) evas_list_free(apps);
		  evas_list_free(mimes);
		  return;
	       }
	  }
     }
   evas_list_free(mimes);
   _open_dialog_new(fwin, apps);
}

/* Private Functions */
static E_Fwin_Exec_Type 
_file_is_exec(E_Fm2_Icon_Info *ici) 
{
   /* special file or dir - can't exec anyway */
  if ((S_ISCHR(ici->statinfo.st_mode)) || (S_ISBLK(ici->statinfo.st_mode)) ||
       (S_ISFIFO(ici->statinfo.st_mode)) || (S_ISSOCK(ici->statinfo.st_mode)))
     return E_FWIN_EXEC_NONE;
   /* it is executable */
   if ((ici->statinfo.st_mode & S_IXOTH) ||
       ((getgid() == ici->statinfo.st_gid) && (ici->statinfo.st_mode & S_IXGRP)) ||
       ((getuid() == ici->statinfo.st_uid) && (ici->statinfo.st_mode & S_IXUSR)))
     {
	/* no mimetype */
	if (!ici->mime)
	  return E_FWIN_EXEC_DIRECT;
	/* mimetype */
	else
	  {
	     /* FIXME: - this could be config */
	     if (!strcmp(ici->mime, "application/x-desktop"))
	       return E_FWIN_EXEC_DESKTOP;
	     else if (!strcmp(ici->mime, "image/png"))
	       return E_FWIN_EXEC_DESKTOP;
	     else if ((!strcmp(ici->mime, "application/x-sh")) ||
		      (!strcmp(ici->mime, "application/x-shellscript")) ||
		      (!strcmp(ici->mime, "application/x-csh")) ||
		      (!strcmp(ici->mime, "application/x-perl")) ||
		      (!strcmp(ici->mime, "application/x-shar")) ||
		      (!strcmp(ici->mime, "text/x-csh")) ||
		      (!strcmp(ici->mime, "text/x-python")) ||
		      (!strcmp(ici->mime, "text/x-sh")))
	       {
		  return E_FWIN_EXEC_DIRECT;
	       }
	  }
     }
   else
     {
	/* mimetype */
	if (ici->mime)
	  {
	     /* FIXME: - this could be config */
	     if (!strcmp(ici->mime, "application/x-desktop"))
	       return E_FWIN_EXEC_DESKTOP;
	     else if ((!strcmp(ici->mime, "application/x-sh")) ||
		      (!strcmp(ici->mime, "application/x-shellscript")) ||
		      (!strcmp(ici->mime, "text/x-sh")))
	       {
		  return E_FWIN_EXEC_TERMINAL_SH;
	       }
	  }
	else if ((e_util_glob_match(ici->file, "*.desktop")) ||
		 (e_util_glob_match(ici->file, "*.kdelink")))
	  {
	     return E_FWIN_EXEC_DESKTOP;
	  }
	else if (e_util_glob_match(ici->file, "*.run"))
	  return E_FWIN_EXEC_TERMINAL_SH;
     }
   return E_FWIN_EXEC_NONE;   
}

static void 
_file_exec(E_Fwin *fwin, E_Fm2_Icon_Info *ici, E_Fwin_Exec_Type ext)
{
   char buf[4096];
   Efreet_Desktop *desktop;
   const char *file = NULL;
   
   file = strdup(ici->file);
   switch (ext)
     {
      case E_FWIN_EXEC_NONE:
	printf("File: %s is Exec None\n", file);
	break;
      case E_FWIN_EXEC_DIRECT:
	if (fwin->win)
	  e_exec(fwin->win->border->zone, NULL, file, NULL, "fwin");
	else if (fwin->zone)
	  e_exec(fwin->zone, NULL, file, NULL, "fwin");
	break;
      case E_FWIN_EXEC_SH:
	snprintf(buf, sizeof(buf), "/bin/sh %s", e_util_filename_escape(file));
	if (fwin->win)
	  e_exec(fwin->win->border->zone, NULL, buf, NULL, NULL);
	else if (fwin->zone)
	  e_exec(fwin->zone, NULL, buf, NULL, NULL);
	break;
      case E_FWIN_EXEC_TERMINAL_DIRECT:
	snprintf(buf, sizeof(buf), "%s %s", e_config->exebuf_term_cmd, e_util_filename_escape(file));
	if (fwin->win)
	  e_exec(fwin->win->border->zone, NULL, buf, NULL, NULL);
	else if (fwin->zone)
	  e_exec(fwin->zone, NULL, buf, NULL, NULL);
	break;
      case E_FWIN_EXEC_TERMINAL_SH:
	snprintf(buf, sizeof(buf), "%s /bin/sh %s", e_config->exebuf_term_cmd, e_util_filename_escape(file));
	if (fwin->win)
	  e_exec(fwin->win->border->zone, NULL, buf, NULL, NULL);
	else if (fwin->zone)
	  e_exec(fwin->zone, NULL, buf, NULL, NULL);
	break;
      case E_FWIN_EXEC_DESKTOP:
	snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(fwin->o_fm), file);
	desktop = efreet_desktop_new(file);
	if (desktop)
	  {
	     if (fwin->win)
	       e_exec(fwin->win->border->zone, desktop, NULL, NULL, NULL);
	     else if (fwin->zone)
	       e_exec(fwin->zone, desktop, NULL, NULL, NULL);
	     efreet_desktop_free(desktop);
	  }
	break;
      default:
	break;
     }   
}

static void 
_open_dialog_new(E_Fwin *fwin, Evas_List *apps) 
{
   E_Fwin_Apps_Dialog *fad = NULL;
   E_Container *con = NULL;
   E_Dialog *dia = NULL;
   Evas *evas = NULL;
   Evas_Object *o, *of, *o_list;
   Evas_List *l = NULL, *dl = NULL;
   Evas_Coord w, h;
   
   E_OBJECT_CHECK(fwin);
   if (fwin->win)
     con = fwin->win->border->zone->container;
   else if (fwin->zone)
     con = fwin->zone->container;
   if (!con) con = e_container_current_get(e_manager_current_get());

   fad = E_NEW(E_Fwin_Apps_Dialog, 1);
   fwin->fads = evas_list_append(fwin->fads, fad);
   dia = e_dialog_new(con, "E", "_fwin_open_apps");
   e_dialog_title_set(dia, _("Open With..."));
   e_dialog_border_icon_set(dia, "enlightenment/applications");
   e_dialog_button_add(dia, _("Open"), "enlightenment/open", _dia_cb_open, fad);
   e_dialog_button_add(dia, _("Close"), "enlightenment/close", _dia_cb_close, fad);
   fad->dia = dia;
   fad->fwin = fwin;
   dia->data = fad;
   e_object_free_attach_func_set(E_OBJECT(dia), _dia_cb_free);

   evas = e_win_evas_get(dia->win);
   o_list = e_widget_list_add(evas, 1, 1);
   if (apps) 
     {
	of = e_widget_framelist_add(evas, _("Specific Applications"), 0);
	o = e_widget_ilist_add(evas, 24, 24, &(fad->app1));
	evas_event_freeze(evas);
	edje_freeze();
	e_widget_ilist_freeze(o);
	fad->o_ilist = o;
	for (l = apps; l; l = l->next) 
	  {
	     Efreet_Desktop *desk = NULL;
	     Evas_Object *oi = NULL;
	     
	     desk = l->data;
	     if (!desk) continue;
	     oi = e_util_desktop_icon_add(desk, "24x24", evas);
	     e_widget_ilist_append(o, oi, desk->name, NULL, NULL, 
				   efreet_util_path_to_file_id(desk->orig_path));
	  }
	e_widget_ilist_go(o);
	e_widget_min_size_set(o, 200, 240);
	e_widget_ilist_thaw(o);
	edje_thaw();
	evas_event_thaw(evas);
	e_widget_framelist_object_append(of, o);
	e_widget_list_object_append(o_list, of, 1, 1, 0.5);
	evas_object_smart_callback_add(o, "selected", _dia_cb_selected, fad);
     }
   
   /* Build list of All Apps */
   dl = _dia_get_cats();

   of = e_widget_framelist_add(evas, _("All Applications"), 0);
   o = e_widget_ilist_add(evas, 24, 24, &(fad->app2));
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(o);
   for (l = dl; l; l = l->next) 
     {
	Evas_Object *icon = NULL;
	Efreet_Desktop *desk = NULL;
	
	desk = l->data;
	if (!desk) continue;
	icon = e_util_desktop_icon_add(desk, "24x24", evas);
	e_widget_ilist_append(o, icon, desk->name, NULL, NULL, 
			      efreet_util_path_to_file_id(desk->orig_path));
     }
   
   e_widget_ilist_go(o);
   e_widget_min_size_set(o, 200, 240);
   e_widget_ilist_thaw(o);
   edje_thaw();
   evas_event_thaw(evas);
   
   e_widget_framelist_object_append(of, o);
   e_widget_list_object_append(o_list, of, 1, 1, 0.5);
   
   e_widget_min_size_get(o_list, &w, &h);
   e_dialog_content_set(dia, o_list, w, h);
   e_dialog_show(dia);
}

static void 
_dia_cb_open(void *data, E_Dialog *dia) 
{
   E_Fwin_Apps_Dialog *fad;
   Efreet_Desktop *desktop = NULL;
   char pcwd[4096], buf[4096];
   Evas_List *selected = NULL, *l = NULL;
   E_Fm2_Icon_Info *ici;
   Ecore_List *files = NULL;
   
   fad = data;
   if (fad->app1) 
     desktop = efreet_util_desktop_file_id_find(fad->app1);
   else if (fad->app2) 
     desktop = efreet_util_desktop_file_id_find(fad->app2);
   if (desktop)
     {
	getcwd(pcwd, sizeof(pcwd));
	chdir(e_fm2_real_path_get(fad->fwin->o_fm));

	selected = e_fm2_selected_list_get(fad->fwin->o_fm);
	if (selected)
	  {
	     files = ecore_list_new();
	     ecore_list_free_cb_set(files, free);
	     for (l = selected; l; l = l->next)
	       {
		  E_Fwin_Exec_Type ext;
		  
		  ici = l->data;
		  /* this snprintf is silly - but it's here in case i really do
		   * need to provide full paths (seems silly since we chdir
		   * into the dir)
		   */
		  buf[0] = 0;
		  ext = _file_is_exec(ici);
		  if (ext != E_FWIN_EXEC_NONE)
		    _file_exec(fad->fwin, ici, ext);
		  else
		    {
		       if (!((ici->link) && (ici->mount)))
			 {
			    if (ici->link)
			      {
				 if (!S_ISDIR(ici->statinfo.st_mode))
				   snprintf(buf, sizeof(buf), "%s", ici->file);
			      }
			    else
			      {
				 if (!S_ISDIR(ici->statinfo.st_mode))
				   snprintf(buf, sizeof(buf), "%s", ici->file);
			      }
			 }
		    }
		  if (buf[0] != 0)
		    {
		       if (ici->mime)
			 e_exehist_mime_desktop_add(ici->mime, desktop);
		       ecore_list_append(files, strdup(ici->file));
		    }
	       }
	     evas_list_free(selected);
	     if (fad->fwin->win)
	       e_exec(fad->fwin->win->border->zone, desktop, NULL, files, "fwin");
	     else if (fad->fwin->zone)
	       e_exec(fad->fwin->zone, desktop, NULL, files, "fwin");
	     ecore_list_destroy(files);
	  }
	chdir(pcwd);
     }
   e_object_del(E_OBJECT(fad->dia));
}

static void 
_dia_cb_close(void *data, E_Dialog *dia) 
{
   E_Fwin_Apps_Dialog *fad = NULL;

   fad = data;
   if (!fad) return;
   e_object_del(E_OBJECT(fad->dia));
}

static void
_dia_cb_free(void *data) 
{
   E_Dialog *dia = NULL;
   E_Fwin_Apps_Dialog *fad = NULL;
   
   dia = (E_Dialog *)data;
   fad = dia->data;
   if (fad->fwin->fads) 
     {
	if (evas_list_find(fad->fwin->fads, fad))
	  fad->fwin->fads = evas_list_remove(fad->fwin->fads, fad);
     }
   E_FREE(fad->app1);
   E_FREE(fad->app2);
   E_FREE(fad);
}

static void 
_dia_cb_selected(void *data, Evas_Object *obj, void *event_info) 
{
   E_Fwin_Apps_Dialog *fad = NULL;
   
   fad = data;
   if (!fad) return;
   
   _dia_cb_open(fad, fad->dia);
}

static int 
_dia_cb_desk_sort(Efreet_Desktop *d1, Efreet_Desktop *d2) 
{
   return (strcmp(d1->name, d2->name));
}

static Evas_List *
_dia_get_cats(void) 
{
   Ecore_List *cats = NULL;
   Evas_List *l = NULL;
   char *cat;
   
   cats = efreet_util_desktop_categories_list();
   ecore_list_sort(cats, ECORE_COMPARE_CB(strcmp), ECORE_SORT_MIN);
   ecore_list_first_goto(cats);
   while ((cat = ecore_list_next(cats))) 
     {
	Ecore_List *desks = NULL;
	Efreet_Desktop *desk = NULL;
	
	desks = efreet_util_desktop_category_list(cat);
	ecore_list_sort(desks, ECORE_COMPARE_CB(_dia_cb_desk_sort), ECORE_SORT_MIN);
	ecore_list_first_goto(desks);
	while ((desk = ecore_list_next(desks))) 
	  {
	     if (!evas_list_find(l, desk))
	       l = evas_list_append(l, desk);
	  }
     }
   if (l)
     l = evas_list_sort(l, -1, _dia_cb_desk_list_sort);
   return l;
}

static int 
_dia_cb_desk_list_sort(void *data1, void *data2) 
{
   Efreet_Desktop *d1, *d2;

   if (!data1) return 1;
   if (!data2) return -1;
   d1 = data1;
   d2 = data2;
   return (strcmp(d1->name, d2->name));
}
