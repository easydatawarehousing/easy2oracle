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

#include "eto_util.h"
#include "eto_log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
// Get rid of C4013 warnings
extern int __cdecl toupper(_In_ int _C);
extern int __cdecl tolower(_In_ int _C);
#endif

ETO_PARAMS* eto_new_ini_params(const char* ini_file_name)
{
	ETO_PARAMS* params;
	unsigned int i, j;
	bool copy_port = true;

	if (!ini_file_name)
	{
		eto_log_add_entry("ETO-10102 No ini filename: quitting.");
		return NULL;
	}

	params = (ETO_PARAMS*)malloc( sizeof(ETO_PARAMS) );

	if (!params)
		exit(1);

	// Parse input file
	params->ini = iniparser_load(ini_file_name);
		
	if (params->ini == NULL)
	{
		eto_log_add_entry_s("ETO-10102 Parse ini file failed: quitting. File =", ini_file_name);
		eto_delete_ini_params(params);
		params = NULL;
	}
	else
	{
		// Read parameters
		params->dbs  = iniparser_getstring(params->ini, (char*)"EasyXTOra:database",        NULL);
		params->prt  = iniparser_getint   (params->ini, (char*)"EasyXTOra:port",               0);
		params->usr  = iniparser_getstring(params->ini, (char*)"EasyXTOra:user",            NULL);
		params->pwd  = iniparser_getstring(params->ini, (char*)"EasyXTOra:password",        NULL);
		params->cat  = iniparser_getstring(params->ini, (char*)"EasyXTOra:catalog",         NULL);
		params->scm  = iniparser_getstring(params->ini, (char*)"EasyXTOra:schema",          NULL);
		params->tbl  = iniparser_getstring(params->ini, (char*)"EasyXTOra:table",           NULL);
		params->sql  = iniparser_getstring(params->ini, (char*)"EasyXTOra:sql",             NULL);
		params->rws  = iniparser_getint   (params->ini, (char*)"EasyXTOra:cache_rows",         1);
		params->lfn  = iniparser_getstring(params->ini, (char*)"EasyXTOra:logfilename",     NULL);
		params->did  = iniparser_getint   (params->ini, (char*)"EasyXTOra:describe_id",        0);
		params->fsep = iniparser_getstring(params->ini, (char*)"EasyXTOra:fieldseparator",  NULL);
		params->lsep = iniparser_getstring(params->ini, (char*)"EasyXTOra:lineseparator",   NULL);
		params->hdr  = iniparser_getint   (params->ini, (char*)"EasyXTOra:addheader",          0);
		params->vml  = iniparser_getint   (params->ini, (char*)"EasyXTOra:maxvarcharlen",   4000);

		if (params->vml < 2000)
			params->vml = 2000;

		// Check if tcp port is included in database string
		for (i = 0; i < strlen(params->dbs); i++)
		{
			// Look for a colon
			if (strncmp(params->dbs + i, ":", 1) == 0)
			{
				// Check that the portnumber is not followed by a string
				// for instance an Oracle servicename
				for (j = i + 1; j < strlen(params->dbs); j++)
				{
					if ( ! (strncmp(params->dbs + j, "0", 1) >= 0 && strncmp(params->dbs + j, "9", 1) <= 0) )
					{
						copy_port = false;
						break;
					}
				}

				if (copy_port)
				{
					params->prt = atoi(params->dbs + i + 1);
					params->dbs[i] = 0;	// Modifying an iniparser string here !
				}

				break;
			}
		}

		// Set logfile name
		if (params->lfn)
			eto_log_set_file_name(params->lfn);
	}

	return params;
}

void eto_delete_ini_params(ETO_PARAMS* params)
{
	if (!params)
		return;

	iniparser_freedict(params->ini);

//	free(params->prt)

	free(params);
}

