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

#ifndef EASYTOORA_UTIL_HH
#define EASYTOORA_UTIL_HH

/* --- Includes ------------------------------------------------------------------------------------------------------- */
#include "iniparser.h"

/* --- Typedefs ------------------------------------------------------------------------------------------------------- */
#ifndef bool
#define bool	signed char
#define true	1
#define false	0
#endif

/* --- Structs -------------------------------------------------------------------------------------------------------- */
typedef struct ST_ETO_PARAMS
{
	dictionary		*ini;	/**< Ini-parser dictionary */
	char			*dbs;	/**< Databasename */
	int				 prt;	/**< Tcp port */
	char			*usr;	/**< Username */
	char			*pwd;	/**< Password */
	char			*cat;	/**< Catalog name */
	char			*scm;	/**< Schema name */
	char			*tbl;	/**< Tablename */
	char			*sql;	/**< SQL statement */
	char			*lfn;	/**< Logfile name */
	char			*fsep;	/**< Field separator */
	char			*lsep;	/**< Line separator */
	int				 rws;	/**< Number of rows to cache */
	int				 did;	/**< Describe ID */
	int				 hdr;	/**< Add header record */
	unsigned int	 vml;	/**< Maximum column length for varchar2 types */
} ETO_PARAMS;

typedef struct ST_QUERYDATA
{
	char			 fsep[3];
	char			 lsep[3];
	bool			 is_describe;
	int				 describe_id;
	bool			 hdr;
	bool			 hdr_printed;
	char			*sql;
	int				 cache_rows;
	int				 rows_processed;
	char 			*cache;
	unsigned int 	 cache_size;
	unsigned int	 vml;
} QUERYDATA;

/* --- Functions ------------------------------------------------------------------------------------------------------ */
ETO_PARAMS* eto_new_ini_params(const char* ini_file_name);
void eto_delete_ini_params(ETO_PARAMS* params);

QUERYDATA* eto_new_querydata(const char *in_statement, const char *in_tablename);
void eto_delete_querydata(QUERYDATA *qdata);

bool eto_convert_separators(char *field_separator, char *line_separator, const char *fsep, const char *lsep);
bool eto_create_query(char **sql, const char *in_statement, const char *in_tablename, const bool in_describe, const bool in_show_allowed);

#endif // EASYTOORA_UTIL_HH
