/********************************************************************************
*                                                                               *
*                R e s o u r c e   W r a p p i n g   U t i l i t y              *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2003 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id: reswrap.cpp,v 1.5 2003/04/05 14:22:37 fox Exp $                          *
********************************************************************************/
#include "xincs.h"
#include "fxdefs.h"

#define PPM_MAGIC1  'P'
#define PPM_MAGIC2  '3'
#define RPPM_MAGIC2 '6'
#define PPM_FORMAT  (PPM_MAGIC1*256+PPM_MAGIC2)
#define RPPM_FORMAT (PPM_MAGIC1*256+RPPM_MAGIC2)



/*

  To do:
  - Need option to place #include "icons.h" or something into icons.cc

*/

const char version[]="1.0.4";



/* Read magin number */
int ppm_readmagicnumber(FILE* file){
  int ich1, ich2;
  ich1=getc(file);
  if(ich1==EOF){
    fprintf(stderr,"EOF encountered.\n");
    exit(1);
    }
  ich2=getc( file );
  if(ich2==EOF){
    fprintf(stderr,"EOF encountered.\n");
    exit(1);
    }
  return ich1*256+ich2;
  }


/* Get character, skipping comments */
char ppm_getc(FILE* file){
  register int ich;
  register char ch;
  ich=getc( file );
  if(ich==EOF){
    fprintf(stderr,"EOF encountered.\n");
    exit(1);
    }
  ch=(char)ich;
  if(ch=='#'){
    do{
      ich=getc(file);
      if(ich==EOF){
        fprintf(stderr,"EOF encountered.\n");
        exit(1);
        }
      ch=(char)ich;
      }
    while(ch!='\n' && ch!='\r');
    }
  return ch;
  }


/* Get integer */
int ppm_getint(FILE* file){
  register char ch;
  register int i;
  do{
    ch=ppm_getc(file);
    }
  while(ch==' ' || ch=='\t' || ch=='\n' || ch=='\r');
  if(ch<'0' || ch>'9'){
    fprintf(stderr,"Expected a number.\n");
    exit(1);
    }
  i=0;
  do{
    i=i*10+ch-'0'; ch=ppm_getc(file);
    }
  while(ch>='0' && ch<='9');
  return i;
  }


/* Get raw byte */
unsigned char ppm_getrawbyte(FILE* file){
  register int iby;
  iby=getc(file);
  if(iby==EOF){
    fprintf(stderr,"EOF encountered.\n");
    exit(1);
    }
  return (unsigned char)iby;
  }


/* Read PPM file */
unsigned char* ppm_read(FILE* file,int& cols,int& rows){
  unsigned char* pixels,*pix;
  unsigned int r,g,b;
  int format,maxval;
  int row,col;

  /* Get format */
  format=ppm_readmagicnumber(file);

  /* Get size */
  cols=ppm_getint(file);
  rows=ppm_getint(file);
  maxval=ppm_getint(file);
  if(maxval<=0 || maxval>1023){
    fprintf(stderr,"Illegal maxval value: %d.\n",maxval);
    exit(1);
    }

  /* Create memory */
  pixels=(unsigned char*)malloc(3*rows*cols);
  if(!pixels){
    fprintf(stderr,"Image too big\n");
    exit(1);
    }

  /* Load pixels */
  pix=pixels;
  switch(format){
    case PPM_FORMAT:
      for(row=0; row<rows; row++){
        for(col=0; col<cols; col++){
          r = (255*ppm_getint(file))/maxval;
          g = (255*ppm_getint(file))/maxval;
          b = (255*ppm_getint(file))/maxval;
          *pix++ = r;
          *pix++ = g;
          *pix++ = b;
          }
        }
      break;

    case RPPM_FORMAT:
      for(row=0; row<rows; row++){
        for(col=0; col<cols; col++){
          r = ppm_getrawbyte(file);
          g = ppm_getrawbyte(file);
          b = ppm_getrawbyte(file);
          *pix++ = r;
          *pix++ = g;
          *pix++ = b;
          }
        }
      break;

    default:
      fprintf(stderr,"Unknown format.\n");
      exit(1);
      break;
    }
  return pixels;
  }



/* Print some help */
void printusage(){
  fprintf(stderr,"Usage: reswrap [options] [-o[a] outfile] files...\n");
  fprintf(stderr,"  options:\n");
  fprintf(stderr,"  -h       Print help\n");
  fprintf(stderr,"  -v       Print version number\n");
  fprintf(stderr,"  -d       Output as decimal\n");
  fprintf(stderr,"  -x       Output as hex (default)\n");
  fprintf(stderr,"  -e       Generate external reference declaration\n");
  fprintf(stderr,"  -i       Build an include file\n");
  fprintf(stderr,"  -s       Suppress header in output file\n");
  fprintf(stderr,"  -n name  Override resource name\n");
  fprintf(stderr,"  -c cols  Change number of columns in output to cols\n");
  fprintf(stderr,"  -ppm     Convert PPM file\n");
  }



