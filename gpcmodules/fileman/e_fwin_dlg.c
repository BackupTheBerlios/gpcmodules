#include <e.h>
#include "e_fwin_common.h"
#include "e_fwin_dlg.h"

static void _e_fwin_dlg_new(E_Fwin *fwin, Evas_List *apps);
static E_Fwin_Exec_Type _e_fwin_file_is_exec(E_Fm2_Icon_Info *ici);
static Evas_Bool _e_fwin_hash_cb_foreach(Evas_Hash *hash __UNUSED__, 
					 const char *key, void *data __UNUSED__,
					 void *fdata);
static void _e_fwin_file_exec(E_Fwin *fwin, E_Fm2_Icon_Info *ici, E_Fwin_Exec_Type ext);
static void _e_fwin_cb_open(void *data, E_Dialog *dia);
static void _e_fwin_cb_close(void *data, E_Dialog *dia);
static void _e_fwin_dia_cb_free(void *obj);
static void _e_fwin_dlg_cb_selected(void *data, Evas_Object *obj, void *event_info);
static int _e_fwin_cb_desk_sort(Efreet_Desktop *d1, Efreet_Desktop *d2);
static int _e_fwin_cb_desk_list_sort(void *data1, void *data2);
static Evas_Bool _e_fwin_hash_cb_free(Evas_Hash *hash __UNUSED__, const char *key,
				      void *data, void *fdata __UNUSED__);

EAPI void
e_fwin_open_dialog(E_Fwin *fwin, Evas_List *sel) 
{
   Evas_List *l = NULL, *mlist = NULL, *apps = NULL;
   Evas_Hash *mimes = NULL;
   
   E_OBJECT_CHECK(fwin);
   if (fwin->fad) 
     {
	e_object_del(E_OBJECT(fwin->fad->dia));
	fwin->fad = NULL;
     }
   for (l = sel; l; l = l->next) 
     {
	E_Fm2_Icon_Info *ici;
	const char *f = NULL;
	char buf[4096];
	
	ici = l->data;
	if (!ici) continue;
	if (!((ici->link) && (ici->mount))) 
	  {
	     if (_e_fwin_file_is_exec(ici) == E_FWIN_EXEC_NONE) 
	       {
		  if (ici->link) 
		    {
		       f = e_fm_mime_filename_get(ici->link);
		       mimes = evas_hash_del(mimes, f, (void *)1);
		       mimes = evas_hash_direct_add(mimes, f, (void *)1);
		    }
		  else 
		    {
		       snprintf(buf, sizeof(buf), "%s/%s", 
				e_fm2_real_path_get(fwin->o_fm), ici->file);
		       mimes = evas_hash_del(mimes, ici->mime, (void *)1);
		       mimes = evas_hash_direct_add(mimes, ici->mime, (void *)1);
		    }
	       }
	  }
     }
   if (mimes) 
     {
	evas_hash_foreach(mimes, _e_fwin_hash_cb_foreach, &mlist);
	evas_hash_free(mimes);
     }
   
   if (mlist) 
     {
	Ecore_List *ml;
	
	for (l = mlist; l; l = l->next) 
	  {
	     Efreet_Desktop *desk;
	     
	     ml = efreet_util_desktop_mime_list(l->data);
	     if (!ml) continue;
	     ecore_list_first_goto(ml);
	     while ((desk = ecore_list_next(ml)))
	       apps = evas_list_append(apps, desk);
	     ecore_list_destroy(ml);
	  }
	
	if (evas_list_count(mlist) <= 1) 
	  {
	     Efreet_Desktop *desk = NULL;
	     Ecore_List *files = NULL;
	     char pcwd[4096];
	     int need_dia = 1;
	     
	     if (mlist) desk = e_exehist_mime_desktop_get(mlist->data);
	     getcwd(pcwd, sizeof(pcwd));
	     chdir(e_fm2_real_path_get(fwin->o_fm));
	     
	     files = ecore_list_new();
	     ecore_list_free_cb_set(files, free);
	     for (l = sel; l; l = l->next) 
	       {
		  E_Fm2_Icon_Info *ici;
		  
		  ici = l->data;
		  if (_e_fwin_file_is_exec(ici) == E_FWIN_EXEC_NONE)
		    ecore_list_append(files, strdup(ici->file));
	       }
	     
	     for (l = sel; l; l = l->next) 
	       {
		  E_Fwin_Exec_Type ext;
		  E_Fm2_Icon_Info *ici;
		  
		  ici = l->data;
		  ext = _e_fwin_file_is_exec(ici);
		  if (ext != E_FWIN_EXEC_NONE) 
		    {
		       _e_fwin_file_exec(fwin, ici, ext);
		       need_dia = 0;
		    }
	       }
	     if (desk) 
	       {
		  if (fwin->win) 
		    {
		       if (e_exec(fwin->win->border->zone, desk, NULL, files, "fwin"))
			 need_dia = 0;
		    }
		  else if (fwin->zone) 
		    {
		       if (e_exec(fwin->zone, desk, NULL, files, "fwin"))
			 need_dia = 0;
		    }
	       }
	     ecore_list_destroy(files);
	     
	     chdir(pcwd);
	     if (!need_dia) 
	       {
		  if (apps) evas_list_free(apps);
		  if (mlist) evas_list_free(mlist);
		  return;
	       }
	  }
     }
   if (mlist) evas_list_free(mlist);
   
   /* Actually create the dialog */
   _e_fwin_dlg_new(fwin, apps);
}

