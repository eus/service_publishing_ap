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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "app_err.h"
#include "logger.h"
#include "service_list.h"
#include "sde.h"

#ifndef UI_FILE
#define UI_FILE "./ui.html"
#endif

#ifndef SERVICE_PUBLISHER_LOG_FILE
#define SERVICE_PUBLISHER_LOG_FILE "./service_publisher.log"
#endif

GLOBAL_LOGGER;

static int should_close_html = 0;
static const char *err_msg = NULL;
static service_list *sl = NULL;

void
print_categories (void)
{
}

void
print_published_services (void)
{
  unsigned int i;
  size_t service_count;
  struct service *s;

  printf ("services = new Array();");

  if (sl == NULL && load_service_list (&sl))
    {
      err_msg = "Cannot load service list for reading";
      return;
    }

  service_count = count_service (sl);
  for (i = 0; i < service_count; i++)
    {
      if (get_service_at (sl, &s, i))
	{
	  err_msg = "Cannot read a service from the list";
	  return;
	}

      printf ("services[%u] = new Service(%lu, '%s'",
	      i, s->cat_id, s->uri);

      if (s->desc)
	{
	  printf (", '%s'", s->desc);
	}
      else
	{
	  printf (", null");
	}

      if (s->long_desc)
	{
	  printf (", '%s'", s->long_desc);
	}
      else
	{
	  printf (", null");
	}

      printf (");\n");

      destroy_service (&s);
    }

  destroy_service_list (&sl);
}

void
close_html (void)
{
  if (!should_close_html)
    {
      return;
    }

  printf ("<script type=\"text/javascript\">\n");
  if (sl != NULL || err_msg == NULL)
    {
      print_categories ();
      print_published_services ();
    }

  if (err_msg != NULL)
    {
      printf ("categories = new Array();"
	      "services = new Array();"
	      "errorMsg = '%s';", err_msg);
    }

  printf ("</script></body></html>");
}

/**
 * Only decodes up to the first '&' or end of data buffer.
 *
 * @return the decoded data length excluding '&' if it exists (i.e., next
 *         decoding should start from data_buffer + decoded_data_length + 1
 *         if '&' presents in data buffer).
 */
static size_t
url_decode (char *data_buffer, const char *buffer_end)
{
  const char *itr;
  char *decoder = data_buffer;
  char hexcode[3] = {0};
  int hexcode_idx = 0;
  int is_reading_hexcode = 0;

  for (itr = data_buffer; itr < buffer_end; itr++)
    {
      if (*itr == '&')
	{
	  break;
	}
      if (is_reading_hexcode)
	{
	  hexcode[hexcode_idx++] = *itr;

	  if (hexcode_idx == sizeof (hexcode) - 1)
	    {
	      is_reading_hexcode = 0;
	      *decoder = (char) strtoul (hexcode, NULL, 16);
	      decoder++;
	    }
	}
      else
	{
	  if (*itr == '%')
	    {
	      is_reading_hexcode = 1;
	      hexcode_idx = 0;
	      continue;
	    }

	  *decoder = ((*itr == '+') ? ' ' : *itr);
	  decoder++;
	}
    }

  return decoder - data_buffer;
}

/**
 * Parses the TLV data as sent by UI.html. The first call to this function
 * with respect to the given iterator will initialize the iterator and no
 * data are parsed. The subsequent call to the same iterator, however, will
 * return the desired data.
 *
 * @return -1 if no data is parsed after the iterator has been initialized,
 *         0 if there is no error and data is parsed, or positive integer
 *         for other errors.
 */
static int
parse_next_service (char **itr, const char *end_ptr, struct service *s)
{
  char *ptr = *itr;

  while (ptr < end_ptr)
    {
      unsigned long type;
      unsigned long length;
      char *colon;

      /* Get type */
      if ((colon = memchr (ptr, ':', end_ptr - ptr)) == NULL)
	{
	  return -1; /* no more data to parse */
	}
      *colon = '\0';
      type = strtoul (ptr, NULL, 10);
      ptr = colon + 1;

      /* Get length */
      if (ptr >= end_ptr)
	{
	  l->APP_ERR (ERR_PARSE_DATA, "Missing length");
	  return ERR_PARSE_DATA;
	}
      if ((colon = memchr (ptr, ':', end_ptr - ptr)) == NULL)
	{
	  l->APP_ERR (ERR_PARSE_DATA, "Cannot find length's ':'");
	  return ERR_PARSE_DATA;
	}
      *colon = '\0';
      length = strtoul (ptr, NULL, 10);
      ptr = colon + 1;
      
      /* Get value */
      if (ptr >= end_ptr || ptr + length > end_ptr)
	{
	  l->APP_ERR (ERR_PARSE_DATA, "Missing or corrupted value");
	  return ERR_PARSE_DATA;
	}
      switch (type)
	{
	case DESCRIPTION:
	  *itr = ptr;
	  return ERR_SUCCESS;
	case SERVICE_CAT_ID:
	  ptr[length] = '\0';
	  s->cat_id = strtoul (ptr, NULL, 10);
	  break;
	case SERVICE_URI:
	  ptr[length] = '\0';
	  s->uri = ptr;
	  break;
	case SERVICE_DESC:
	  ptr[length] = '\0';
	  s->desc = ptr;
	  break;
	case SERVICE_LONG_DESC:
	  ptr[length] = '\0';
	  s->long_desc = ptr;
	  break;
	default:
	  l->APP_ERR (ERR_PARSE_DATA, "Unexpected type %lu", type);
	  return ERR_PARSE_DATA;
	}
      ptr += length + 1;
    }

  *itr = ptr;
  return ERR_SUCCESS;
}

