#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define STATUS_FILE "/var/lib/lowmem"
#define TEMPLATE_FILE_EXTENSION ".templates"
#define TEMPLATE_LINE_MAX_LENGTH 2048
#define DEFAULT_TO_REMOVE "Default-"
#define DESCRIPTION_TO_REMOVE "Description-"
#define CHOICES_TO_REMOVE "Choices-"
#define FILENAME_LENGTH 256

int main(int argc, char** argv) {
     FILE *fd = NULL;
     FILE *tmpfd = NULL;
     DIR  *dip = NULL;
     struct dirent *dit = NULL;
     char template_line[TEMPLATE_LINE_MAX_LENGTH];
     char *dir = NULL;
     char abs_path_file_name[FILENAME_LENGTH];
     char abs_path_tmpfile_name[FILENAME_LENGTH];

     int ignore = 0;

     if (argc < 2) {
	  return 0;
     }

     dir = argv[1];

     /* test if lowmem is activated */
     if( rename(STATUS_FILE, STATUS_FILE) != 0) { 
	  return 0;
     } 
  
     if ((dip = opendir(dir)) == NULL) {
	  return 0;
     }
     while ((dit = readdir(dip)) != NULL) {
	  /* select only template files */

	  if (strstr(dit->d_name, TEMPLATE_FILE_EXTENSION) != NULL) {
	       
	       snprintf(abs_path_file_name,
			FILENAME_LENGTH,"%s/%s",dir,dit->d_name);
	       
 	       if ((fd = fopen(abs_path_file_name, "r")) == NULL) {
		    perror("unable to open template file");
		    return 0;
	       }
	       snprintf(abs_path_tmpfile_name,
			FILENAME_LENGTH,"%s.%s",
			abs_path_file_name,"tmp");

 	       if ((tmpfd = fopen(abs_path_tmpfile_name, "w")) == NULL) {
		    perror("unable to open temp file");
		    return 0;
	       }

	       ignore = 0;
	       
	       /* parse template file */
	       while (fgets(template_line, 
			    TEMPLATE_LINE_MAX_LENGTH, fd) != NULL) {
		    if (ignore && 
			strstr(template_line, " ") == template_line) {
			 continue;
		    }
		    ignore = 0;
		    if (strstr(template_line, DESCRIPTION_TO_REMOVE) 
			|| strstr(template_line, CHOICES_TO_REMOVE)
			|| strstr(template_line, DEFAULT_TO_REMOVE)
			 ) {
			 ignore = 1;
			 continue;
		    }
		    if (fputs(template_line, tmpfd) == EOF) {
			 perror("unable to write to temp file");
			 return 0;
		    }
	       }
	       
	       fclose(tmpfd);
	       fclose(fd);

	       if( rename(abs_path_tmpfile_name, abs_path_file_name) != 0) { 
			 perror("unable to rename temp to template file");
		    return 0;
	       } 
	  }
     }

     if (closedir(dip) == -1) {
	  return 0;
     }
  
     return 1;
}


