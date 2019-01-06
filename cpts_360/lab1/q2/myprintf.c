typedef unsigned int u32;

char *ctable = "0123456789ABCDEF";
int  BASE = 10;

int rpu(u32 x)
{  
    char c;
    if (x){
       c = ctable[x % BASE];
       rpu(x / BASE);
       putchar(c);
    }
}

int printu(u32 x)
{
  BASE = 10;
  (x==0)? putchar('0') : rpu(x);
  putchar(' ');
  putchar('\n');
}

int prints(char *s)
{
  while(*s != '\0')
    {
      putchar(*s);
      s++;
    }
   putchar(' ');
   putchar('\n');
}

int printd(int x)
{
  BASE = 10;
  if (x<0)
    putchar('-');
  
  (x==0)? putchar('0') : rpu(abs(x));
  putchar(' ');
  putchar('\n');
}

int printx(u32 x)
{
  BASE = 16;
  putchar('0');
  putchar('x');
  (x==0)? putchar('0') : rpu(x);
  putchar(' ');
  putchar('\n');
}

int printo(u32 x)
{
  BASE = 8;
  putchar('0');
  (x==0)? putchar('0') : rpu(x);
  putchar(' ');
  putchar('\n');
}

int myprintf(char *fmt, ...)
{
  char *cp;
  int *ip;

  cp = fmt;
  ip = &fmt + 1 ;
  
  while(*cp != '\0')
    {      
      if (*cp == '%')
	{
	  cp++;
	  switch(*cp)
	    {
	      case 'c':
		putchar(*ip);
		break;
	      case 's':
		prints(*ip);
		break;
	      case 'u':
		printu(*ip);
		break;
	      case 'd':
		printd(*ip);
		break;
	      case 'o':
		printo(*ip);
		break;
	      case 'x':
		printx(*ip);
		break;
	      default:
		prints("bad type");
	    }
	  ip++;	  
	}
      
      else
	cp++;
    }
  
}

int main(int argc, char *argv[], char *env[])
{
  u32 a=10;
  char my_str[] = "cpts_360:Systems Programing";
  int i = -10; int i_1 = 0;
  u32 hex=164;
  u32 oct=89;

  myprintf("%s", "------------my tests-------------");
  printu(a);
  prints(my_str);
  printd(i); printd(i_1);
  printx(hex);
  printo(oct);
  printd(oct);

  myprintf("%s", "------------assignment test------------");
  myprintf("cha=%c string=%s  dec=%d hex=%x oct=%o neg=%d\n",
	    'A', "this is a test", 100,    100,   100,  -100);
  myprintf("%s", "------------assignment------------");
  for(int i=0; i<argc; i++){
    printf("argc=%i argv=%s env=%s\n", argc, argv[i], env[i]);
      }
}