int
main (int argc, char **argv, char **envp)
{
  char *request_method = getenv ("REQUEST_METHOD");
  FILE *ui_file;
  char *buffer;
  struct stat ui_file_stat;

  SETUP_LOGGER (SERVICE_PUBLISHER_LOG_FILE, errtostr);

  if (atexit (close_html))
    {
      l->ERR ("Cannot register close_html at exit");
      exit (EXIT_FAILURE);
    }

  if ((ui_file = fopen (UI_FILE, "r")) == NULL)
    {
      l->SYS_ERR ("Cannot open " UI_FILE);
      exit (EXIT_FAILURE);
    }

  if (fstat (fileno (ui_file), &ui_file_stat) == -1)
    {
      l->SYS_ERR ("Cannot allocate buffer to stat " UI_FILE);
      exit (EXIT_FAILURE);
    }

  buffer = malloc (ui_file_stat.st_size);
  if (buffer == NULL)
    {
      l->SYS_ERR ("Cannot allocate buffer to read " UI_FILE);
      exit (EXIT_FAILURE);
    }

  if (fread (buffer, ui_file_stat.st_size, 1, ui_file) == 0)
    {
      l->ERR ("Cannot read " UI_FILE);
      exit (EXIT_FAILURE);
    }
  if (fclose (ui_file) == -1)
    {
      l->SYS_ERR ("Cannot close " UI_FILE);
    }

  should_close_html = 1;
  printf ("Content-type: text/html\n\n");
  fwrite (buffer, ui_file_stat.st_size, 1, stdout);
  free (buffer);
  buffer = NULL;

  if (strcmp (request_method, "POST") == 0)
    {
      char key[] = "serializedServices=";
      unsigned long data_len = strtoul (getenv ("CONTENT_LENGTH"), NULL, 10);
      char *data_buffer = malloc (data_len);

      if (data_buffer == NULL)
	{
	  l->APP_ERR (ERR_MEM, "Cannot read POST data");
	  err_msg = "Not enough memory to read POST data";
	  exit (EXIT_SUCCESS);
	}

      if (fread (data_buffer, data_len, 1, stdin) == 0)
	{
	  err_msg = "POST data cannot be read";
	  exit (EXIT_SUCCESS);
	}

      if (strncmp (data_buffer, key, strlen (key)) == 0)
	{
	  if (load_service_list (&sl))
	    {
	      err_msg = "Cannot load service list for writing";
	    }
	  else
	    {
	      if (del_service_all (sl))
		{
		  err_msg = "Cannot empty service list";
		}
	      else
		{
		  int rc;
		  struct service s;
		  char *itr = data_buffer + strlen (key);
		  const char *itr_end = data_buffer + data_len;

		  data_len = url_decode (itr, itr_end);
		  if (parse_next_service (&itr, itr_end, &s) > 0)
		    {
		      err_msg = "Cannot initialize POST data iterator";
		    }
		  else
		    {
		      while (itr < itr_end)
			{
			  memset (&s, 0, sizeof (s));
			  rc = parse_next_service (&itr, itr_end, &s);
			  if (rc == -1)
			    {
			      break;
			    }
			  if (rc != 0)
			    {
			      err_msg = "Cannot parse the next service";
			      break;
			    }
			  if (add_service_last (sl, &s))
			    {
			      err_msg = "Cannot add service";
			      break;
			    }
			}
		    }
		  if (err_msg == NULL)
		    {
		      if ((rc = save_service_list (sl)) == ERR_SSID_TOO_LONG)
			{
			  err_msg =
			    "Services do not fit into the SSID (try to reduce"
			    " the character count of the descriptions or the"
			    " number of services)";
			  free (data_buffer);
			  exit (EXIT_FAILURE);
			}
		      else if (rc != 0)
			{
			  err_msg = "Error in saving the service list";
			}
		      else
			{
			  free (data_buffer);
			  exit (EXIT_SUCCESS);
			}
		    }
		}
	    }

	  destroy_service_list (&sl);
	}
      else
	{
	  err_msg = "Invalid POST data";
	}

      free (data_buffer);
    }
  else if (strcmp (request_method, "GET") != 0)
    {
      err_msg = "Invalid request method (not GET nor POST)";
    }

  exit (EXIT_SUCCESS);
}

