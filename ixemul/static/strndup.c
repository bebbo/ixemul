//http://codase.com/search/display?file=L2dlbnRvbzIvdmFyL3RtcC9yZXBvcy9jb2Rhc2UuYy9tcDNibGFzdGVyLTMuMi4wL3dvcmsvbXAzYmxhc3Rlci0zLjIuMC9tcGVnc291bmQvaHR0cGlucHV0LmNj&lang=c%2B%2B&off=1311+1318+
/* MPEG/WAVE Sound library

   (C) 1997 by Woo-jae Jung */

// Httpinputstream.cc
// Inputstream for http

// It's from mpg123 
char *strndup(char *src,int num)
{
  char *dst;

  if(!(dst=(char *)malloc(num+1)))return 0;
  dst[num]='\0';

  return strncpy(dst, src, num);
}
