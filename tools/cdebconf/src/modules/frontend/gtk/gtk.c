/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: gtk.c
 *
 * Description: gtk UI for cdebconf
 * Some notes on the implementation - optimistic at best. 
 *  mbc - just to get this off of the ground, Im' creating a dialog
 *        and calling gtk_main for each question. once I get the tests
 *        running, I'll probably send a delete_event signal in the
 *        next and back button callbacks. 
 *    
 *        There is some rudimentary attempt at implementing the next
 *        and back functionality. 
 *
 * $Id: gtk.c,v 1.10 2002/12/17 23:07:44 barbier Exp $
 *
 * cdebconf is (c) 2000-2001 Randolph Chung and others under the following
 * license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ***********************************************************************/
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

#include <gtk/gtk.h>

struct multicheck_data
{
  int multiChoices[100];
  GtkWidget **choices;
};


/*
 * Function: passwd_callback
 * Input: none
 * Output: none
 * Description: catch value in password entry box
 * Assumptions: 
 */
void passwd_callback( GtkWidget *widget,
                     GtkWidget *data )
{
  const char *entrytext;
  char **callstring;

  callstring = (char **)data;

  entrytext = gtk_entry_get_text (GTK_ENTRY (widget));
  
  *callstring = strdup(entrytext);

  printf("Password captured.\n");
}

/*
 * Function: check_callback
 * Input: none
 * Output: none
 * Description: catch value of checkbox when it changes
 * Assumptions: 
 */
void check_callback( GtkWidget *widget,
            gpointer   data )
{
  gboolean *gbool;

  gbool = (gboolean *)data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) 
    {
      /* If control reaches here, the toggle button is down */
      *gbool = TRUE;
    
    } else {
    
      /* If control reaches here, the toggle button is up */
      *gbool = FALSE;
    }
}

/*
 * Function: mullticheck_callback
 * Input: none
 * Output: none
 * Description: add choice to list of answers
 * Assumptions: 
 */
void multicheck_callback( GtkWidget *widget,
            gpointer   data )
{
  int i, val; 
  gboolean done;
  struct multicheck_data *call_data;
  GtkWidget **choices;

  call_data = (struct multicheck_data *)data;
  choices = call_data->choices;

  i = 0;
  done = FALSE;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) 
    {
      /* If control reaches here, the toggle button is down */
      val = 1;
    
    } else {
    
      /* If control reaches here, the toggle button is up */
      val = 0;
    }


  while(*choices && !done)
    {
      if(*choices == widget)
	{
	  call_data->multiChoices[i] = val;
	  done = TRUE;
	}

      i++;
      choices++;
    }

}


/*
 * Function: select_callback
 * Input: none
 * Output: none
 * Description: add choice to list of answers
 * Assumptions: 
 */
void select_callback( GtkWidget *widget,
            gpointer   data )
{
  GtkWidget **radio;

  radio = (GtkWidget **)data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) 
    {
      /* If control reaches here, the toggle button is down */
      printf("selected: %p\n", widget);
      *radio = widget;

    } else {
    
      /* If control reaches here, the toggle button is up */
      printf("deselected: %p\n", widget);
    }

}


/*
 * Function: next
 * Input: none
 * Output: none
 * Description: continue to next question 
 * Assumptions: 
 */
void next( GtkWidget *widget,
            gpointer   data )
{
  GtkWidget *window;

  window = (GtkWidget *)data;

  g_print (_("next\n"));
  gtk_widget_hide(window);
  gtk_main_quit();
}

/*
 * Function: back
 * Input: none
 * Output: none
 * Description: go back to previous question. 
 * Assumptions: the question handler loop will increment before continuing
 *              so move back two questions in order to move back one. 
 */
void back( GtkWidget *widget,
            gpointer   data )
{
  struct question **pq;
  
  pq = (struct question **)data;

  *pq = (*pq)->prev->prev;

  g_print (_("back\n"));
  gtk_main_quit();
}

/*
 * Function: delete_event
 * Input: none
 * Output: none
 * Description: callback function for delete event
 * Assumptions: 
 */

