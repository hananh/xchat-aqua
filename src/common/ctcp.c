/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "xchat.h"
#include "cfgfiles.h"
#include "util.h"
#include "modes.h"
#include "outbound.h"
#include "ignore.h"
#include "inbound.h"
#include "dcc.h"
#include "text.h"
#include "ctcp.h"
#include "server.h"
#include "xchatc.h"


static void
ctcp_reply (session *sess, char *nick, char *word[], char *word_eol[],
				char *conf)
{
	char tbuf[4096];	/* can receive 2048 from IRC, so this is enough */

	conf = strdup (conf);
	/* process %C %B etc */
	check_special_chars (conf, TRUE);
	auto_insert (tbuf, sizeof (tbuf), conf, word, word_eol, "", "", word_eol[5],
					 server_get_network (sess->server, TRUE), "", "", nick);
	free (conf);
	handle_command (sess, tbuf, FALSE);
}

static int
ctcp_check (session *sess, char *nick, char *word[], char *word_eol[],
				char *ctcp)
{
	int ret = 0;
	char *po;
	struct popup *pop;
	GSList *list = ctcp_list;

	po = strchr (ctcp, '\001');
	if (po)
		*po = 0;

	po = strchr (word_eol[5], '\001');
	if (po)
		*po = 0;

	while (list)
	{
		pop = (struct popup *) list->data;
		if (!strcasecmp (ctcp, pop->name))
		{
			ctcp_reply (sess, nick, word, word_eol, pop->cmd);
			ret = 1;
		}
		list = list->next;
	}
	return ret;
}

void
ctcp_handle (session *sess, char *to, char *nick,
				 char *msg, char *word[], char *word_eol[], int id)
{
	char *po;
	session *chansess;
	server *serv = sess->server;
	char outbuf[1024];

	/* consider DCC to be different from other CTCPs */
	if (!strncasecmp (msg, "DCC", 3))
	{
		/* but still let CTCP replies override it */
		if (!ctcp_check (sess, nick, word, word_eol, word[4] + 2))
		{
			if (!ignore_check (word[1], IG_DCC))
				handle_dcc (sess, nick, word, word_eol);
		}
		return;
	}

	/* consider ACTION to be different from other CTCPs. Check
      ignore as if it was a PRIV/CHAN. */
	if (!strncasecmp (msg, "ACTION ", 7))
	{
		if (is_channel (serv, to))
		{
			/* treat a channel action as a CHAN */
			if (ignore_check (word[1], IG_CHAN))
				return;
		} else
		{
			/* treat a private action as a PRIV */
			if (ignore_check (word[1], IG_PRIV))
				return;
		}

		/* but still let CTCP replies override it */
		if (ctcp_check (sess, nick, word, word_eol, word[4] + 2))
			goto generic;

		inbound_action (sess, to, nick, msg + 7, FALSE, id);
		return;
	}

	if (ignore_check (word[1], IG_CTCP))
		return;

	if (!strcasecmp (msg, "VERSION") && !prefs.hidever)
	{
#ifdef FE_AQUA
		snprintf (outbuf, sizeof (outbuf), "VERSION X-Chat Aqua %s (xchat "PACKAGE_VERSION") %s",
                  prefs.xchat_aqua_version, get_cpu_str ());
#else
		snprintf (outbuf, sizeof (outbuf), "VERSION xchat "PACKAGE_VERSION" %s",
					 get_cpu_str ());
#endif
		serv->p_nctcp (serv, nick, outbuf);
	}

	if (!ctcp_check (sess, nick, word, word_eol, word[4] + 2))
	{
		if (!strncasecmp (msg, "SOUND", 5))
		{
			po = strchr (word[5], '\001');
			if (po)
				po[0] = 0;

			if (is_channel (sess->server, to))
			{
				chansess = find_channel (sess->server, to);
				if (!chansess)
					chansess = sess;

				EMIT_SIGNAL (XP_TE_CTCPSNDC, chansess, word[5],
								 nick, to, NULL, 0);
			} else
			{
				EMIT_SIGNAL (XP_TE_CTCPSND, sess->server->front_session, word[5],
								 nick, NULL, NULL, 0);
			}

			/* don't let IRCers specify path */
#ifdef WIN32
			if (strchr (word[5], '/') == NULL && strchr (word[5], '\\') == NULL)
#else
			if (strchr (word[5], '/') == NULL)
#endif
				sound_play (word[5], TRUE);
			return;
		}
	}

generic:
	po = strchr (msg, '\001');
	if (po)
		po[0] = 0;

	if (!is_channel (sess->server, to))
	{
		EMIT_SIGNAL (XP_TE_CTCPGEN, sess->server->front_session, msg, nick,
						 NULL, NULL, 0);
	} else
	{
		chansess = find_channel (sess->server, to);
		if (!chansess)
			chansess = sess;
		EMIT_SIGNAL (XP_TE_CTCPGENC, chansess, msg, nick, to, NULL, 0);
	}
}
