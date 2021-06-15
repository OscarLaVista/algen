/* ALGEN.C -- Copyright 1992-93 -- By Richard Butler -- EUROPA Software

    This code carries no warranties, whether expressed or implied.

    This code, it's listing, object file, and executable may not be
    distributed in any modified form.

    Thanks to Harvey Parisien for his HLIST program upon which ALGEN is based.

    Thanks to Erik Vanriper for releasing the source for his FILELIST program.
    It was a very useful reference while writing ALGEN.

    ---------------------------------------------------------------------------

    Richard Butler
    BBS: (504) 652-4916
    Fidonet 1:396/61
    ibmNET 40:4372/61
    OS2Net 81:10/20

*/

#define INCL_NOPMAPI
#define INCL_DOSFILEMGR

#include <stdio.h>
#include <time.h>
#include <sys\types.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <os2.h>
#include <errno.h>
#include "mstruct.h"
#include "fb.h"

#define VERSION "ALGEN v1.21 - (c)1992-93 by Richard Butler"
#define MAXLSIZE 512

typedef struct _areabuf {
    char path[80];
    char name[10];
    char desc[80];
    char files_bbs[80];
    unsigned file_count;
    unsigned new_file_count;
    unsigned long area_bytes;
    unsigned long new_area_bytes;
} AREABUF;

typedef struct _config {
    char system_name[40];
    char header_file[80];
    char new_header_file[80];
    int new_days;
    char list_file[80];
    char new_list_file[80];
    char stripflags;
    char noibmchars;
    char showmaxnames;
    char noemptyareas;
    char indent;
    char use_fidx;
    char showsummary;
    char ignorebaddates;
    char areadat_path[80];
} CONFIG;

int Check_Exclude(char *key, char **list, int num);

int Check_Empty_Area(char *file_path);

char *Get_Arg(char *string);

int GetFileInfo(FIDX **f_idx, char *file_name, ULONG *file_size, FDATE *file_date, char use_fidx, unsigned idx_recs, FILE *fp, int fdat_length);

FIDX **LoadFIDX(char *file_path, unsigned *num_recs);

unsigned SearchFIDX(FIDX **f_idx, int num_recs, char *key);

void strsort (FIDX **array, size_t array_size);

FILE *SH_fopen(char *filename, char *mode);

char *whitespace = " \t\n\r";