gint delete_event( GtkWidget *widget,
                   GdkEvent  *event,
		   gpointer   data )
{
  /* If you return FALSE in the "delete_event" signal handler,
   * GTK will emit the "destroy" signal. Returning TRUE means
   * you don't want the window to be destroyed.
   * This is useful for popping up 'are you sure you want to quit?'
   * type dialogs. */

  g_print (_("delete event occurred\n"));

  /* Change TRUE to FALSE and the main window will be destroyed with
   * a "delete_event". */

  return TRUE;
}

/*
 * Function: destroy
 * Input: none
 * Output: none
 * Description: callback function for close button
 * Assumptions: 
 */
void destroy( GtkWidget *widget,
              gpointer   data )
{
  gtk_main_quit ();
}

////////end callbacks

void add_common_buttons(GtkWidget *window, GtkWidget *box, GtkWidget *bigbox, struct question *q)
{
	GtkWidget *nextButton, *backButton;
	GtkWidget *descbox, *label, *elabel, *separator;

	gtk_window_set_title(GTK_WINDOW(window),
			question_get_field(q, "", "description"));

	//set up window signal handlers
	//TODO: move this out of here!
	g_signal_connect (G_OBJECT (window), "delete_event",
			  G_CALLBACK (delete_event), NULL);
	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (destroy), NULL);

	gtk_container_set_border_width (GTK_CONTAINER (window), 5);

	//add question descriptions
	descbox = gtk_vbox_new(0, 0);

	label = gtk_label_new(question_get_field(q, "", "description"));
	elabel = gtk_label_new(question_get_field(q, "", "extended_description"));
	gtk_box_pack_start(GTK_BOX(descbox), label, FALSE, 5, 5);
	gtk_box_pack_start(GTK_BOX(descbox), elabel, FALSE, 5, 5);
	gtk_widget_show(label);
	gtk_widget_show(elabel);
	gtk_box_pack_start(GTK_BOX(bigbox), descbox, FALSE, 5, 5);	
	gtk_widget_show(descbox);

	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (descbox), separator, FALSE, TRUE, 0);
	gtk_widget_show (separator);

	//add next and back buttons
	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (box), separator, FALSE, TRUE, 0);
	gtk_widget_show (separator);


	if(q->prev)
	  {
	    backButton = gtk_button_new_with_label ("Back");
	    g_signal_connect (G_OBJECT (backButton), "clicked",
			  G_CALLBACK (back), &q);
	    gtk_box_pack_start(GTK_BOX(box), backButton, FALSE, 5, 5);
	    gtk_widget_show (backButton);
	  }


	if(q->next)
	  nextButton = gtk_button_new_with_label ("Next");
	else
	  nextButton = gtk_button_new_with_label("Finish");

	g_signal_connect (G_OBJECT (nextButton), "clicked",
			  G_CALLBACK (next), window);

	gtk_box_pack_start(GTK_BOX(box), nextButton, FALSE, 5, 5);
	gtk_widget_show (nextButton);

	//not sure how to make the layout prettier, revisit this later...
	//gtk_misc_set_alignment (GTK_MISC (backButton), (gfloat)1.0, (gfloat)1.0);

}



/*
 * Function: gtkhandler_boolean
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the boolean question type
 * Assumptions: none
 */
static int gtkhandler_boolean(struct frontend *obj, struct question *q)
{
	int def = -1;
	const char *defval;
	GtkWidget *window;
	GtkWidget *boolButton;
	GtkWidget *hboxtop, *hboxbottom, *bigbox;
	gboolean gbool;

	defval = question_getvalue(q, "");

	if (defval)
	{
		if (strcmp(defval, "true") == 0)
			def = 1;
		else 
			def = 0;
	}

	//create the window for the question
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	bigbox = gtk_vbox_new(FALSE, 0);
	hboxtop = gtk_hbox_new(0, 0);
	hboxbottom = gtk_hbox_new(0, 0);

	//add a checkbox for the boolean, we wouldn't really want a whole dialog for one boolean,
	// but this is a start, in the future, I'll have to step through the questions and make
	// intelligent groupings

	boolButton = gtk_check_button_new_with_label ( (gchar *)question_get_field(q, "", "description") );
	g_signal_connect (G_OBJECT (boolButton), "clicked",
			  G_CALLBACK (check_callback), &gbool);
	gtk_box_pack_start(GTK_BOX(hboxtop), boolButton, FALSE, 5, 5);
	gtk_widget_show (boolButton);		

	add_common_buttons(window, hboxbottom, bigbox, q);

	gtk_box_pack_start(GTK_BOX(bigbox), hboxtop, FALSE, 5, 5);
	gtk_box_pack_start(GTK_BOX(bigbox), hboxbottom, FALSE, 5, 5);

	gtk_container_add(GTK_CONTAINER(window), bigbox);

	//show the dialog containing the question
	gtk_widget_show (hboxbottom);	
	gtk_widget_show (hboxtop);		
	gtk_widget_show (bigbox);		
	gtk_widget_show (window);

	gtk_main ();

	question_setvalue(q, (gbool ? "true" : "false"));

	return DC_OK;
}

