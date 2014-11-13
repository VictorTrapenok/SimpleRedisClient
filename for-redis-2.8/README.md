<h1>Клиент для работы с редисом версии 2.8</h1>

<b>
This client tested only on Ubuntu, Debian, CentOS. (Этот клиент тестировался только под Ubuntu, Debian, CentOS)
</b>

<p>Тестировался для redis версии 2.8</p>
<p>В разных версиях redis имеются отличия в протоколе, чтоб не усложнять код и не плодить баги для каждой версии redis свой клинт.</p>

<h1>Последние изменения</h1>
<p>
<ul>
    <li>Добавлена поддержка redis версии 2.8</li>
    <li>Автоматически переподключается при потере соединения</li>
    <li>Добавлены некоторые функции от redis такие как select, dbsize и другие</li>
    <li>Поправлены баги работы с сетью</li>
</ul>
</p>

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

