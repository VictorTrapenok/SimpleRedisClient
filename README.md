

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
	  
	  rc = "MYKEY my-value-tester";
	  
	  
	  if(rc["MYKEY"])
	  {
		  printf("MYKEY == [%d][%s]\n", (int)rc, (char*)rc);
	  }
	  
	  rc.sadd_printf("%s %d", "fff", 123);
	  rc.sadd_printf("%s %d", "fff", 14);
	  
	  rc.smembers("fff");
	  
	  if(rc.getMultiBulkDataAmount())
	  {
		  for(int i =0; i< rc.getMultiBulkDataAmount(); i++ )
		  {
		      printf("Answer[%d]->%s\n", i, rc.getData(i));
		  }
	  }

	  printf("-------------------\n");
	  rc.redis_close();

	  return 0;
}
</pre>