/*
 * Function: gtkhandler_multiselect
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the multiselect question type
 * Assumptions: none
 *
 * TODO: factor common code with select
 *       once I figure out how the flow works, make this multiselect instead of select
 */
static int gtkhandler_multiselect(struct frontend *obj, struct question *q)
{
	char *choices[100] = {0};
	char *choices_translated[100] = {0};
	char *defaults[100] = {0};
	char selected[100] = {0};
	char answer[1024];
	int i, j, count, dcount;
	GtkWidget *window;
	GtkWidget *hboxtop, *hboxbottom, *bigbox;
	GtkWidget **choiceButtons;
	struct multicheck_data call_data;

	memset(&call_data.choices, 0, sizeof(call_data.multiChoices));

	count = strchoicesplit(question_get_field(q, NULL, "choices"), choices, DIM(choices));
	if (count <= 0) return DC_NOTOK;

	strchoicesplit(question_get_field(q, "", "choices"), choices_translated, DIM(choices_translated));
	dcount = strchoicesplit(question_get_field(q, NULL, "value"), defaults, DIM(defaults));

	choiceButtons = (GtkWidget **)malloc( (count + 1) * (sizeof(GtkWidget *)) );
	choiceButtons[count] = 0;

	for (j = 0; j < dcount; j++)
		for (i = 0; i < count; i++)
			if (strcmp(choices[i], defaults[j]) == 0)
			    {
				selected[i] = 1;
				call_data.multiChoices[i] = 1;
			    }

	//create the window for the question
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	bigbox = gtk_vbox_new(FALSE, 0);
	hboxbottom = gtk_hbox_new(0, 0);
	hboxtop = gtk_vbox_new(0, 0);

	//add the next and back buttons

	add_common_buttons(window, hboxbottom, bigbox, q);

	call_data.choices = choiceButtons;

	for (i = 0; i < count; i++)
	  {
	    //create a button for each choice here and init the global place to store them

	    choiceButtons[i] = gtk_check_button_new_with_label ( (gchar *)choices_translated[i] );
	    g_signal_connect (G_OBJECT (choiceButtons[i]), "clicked",
			      G_CALLBACK (multicheck_callback), &call_data);
	    gtk_box_pack_start(GTK_BOX(hboxtop), choiceButtons[i], FALSE, 5, 0);
	    gtk_widget_show (choiceButtons[i]);
			
	  }

	gtk_box_pack_start(GTK_BOX(bigbox), hboxtop, FALSE, 5, 5);
	gtk_box_pack_start(GTK_BOX(bigbox), hboxbottom, FALSE, 5, 5);

	//add the main box to the layout
	gtk_container_add(GTK_CONTAINER(window), bigbox);

	//show the dialog containing the question
	gtk_widget_show (hboxbottom);	
	gtk_widget_show (hboxtop);		
	gtk_widget_show (bigbox);		
	gtk_widget_show (window);

	gtk_main();

	//once the dialog returns, handle the result

	*answer = 0;
	j = 0;

	for (i = 0; i < count; i++)
	  {
	    if(call_data.multiChoices[i])
	      {
		if(j)
		  {
		    strcat(answer, ", ");
		  }

		strcat(answer, choices[i]);
		j++;
	      }
	    free(choices[i]);
	    free(choices_translated[i]);
	  }

	for (i = 0; i < dcount; i++)
		free(defaults[i]);

	free(choiceButtons);
	
	printf("%s", answer);

	question_setvalue(q, answer);
	
	return DC_OK;
}

