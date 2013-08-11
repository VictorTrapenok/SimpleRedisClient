Вдохновением послужили исходники credis


<pre>
int main(int argc, char *argv[])
{
 
  SimpleRedisClient rc;

  rc.redis_conect();

  printf("-------------------\n");
  if(rc.set("MYKEY","MYVALUE"))
  {
      printf("MYKEY == [%d][%s]\n", rc.getDataSize(), rc.getData());
  }
  printf("-------------------\n");
   
  if(rc.get("MYKEY"))
  {
      printf("MYKEY == [%d][%s]\n", rc.getDataSize(), rc.getData());
  }
  
  
  if(rc["MYKEY"])
  {
      printf("MYKEY == [%d][%s]\n", (int)rc, (char*)rc);
  }

  printf("-------------------\n");
  rc.redis_close();
}
</pre>