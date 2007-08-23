#ifdef E_TYPEDEFS

#else
#ifndef E_FWIN_TB_H
#define E_FWIN_TB_H

EAPI Evas_Object *e_toolbar_add(Evas *evas);
EAPI void         e_toolbar_path_set(Evas_Object *obj, const char *path);
EAPI const char  *e_toolbar_path_get(Evas_Object *obj);
EAPI void         e_toolbar_button_enable(Evas_Object *obj, int enable);

#endif
#endif