/*
 * Function: gtkhandler_note
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the note question type
 * Assumptions: none
 *
 * Note: this is the simplest gtk question. It can be used as a starting 
 *  point for adding question handlers
 *
 */
static int gtkhandler_note(struct frontend *obj, struct question *q)
{
	GtkWidget *window;
	GtkWidget *hboxtop, *hboxbottom, *bigbox;

	//create the window for the question
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	bigbox = gtk_vbox_new(FALSE, 0);
	hboxbottom = gtk_hbox_new(0, 0);
	hboxtop = gtk_vbox_new(0, 0);

	add_common_buttons(window, hboxbottom, bigbox, q);

	gtk_box_pack_start(GTK_BOX(bigbox), hboxtop, FALSE, 5, 5);
	gtk_box_pack_start(GTK_BOX(bigbox), hboxbottom, FALSE, 5, 5);

	//add the main box to the layout
	gtk_container_add(GTK_CONTAINER(window), bigbox);

	//show the dialog containing the question
	gtk_widget_show (hboxbottom);	
	gtk_widget_show (hboxtop);		
	gtk_widget_show (bigbox);		
	gtk_widget_show (window);

	gtk_main();

	return DC_OK;
}

/*
 * Function: gtkhandler_password
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the password question type
 * Assumptions: none
 *
 * TODO: test this! not sure this makes sense
 *
 *
 */
static int gtkhandler_password(struct frontend *obj, struct question *q)
{
	char passwd[256] = {0};
	GtkWidget *window, *hboxbottom, *hboxtop, *bigbox;
	GtkWidget *entry;
	char *callstring = NULL;

	//create the window for the question
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	bigbox = gtk_vbox_new(FALSE, 0);
	hboxbottom = gtk_hbox_new(0, 0);
	hboxtop = gtk_hbox_new(0, 0);

	add_common_buttons(window, hboxbottom, bigbox, q);

	//add actual password field
	entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
	
	g_signal_connect (G_OBJECT (entry), "activate",
			  G_CALLBACK (passwd_callback),
			  &callstring);
	gtk_box_pack_start (GTK_BOX (hboxtop), entry, TRUE, TRUE, 0);
	gtk_widget_show (entry);

	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);

	gtk_box_pack_start(GTK_BOX(bigbox), hboxtop, FALSE, 5, 5);
	gtk_box_pack_start(GTK_BOX(bigbox), hboxbottom, FALSE, 5, 5);

	//add main box to layout
	gtk_container_add(GTK_CONTAINER(window), bigbox);

	//show the dialog containing the question
	gtk_widget_show (hboxbottom);	
	gtk_widget_show (hboxtop);		
	gtk_widget_show (bigbox);		
	gtk_widget_show (window);

	gtk_main();

	if(callstring)
	  strcpy(passwd, callstring);
	else
	  strcpy(passwd, "");

	question_setvalue(q, passwd);
	return DC_OK;
}


/*
 * Function: gtkhandler_select
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the select question type
 * Assumptions: none
 *
 * TODO: factor common code with multiselect
 */
