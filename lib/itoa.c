void itoa(char * buf, int i)
{
  char tmp[20];
  int m, index = 0, minus_flag = 0;
  
  if (i<0){ minus_flag = 1;    i = -i;}
  while(1){
    m = (i%10) + '0';
    tmp[index] = m;
    index ++ ;
    i = i / 10;
    if (i == 0) break;
  }
  int j = 0;
  if (minus_flag) {
    buf[0] = '-';
    buf = buf + 1;
  }
  for (;j < index; j++) {
    buf[j] = tmp[index-1-j];
  }
  buf[j] = 0;
  
}
