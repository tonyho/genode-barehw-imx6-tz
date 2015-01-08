/*
 * \brief  Testing 'printf()' with negative integer
 * \author Christian Prochaska
 * \date   2012-04-20
 *
 */

/*
 * Copyright (C) 2012-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/printf.h>

int main(int argc, char **argv)
{
	Genode::printf("-1 = %d = %ld\n", -1, -1L);
	Genode::printf("Hello world i.MX-6 Sabre SD...\n");

	/*
	int i = 100000000;
	while(1) {
	  Genode::printf("Hello world i.MX-6 Sabre SD...\n");
	  while(i > 0) {
	    i--;
	  }
	  i = 100000000;
	}
	*/

	return 0;
}