QUERYDATA* eto_new_querydata(const char *in_statement, const char *in_tablename)
{
	unsigned int sqlsize = 0;
	QUERYDATA *qdata = (QUERYDATA*)malloc( sizeof(QUERYDATA) );

	if (!qdata)
	{
		eto_log_add_entry("ETO-10204 Open document failed: memory allocation error");
		return NULL;
	}

	memset(qdata->fsep, 0, 3);
	memset(qdata->lsep, 0, 3);
	qdata->is_describe		= false;
	qdata->describe_id		= 0;
	qdata->hdr				= false;
	qdata->hdr_printed		= false;
	qdata->sql				= NULL;
	qdata->cache_rows		= 0;
	qdata->rows_processed	= 0;
	qdata->cache			= NULL;
	qdata->cache_size		= 0;
	qdata->vml				= 0;

	// Reserve memory for sql statement
	if (in_statement)
		sqlsize = (unsigned int)strlen(in_statement);

	if (in_tablename && strlen(in_tablename) > sqlsize)
		sqlsize = (unsigned int)strlen(in_tablename);

	sqlsize += 50;	// Reserve some extra space in case we need to create a describe query

	sqlsize *= sizeof(char);

	qdata->sql = (char*)malloc(sqlsize);

	if (!qdata->sql)
	{
		eto_log_add_entry("ETO-10204 Open document failed: memory allocation error");
		free(qdata);
		return NULL;
	}

	memset(qdata->sql, 0, sqlsize);

	// Reserve 1 byte for cache
	qdata->cache_size = sizeof(char);
	qdata->cache = (char*)malloc(qdata->cache_size);

	if (!qdata->cache)
	{
		eto_log_add_entry("ETO-10204 Open document failed: memory allocation error");
		free(qdata);
		return NULL;
	}

	qdata->cache[0] = 0;

	return qdata;
}

void eto_delete_querydata(QUERYDATA *qdata)
{
	if (!qdata)
		return;

	if (qdata->sql)
		free(qdata->sql);

	if (qdata->cache)
		free(qdata->cache);

	free(qdata);
}

bool eto_convert_separators(char *field_separator, char *line_separator, const char *fsep, const char *lsep)
{
	strcpy(field_separator, "\2");

	if (fsep)	// length should be 1 character
	{
		if (!strcmp(fsep, "\\t"))
		{
			strcpy(field_separator, "\t");
		}
		else if (!strcmp(fsep, ","))
		{
			strcpy(field_separator, ",");
		}
		else if (!strcmp(fsep, ";"))
		{
			strcpy(field_separator, ";");
		}
		else if (!strcmp(fsep, field_separator))	// \2
		{
		}
		else
		{
			eto_log_add_entry("ETO-10204 Open document: not a valid field separator, using default 0x02");
		}
	}

	strcpy(line_separator, "\1");

	if (lsep)	// length should be 1 or 2 character(s)
	{
		if (!strcmp(lsep, "\\r\\n"))
		{
			strcpy(line_separator, "\r\n");
		}
		else if (!strcmp(lsep, "\\n\\r"))
		{
			strcpy(line_separator, "\r\n");	// Assuming typo
		}
		else if (!strcmp(lsep, "\\r"))
		{
			strcpy(line_separator, "\r");
		}
		else if (!strcmp(lsep, "\\n"))
		{
			strcpy(line_separator, "\n");
		}
		else if (!strcmp(lsep, line_separator))	// \1
		{
		}
		else
		{
			eto_log_add_entry("ETO-10204 Open document: not a valid line separator, using default 0x01");
		}
	}

	if (!strcmp(field_separator, line_separator))
	{
		eto_log_add_entry("ETO-10204 Open document failed: field and line separator are the same");
		return false;
	}

	return true;
}

bool eto_create_query(char **sql, const char *in_statement, const char *in_tablename, const bool in_describe, const bool in_show_allowed)
{
	unsigned int i;
	char check[8], *tempsql;
	memset(check, 0, 8);

	// Check/copy/create sql
	if (in_statement != NULL && strlen(in_statement) > 0)
	{
		strcpy(*sql, in_statement);

		// Convert first 7 characters to lowercase
		for (i = 0; i < strlen(*sql) && i < 7; i++)
			check[i] = (char)tolower( (*sql)[i] );

		// Statement must begin with: select or if allowed: show
		if (
				! (
					(strncmp(check, "select ", 7) == 0) ||
					(in_show_allowed && strncmp(check, "show ", 4) == 0)
				)
			)
		{
			eto_log_add_entry("ETO-10209 Open database: SQL is not a select or show statement");
			return false;
		}
	}
	else
	{
		// tablename may not contain spaces, to prevent embedding of other sql/dml/ddl statements
		for (i = 0; i < strlen(in_tablename);i++)
		{
			if (in_tablename[i] == 32)
			{
				eto_log_add_entry("ETO-10210 Open database: Tablename contains illegal character(s)");
				return false;
			}
		}

		// Create select statement
		strcpy(*sql, "SELECT * FROM ");
		strcat(*sql, in_tablename);
	}

	// Add describe clause if necessary
	if (in_describe)
	{
		tempsql = (char*)calloc(strlen(*sql) + 1, sizeof(char) );

		if (!tempsql)
			exit(1);

		strcpy(tempsql, *sql);
		strcpy(*sql, "SELECT * FROM (");
		strcat(*sql, tempsql);
		strcat(*sql, ") WHERE 1=0");
		free(tempsql);
	}

	return true;
}