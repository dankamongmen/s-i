/*
 * file : memdetect 
 * purpose : detect memory for the Debian/GNU Linux installer
 * author : David Whedon <dwhedon@gordian.com>
 */


#include <stdio.h>
#include <discover.h>
#include <unistd.h>


static void
usage (char *basename)
{
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "memdetect [options]\n");
  fprintf (stderr, "-v\tverbose\n");
  exit (1);
}


int
main (int argc, char *argv[])
{
  struct memory_info *memory = (struct memory_info *) NULL;
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

  memory = memory_detect ();

  if (debug == 1)
    {
      printf ("\n%s:%s:%s:%s:%s:%s:%s\n",
	      "total",
	      "free",
	      "shared", "buffers", "cached", "free_swap", "total_swap");
    }				/*endif */
  printf ("%d:%d:%d:%d:%d:%d:%d\n",
	  memory->total,
	  memory->free,
	  memory->shared,
	  memory->buffers,
	  memory->cached, memory->swap_free, memory->swap_total);

  return 0;
}
