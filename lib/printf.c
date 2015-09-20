
#define va_list     char*

static void aux(char * format, va_list arg, char * buf)
{
  int i = 0;
  int len = strlen(format) + 1; /* 加上末尾 0  */
  int buf_index = 0;
  char tmp[20];
  for (;i < len; i++) {
    if (format[i] != '%') {
      buf[buf_index] = format[i];
      buf_index++;
      continue;
    }
    i++;			/* skip '%' */
    switch(format[i]){
    case 'd':
      itoa(tmp, *(int*)arg);      
      strcpy(tmp, &buf[buf_index]);
      buf_index += strlen(tmp);
      arg = (void*)arg + 4;
      break;
    case 's':
      strcpy(*(char**)arg, &buf[buf_index]);
      buf_index += strlen(*(char**)arg);
      arg = (void*)arg + 4;
      break;
    default:
      break;
    }
  }
}

#define CURRENT_PROC_STDOUT         -1 /* 指当前进程的标准输出, 话说这样好像标准输出就不能自由变了，先这样把 */

void printf(char * format, ...)
{
  char buf[128];
  va_list arg = (va_list)((void *)&format + 4);
  aux(format, arg, buf);
  write(CURRENT_PROC_STDOUT, buf, strlen(buf));
}

void printk(int magic_num, char * format , ...)
{
  char buf[128];
  va_list arg = (va_list)((void *)&format + 4);
  aux(format, arg, buf);
  writek(buf, magic_num);
}

void sprintf(char * s, char * format, ...)
{
  char buf[128];
  va_list arg = (va_list)((void *)&format + 4);
  aux(format, arg, buf);
  strcpy(buf, s);
}
