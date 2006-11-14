#include "common.h"
#include "template.h"
#include "question.h"
#include "frontend.h"
#include "database.h"
#include "strutl.h"

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>

#include <gtk/gtk.h>

/* horizontal padding between a question and questionbox frame */
#define QUESTIONBOX_HPADDING 6
/* vertical padding between two adjacent questions inside questionbox */
#define QUESTIONBOX_VPADDING 3
/* default padding used troughout the GTK frontend */
#define DEFAULT_PADDING 6

/* used by the treeview widgets */
enum
{
    SELECT_COL_NAME,
    SELECT_NUM_COLS
} ;

enum
{
  MULTISELECT_COL_BOOL ,
  MULTISELECT_COL_NAME ,
  MULTISELECT_NUM_COLS
} ;

/* Use this struct to store data that shall live between the questions */
struct frontend_data
{
  /* Main window of the frontend */
  GtkWidget *window;

  /* Label used to display frontend's title */
  GtkWidget *title;
  
  /* Pointer to the box, where question widgets shall be stored in */
  GtkWidget *target_box; 

  /* Buttons for the druid-like interface */
  GtkWidget *button_next;
  GtkWidget *button_prev;
  GtkWidget *button_screenshot;
  GtkWidget *button_cancel;

  /* Pointer to the Progress Bar, when initialized */
  GtkWidget *progress_bar; 
  
  /* Pointer to the progressbar's label */
  GtkWidget *progress_bar_label;

  /* Pointer to the progressbar's container */
  GtkWidget *progress_bar_box;

  /* Struct to register the Set Functions of the Widgets */
  struct setter_struct *setters; 
  /* Value of the button pressed to leave a form */
  int button_val;
};

/* Embed frontend ans question in this object to pass it through an event handler */
struct frontend_question_data
{
    struct frontend *obj;
    struct question *q;
};

/* needed to pass data to single MULTISELECT question callback function */
struct question_treemodel_data
{
    struct question *q;
	GtkTreeModel *treemodel;
};

/* used to pass default path and callback function id to single SELECT question callback function */
struct treeview_expose_callback_data
{
    gchar *path;
    gulong callback_function;
};

/* Functions registered here will be called after each question run. It is to be used
   to retrieve the data from the widgets and store it in the question database */
void register_setter(void (*func)(void*, struct question*), 
		     void *data, struct question *q, struct frontend *obj);

/* Returns TRUE if the question specified is the first question in a row. To be used to
   determine if an associated widget should grab the focus. */
gboolean is_first_question(struct question *q);

/* Used to free the frontend_question_data struct which is used to transport data
   via callbacks. Can be used as a callback for the destroy signal of a widget associated
   to the question.*/
void free_description_data( GtkObject *obj, struct frontend_question_data* data );
