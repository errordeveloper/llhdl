#include <config.h>
#include <stdio.h>

#include <banner/banner.h>

void banner(const char *app)
{
	printf("%s - LLHDL %d.%d.%d\n\n", app, LLHDL_VERSION_MAJOR, LLHDL_VERSION_MINOR, LLHDL_VERSION_PATCH);
	printf("Copyright (C) 2010, 2011 Sebastien Bourdeauducq\n\n");
	printf("This program is free software: you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation, version 3 of the License.\n"
		"\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n");
}