main(int argc, char *argv[])
{
    AREABUF **area;
    FIDX **files_idx;
    CONFIG cfg;
    struct tm current_date;
    struct tm new_date;
    struct tm *tm_ptr;
    time_t current_time;
    time_t new_time;
    time_t file_time;
    unsigned long ws_new_date;
    unsigned long ws_current_date;
    unsigned long ws_file_date;
    unsigned total_count = 0;
    unsigned total_new_count = 0;
    unsigned long total_bytes = 0;
    unsigned long total_new_bytes = 0;
    FILE *config_file_p;
    FILE *header_file_p;
    FILE *list_file_p;
    FILE *new_list_file_p = NULL;
    FILE *fp;
    char config_file[80] = "ALGEN.CFG";     /* Default config file */
    int i = 0;
    size_t len;
    char io_buf[MAXLSIZE];
    char *ptr;
    char *ws_ptr;
    int num_areas = 0;
    char file_path[128];
    char file_path2[128];
    char fdat_path[128];
    char fidx_path[128];
    char file_name[13];
    unsigned char new;
    char *fd_ptr;
    int length;
    APIRET rc;
    char buffer[80];
    char *p_buffer;
    char f_date[25];
    FILE *area_fp;
    struct _area areadat;
    char **exclude;
    int num_exclude = 0;
    char drive[3];
    char dir[66];
    char fname[9];
    char ext[5];
    char temp_fl[80];
    char temp_nfl[80];
    char area_banner[256];
    int x;
    char file_desc[MAXLSIZE];
    FDATE file_date;
    ULONG file_size;
    FDAT f_dat;
    FILE *fdat_fp;
    unsigned idx_recs;
    int fdat_length;

    printf("\nÖÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ·");
    printf("\nº %-58s Fidonet 1:396/61 º",VERSION);
    printf("\nÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½");
    fflush(stdout);
    if((area = malloc(sizeof(AREABUF *))) == NULL) {
        printf("\n\nNot enough memory!  Exiting....\n");
        exit(255);
    }
    current_time = time(NULL);
    tm_ptr = localtime(&current_time);
    current_date = *tm_ptr;
    ws_current_date = ((current_date.tm_year * 10000) + ((current_date.tm_mon + 1) * 100) + current_date.tm_mday);
    strftime(f_date,24,"%a %b %d, %Y %H:%M",&current_date);
    if(argc > 1)            /* assume that argv[1] is a valid config file */
        strncpy(config_file, argv[1], 79);
    if((config_file_p = fopen(config_file,"r")) == NULL) {
        printf("\n\nError opening ConfigFile '%s'!  Exiting....\n",config_file);
        exit(255);
    }

/*---------------------------------------------------------------------------*/
    /* Parse config file */

    *cfg.system_name = '\0';
    *cfg.header_file = '\0';
    *cfg.new_header_file = '\0';
    cfg.new_days = 14;
    *cfg.list_file = '\0';
    *cfg.new_list_file = '\0';
    cfg.stripflags = FALSE;
    cfg.noibmchars = FALSE;
    cfg.showmaxnames = FALSE;
    cfg.noemptyareas = FALSE;
    cfg.indent = FALSE;
    cfg.use_fidx = FALSE;
    cfg.showsummary = FALSE;
    cfg.ignorebaddates = FALSE;
    *cfg.areadat_path = '\0';

    printf("\n\nReading %s....",config_file);
    fflush(stdout);
    while(fgets(io_buf,MAXLSIZE,config_file_p)) {
        if(ptr = strchr(io_buf,'\n'))
            *ptr = '\0';
        if(io_buf[0] == '\0')
            continue;
        ws_ptr = io_buf + strspn(io_buf,whitespace);
        if(*ws_ptr == '\0' || *ws_ptr == ';')
            continue;
        sscanf(ws_ptr,"%79s",&buffer);
        for(i = 0; buffer[i] != '\0'; i++)
            buffer[i] = toupper(buffer[i]);

        if(strcmp("SYSTEM",buffer) == 0) {
            ptr = Get_Arg(ws_ptr);
            strncpy(cfg.system_name,ptr,39);
        }
        else if(strcmp("HEADER",buffer) == 0) {
            ptr = Get_Arg(ws_ptr);
            strncpy(cfg.header_file,ptr,79);
        }
        else if(strcmp("NEWHEADER",buffer) == 0) {
            ptr = Get_Arg(ws_ptr);
            strncpy(cfg.new_header_file,ptr,79);
        }
        else if(strcmp("NEWDAYS",buffer) == 0) {
            ptr = Get_Arg(ws_ptr);
            cfg.new_days = atoi(ptr);
            new_time = (current_time - (cfg.new_days * 86400));
            tm_ptr = localtime(&new_time);
            new_date = *tm_ptr;
            ws_new_date = ((new_date.tm_year * 10000) + ((new_date.tm_mon + 1) * 100) + new_date.tm_mday);
        }
        else if(strcmp("INDENT",buffer) == 0) {
            ptr = Get_Arg(ws_ptr);
            cfg.indent = atoi(ptr);
            if(cfg.indent < 0)
                cfg.indent = 0;
            if(cfg.indent > 16)
                cfg.indent = 16;
        }
        else if(strcmp("FILELIST",buffer) == 0) {
            ptr = Get_Arg(ws_ptr);
            strncpy(cfg.list_file,ptr,79);
        }
        else if(strcmp("NEWFILELIST",buffer) == 0) {
            ptr = Get_Arg(ws_ptr);
            strncpy(cfg.new_list_file,ptr,79);
        }
        else if(strcmp("STRIPFLAGS",buffer) == 0) {
            cfg.stripflags = TRUE;
        }
        else if(strcmp("NOIBMCHARS",buffer) == 0) {
            cfg.noibmchars = TRUE;
        }
        else if(strcmp("SHOWMAXNAMES",buffer) == 0) {
            cfg.showmaxnames = TRUE;
        }
        else if(strcmp("NOEMPTYAREAS",buffer) == 0) {
            cfg.noemptyareas = TRUE;
        }
        else if(strcmp("USEFIDX",buffer) == 0) {
            cfg.use_fidx = TRUE;
        }
        else if(strcmp("SHOWSUMMARY",buffer) == 0) {
            cfg.showsummary = TRUE;
        }
        else if(strcmp("IGNOREBADDATES",buffer) == 0) {
            cfg.ignorebaddates = TRUE;
        }
        else if(strcmp("MAXAREAS",buffer) == 0) {
            ptr = Get_Arg(ws_ptr);
            strncpy(cfg.areadat_path,ptr,79);
        }
        else if(strcmp("AREAINCLUDE",buffer) == 0) {
            num_areas = 0;
            while(fgets(io_buf,MAXLSIZE,config_file_p) != NULL) {
                if(ptr = strchr(io_buf,'\n'))
                    *ptr = '\0';
                if(io_buf[0] == '\0')
                    continue;
                ws_ptr = io_buf + strspn(io_buf,whitespace);
                if(*ws_ptr == '\0' || *ws_ptr == ';')
                    continue;
                sscanf(ws_ptr,"%79s",&buffer);
                for(i = 0; buffer[i] != '\0'; i++)
                    buffer[i] = toupper(buffer[i]);
                if(strcmp("END",buffer) == 0)
                    break;
                if((area[num_areas] = malloc(sizeof(AREABUF))) == NULL) {
                    printf("\n\nNot enough memory!  Exiting....");
                    exit(255);
                }
                if((area = realloc(area,((num_areas + 2) * sizeof(AREABUF *)))) == NULL) {
                    printf("\n\nNot enough memory!  Exiting....");
                    exit(255);
                }
                strncpy(area[num_areas]->path,ws_ptr,79);
                len = strcspn(area[num_areas]->path,whitespace);
                area[num_areas]->path[len] = '\0';
                if(area[num_areas]->path[len - 1] != '\\')
                    strncat(area[num_areas]->path,"\\",79 - len);
                ptr = Get_Arg(ws_ptr);
                strncpy(area[num_areas]->desc,ptr,79);
                if(ptr = strchr(area[num_areas]->desc, '^')) {
                    *ptr = '\0';
                }
                if(ptr = strchr(ws_ptr, '^')) {
                    strncpy(area[num_areas]->files_bbs,++ptr,79);
                }
                else {
                    strcpy(area[num_areas]->files_bbs,area[num_areas]->path);
                    strncat(area[num_areas]->files_bbs,"FILES.BBS",78 - len);
                }
                area[num_areas]->name[0] = '\0';
                num_areas++;
            }
        }
        else if(strcmp("END",buffer) == 0) {
            continue;
        }
        else if(strcmp("AREAEXCLUDE",buffer) == 0) {
            num_exclude = 0;
            if((exclude = malloc(sizeof(char *))) == NULL) {
                printf("\n\nNot enough memory!  Exiting....\n");
                exit(255);
            }
            while(fgets(io_buf,MAXLSIZE,config_file_p) != NULL) {
                if(ptr = strchr(io_buf,'\n'))
                    *ptr = '\0';
                if(io_buf[0] == '\0')
                    continue;
                ws_ptr = io_buf + strspn(io_buf,whitespace);
                if(*ws_ptr == '\0' || *ws_ptr == ';')
                    continue;
                sscanf(ws_ptr,"%79s",&buffer);
                for(i = 0; buffer[i] != '\0'; i++)
                    buffer[i] = toupper(buffer[i]);
                if(strcmp("END",buffer) == 0)
                    break;
                if((exclude[num_exclude] = malloc(sizeof(areadat.name))) == NULL) {
                    printf("\n\nNot enough memory!  Exiting....");
                    exit(255);
                }
                if((exclude = realloc(exclude,((num_exclude + 2) * sizeof(char *)))) == NULL) {
                    printf("\n\nNot enough memory!  Exiting....");
                    exit(255);
                }
                strncpy(exclude[num_exclude],buffer,39);
                num_exclude++;
            }
        }
        else if(strcmp("END",buffer) == 0) {
            continue;
        }
        else {
            printf("\n\nUnknown Keyword '%s'  Exiting....\n",buffer);
            exit(255);
        }
    }
    fclose(config_file_p);

    /* Read Area.Dat */

    if(cfg.areadat_path[0] != '\0') {
        if((area_fp = SH_fopen(cfg.areadat_path,"rb")) == NULL) {
            printf("\n\nError opening MaxAreas '%s'!  Exiting....\n",cfg.areadat_path);
            exit(255);
        }
        printf("\nReading %s....",cfg.areadat_path);
        fflush(stdout);
        fread(&areadat,sizeof(struct _area),1,area_fp);
        length = areadat.struct_len;
        for(i = 0; !feof(area_fp); i++) {
            fseek(area_fp,i*(long)length,SEEK_SET);
            rc = fread(&areadat,sizeof(struct _area),1,area_fp);
            if(rc != 0 && areadat.filepath[0] != '\0' && areadat.filepath[0] != ' ' &&
               Check_Exclude(areadat.name,exclude,num_exclude) != 1) {
                if((area[num_areas] = malloc(sizeof(AREABUF))) == NULL) {
                    printf("\n\nNot enough memory!  Exiting....");
                    exit(255);
                }
                if((area = realloc(area,((num_areas + 2) * sizeof(AREABUF *)))) == NULL) {
                    printf("\n\nNot enough memory!  Exiting....");
                    exit(255);
                }
                strncpy(area[num_areas]->path,areadat.filepath,79);
                len = strcspn(area[num_areas]->path,whitespace);
                area[num_areas]->path[len] = '\0';
                if(area[num_areas]->path[len - 1] != '\\')
                    strncat(area[num_areas]->path,"\\",79 - len);
                if(areadat.fileinfo[0] != '\0' && areadat.fileinfo[0] != ' ') {
                    if(cfg.showmaxnames) {
                        strncpy(area[num_areas]->name,areadat.name,9);
                    }
                    else {
                        area[num_areas]->name[0] = '\0';
                    }
                    strncpy(area[num_areas]->desc,areadat.fileinfo,79);
                }
                if(areadat.filesbbs[0] != '\0' && areadat.filesbbs[0] != ' ') {
                    strncpy(area[num_areas]->files_bbs,areadat.filesbbs,79);
                }
                else {
                    strcpy(area[num_areas]->files_bbs,area[num_areas]->path);
                    strncat(area[num_areas]->files_bbs,"FILES.BBS",78 - len);
                }
                num_areas++;
            }
        }
        fclose(area_fp);
    }

    /* free memory used by exclude array */

    if(num_exclude != 0) {
        for(i = num_exclude - 1; i >= 0; i--) {
            free(exclude[i]);
        }
        free(exclude);
    }

/*---------------------------------------------------------------------------*/

    printf("\n\nProcessing....\n");
    fflush(stdout);

    /* open output files */

    if(cfg.list_file[0]) {
        _splitpath(cfg.list_file,drive,dir,fname,ext);
        strcpy(ext,".$$$");
        _makepath(temp_fl,drive,dir,fname,ext);
    }
    else {
        printf("\n\nA FileList name is required!  Exiting....\n");
        exit(255);
    }

    if((list_file_p = fopen(temp_fl,"w")) == NULL) {
        printf("\n\nError opening FileList '%s'!  Exiting....\n",temp_fl);
        exit(255);
    }
    if(cfg.new_list_file[0]) {
        _splitpath(cfg.new_list_file,drive,dir,fname,ext);
        strcpy(ext,".###");
        _makepath(temp_nfl,drive,dir,fname,ext);
        if((new_list_file_p = fopen(temp_nfl,"w")) == NULL) {
            printf("\n\nError opening NewFileList '%s'!  Exiting....\n",temp_nfl);
            exit(255);
        }
    }
    if(cfg.header_file[0]) {
        if((header_file_p = fopen(cfg.header_file,"r")) != NULL) {
            while(fgets(io_buf,MAXLSIZE,header_file_p) != NULL)
                fputs(io_buf,list_file_p);
            fclose(header_file_p);
        }
        else {
            printf("\nError opening Header '%s'!  Continuing....\n",cfg.header_file);
            fflush(stdout);
        }
    }
    if(cfg.new_header_file[0] && new_list_file_p) {
        if((header_file_p = fopen(cfg.new_header_file,"r")) != NULL) {
            while(fgets(io_buf,MAXLSIZE,header_file_p) != NULL)
                fputs(io_buf,new_list_file_p);
            fclose(header_file_p);
        }
        else {
            printf("\nError opening NewHeader '%s'!  Continuing....\n",cfg.new_header_file);
            fflush(stdout);
        }
    }

    if(cfg.noibmchars) {
        fprintf(list_file_p,"+-----------------------------------------------------------------------------+\n");
        fprintf(list_file_p,"|        This is a complete list of all files available on this system        |\n");
        fprintf(list_file_p,"|                                                                             |\n");
        sprintf(io_buf,"|   File dates followed by a * are < than %d days old, which is >= %.2d-%.2d-%d",cfg.new_days,(new_date.tm_mon + 1),new_date.tm_mday,new_date.tm_year);
        strcat(io_buf,"          ");
        io_buf[78] = '|';
        io_buf[79] = '\0';
        fprintf(list_file_p,"%s\n",io_buf);
        fprintf(list_file_p,"|                                                                             |\n");
        fprintf(list_file_p,"| %-53s%s |\n",cfg.system_name,f_date);
        fprintf(list_file_p,"+-----------------------------------------------------------------------------+\n");
        fprintf(list_file_p," %-60s Fidonet 1:396/61 \n\n",VERSION);
    }
    else {
        fprintf(list_file_p,"ÖÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ·\n");
        fprintf(list_file_p,"º        This is a complete list of all files available on this system        º\n");
        fprintf(list_file_p,"º                                                                             º\n");
        sprintf(io_buf,"º   File dates followed by a * are < than %d days old, which is >= %.2d-%.2d-%d",cfg.new_days,(new_date.tm_mon + 1),new_date.tm_mday,new_date.tm_year);
        strcat(io_buf,"          ");
        io_buf[78] = 'º';
        io_buf[79] = '\0';
        fprintf(list_file_p,"%s\n",io_buf);
        fprintf(list_file_p,"º                                                                             º\n");
        fprintf(list_file_p,"º %-53s%s º\n",cfg.system_name,f_date);
        fprintf(list_file_p,"ÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½\n");
        fprintf(list_file_p," %-60s Fidonet 1:396/61 \n\n",VERSION);
    }

    if(new_list_file_p) {
        if(cfg.noibmchars) {
            fprintf(new_list_file_p,"+-----------------------------------------------------------------------------+\n");
            fprintf(new_list_file_p,"|         This is a list of all 'NEW' files available on this system          |\n");
            fprintf(new_list_file_p,"|                                                                             |\n");
            sprintf(io_buf,"|             Files are < than %d days old, which is >= %.2d-%.2d-%d",cfg.new_days,(new_date.tm_mon + 1),new_date.tm_mday,new_date.tm_year);
            strcat(io_buf,"               ");
            io_buf[78] = '|';
            io_buf[79] = '\0';
            fprintf(new_list_file_p,"%s\n",io_buf);
            fprintf(new_list_file_p,"|                                                                             |\n");
            fprintf(new_list_file_p,"| %-53s%s |\n",cfg.system_name,f_date);
            fprintf(new_list_file_p,"+-----------------------------------------------------------------------------+\n");
            fprintf(new_list_file_p," %-60s Fidonet 1:396/61 \n\n",VERSION);
        }
        else {
            fprintf(new_list_file_p,"ÖÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ·\n");
            fprintf(new_list_file_p,"º         This is a list of all 'NEW' files available on this system          º\n");
            fprintf(new_list_file_p,"º                                                                             º\n");
            sprintf(io_buf,"º             Files are < than %d days old, which is >= %.2d-%.2d-%d",cfg.new_days,(new_date.tm_mon + 1),new_date.tm_mday,new_date.tm_year);
            strcat(io_buf,"               ");
            io_buf[78] = 'º';
            io_buf[79] = '\0';
            fprintf(new_list_file_p,"%s\n",io_buf);
            fprintf(new_list_file_p,"º                                                                             º\n");
            fprintf(new_list_file_p,"º %-53s%s º\n",cfg.system_name,f_date);
            fprintf(new_list_file_p,"ÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½\n");
            fprintf(new_list_file_p," %-60s Fidonet 1:396/61 \n\n",VERSION);
        }
    }

    /* process files.bbs */

    for(i = 0 ; i < num_areas ; i++) {
        printf("\n\t%.71s",area[i]->desc);
        fflush(stdout);
        if(cfg.noemptyareas) {
            if(Check_Empty_Area(area[i]->files_bbs)) {
                printf("\nSkipping empty area....");
                continue;
            }
        }
        area[i]->file_count = 0;
        area[i]->new_file_count = 0;
        area[i]->area_bytes = 0;
        area[i]->new_area_bytes = 0;
        if(cfg.noibmchars) {
            strcpy(area_banner,"############");
            if(cfg.showmaxnames && area[i]->name[0]) {
                strcat(area_banner," ");
                strcat(area_banner,area[i]->name);
                strcat(area_banner,": ");
                strcat(area_banner,area[i]->desc);
                strcat(area_banner," ");
                area_banner[77] = ' ';
                area_banner[78] = '\0';
            }
            else if(area[i]->desc[0]) {
                strcat(area_banner," ");
                strcat(area_banner,area[i]->desc);
                strcat(area_banner," ");
                area_banner[77] = ' ';
                area_banner[78] = '\0';
            }
            strcat(area_banner,"###################################################################");
        }
        else {
            strcpy(area_banner,"±±±±±±±±±±±±");
            if(cfg.showmaxnames && area[i]->name[0]) {
                strcat(area_banner," ");
                strcat(area_banner,area[i]->name);
                strcat(area_banner,": ");
                strcat(area_banner,area[i]->desc);
                strcat(area_banner," ");
                area_banner[77] = ' ';
                area_banner[78] = '\0';
            }
            else if(area[i]->desc[0]) {
                strcat(area_banner," ");
                strcat(area_banner,area[i]->desc);
                strcat(area_banner," ");
                area_banner[77] = ' ';
                area_banner[78] = '\0';
            }
            strcat(area_banner,"±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±");
        }
        fprintf(list_file_p,"\n%.79s\n",area_banner);
        if(new_list_file_p && !cfg.noemptyareas)
            fprintf(new_list_file_p,"\n%.79s\n",area_banner);
        if((fp = SH_fopen(area[i]->files_bbs,"r")) == NULL) {
            printf("\nError opening '%s'!  Continuing....",area[i]->files_bbs);
            fflush(stdout);
        }
        else {
            if(cfg.use_fidx) {
                _splitpath(area[i]->files_bbs,drive,dir,fname,ext);
                strcpy(ext,".DAT");
                _makepath(fdat_path,drive,dir,fname,ext);
                if((fdat_fp = SH_fopen(fdat_path,"r")) == NULL) {
                    printf("\nError opening '%s'! Continuing....",fdat_path);
                    fflush(stdout);
                    idx_recs = 0;
                }
                else {
                    fread(&f_dat,sizeof(FDAT),1,fdat_fp);
                    fdat_length = f_dat.struct_len;
                    _splitpath(area[i]->files_bbs,drive,dir,fname,ext);
                    strcpy(ext,".IDX");
                    _makepath(fidx_path,drive,dir,fname,ext);
                    files_idx = LoadFIDX(fidx_path,&idx_recs);
                }
            }
            while(fgets(io_buf,MAXLSIZE,fp) != NULL) {
                if(io_buf[0] == '-' || io_buf[0] == ' ' || io_buf[0] == '\n') {
                    if(ptr = strchr(io_buf, '\n'))
                        *ptr = '\0';
                    io_buf[79] = '\0';
                    fprintf(list_file_p,"\n%s",io_buf);
                }
                else {
                    sscanf(io_buf,"%127s",file_path2);
                    _splitpath(file_path2,drive,dir,fname,ext);
                    strcpy(file_path2,drive);
                    strcat(file_path2,dir);
                    strcpy(file_name,fname);
                    strcat(file_name,ext);
                    if(file_path2[0] == '\0') {
                        strcpy(file_path,area[i]->path);
                        strcat(file_path,file_name);
                    }
                    else {
                        strcpy(file_path,file_path2);
                        strcat(file_path,file_name);
                    }
                    rc = GetFileInfo(files_idx,file_path,&file_size,&file_date,cfg.use_fidx,idx_recs,fdat_fp,fdat_length);
                    if(rc != 0) {
                        fprintf(list_file_p,"\n%-12s (stored offline)  ",file_name);
                        new = 0;
                    }
                    else {
                        p_buffer = _itoa(file_date.year + 80,buffer,10);
                        len = strlen(p_buffer);
                        fprintf(list_file_p,"\n%-12s %7ld %.2d-%.2d-%c%c",file_name,file_size,file_date.month,file_date.day,buffer[len - 2],buffer[len - 1]);
                        area[i]->file_count++;
                        total_count++;
                        area[i]->area_bytes += file_size;
                        total_bytes += file_size;
                        ws_file_date = ((file_date.year + 80) * 10000) + (file_date.month * 100) + file_date.day;
                        new = FALSE;
                        if((!cfg.ignorebaddates && ws_file_date >= ws_new_date) ||
                                 (cfg.ignorebaddates && ws_file_date >= ws_new_date && ws_file_date <= ws_current_date)) {
                            new = TRUE;
                            fprintf(list_file_p,"* ");
                            if(new_list_file_p) {
                                if(cfg.noemptyareas && area[i]->new_file_count == 0)
                                    fprintf(new_list_file_p,"\n%.79s\n",area_banner);
                                fprintf(new_list_file_p,"\n%-12s %7ld %.2d-%.2d-%c%c",file_name,file_size,file_date.month,file_date.day,buffer[len - 2],buffer[len - 1]);
                                fprintf(new_list_file_p,"* ");
                                area[i]->new_file_count++;
                                total_new_count++;
                                area[i]->new_area_bytes += file_size;
                                total_new_bytes += file_size;
                            }
                        }
                        else
                            fprintf(list_file_p,"  ");
                    }
                    strcpy(file_desc,io_buf);
                    if(fd_ptr = strchr(file_desc,' ')) {
                        fd_ptr += (strspn(fd_ptr,whitespace));
                        if(cfg.stripflags && (fd_ptr[0] == '/')) {
                            fd_ptr = strpbrk(fd_ptr,whitespace);
                            fd_ptr += (strspn(fd_ptr,whitespace));
                        }
                        if(ptr = strchr(fd_ptr,'\n'))
                            *ptr = '\0';
                        strncpy(io_buf,fd_ptr,49);
                        io_buf[49] = '\0';
                        if(strlen(io_buf) > 48) {
                            if(ptr = strrchr(io_buf,' '))
                                *ptr = '\0';
                        }
                        fprintf(list_file_p,"%s",io_buf);
                        if(new && new_list_file_p)
                            fprintf(new_list_file_p,"%s",io_buf);
                        fd_ptr += (strlen(io_buf));
                        fd_ptr += (strspn(fd_ptr,whitespace));
                        while(*fd_ptr != '\0') {
                            strcpy(io_buf,"                                                  ");
                            io_buf[31 + cfg.indent] = '\0';
                            fprintf(list_file_p,"\n%s",io_buf);
                            if(new && new_list_file_p)
                                fprintf(new_list_file_p,"\n%s",io_buf);
                            strncpy(io_buf,fd_ptr,(49 - cfg.indent));
                            io_buf[49 - cfg.indent] = '\0';
                            if(strlen(io_buf) > (48 - cfg.indent)) {
                                if(ptr = strrchr(io_buf,' '))
                                    *ptr = '\0';
                            }
                            fprintf(list_file_p,"%s",io_buf);
                            if(new && new_list_file_p)
                                fprintf(new_list_file_p,"%s",io_buf);
                            fd_ptr += (strlen(io_buf));
                            fd_ptr += (strspn(fd_ptr,whitespace));
                        }
                    }
                }
            }
            fclose(fp);
            if(fdat_fp) {
                fclose(fdat_fp);
            }
            if(cfg.use_fidx && idx_recs != 0) {
                for(x = idx_recs - 1; x >= 0; x--) {
                    free(files_idx[x]);
                }
                free(files_idx);
            }
        }
        fprintf(list_file_p,"\n\n  ** %d files in this area (%ld bytes)\n\n",area[i]->file_count,area[i]->area_bytes);
        if(new_list_file_p && (!cfg.noemptyareas || area[i]->new_file_count > 0))
            fprintf(new_list_file_p,"\n\n  ** %d new files in this area (%ld bytes)\n\n",area[i]->new_file_count,area[i]->new_area_bytes);
    }

    if(cfg.showsummary) {
        if(cfg.noibmchars) {
            strcpy(area_banner,"############");
            strcat(area_banner," SUMMARY ");
            strcat(area_banner,"###################################################################");
        }
        else {
            strcpy(area_banner,"±±±±±±±±±±±±");
            strcat(area_banner," SUMMARY ");
            strcat(area_banner,"±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±");
        }
        fprintf(list_file_p,"\n%.79s\n",area_banner);
        fprintf(list_file_p,"\n%s\n", "Area Description                                                 Files   Bytes");
        if(cfg.noibmchars) {
            fprintf(list_file_p,"%s\n","---------------------------------------------------------------- ----- --------");
        }
        else {
            fprintf(list_file_p,"%s\n","ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ ÄÄÄÄÄ ÄÄÄÄÄÄÄÄ");
        }
        if(new_list_file_p) {
            fprintf(new_list_file_p,"\n%.79s\n",area_banner);
            fprintf(new_list_file_p,"\n%s\n", "Area Description                                                 Files   Bytes");
            if(cfg.noibmchars) {
                fprintf(new_list_file_p,"%s\n","---------------------------------------------------------------- ----- --------");
            }
            else {
                fprintf(new_list_file_p,"%s\n","ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ ÄÄÄÄÄ ÄÄÄÄÄÄÄÄ");
            }
        }
        for(i = 0 ; i < num_areas ; i++) {
            if(cfg.showmaxnames) {
                strcpy(buffer,area[i]->name);
                strncat(buffer,"         ",9-strlen(area[i]->name));
                strcat(buffer," : ");
                strncat(buffer,area[i]->desc,67);
            }
            else {
                strcpy(buffer,area[i]->desc);
            }
            if(!cfg.noemptyareas || area[i]->file_count > 0) {
                fprintf(list_file_p,"%-64.64s %5u %7.0fK\n",buffer,area[i]->file_count,((double)(area[i]->area_bytes)) / 1000);
                if(new_list_file_p && (!cfg.noemptyareas || area[i]->new_file_count > 0)) {
                    fprintf(new_list_file_p,"%-64.64s %5u %7.0fK\n",buffer,area[i]->new_file_count,((double)(area[i]->new_area_bytes)) / 1000);
                }
            }
        }
        if(cfg.noibmchars) {
            fprintf(list_file_p,"%s\n","-------------------------------------------------------------------------------");
        }
        else {
            fprintf(list_file_p,"%s\n","ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
        }
        if(new_list_file_p) {
            if(cfg.noibmchars) {
                fprintf(new_list_file_p,"%s\n","-------------------------------------------------------------------------------");
            }
            else {
                fprintf(new_list_file_p,"%s\n","ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
            }
        }
    }

    /* finish things up */

    if(cfg.noibmchars) {
        fprintf(list_file_p,"\n %-55s%s\n",cfg.system_name,f_date);
        fprintf(list_file_p,"+-----------------------------------------------------------------------------+\n");
        sprintf(io_buf,"| There are a total of %d files on this system (%ld bytes)",total_count,total_bytes);
        strcat(io_buf,"                               ");
        io_buf[78] = '|';
        io_buf[79] = '\0';
        fprintf(list_file_p,"%s\n",io_buf);
        fprintf(list_file_p,"|                                                                             |\n");
        fprintf(list_file_p,"| %-58s Fidonet 1:396/61 |\n",VERSION);
        fprintf(list_file_p,"+-----------------------------------------------------------------------------+\n");
    }
    else {
        fprintf(list_file_p,"\n %-55s%s\n",cfg.system_name,f_date);
        fprintf(list_file_p,"ÖÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ·\n");
        sprintf(io_buf,"º There are a total of %d files on this system (%ld bytes)",total_count,total_bytes);
        strcat(io_buf,"                               ");
        io_buf[78] = 'º';
        io_buf[79] = '\0';
        fprintf(list_file_p,"%s\n",io_buf);
        fprintf(list_file_p,"º                                                                             º\n");
        fprintf(list_file_p,"º %-58s Fidonet 1:396/61 º\n",VERSION);
        fprintf(list_file_p,"ÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½\n");
    }
    fclose(list_file_p);
    if(remove(cfg.list_file) != 0 && errno == EACCESS)
        printf("\n\nError deleting old FileList '%s'!",cfg.list_file);
    if(rename(temp_fl,cfg.list_file) != 0)
        printf("\n\nError renaming '%s' to '%s'!",temp_fl,cfg.list_file);

    if(new_list_file_p) {
        if(cfg.noibmchars) {
            fprintf(new_list_file_p,"\n %-55s%s\n",cfg.system_name,f_date);
            fprintf(new_list_file_p,"+-----------------------------------------------------------------------------+\n");
            sprintf(io_buf,"| There are a total of %d new files on this system (%ld bytes)",total_new_count,total_new_bytes);
            strcat(io_buf,"                               ");
            io_buf[78] = '|';
            io_buf[79] = '\0';
            fprintf(new_list_file_p,"%s\n",io_buf);
            fprintf(new_list_file_p,"|                                                                             |\n");
            fprintf(new_list_file_p,"| %-58s Fidonet 1:396/61 |\n",VERSION);
            fprintf(new_list_file_p,"+-----------------------------------------------------------------------------+\n");
        }
        else {
            fprintf(new_list_file_p,"\n %-55s%s\n",cfg.system_name,f_date);
            fprintf(new_list_file_p,"ÖÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ·\n");
            sprintf(io_buf,"º There are a total of %d new files on this system (%ld bytes)",total_new_count,total_new_bytes);
            strcat(io_buf,"                               ");
            io_buf[78] = 'º';
            io_buf[79] = '\0';
            fprintf(new_list_file_p,"%s\n",io_buf);
            fprintf(new_list_file_p,"º                                                                             º\n");
            fprintf(new_list_file_p,"º %-58s Fidonet 1:396/61 º\n",VERSION);
            fprintf(new_list_file_p,"ÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½\n");
        }
        fclose(new_list_file_p);
        if(remove(cfg.new_list_file) != 0 && errno == EACCESS)
            printf("\n\nError deleting old NewFileList '%s'!",cfg.new_list_file);
        if(rename(temp_nfl,cfg.new_list_file) != 0)
            printf("\n\nError renaming '%s' to '%s'!",temp_nfl,cfg.new_list_file);
    }
    printf("\n\n....Done!\n");
}

int Check_Exclude(char *key, char **list, int num)
{
    int i = 0;
    char found = FALSE;

    while(i < num && !found) {
        if(strcmp(key,list[i]) == 0) {
            found = TRUE;
        }
        i++;
    }
    return(found);
}

int Check_Empty_Area(char *file_path)
{
    FILE *fp;
    char io_buf[MAXLSIZE];

    if((fp = SH_fopen(file_path,"r")) == NULL) {
        printf("\nError opening '%s'!  Continuing....",file_path);
        fflush(stdout);
        return(1);
    }
    else {
        while(fgets(io_buf,MAXLSIZE,fp) != NULL) {
            if(io_buf[0] != '-' && io_buf[0] != ' ' && io_buf[0] != '\n') {
                fclose(fp);
                return(0);
            }
        }
        fclose(fp);
        return(1);
    }
}

char *Get_Arg(char *string)
{
    char *ptr;

    ptr = string + strcspn(string,whitespace);
    ptr += strspn(ptr,whitespace);
    return(ptr);
}

int GetFileInfo(FIDX **f_idx, char *file_name, ULONG *file_size, FDATE *file_date, char use_fidx, unsigned idx_recs, FILE *fdat_fp, int fdat_length)
{
    ULONG PathInfoLevel = 1;
    FILESTATUS3 PathInfoBuf;
    APIRET rc;
    unsigned f_idx_rec;
    char drive[3];
    char dir[66];
    char fname[9];
    char ext[5];
    char key[13];
    FDAT f_dat;

    if(use_fidx && idx_recs != 0 && fdat_fp != NULL) {
        _splitpath(file_name,drive,dir,fname,ext);
        strcpy(key,fname);
        strcat(key,ext);
        f_idx_rec = SearchFIDX(f_idx,idx_recs,key);
        if(f_idx_rec != -1) {
            fseek(fdat_fp,f_idx[f_idx_rec]->fpos * (long)fdat_length,SEEK_SET);
            if(fread(&f_dat,sizeof(FDAT),1,fdat_fp)) {
                if(f_dat.fsize == 0 && f_dat.udate.msg_st.date.da == 0 && f_dat.udate.msg_st.date.mo == 0 && f_dat.udate.msg_st.date.yr == 0) {
                    return(1);
                }
                else {
                    *file_size = f_dat.fsize;
                    file_date->day = f_dat.udate.msg_st.date.da;
                    file_date->month = f_dat.udate.msg_st.date.mo;
                    file_date->year = f_dat.udate.msg_st.date.yr;
                    return(0);
                }
            }
        }
    }

    rc = DosQueryPathInfo(file_name,PathInfoLevel,&PathInfoBuf,sizeof(FILESTATUS3));
    if(rc == 0) {
        *file_size = PathInfoBuf.cbFile;
        *file_date = PathInfoBuf.fdateLastWrite;
        return(0);
    }
    else {
        return(1);
    }
}

FIDX **LoadFIDX(char *file_path, unsigned *num_recs)
{
    unsigned ws_recs;
    ULONG PathInfoLevel = 1;
    FILESTATUS3 PathInfoBuf;
    APIRET rc;
    int i;
    int x;
    FILE *fidx_fp;
    FIDX **f_idx;
    FIDX fidx_rec;

    if((fidx_fp = SH_fopen(file_path,"rb")) == NULL) {
        printf("\nError opening '%s'!  Continuing....",file_path);
        *num_recs = 0;
        return(NULL);
    }
    if((f_idx = malloc(sizeof(FIDX *))) == NULL) {
        printf("\nNot enough memory to load FILES.IDX!  Exiting....");
        exit(255);
    }
    for(i=0; fread(&fidx_rec,sizeof(FIDX),1,fidx_fp); i++) {
        if((f_idx[i] = malloc(sizeof(FIDX))) == NULL) {
            printf("\nNot enough memory to load FILES.IDX!  Exiting....");
            exit(255);
        }
        strncpy(f_idx[i]->name,fidx_rec.name,MAX_FN_LEN);
        f_idx[i]->anum = fidx_rec.anum;
        f_idx[i]->fpos = fidx_rec.fpos;
        if((f_idx = realloc(f_idx,((i + 2) * sizeof(FIDX *)))) == NULL) {
            printf("\nNot enough memory to load FILES.IDX!  Exiting....");
            exit(255);
        }
    }
    fclose(fidx_fp);
    *num_recs = i;
    strsort(f_idx, *num_recs);
    return(f_idx);
}

unsigned SearchFIDX(FIDX **f_idx, int num_recs, char *key)
{
    unsigned low;
    unsigned high;
    unsigned median;

    low = 0;
    high = num_recs - 1;

    do {
        median = (low + high) / 2;
        if(strnicmp(key,f_idx[median]->name,MAX_FN_LEN) < 0) {
            high = median - 1;
        }
        else {
            low = median + 1;
        }
    } while(!(strnicmp(key,f_idx[median]->name,MAX_FN_LEN) == 0 || low > high));
    if(strnicmp(key,f_idx[median]->name,MAX_FN_LEN) == 0) {
        return(median);
    }
    else {
        return(-1);
    }
}

/*
**  strsort() -- Shell sort an array of string pointers via strcmp()
**  public domain by Ray Gardner   Denver, CO   12/88
*/

void strsort (FIDX **array, size_t array_size)
{
      size_t gap, i, j;
      FIDX **a, **b, *tmp;

      for (gap = 0; ++gap < array_size; )
            gap *= 2;
      while (gap /= 2)
      {
            for (i = gap; i < array_size; i++)
            {
                  for (j = i - gap; ;j -= gap)
                  {
                        a = array + j;
                        b = a + gap;
                        if (strnicmp((*a)->name, (*b)->name, MAX_FN_LEN) <= 0)
                              break;
                        tmp = *a;
                        *a = *b;
                        *b = tmp;
                        if (j < gap)
                              break;
                  }
            }
      }
}

FILE *SH_fopen(char *filename, char *mode)
{
    HFILE file_handle;
    ULONG action_taken;
    APIRET rc;
    FILE *fp;

    rc = DosOpen(filename,&file_handle,&action_taken,0L,FILE_NORMAL,
                 FILE_OPEN | OPEN_ACTION_FAIL_IF_NEW,
                 OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY,0L);
    if(rc != 0) {
        return(NULL);
    }
    else {
        if((fp = fdopen(file_handle,mode)) == NULL) {
            DosClose(file_handle);
            return(NULL);
        }
        else {
            return(fp);
        }
    }
}
