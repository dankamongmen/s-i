

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

#include "config.h"
# include <locale.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
# ifdef gettext_noop
#  define N_(String) gettext_noop (String)
# else
#  define N_(String) (String)
# endif
#else
# undef bindtextdomain
# define bindtextdomain(Domain, Directory) /* empty */
# undef textdomain
# define textdomain(Domain) /* empty */
# define _(Text) (Text)
# define gettext(Text) (Text)
# define dgettext(domain,Text) (Text)
# define N_(Text) (Text)
#endif


