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
 * $Id: gtk.c,v 1.1 2002/09/05 12:30:23 tfheen Exp $
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

#ifndef _
#define _(x) x
#endif

int gBool;
int gSelectNum;
char *gPassword;
int gMultiChoices[100];
gboolean gBack, gNext;
GtkWidget *gRadio;

/*
 * Function: passwd_callback
 * Input: none
 * Output: none
 * Description: catch value in password entry box
 * Assumptions: 
 */
void passwd_callback( GtkWidget *widget,
                     GtkWidget *entry )
{
  const char *entrytext;
  entrytext = gtk_entry_get_text (GTK_ENTRY (entry));
  
  gPassword = malloc(strlen(entrytext));
  strcpy(gPassword, entrytext);
  
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
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) 
    {
      /* If control reaches here, the toggle button is down */
      gBool = 1;
    
    } else {
    
      /* If control reaches here, the toggle button is up */
      gBool = 0;
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
  GtkWidget **choices;
  gboolean done;

  choices = (GtkWidget **)data;
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
	  gMultiChoices[i] = val;
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

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) 
    {
      /* If control reaches here, the toggle button is down */
      printf("selected: %p\n", widget);
      gRadio = widget;

    } else {
    
      /* If control reaches here, the toggle button is up */
      printf("deselected: %p\n", widget);
    }

}


/*
 * Function: choice_callback
 * Input: none
 * Output: none
 * Description: respond to button press and determing which button was pressed
 * Assumptions: 
 */
void choice_callback( GtkWidget *widget,
            gpointer   data )
{
  int i; 
  GtkWidget **choices;
  gboolean done;

  choices = (GtkWidget **)data;
  i = 0;
  done = FALSE;

  while(*choices && !done)
    {
      if(*choices == widget)
	{
	  gSelectNum = i;
	  done = TRUE;
	}

      i++;
      choices++;
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
  gNext = TRUE;
  gBack = FALSE;

  g_print ("next\n");
  gtk_main_quit();
}

/*
 * Function: back
 * Input: none
 * Output: none
 * Description: go back to previous question
 * Assumptions: 
 */
void back( GtkWidget *widget,
            gpointer   data )
{
  gBack = TRUE;
  gNext = FALSE;

  g_print ("back\n");
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

  g_print ("delete event occurred\n");

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

	//set up window signal handlers
	//TODO: move this out of here!
	g_signal_connect (G_OBJECT (window), "delete_event",
			  G_CALLBACK (delete_event), NULL);
	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (destroy), NULL);

	gtk_container_set_border_width (GTK_CONTAINER (window), 5);

	//add question descriptions
	descbox = gtk_vbox_new(0, 0);

	label = gtk_label_new( question_description(q) );
	elabel = gtk_label_new( question_extended_description(q) );
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
			  G_CALLBACK (back), NULL);
	    gtk_box_pack_start(GTK_BOX(box), backButton, FALSE, 5, 5);
	    gtk_widget_show (backButton);
	  }


	if(q->next)
	  nextButton = gtk_button_new_with_label ("Next");
	else
	  nextButton = gtk_button_new_with_label("Finish");

	g_signal_connect (G_OBJECT (nextButton), "clicked",
			  G_CALLBACK (next), NULL);
	gtk_box_pack_start(GTK_BOX(box), nextButton, FALSE, 5, 5);
	gtk_widget_show (nextButton);

	//not sure how to make the layout prettier, revisit this later...
	//gtk_misc_set_alignment (GTK_MISC (backButton), (gfloat)1.0, (gfloat)1.0);

}



/*
 * Function: getwidth
 * Input: none
 * Output: int - width of screen
 * Description: get the width of the current terminal
 * Assumptions: doesn't handle resizing; caches value on first call
 * 
 * TODO: implement this to get the size of the X desktop
 
static const int getwidth(void)
{
	static int res = 80;
	static int inited = 0;
	int fd;
	struct winsize ws;

	if (inited == 0)
	{
		inited = 1;
		if ((fd = open("/dev/tty", O_RDONLY)) > 0)
		{
			if (ioctl(fd, TIOCGWINSZ, &ws) == 0)
				res = ws.ws_col;
			close(fd);
		}
	}
	return res;
}
*/


/*
 * Function: wrap_print
 * Input: const char *str - string to display
 * Output: none
 * Description: prints a string to the screen with word wrapping 
 * Assumptions: string fits in <500 lines
 *  Simple greedy line-wrapper 
 
static void wrap_print(const char *str)
{

	int i, lc;
	char *lines[500];

	lc = strwrap(str, getwidth() - 1, lines, DIM(lines));

	for (i = 0; i < lc; i++)
	{
		printf("%s\n", lines[i]);
		DELETE(lines[i]);
	}
}
*/

