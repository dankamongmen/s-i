#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define STATUS_FILE "/var/lib/lowmem"
#define TEMPLATE_FILE_EXTENSION ".templates"
#define TEMPLATE_LINE_MAX_LENGTH 10000 /* languagechooser/country-name is
					* very very long */
#define FILENAME_LENGTH 256
#define TAG_LENGTH 30

#define LANG_TO_KEEP "en"
char tags_to_remove[][TAG_LENGTH] = {
	"Default-",
	"Description-",
	"Choices-",
	"Indices-"
};

int trimtemplate(char *filename) {
     int ignore = 0;
     FILE *fd = NULL;
     FILE *tmpfd = NULL;
     char template_line[TEMPLATE_LINE_MAX_LENGTH];
     char tmpfilename[FILENAME_LENGTH];
     int i=0;
     int nbr_tags = sizeof(tags_to_remove)/TAG_LENGTH;
     char tag_to_keep[TAG_LENGTH];

     if ((fd = fopen(filename, "r")) == NULL) {
	  perror("unable to open template file");
	  return 0;
     }
     snprintf(tmpfilename,
	      FILENAME_LENGTH,"%s.%s",
	      filename,"tmp");

     if ((tmpfd = fopen(tmpfilename, "w")) == NULL) {
	  perror("unable to open temp file");
	  return 0;
     }

     ignore = 0;
	       
     /* parse template file */
     while (fgets(template_line, 
		  TEMPLATE_LINE_MAX_LENGTH, fd) != NULL) {
	  if (ignore == 1 && 
	      strstr(template_line, " ") == template_line) {
	       continue;
	  }
	  ignore = 0;
	  for( i=0; i<nbr_tags && ignore == 0; i++ ) {
		  snprintf(tag_to_keep,
			   TAG_LENGTH,"%s%s",
			   tags_to_remove[i], LANG_TO_KEEP);

		  if( strstr(template_line, tag_to_keep) == NULL &&
		      strstr(template_line, tags_to_remove[i]) != NULL ) {
			  ignore = 1;
		  }
	  }
	  if (ignore == 0 && fputs(template_line, tmpfd) == EOF) {
	       perror("unable to write to temp file");
	       return 0;
	  }
     }
	       
     fclose(tmpfd);
     fclose(fd);

     if( rename(tmpfilename, filename) != 0) { 
	  perror("unable to rename temp to template file");
	  return 0;
     } 
     return 1;
}


int main(int argc, char** argv) {
     struct stat buf;
     
     if (argc < 2) {
	  return 0;
     }

     if ( stat(argv[1], &buf) == -1 ) {
	  perror("unable to get info about specified file or directory");
	  return 0;
     }
     
     /* test if lowmem is activated */
     if( rename(STATUS_FILE, STATUS_FILE) != 0) { 
       return 0;
     } 

     if (S_ISDIR(buf.st_mode)) {
	  DIR  *dip = NULL;
	  struct dirent *dit = NULL;
	  char *dir = NULL;
	  char abs_path_file_name[FILENAME_LENGTH];

	  dir = argv[1];
       
	  if ((dip = opendir(dir)) == NULL) {
	       return 0;
	  }
	  while ((dit = readdir(dip)) != NULL) {
	       /* select only template files */
	 
	       if (strstr(dit->d_name, TEMPLATE_FILE_EXTENSION) != NULL) {
	   
		    snprintf(abs_path_file_name,
			     FILENAME_LENGTH,"%s/%s",dir,dit->d_name);
		    if (trimtemplate(abs_path_file_name) == 0) {
			 return 0;
		    }
	       }
	  }
       
	  if (closedir(dip) == -1) {
	       return 0;
	  }
	  
	  return 1;
     }

     if (S_ISREG(buf.st_mode)) {
	  char *filename = NULL;

	  filename = argv[1];

	  if (trimtemplate(filename) == 0) {
	       return 0;
	  }
	  return 1;
     }

     return 0;
}


