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
#include "eto_dbs.h"

int main(int argc, char **argv)
{
	ETO_PARAMS* params;

	// Init logfile
	eto_log_set_file_name(NULL);
	
	// Read commandline parameters
	if (argc > 2)
	{
		eto_log_add_entry_s("ETO-10100 Warning: Too many input parameters. Only using first one:", argv[1]);
	}

	if (argc < 2)
	{
		eto_log_add_entry("ETO-10101 No input parameters. Quitting");
	}
	else
	{
		// Parse input file
		params = eto_new_ini_params(argv[1]);
		
		// Execute query
		if (params)
		{
			if (!eto_open_database(params) )
				eto_log_add_entry("ETO-10121 Query execution failed");
		}

		eto_delete_ini_params(params);
	}

	eto_log_close();

	return 0;
}