/*
 * Function: texthandler_displaydesc
 * Input: struct frontend *obj - UI object
 *        struct question *q - question for which to display the description
 * Output: none
 * Description: displays the description for a given question 
 * Assumptions: none
 *
static void texthandler_displaydesc(struct frontend *obj, struct question *q) 
{
	wrap_print(question_description(q));
	wrap_print(question_extended_description(q));
}*/

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
	int ans = -1;
	int def = -1;
	const char *defval;
	GtkWidget *window;
	GtkWidget *boolButton;
	GtkWidget *hboxtop, *hboxbottom, *bigbox;

	defval = question_defaultval(q);
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

	boolButton = gtk_check_button_new_with_label ( (gchar *)question_description(q) );
	g_signal_connect (G_OBJECT (boolButton), "clicked",
			  G_CALLBACK (check_callback), NULL);
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

	if(gBool)
	  {
	    ans = 1;
	  }
	else
	  {
	    ans = 0;
	  }

	question_setvalue(q, (ans ? "true" : "false"));
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
	char *defaults[100] = {0};
	char selected[100] = {0};
	char answer[1024];
	int i, j, count, dcount;
	GtkWidget *window;
	GtkWidget *hboxtop, *hboxbottom, *bigbox;
	GtkWidget **choiceButtons;

	count = strchoicesplit(question_choices(q), choices, DIM(choices));
	dcount = strchoicesplit(question_defaultval(q), defaults, DIM(defaults));

	choiceButtons = (GtkWidget **)malloc( (count * (sizeof(GtkWidget *)) + 1) );

	for(i = 0; i < count; i++)
	  choiceButtons[i] = (GtkWidget *)malloc(sizeof(GtkWidget *));

	choiceButtons[count + 1] = 0;

	if (dcount > 0)
		for (i = 0; i < count; i++)
			for (j = 0; j < dcount; j++)
				if (strcmp(choices[i], defaults[j]) == 0)
					{
					  selected[i] = 1;
					  gMultiChoices[i] = 1;
					}

	//create the window for the question
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	bigbox = gtk_vbox_new(FALSE, 0);
	hboxbottom = gtk_hbox_new(0, 0);
	hboxtop = gtk_vbox_new(0, 0);

	//add the next and back buttons

	add_common_buttons(window, hboxbottom, bigbox, q);

	for (i = 0; i < count; i++)
	  {
	    //create a button for each choice here and init the global place to store them

	    choiceButtons[i] = gtk_check_button_new_with_label ( (gchar *)choices[i] );
	    g_signal_connect (G_OBJECT (choiceButtons[i]), "clicked",
			      G_CALLBACK (multicheck_callback), choiceButtons);
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
	    if(gMultiChoices[i])
	      {
		if(j)
		  {
		    strcat(answer, ", ");
		  }

		strcat(answer, choices[i]);
		j++;
	      }
	  }

	for (i = 0; i < count; i++)
		free(choiceButtons[i]);

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
			  entry);
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

	strcpy(passwd, gPassword);

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

	char *choices[100] = {"rand", "flaubert", "sartre"};
	char *defaults[100] = {"sartre"};
	char selected[100] = {0};
	char answer[1024];
	int i, j, count, dcount;
	GtkWidget *window;
	GtkWidget *hboxtop, *hboxbottom, *bigbox;
	GtkWidget **choiceButtons;
	GtkWidget *firstButton, *nextButton;

	count = strchoicesplit(question_choices(q), choices, DIM(choices));
	dcount = strchoicesplit(question_defaultval(q), defaults, DIM(defaults));

	choiceButtons = (GtkWidget **)malloc( (count * (sizeof(GtkWidget *)) + 1) );

	for(i = 0; i < count; i++)
	  choiceButtons[i] = (GtkWidget *)malloc(sizeof(GtkWidget *));

	choiceButtons[count + 1] = 0;

	if (dcount > 0)
		for (i = 0; i < count; i++)
			for (j = 0; j < dcount; j++)
				if (strcmp(choices[i], defaults[j]) == 0)
					{
					  selected[i] = 1;
					  gSelectNum = i;
					}

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
	        nextButton = gtk_radio_button_new_with_label (NULL, choices[i]);
	      }
	    else
	      {
		nextButton = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON (firstButton), 
								      choices[i]);
	      }

	    choiceButtons[i] = nextButton;
	    
	    g_signal_connect (G_OBJECT (nextButton), "clicked",
			      G_CALLBACK (select_callback), choiceButtons);

	    gtk_box_pack_start (GTK_BOX (hboxtop), nextButton, TRUE, TRUE, 0);
	    gtk_widget_show (nextButton);

	    if(i == gSelectNum)
	      {
		printf("setting default to %d\n", i);
		GTK_WIDGET_SET_FLAGS (nextButton, GTK_CAN_DEFAULT);
		gtk_widget_grab_default (nextButton);
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

	i = 0;

	while(*choiceButtons)
	  {
	    if(*choiceButtons == gRadio)
	      {
		gSelectNum = i;
		break;
	      }
	    else
	      {
		i++;
		choiceButtons++;
	      }
	  }

	printf("selected: %d\n", gSelectNum);

	//once the dialog returns, handle the result
	printf("freeing %d buttons\n", count);

	//FIXME: why does this crash?!
	/*	for (i = 0; i < count; i++)
	  {
	    free(choiceButtons[i]);
	    }*/

	printf("selectnum: %d\n", gSelectNum);

	if(gSelectNum >= 0 && gSelectNum < count)
	  {
	    strcpy(answer, choices[gSelectNum]);
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
			  entry);
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

	strcpy(passwd, gPassword);

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
	GtkWidget *view;
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
	gtk_box_pack_start(GTK_BOX(hboxtop), view, FALSE, 5, 5);
	gtk_widget_show (view);	

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);

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
                struct template *t = obj->tdb->methods.get(obj->tdb, q->tag);
                template_deref(q->template);
                q->template = t;

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
				if(gBack && !gNext)
				  q = q->prev->prev;

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
