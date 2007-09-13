typedef struct _E_Fwin E_Fwin;
typedef struct _E_Fwin_Apps_Dialog E_Fwin_Apps_Dialog;

#define E_FWIN_TYPE 0xE0b0101f

struct _E_Fwin 
{
   E_Object e_obj_inherit;
   
   E_Win *win;
   E_Zone *zone;
   E_Fwin_Apps_Dialog *fad;
   
   Evas_Object *o_scroll;
   Evas_Object *o_fm, *o_bg, *o_tb;
   Evas_Object *o_under, *o_over;
   
   struct 
     {
	Evas_Coord x, y, mx, my, w, h;
     } fm_pan, fm_pan_last;
   
   const char *bg_file, *over_file, *scroll_file, *theme_file;

   Ecore_Event_Handler *zone_handler;
};

struct _E_Fwin_Apps_Dialog 
{
   E_Dialog *dia;
   E_Fwin *fwin;
   char *app1, *app2;
   Evas_Object *o_ilist, *o_all;
};

typedef enum 
{
   E_FWIN_EXEC_NONE,
   E_FWIN_EXEC_DIRECT,
   E_FWIN_EXEC_SH,
   E_FWIN_EXEC_TERMINAL_DIRECT,
   E_FWIN_EXEC_TERMINAL_SH,
   E_FWIN_EXEC_DESKTOP
} E_Fwin_Exec_Type;