/* Main */
int main(int argc,char **argv){
  FILE *resfile,*outfile;
  int i,j,k,cc,b,first,col,maxcols,hex,external,header,override,include,ppm;
  int ppm_w,ppm_h,num,nbytes;
  char name[100],*filename,*ptr;
  unsigned char *data;

  if(argc<2){
    printusage();
    exit(1);
    }
  outfile=stdout;
  maxcols=16;
  hex=1;
  external=0;
  header=1;
  override=0;
  include=0;
  ppm=0;
  data=0;

  /* Process options */
  for(i=1; i<argc; i++){

    /* Option */
    if(argv[i][0]=='-'){

      /* Change output file */
      if(argv[i][1]=='o'){
        i++;
        if(i>=argc){
          fprintf(stderr,"reswrap: missing argument for -o option\n");
          exit(1);
          }
        if(outfile!=stdout) fclose(outfile);
        if(argv[i-1][2]=='a'){
          outfile=fopen(argv[i],"a");
          }
        else{
          outfile=fopen(argv[i],"w");
          }
        if(!outfile){
          fprintf(stderr,"reswrap: unable to open output file %s\n",argv[i]);
          exit(1);
          }
        }

      /* Print help */
      else if(argv[i][1]=='h'){
        printusage();
        exit(0);
        }

      /* Print version */
      else if(argv[i][1]=='v'){
        fprintf(stderr,"reswrap version %s\n",version);
        exit(0);
        }

      /* Switch to decimal */
      else if(argv[i][1]=='d'){
        hex=0;
        }

      /* Switch to hex */
      else if(argv[i][1]=='x'){
        hex=1;
        }

      /* Suppress header */
      else if(argv[i][1]=='s'){
        header=0;
        }

      /* Generate as external reference */
      else if(argv[i][1]=='e'){
        external=1;
        }

      /* Building include file implies also extern */
      else if(argv[i][1]=='i'){
        include=1;
        external=1;
        }

      /* Change number of columns */
      else if(argv[i][1]=='c'){
        i++;
        if(i>=argc){
          fprintf(stderr,"reswrap: missing argument for -c option\n");
          exit(1);
          }
        if(sscanf(argv[i],"%d",&maxcols)==1 && maxcols<1){
          fprintf(stderr,"reswrap: illegal argument for number of columns\n");
          exit(1);
          }
        }

      /* Override resource name */
      else if(argv[i][1]=='n'){
        i++;
        if(i>=argc){
          fprintf(stderr,"reswrap: missing argument for -n option\n");
          exit(1);
          }
        strncpy(name,argv[i],sizeof(name));
        name[sizeof(name)-1]=0;
        override=1;
        }

      /* PPM File */
      else if(argv[i][1]=='p' && argv[i][2]=='p' && argv[i][3]=='m'){
        ppm=1;
        }
      }

    /* Resource */
    else{
      col=0;
      first=1;

      /* Get file name */
#ifndef WIN32
      if((filename=strrchr(argv[i],PATHSEP))!=0)
        filename=filename+1;
      else
        filename=argv[i];
#else
      if((filename=strrchr(argv[i],'\\'))!=0)
        filename=filename+1;
      else if((filename=strrchr(argv[i],'/'))!=0) // For CYGWIN bash
	filename=filename+1;
      else
        filename=argv[i];
#endif

      /* Output header */
      if(header){
        fprintf(outfile,"/* Generated by reswrap from file %s */\n",filename);
        }

      /* Determine resource name from file name */
      if(!override){
        strncpy(name,filename,sizeof(name));
        name[sizeof(name)-1]=0;
        if((ptr=strrchr(name,'.'))!=0) *ptr=0;
        for(k=j=0; name[j]; j++){
          cc=name[j];
          if(cc=='.') cc='_';
          if(k==0 && !(isalpha(cc) || cc=='_')) continue;
          if(!(isalnum(cc) || cc=='_')) continue;
          name[k++]=cc;
          }
        name[k]=0;
        if(k==0){
          fprintf(stderr,"reswrap: cannot generate resource name from %s\n",filename);
          exit(1);
          }
        }

      /* Open data file */
      resfile=fopen(argv[i],"rb");
      if(!resfile){
        fprintf(stderr,"reswrap: unable to open input file %s\n",argv[i]);
        exit(1);
        }

      /* Read in if PPM file */
      if(ppm){
        data=ppm_read(resfile,ppm_w,ppm_h);
        if(header){
          fprintf(outfile,"#define %s_width  %d\n",name,ppm_w);
          fprintf(outfile,"#define %s_height %d\n",name,ppm_h);
          }
        }

      /* Generate external reference for #include's */
      if(external){ fprintf(outfile,"extern "); }

      /* Output declaration */
      fprintf(outfile,"const unsigned char %s[]",name);

      /* Generate resource array */
      if(!include){
        fprintf(outfile,"={\n  ");
        if(ppm){
          nbytes=3*ppm_w*ppm_h;
          for(num=0; num<nbytes; num++){
            b=data[num];
            if(!first){
              fprintf(outfile,",");
              }
            if(col>=maxcols){
              fprintf(outfile,"\n  ");
              col=0;
              }
            if(hex)
              fprintf(outfile,"0x%02x",b);
            else
              fprintf(outfile,"%3d",b);
            first=0;
            col++;
            }
          }
        else{
          while((b=fgetc(resfile))!=EOF){
            if(!first){
              fprintf(outfile,",");
              }
            if(col>=maxcols){
              fprintf(outfile,"\n  ");
              col=0;
              }
            if(hex)
              fprintf(outfile,"0x%02x",b);
            else
              fprintf(outfile,"%3d",b);
            first=0;
            col++;
            }
          }
        fprintf(outfile,"\n  }");
        }

      fprintf(outfile,";\n\n");

      if(data) free(data);

      fclose(resfile);
      override=0;
      }
    }
  return 0;
  }



