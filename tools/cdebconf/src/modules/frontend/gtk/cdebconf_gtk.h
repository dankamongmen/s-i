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


/* Use this struct to store data that shall live between the questions */
struct frontend_data
{
  /* Main window of the frontend */
  GtkWidget *window;
  /* Pointer to the box, where question widgets shall be stored in */
  GtkWidget *target_box; 

  /* Buttons for the duid-like interface */
  GtkWidget *button_next;
  GtkWidget *button_prev;

  /* Pointer to the Progress Bar, when initialized */
  GtkWidget *progress_bar; 

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

/* Function which can be used as callback. Shows the description of specified question 
   in the description area. Can be used when recieiving focus */
gboolean show_description( GtkWidget *widget, struct frontend_question_data* data );
