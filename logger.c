/*****************************************************************************
 * Copyright (C) 2010  Tadeus Prastowo (eus@member.fsf.org)                  *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation, either version 3 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "logger.h"

/** Logger's private data. */
struct logger_data
{
  const char *(*app_err2str) (int); /**<
				    * The application error code to error
				    * string translator.
				    */
  FILE *out; /**< The file stream into which messages will be logged. */
};

static void
sys_err (const char *file, unsigned int line, const char *msg, ...)
{
  int error_num = errno;
  va_list ap;
  char buffer[128];

  fprintf (l->private->out, "[SYS ERR] %s:%d: ", file, line);

  va_start (ap, msg);
  vfprintf (l->private->out, msg, ap);

#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
  if (strerror_r (error_num, buffer, sizeof (buffer)) == -1)
    fprintf (l->private->out, "\n");
  else
    fprintf (l->private->out, " (%s)\n", buffer);
#else
  fprintf (l->private->out, " (%s)\n",
	   strerror_r (error_num, buffer, sizeof (buffer)));
#endif
}

static void
app_err (const char *file, unsigned int line, int error_num,
	 const char *msg, ...)
{
  va_list ap;

  fprintf (l->private->out, "[APP ERR] %s:%d: ", file, line);

  va_start (ap, msg);
  vfprintf (l->private->out, msg, ap);

  fprintf (l->private->out, " (");
  if (l->private->app_err2str)
    {
      fprintf (l->private->out, "%s", l->private->app_err2str (error_num));
    }
  else
    {
      fprintf (l->private->out, "%d", error_num);
    }
  fprintf (l->private->out, ")\n");
}

static void
err (const char *file, unsigned int line, const char *msg, ...)
{
  va_list ap;

  fprintf (l->private->out, "[ERR] %s:%d: ", file, line);

  va_start (ap, msg);
  vfprintf (l->private->out, msg, ap);

  fprintf (l->private->out, "\n");
}

int
init_logger (const char *log_output, const char *(*err2str) (int))
{
  FILE *out = fopen (log_output, "a");

  if (out == NULL)
    {
      return -1;
    }

  l->private = malloc (sizeof (struct logger_data));
  if (l->private == NULL)
    {
      fclose (out);
      return -1;
    }
  l->private->app_err2str = err2str;
   l->private->out = out;

  l->sys_err = sys_err;
  l->app_err = app_err;
  l->err = err;
 
  return 0;
}

void
destroy_logger (void)
{
  fclose (l->private->out);
  free (l->private);
  l->private = NULL;
}
