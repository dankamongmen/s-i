/*
 * file : cpudetect 
 * purpose : detect ethernet cards for the Debian/GNU Linux installer
 * author : David Whedon <dwhedon@gordian.com>
 */


#include <stdio.h>
#include <discover.h>
#include <unistd.h>

#include "ddetect.h"


static void
usage (char *basename)
{
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "cpudetect [options]\n");
  fprintf (stderr, "-v\tverbose\n");
  exit (1);
}


int
main (int argc, char *argv[])
{
  struct cpu_info *cpu = (struct cpu_info *) NULL;
  int i = 1;


  while (argv[i] != NULL)
    {
      if (argv[i][0] != '-')
	usage (argv[0]);
      switch (argv[i][1])
	{
	case 'v':
	  debug = 1;
	  break;
	default:
	  usage (argv[0]);
	}
      i++;
    }



  sync ();

  if (((cpu = cpu_detect ()) != NULL) && (debug == 1))
    {
      printf ("\n%s:%s:%s:[%s]:%s:%s\n",
	      "vendor", "model", "frequency", "flags", "bogomips", "bugs");
    }				/*endif */
  for (; cpu; cpu = cpu->next)
    {
      printf ("%s:%s:%d:%s:%s:%s\n",
	      cpu->vendor,
	      cpu->model,
	      cpu->freq,
	      options2str (cpu->options), cpu->bogomips, cpu->bugs);
    }				/*next cpu */

  return 0;
}