static void 
_e_fwin_dlg_new(E_Fwin *fwin, Evas_List *apps) 
{
   E_Fwin_Apps_Dialog *fad;
   E_Dialog *dia;
   Evas *evas;
   Evas_Object *o_list, *o, *of;
   Evas_List *l = NULL, *dl = NULL;
   Ecore_List *cats;
   Evas_Hash *d = NULL;
   Evas_Coord w, h;
   char *cat;
   
   E_OBJECT_CHECK(fwin);
   
   fad = E_NEW(E_Fwin_Apps_Dialog, 1);
   if (fwin->win)
     dia = e_dialog_new(fwin->win->border->zone->container, "E", "_fwin_open_apps");
   else if (fwin->zone)
     dia = e_dialog_new(fwin->zone->container, "E", "_fwin_open_apps");
   e_dialog_title_set(dia, _("Open With..."));
   e_dialog_border_icon_set(dia, "enlightenment/applications");
   e_dialog_button_add(dia, _("Open"), "enlightenment/open", 
		       _e_fwin_cb_open, fad);
   e_dialog_button_add(dia, _("Close"), "enlightenment/close", 
		       _e_fwin_cb_close, fad);
   fad->dia = dia;
   fad->fwin = fwin;
   fwin->fad = fad;
   dia->data = fad;
   e_object_free_attach_func_set(E_OBJECT(dia), _e_fwin_dia_cb_free);
   
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
	     Efreet_Desktop *desk;
	     Evas_Object *oi;
	     
	     desk = l->data;
	     if (!desk) continue;
	     oi = e_util_desktop_icon_add(desk, "24x24", evas);
	     e_widget_ilist_append(o, oi, desk->name, NULL, NULL, 
				   efreet_util_path_to_file_id(desk->orig_path));
	  }
	evas_list_free(apps);
	e_widget_ilist_go(o);
	e_widget_min_size_set(o, 160, 240);
	e_widget_ilist_thaw(o);
	edje_thaw();
	evas_event_thaw(evas);
	e_widget_framelist_object_append(of, o);
	e_widget_list_object_append(o_list, of, 1, 1, 0.5);
	evas_object_smart_callback_add(o, "selected", 
				       _e_fwin_dlg_cb_selected, fad);
     }
   
   /* Build list of All Apps */
   cats = efreet_util_desktop_categories_list();
   ecore_list_sort(cats, ECORE_COMPARE_CB(strcmp), ECORE_SORT_MIN);
   ecore_list_first_goto(cats);
   while ((cat = ecore_list_next(cats))) 
     {
	Ecore_List *desks;
	Efreet_Desktop *desk;
	
	desks = efreet_util_desktop_category_list(cat);
	ecore_list_sort(desks, ECORE_COMPARE_CB(_e_fwin_cb_desk_sort), ECORE_SORT_MIN);
	ecore_list_first_goto(desks);
	while ((desk = ecore_list_next(desks))) 
	  {
	     if (!evas_hash_find(d, desk->name)) 
	       {
		  d = evas_hash_direct_add(d, evas_stringshare_add(desk->name), 
					   desk);
		  dl = evas_list_append(dl, strdup(desk->name));
	       }
	  }
     }
   if (dl)
     dl = evas_list_sort(dl, -1, _e_fwin_cb_desk_list_sort);
   
   of = e_widget_framelist_add(evas, _("All Applications"), 0);
   o = e_widget_ilist_add(evas, 24, 24, &(fad->app2));
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(o);
   
   if ((d) && (dl)) 
     {
	for (l = dl; l; l = l->next) 
	  {
	     Evas_Object *icon;
	     Efreet_Desktop *desk;
	     char *name;
	     
	     name = l->data;
	     if (!name) continue;
	     
	     desk = evas_hash_find(d, name);
	     if (!desk) continue;
	     
	     icon = e_util_desktop_icon_add(desk, "24x24", evas);
	     e_widget_ilist_append(o, icon, desk->name, NULL, NULL,
				   efreet_util_path_to_file_id(desk->orig_path));
	  }
	evas_hash_foreach(d, _e_fwin_hash_cb_free, NULL);
	evas_hash_free(d);
	d = NULL;
	
	while (dl) 
	  {
	     char *n;
	     
	     n = dl->data;
	     dl = evas_list_remove_list(dl, dl);
	     free(n);
	  }
     }
   e_widget_ilist_go(o);
   e_widget_min_size_set(o, 160, 240);
   e_widget_ilist_thaw(o);
   edje_thaw();
   evas_event_thaw(evas);
   e_widget_framelist_object_append(of, o);
   e_widget_list_object_append(o_list, of, 1, 1, 0.5);
   
   e_widget_min_size_get(o_list, &w, &h);
   e_dialog_content_set(dia, o_list, w, h);
   e_dialog_show(dia);
}