static int gtkhandler_select(struct frontend *obj, struct question *q)
{

	char *choices[100] = {0};
	char *choices_translated[100] = {0};
	char answer[10];
	int i, count, choice = 1, def = -1;
	const char *defval = question_getvalue(q, "");
	GtkWidget *window;
	GtkWidget *hboxtop, *hboxbottom, *bigbox;
	GtkWidget **choiceButtons;
	GtkWidget *firstButton, *nextButton;
	GtkWidget *radio;

	count = strchoicesplit(question_get_field(q, NULL, "choices"), choices, DIM(choices));
	if (count <= 0) return DC_NOTOK;
	if (count == 1)
		defval = choices[0];

	strchoicesplit(question_get_field(q, "", "choices"), choices_translated, DIM(choices_translated));

	choiceButtons = (GtkWidget **)malloc(  (count+1) * ( sizeof(GtkWidget *) )  );

	choiceButtons[count + 1] = 0;

	if (defval != NULL)
		for (i = 0; i < count; i++) 
			if (strcmp(choices[i], defval) == 0)
				def = i + 1;

	//create the window for the question
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	bigbox = gtk_vbox_new(FALSE, 0);
	hboxbottom = gtk_hbox_new(0, 0);
	hboxtop = gtk_vbox_new(0, 0);

	add_common_buttons(window, hboxbottom, bigbox, q);

	firstButton = NULL;

	for (i = 0; i < count; i++)
	  {
	    /*create a button for each choice here*/
	    if(i == 0)
	      {
	        nextButton = gtk_radio_button_new_with_label (NULL, choices_translated[i]);
	      }
	    else
	      {
		nextButton = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON (firstButton), 
								      choices_translated[i]);
	      }

	    choiceButtons[i] = nextButton;
	    
	    g_signal_connect (G_OBJECT (nextButton), "clicked",
			      G_CALLBACK (select_callback), &radio);

	    gtk_box_pack_start (GTK_BOX (hboxtop), nextButton, TRUE, TRUE, 0);
	    gtk_widget_show (nextButton);

	    if(i == def)
	      {
		printf("setting default to %d\n", i);
		//GTK_WIDGET_SET_FLAGS (nextButton, GTK_CAN_DEFAULT);
		//gtk_widget_grab_default (nextButton);
	      }

	    firstButton = nextButton;
	  }

	gtk_box_pack_start(GTK_BOX(bigbox), hboxtop, FALSE, 5, 5);
	gtk_box_pack_start(GTK_BOX(bigbox), hboxbottom, FALSE, 5, 5);

	gtk_container_add(GTK_CONTAINER(window), bigbox);

	//show the dialog containing the question
	gtk_widget_show (hboxbottom);	
	gtk_widget_show (hboxtop);		
	gtk_widget_show (bigbox);		
	gtk_widget_show (window);

	gtk_main();

        for (i = 0; choiceButtons[i] != NULL; i++)
            if (choiceButtons[i] == radio)
            {
                choice = i;
                break;
            }

	printf("selected: %d\n", choice);

	//once the dialog returns, handle the result
	printf("freeing %d buttons\n", count);

	free(choiceButtons);

	printf("selectnum: %d\n", choice);

	if(choice >= 0 && choice < count)
	  {
	    strcpy(answer, choices[choice]);
	  }

	printf("answer: %s\n", answer);

	question_setvalue(q, answer);

	return DC_OK;
}

/*
 * Function: gtkhandler_string
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the string question type
 * Assumptions: none
 *
 * Todo: Use password code here, just without the visibility disabled
 * should put it into a function with a visibility bool
 */ 
static int gtkhandler_string(struct frontend *obj, struct question *q)
{
	char passwd[256] = {0};
	GtkWidget *window, *hboxbottom, *hboxtop, *bigbox;
	GtkWidget *entry;
	char *callstring = NULL;

	//create the window for the question
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	bigbox = gtk_vbox_new(FALSE, 0);
	hboxbottom = gtk_hbox_new(0, 0);
	hboxtop = gtk_hbox_new(0, 0);

	add_common_buttons(window, hboxbottom, bigbox, q);

	//add actual text entry field
	entry = gtk_entry_new ();

	gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
	g_signal_connect (G_OBJECT (entry), "activate",
			  G_CALLBACK (passwd_callback),
			  &callstring);
	gtk_box_pack_start (GTK_BOX (hboxtop), entry, TRUE, TRUE, 0);
	gtk_widget_show (entry);

	//add the boxes to the main layout object
	gtk_box_pack_start(GTK_BOX(bigbox), hboxtop, FALSE, 5, 5);
	gtk_box_pack_start(GTK_BOX(bigbox), hboxbottom, FALSE, 5, 5);

	//add main box to layout
	gtk_container_add(GTK_CONTAINER(window), bigbox);

	//show the dialog containing the question
	gtk_widget_show (hboxbottom);	
	gtk_widget_show (hboxtop);		
	gtk_widget_show (bigbox);		
	gtk_widget_show (window);

	gtk_main();

	if(callstring)
	  strcpy(passwd, callstring);
	else
	  strcpy(passwd, "");

	question_setvalue(q, passwd);

	return DC_OK;
}


/*
 * Function: gtkhandler_text
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the text question type
 * Assumptions: none
 *
 * need a text entry field here. 
 *
 */
 
