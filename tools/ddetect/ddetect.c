#include <unistd.h>
#include <stdio.h>


extern int debug;
int passive_detection = 1;

static void
usage (char *basename)
{
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "%s [options]\n", basename);
  fprintf (stderr, "-f\tfull detection (passive by default)\n");
  fprintf (stderr, "-v\tverbose\n");
  exit (1);
}


void
ddetect_getopts (int argc, char *argv[])
{

  int i = 1;

  while (argv[i] != NULL)
    {
      if (argv[i][0] != '-')
	usage (argv[0]);
      switch (argv[i][1])
	{
	case 'f':
	  passive_detection = 0;
	  break;
	case 'v':
	  debug = 1;
	  break;
	default:
	  usage (argv[0]);
	}
      i++;
    }
}
