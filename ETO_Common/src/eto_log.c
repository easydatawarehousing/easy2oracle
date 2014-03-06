/* This file is part of Easy to Oracle - Free Open Source Data Integration
*
* Copyright (C) 2011-2014 Ivo Herweijer
*
* Easy to Oracle is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* You can contact me via email: info@easydatawarehousing.com
*/

#include "eto_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Private variables
char*	eto_log_content;
char*	eto_log_filename;
int		eto_log_size = 0;

// Function implementation
void eto_log_open()
{
	eto_log_content = (char*)malloc(1*sizeof(char));
	eto_log_content[0] = 0;
	eto_log_size = 1;
};

void eto_log_add_entry(const char* entry)
{
	if (!eto_log_size)
		eto_log_open();

	if (entry==NULL)
		return;

	eto_log_size += ( (unsigned int)strlen(entry) + 1) * sizeof(char);
	eto_log_content = (char*)realloc((char*)eto_log_content, eto_log_size);
	
	if (eto_log_content!=NULL)
	{
		strcat(eto_log_content, entry);
		strcat(eto_log_content, "\n");
	}
};

void eto_log_add_entry_i(const char* entry, int val)
{
	unsigned int size = sizeof(char) * (unsigned int)strlen(entry) + 20;
	char* txt = (char*)malloc(size);
	sprintf(txt, "%s %i",entry, val);
	eto_log_add_entry(txt);
	free(txt);
};

void eto_log_add_entry_s(const char* entry, const char* val)
{
	unsigned int size = sizeof(char) * (unsigned int)strlen(entry) + (unsigned int)strlen(val) + 2;
	char* txt = (char*)malloc(size);
	sprintf(txt, "%s %s",entry, val);
	eto_log_add_entry(txt);
	free(txt);
};

void eto_log_set_file_name(const char* fn)
{
	if (fn==NULL)
	{
		eto_log_filename = (char*)realloc((char*)eto_log_filename, 16 * sizeof(char) );
		strcpy(eto_log_filename, "eto_default.log");
	}
	else
	{
		eto_log_filename = (char*)realloc((char*)eto_log_filename, (strlen(fn)+1) * sizeof(char) );
		strcpy(eto_log_filename, fn);
	}
};

void eto_log_close()
{
	if (eto_log_size > 0)
	{
		FILE* f = fopen(eto_log_filename, "w");

		if (f)
		{
			fprintf(f,"%s", eto_log_content);
			fclose(f);
		}
	}

	free(eto_log_content);
	free(eto_log_filename);
};