static int gtkhandler_text(struct frontend *obj, struct question *q)
{
	char buf[1024];
	GtkWidget *window;
	GtkWidget *hboxtop, *hboxbottom, *bigbox;
	GtkWidget *view, *sw;
	GtkTextBuffer *buffer;
        char *text;
        GtkTextIter start, end;
      
	//create the window for the question
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	bigbox = gtk_vbox_new(FALSE, 0);
	hboxbottom = gtk_hbox_new(0, 0);
	hboxtop = gtk_vbox_new(0, 0);

	add_common_buttons(window, hboxbottom, bigbox, q);

	//add the text entry field
	view = gtk_text_view_new ();
	//gtk_box_pack_start(GTK_BOX(hboxtop), view, FALSE, 5, 5);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	gtk_box_pack_start(GTK_BOX(hboxtop), sw, FALSE, 5, 5);

	//gtk_container_add (window, vpaned);

	gtk_container_add (GTK_CONTAINER (sw), view);

	gtk_widget_show (view);	
	gtk_widget_show (sw);	

	//adjust the size
      
	gtk_widget_set_size_request(view, 200, 100);

	//add the boxes to the main layout object
	gtk_box_pack_start(GTK_BOX(bigbox), hboxtop, FALSE, 5, 5);
	gtk_box_pack_start(GTK_BOX(bigbox), hboxbottom, FALSE, 5, 5);

	//add main box to layout
	gtk_container_add(GTK_CONTAINER(window), bigbox);

	//show the dialog containing the question
	gtk_widget_show (hboxbottom);	
	gtk_widget_show (hboxtop);		
	gtk_widget_show (bigbox);		
	gtk_widget_show (window);

	gtk_main();

	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);

	text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	printf("text captured: %s.\n", text);

	strcpy(buf, text);

	question_setvalue(q, buf);  

	return DC_OK;
}


/* ----------------------------------------------------------------------- */
struct question_handlers {
	const char *type;
	int (*handler)(struct frontend *obj, struct question *q);
} question_handlers[] = {
        { "boolean",	gtkhandler_boolean },
	{ "multiselect", gtkhandler_multiselect },
	{ "note",	gtkhandler_note },
	{ "password",	gtkhandler_password },
	{ "select",	gtkhandler_select }, //needs work
	{ "string",	gtkhandler_string },
	{ "text",	gtkhandler_text }
};

/*
 * Function: gtk_intitialize
 * Input: struct frontend *obj - frontend UI object
 *        struct configuration *cfg - configuration parameters
 * Output: int - DC_OK, DC_NOTOK
 * Description: initializes the gtk UI
 * Assumptions: none
 *
 * TODO: get argc and argv to pass to gtkinit
 */
static int gtk_initialize(struct frontend *obj, struct configuration *conf)
{
	obj->interactive = 1;
	//signal(SIGINT, SIG_IGN);
	
	gtk_init (0, "cdebconf");

	return DC_OK;
}

/*
 * Function: gtk_go
 * Input: struct frontend *obj - frontend object
 * Output: int - DC_OK, DC_NOTOK
 * Description: asks all pending questions
 * Assumptions: none
 */
static int gtk_go(struct frontend *obj)
{
	struct question *q = obj->questions;
	int i;
	int ret;
	/*	char *window_title;
	int len;

	//printf("%s\n\n", obj->title);

	len = strlen(obj->title);
	window_title = (char *)malloc(len+1);
	strcpy(window_title, obj->title);
	*/

	for (; q != 0; q = q->next)
	{
		for (i = 0; i < DIM(question_handlers); i++)
			if (strcmp(q->template->type, question_handlers[i].type) == 0)
			{
			        //the question descriptions are displayed in add_common_buttons
			        //gtkhandler_displaydesc(obj, q);

			        //maybe I should move the main window stuff out here, so as to handle
			        //  displaying the description once
 
			        ret = question_handlers[i].handler(obj, q);
				if (ret == DC_OK)
					obj->qdb->methods.set(obj->qdb, q);
				else
					return ret;
				
				//I think we need to go back two because the loop 
				//if(gBack && !gNext)
				//q = q->prev->prev;

				break;
			}
	}

	return DC_OK;
}

struct frontend_module debconf_frontend_module =
{
	initialize: gtk_initialize,
	go: gtk_go,
};
