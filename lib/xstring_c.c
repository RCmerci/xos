int strlen(char * str)
{
  int i = 0;
  while (1) {
    if (str[i] == 0) return i;
    ++i;
  }
}

void strcpy(char * from, char * to)
{
  char c;
  while (1) {
    c = *from;
    *to = c;
    from++;
    to++;
    if ((*from) == 0) {
      *to = 0;
      break;
    }
  }
}

int strcmp(char * a, char * b)
{
  int i=0;
  while(1) {
    if (a[i] == b[i] && a[i] == 0) return 0;
    if (a[i] > b[i]) return 1;
    if (a[i] < b[i]) return -1;
    i++;
  }
}
