Версия 0.0 ещё не готова к использованию.
Вдохновением послужили исходники credis


<pre>
int main(int argc, char *argv[])
{
  SimpleRedisClient rc;

  rc.redis_conect();

  printf("-------------------\n");
  rc.set("MYKEY","MYVALUE");
  printf("-------------------\n");
  rc.setex("MYKEY10","MYVALUE10", 10);
  printf("-------------------\n");
  
  char *c;
  rc.get("MYKEY2", &c);
  printf("MYKEY2:%s\n",c);

  printf("-------------------\n");
  rc.redis_close();

  return 0;
}
</pre>