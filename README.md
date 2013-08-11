

<pre>
int main(int argc, char *argv[])
{
    
    SimpleRedisClient rc;

    rc.redis_conect();

    if(rc.set("MYKEY","MYVALUE"))
    {
        printf("MYKEY == [%d][%s]\n", rc.getDataSize(), rc.getData());
    }

    if(rc.get("MYKEY"))
    {
        printf("MYKEY == [%d][%s]\n", rc.getDataSize(), rc.getData());
    }

    rc = "MYKEY my-value-tester"; // Запись как и set


    if(rc["MYKEY"])
    {
        printf("MYKEY == [%d][%s]\n", (int)rc, (char*)rc);
    }


    rc.setex(key_name, redis_save_buf, 600);
 
    rc.set_printf("%s_stat_interval %d", "constCharName",123+6);
    rc.set_printf("stat_interval %d", 123+6);

    rc.redis_close();
}
</pre>