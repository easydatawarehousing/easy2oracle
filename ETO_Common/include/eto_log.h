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

#ifndef EASYTTORA_LOG_HH
#define EASYTTORA_LOG_HH

extern void eto_log_open();
extern void eto_log_add_entry(const char* entry);
extern void eto_log_add_entry_i(const char* entry, int val);
extern void eto_log_add_entry_s(const char* entry, const char* val);
extern void eto_log_set_file_name(const char* fn);
extern void eto_log_close();

#endif //* EASYTTORA_LOG_HH
