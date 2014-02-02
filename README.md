<h1>Клиент для работы с редисом.</h1>

<b>
This client tested only on Ubuntu, Debian, CentOS. (Этот клиент тестировался только под Ubuntu, Debian, CentOS)
</b>

<pre>
int main(int argc, char *argv[])
{
    SimpleRedisClient rc;
  
    rc.setHost(REDIS_HOST);
    rc.auth(REDIS_PW);
    rc.LogLevel(0);

    if(!rc)
    {
        printf("Соединение с redis не установлено\n");
        return -1;
    }
    
    rc = "MYKEY my-value-tester";
    if(rc["MYKEY"])
    {
        printf("MYKEY == [%d][%s]\n", (int)rc, (char*)rc);
    }
  
    printf("-------------------\n");
    rc.sadd_printf("%s %d", "MY_SET", 123);
    rc.sadd_printf("%s %d", "MY_SET", 14);

    rc.smembers("MY_SET");

    if(rc.getMultiBulkDataAmount())
    {
        for(int i =0; i< rc.getMultiBulkDataAmount(); i++ )
        {
            printf("Answer[%d]->%s\n", i, rc.getData(i));
        }
    }

    rc = "MYKEY1 my-value-tester";
    rc = "MYKEY2 my-value-tester";

    rc.delete_keys("MY*");
     
    rc.redis_close();
}
</pre>
