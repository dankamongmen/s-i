
#ifndef _CDEBCONF_CONFIG_NEWT_H_
#define _CDEBCONF_CONFIG_NEWT_H_

/*  Horizontal offset between buttons and text box */
#define TEXT_PADDING 1
/*  Horizontal offset between text box and borders */
#define BUTTON_PADDING 4

#define create_form(scrollbar)          newtForm((scrollbar), NULL, 0)

void newt_create_window(const int width, const int height, const char *title, const char *priority);

int
newt_get_text_height(const char *text, int win_width);

int
newt_get_text_width(const char *text);

#endif /* _CDEBCONF_CONFIG_NEWT_H_ */