static E_Fwin_Exec_Type 
_e_fwin_file_is_exec(E_Fm2_Icon_Info *ici) 
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

static Evas_Bool 
_e_fwin_hash_cb_foreach(Evas_Hash *hash __UNUSED__, 
			const char *key, void *data __UNUSED__, void *fdata) 
{
   Evas_List **mlist;
   
   mlist = fdata;
   *mlist = evas_list_append(*mlist, key);
   return 1;
}

static void
_e_fwin_file_exec(E_Fwin *fwin, E_Fm2_Icon_Info *ici, E_Fwin_Exec_Type ext) 
{
   char buf[4096];
   Efreet_Desktop *desktop;
   
   /* FIXME: execute file ici with either a terminal, the shell, or directly
    * or open the .desktop and exec it */
   switch (ext)
     {
      case E_FWIN_EXEC_NONE:
	break;
      case E_FWIN_EXEC_DIRECT:
	if (fwin->win)
	  e_exec(fwin->win->border->zone, NULL, ici->file, NULL, "fwin");
	else if (fwin->zone)
	  e_exec(fwin->zone, NULL, ici->file, NULL, "fwin");
	break;
      case E_FWIN_EXEC_SH:
	snprintf(buf, sizeof(buf), "/bin/sh %s", e_util_filename_escape(ici->file));
	if (fwin->win)
	  e_exec(fwin->win->border->zone, NULL, buf, NULL, NULL);
	else if (fwin->zone)
	  e_exec(fwin->zone, NULL, buf, NULL, NULL);
	break;
      case E_FWIN_EXEC_TERMINAL_DIRECT:
	snprintf(buf, sizeof(buf), "%s %s", e_config->exebuf_term_cmd, e_util_filename_escape(ici->file));
	if (fwin->win)
	  e_exec(fwin->win->border->zone, NULL, buf, NULL, NULL);
	else if (fwin->zone)
	  e_exec(fwin->zone, NULL, buf, NULL, NULL);
	break;
      case E_FWIN_EXEC_TERMINAL_SH:
	snprintf(buf, sizeof(buf), "%s /bin/sh %s", e_config->exebuf_term_cmd, e_util_filename_escape(ici->file));
	if (fwin->win)
	  e_exec(fwin->win->border->zone, NULL, buf, NULL, NULL);
	else if (fwin->zone)
	  e_exec(fwin->zone, NULL, buf, NULL, NULL);
	break;
      case E_FWIN_EXEC_DESKTOP:
	snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(fwin->o_fm), ici->file);
	desktop = efreet_desktop_new(buf);
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
_e_fwin_cb_open(void *data, E_Dialog *dia) 
{
   E_Fwin_Apps_Dialog *fad;
   Efreet_Desktop *desktop = NULL;
   char pcwd[4096], buf[4096];
   Evas_List *selected, *l;
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
		  ext = _e_fwin_file_is_exec(ici);
		  if (ext != E_FWIN_EXEC_NONE)
		    _e_fwin_file_exec(fad->fwin, ici, ext);
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
_e_fwin_cb_close(void *data, E_Dialog *dia) 
{
   E_Fwin_Apps_Dialog *fad;
   
   fad = data;
   E_OBJECT_CHECK(fad);
   e_object_del(E_OBJECT(fad->dia));
}

static void 
_e_fwin_dia_cb_free(void *obj) 
{
   E_Dialog *dia;
   E_Fwin_Apps_Dialog *fad;
   
   dia = (E_Dialog *)obj;
   fad = dia->data;
   E_FREE(fad->app1);
   E_FREE(fad->app2);
   fad->fwin->fad = NULL;
   E_FREE(fad);
}

static void 
_e_fwin_dlg_cb_selected(void *data, Evas_Object *obj, void *event_info) 
{
   E_Fwin_Apps_Dialog *fad;
   
   fad = data;
   E_OBJECT_CHECK(fad);
   _e_fwin_cb_open(fad, fad->dia);
}

static int 
_e_fwin_cb_desk_sort(Efreet_Desktop *d1, Efreet_Desktop *d2) 
{
   return (strcmp(d1->name, d2->name));
}

static int 
_e_fwin_cb_desk_list_sort(void *data1, void *data2) 
{
   if (!data1) return 1;
   if (!data2) return -1;
   return (strcmp((char *)data1, (char *)data2));
}

static Evas_Bool 
_e_fwin_hash_cb_free(Evas_Hash *hash __UNUSED__, const char *key,
		     void *data, void *fdata __UNUSED__) 
{
   if (key) evas_stringshare_del(key);
   return 1;
}
