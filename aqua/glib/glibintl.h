#ifndef __GLIBINTL_H__
#define __GLIBINTL_H__

#ifndef SIZEOF_CHAR
#error "config.h must be included prior to glibintl.h"
#endif

#ifdef ENABLE_NLS

gchar *_glib_gettext (const gchar *str) G_GNUC_FORMAT (1);

#include <libintl.h>
#define _(String) _glib_gettext(String)

#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else /* NLS is disabled */
#ifdef FE_AQUA
const char * XALocalizeString(const char *);
#define _(String) (char*)XALocalizeString((String))
#define N_(String) XALocalizeString(String)
#define textdomain(String) XALocalizeString(String)
#define gettext(String) XALocalizeString(String)
#define dgettext(Domain,String) XALocalizeString(String)
#define dcgettext(Domain,String,Type) XALocalizeString(String)
#define bindtextdomain(Domain,Directory) XALocalizeString(Domain) 
#else
#define _(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain) 
#endif
#endif

#endif /* __GLIBINTL_H__ */
