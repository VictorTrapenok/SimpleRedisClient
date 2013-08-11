/* 
 * File:   SimpleRedisClient.cpp
 * Author: victor
 * 
 * Created on 10 Август 2013 г., 22:26
 */

#include "SimpleRedisClient.h"
   

    #define RC_ERROR '-'

    /**
     * Строка в ответе
     * @see http://redis.io/topics/protocol
     */
    #define RC_INLINE '+'

    /**
     * Определяет длину аргумента ответа
     * -1 нет ответа
     * @see http://redis.io/topics/protocol
     */
    #define RC_BULK '$'


    /**
     * Определяет количество аргументов ответа
     * -1 нет ответа
     * @see http://redis.io/topics/protocol
     */
    #define RC_MULTIBULK '*'
    #define RC_INT ':'
    #define RC_ANY '?'
    #define RC_NONE ' '



    SimpleRedisClient::SimpleRedisClient()
    {  
        bufer_size = 2048;
        bufer = new char[bufer_size];
    }
    
    SimpleRedisClient::~SimpleRedisClient()
    {
        redis_close();
        
        delete bufer;
        bufer_size = 0;
    }
    
    /*  
     * Returns:
     *  >0  Колво байт
     *   0  Соединение закрыто
     *  -1  error
     *  -2  timeout
     **/
    int SimpleRedisClient::read_select(int fd, int timeout ) const
    {
        struct timeval tv;
        fd_set fds;

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000)*1000;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        return select(fd + 1, &fds, NULL, NULL, &tv); 
    }
    
    /*  
     * Returns:
     *  >0  Колво байт
     *   0  Соединение закрыто
     *  -1  error
     *  -2  timeout
     **/
    int SimpleRedisClient::wright_select(int fd, int timeout ) const
    {
        struct timeval tv;
        fd_set fds;

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000)*1000;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        return select(fd+1, NULL, &fds, NULL, &tv);
    }
    
    int SimpleRedisClient::redis_send(char recvtype, const char *format, ...)
    {
        data = 0;
        data_size = 0;
                        
        bzero(bufer,bufer_size); 
        va_list ap; 
        va_start(ap, format);
        
        int  rc = vsnprintf(bufer, bufer_size, format, ap);
        va_end(ap);

        if( rc < 0 )
        {
            
            return -1;
        }

        if( rc >= bufer_size )
        {
            return -1; // Не хватило буфера
        }

        if(debug > 3) printf("SEND:%s",bufer);
        rc = send_data(bufer);

        if (rc != (int) strlen(bufer))
        {
            if (rc < 0)
            {
                return RC_ERR_SEND;
            }

            return RC_ERR_TIMEOUT;
        }


        bzero(bufer,bufer_size);  
        rc = read_select(fd, timeout);
        
        if (rc > 0)
        {
            rc = recv(fd, bufer, bufer_size, 0);
            if(rc < 0)
            {
                return CR_ERR_RECV; 
            }
            
            if(debug > 3) printf("REDIS BUF: R:%d S:%s",rc, bufer); 
            
            char prefix = bufer[0];

            if (recvtype != RC_ANY && prefix != recvtype && prefix != RC_ERROR)
            {
                    printf("\x1b[31mREDIS RC_ERR_PROTOCOL:%s\x1b[0m\n",bufer);
                    return RC_ERR_PROTOCOL;
            }

            char *p;
            int len = 0;
            
            switch (prefix)
            {
                case RC_ERROR:
                    printf("\x1b[31mREDIS RC_ERROR:%s\x1b[0m\n",bufer);
                        data = bufer;
                        data_size = rc;
                    return rc;
                case RC_INLINE:
                    if(debug) printf("\x1b[33mREDIS RC_INLINE:%s\x1b[0m\n", bufer);
                        data_size = strlen(bufer+1)-2; 
                        data = bufer+1; 
                        data[data_size] = 0;
                    return rc;
                case RC_INT:
                    printf("\x1b[31mREDIS RC_INT:%s\x1b[0m\n", bufer);
                        data = bufer;
                        data_size = rc;
                    return rc;
                case RC_BULK:
                    if(debug) printf("\x1b[33mREDIS RC_BULK:%s\x1b[0m\n", bufer);
                      
                        p = bufer;
                        p++;
                        while(*p != '\r') {
                            len = (len*10)+(*p - '0');
                            p++;
                        }

                        /* Now p points at '\r', and the len is in bulk_len. */
                        if(debug > 3) printf("%d\n", len);
                        
                        data = p+2;
                        data_size = len;
                        data[data_size] = 0;
                        
                    return rc;
                case RC_MULTIBULK:
                    printf("\x1b[31mREDIS RC_MULTIBULK:%s\x1b[0m\n", bufer); 
                        data = bufer;
                        data_size = rc;
                    return rc;
            }
            
            return rc;
        }
        else if (rc == 0)
        {
            return RC_ERR_CONECTION_CLOSE; // Соединение закрыто
        }
        else
        {
            return RC_ERR; // error
        }
    }
 
    
     
    /**
     * Отправляет данные
     * @param buf
     * @return 
     */
    int SimpleRedisClient::send_data( const char *buf ) const
    {
        fd_set fds;
        struct timeval tv;
        int rc, sent = 0;

        /* NOTE: On Linux, select() modifies timeout to reflect the amount
         * of time not slept, on other systems it is likely not the same */
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000)*1000;

        int tosend = strlen(buf); // При отправке бинарных данных возможны баги.

        while (sent < tosend)
        {
            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            rc = select(fd + 1, NULL, &fds, NULL, &tv);

            if (rc > 0)
            {
                rc = send(fd, buf + sent, tosend - sent, 0);
                if (rc < 0)
                {
                    return -1;
                }
                sent += rc;
            }
            else if (rc == 0) /* timeout */
            {
                break;
            }
            else
            {
                return -1;
            }
        }

        return sent;
    }
    
    /**
     *  public:
     */
    
    /**
     * Соединение с редисом.
     */
    int SimpleRedisClient::redis_conect(const char* Host,int Port)
    {
        setPort(Port);
        setHost(Host);
        return redis_conect();
    }
    
    int SimpleRedisClient::redis_conect(const char* Host,int Port, int TimeOut)
    {
        setPort(Port);
        setHost(Host);
        setTimeout(TimeOut);
        return redis_conect();
    }
    
    int SimpleRedisClient::redis_conect()
    {
        if(host == 0)
        {
            setHost("127.0.0.1");
        }
        
        int rc;
        struct sockaddr_in sa;

        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);

        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 || setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&yes, sizeof(yes)) == -1 || setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&yes, sizeof(yes)) == -1)
        {
            if(debug) printf("open error %d\n", fd);
        }

        int err;
        struct addrinfo hints, *info;

        if (inet_aton(host, &sa.sin_addr) == 0)
        {
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            err = getaddrinfo(host, NULL, &hints, &info);
            if (err)
            {
                //DEBUG("getaddrinfo error: %s\n", gai_strerror(err));
            }

            memcpy(&sa.sin_addr.s_addr, &(info->ai_addr->sa_data[2]), sizeof(in_addr_t));
            freeaddrinfo(info);
        }

        int flags = fcntl(fd, F_GETFL);
        if ((rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK)) < 0)
        {
          //DEBUG("Setting socket non-blocking failed with: %d\n", rc);
        }

        if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0)
        {
            if (errno != EINPROGRESS)
            {
                // goto error;
            }

            if (wright_select(fd, timeout) > 0)
            {
                int err;
                unsigned int len = sizeof(err);
                if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err)
                {
                    //goto error;
                }
            }
            else /* timeout or select error */
            {
                //goto error;
            }
        }
        if(debug >1) printf("open ok %d\n", fd);
        return fd;
    }


    /**
     * Ни ключь ни значение не должны содержать "\r\n"
     * @param key
     * @param val
     * @return
     */
    int SimpleRedisClient::set(const char *key, const char *val)
    {
        return redis_send( RC_INLINE, "SET %s %s\r\n",   key, val);
    }

    
    int SimpleRedisClient::setex(const char *key, const char *val, int seconds)
    { 
        return redis_send(RC_INLINE, "SETEX %s %d %s\r\n",key, seconds, val); 
    }
    

    int SimpleRedisClient::get(const char *key)
    {
      return redis_send( RC_BULK, "GET %s\r\n", key); 
    }
    
    
    char* SimpleRedisClient::operator[] (const char *key)
    {  
        redis_send( RC_BULK, "GET %s\r\n", key);
        return getData();
    }
    
    
    SimpleRedisClient::operator char* () const
    { 

        /**
         * Выделет память и скопирует в него последний ответ редиса.
         * Вернёт указатель на ответ или 0 если последний запрос был с ошибкой.
         * 
         * Выделеную память надо будет очистить в ручную.
        char* d = 0;
        if( data )
        {
            char* d = new char[data_size];
            memcpy(d, data,data_size);
        } 
        return d;
         */
        return getData();
    }
    
    SimpleRedisClient::operator int () const
    {
      return data_size;
    }

    int SimpleRedisClient::getset(const char *key, const char *set_val, char **get_val)
    { 
      return redis_send( RC_BULK, "GETSET %s %s\r\n",   key, set_val); 
    }


    int SimpleRedisClient::ping() 
    {
      return redis_send( RC_INLINE, "PING\r\n");
    }

    int SimpleRedisClient::echo(const char *message, char **reply)
    { 
      return redis_send( RC_BULK, "ECHO %s\r\n", message);;
    }

    int SimpleRedisClient::quit() 
    {
      return redis_send( RC_INLINE, "QUIT\r\n");
    }

    int SimpleRedisClient::auth(const char *password)
    {
      return redis_send( RC_INLINE, "AUTH %s\r\n", password); 
    }
    
    
    int SimpleRedisClient::getRedisVersion()
    {
      /* We can receive 2 version formats: x.yz and x.y.z, where x.yz was only used prior 
       * first 1.1.0 release(?), e.g. stable releases 1.02 and 1.2.6 */
      /* TODO check returned error string, "-ERR operation not permitted", to detect if 
       * server require password? */
      if (redis_send( RC_BULK, "INFO\r\n") == 0)
      {
        sscanf(bufer, "redis_version:%d.%d.%d\r\n", &version_major, &version_minor, &version_patch);
        return version_major;
      }

      return 0;
    }
 

    int SimpleRedisClient::setnx(const char *key, const char *val)
    {
      return redis_send( RC_INT, "SETNX %s %s\r\n",  key, val); 
    }
    
 
    int SimpleRedisClient::append(const char *key, const char *val)
    {
      return redis_send(RC_INT, "APPEND %s %s\r\n",  key, val); 
    }

    int SimpleRedisClient::substr( const char *key, int start, int end, char **substr)
    {
      return redis_send(RC_BULK, "SUBSTR %s %d %d\r\n",   key, start, end); 
    }

    int SimpleRedisClient::exists( const char *key)
    {
      return redis_send( RC_INT, "EXISTS %s\r\n", key); 
    }

    int SimpleRedisClient::del( const char *key)
    {
      return redis_send( RC_INT, "DEL %s\r\n", key); 
    }

    int SimpleRedisClient::type( const char *key)
    {
      return redis_send( RC_INLINE, "TYPE %s\r\n", key);

      /*if (rc == 0) {
        char *t = rhnd->reply.line;
        if (!strcmp("string", t))
          rc = CREDIS_TYPE_STRING;
        else if (!strcmp("list", t))
          rc = CREDIS_TYPE_LIST;
        else if (!strcmp("set", t))
          rc = CREDIS_TYPE_SET;
        else
          rc = CREDIS_TYPE_NONE;
      }*/
 
    }

    int SimpleRedisClient::keys( const char *pattern, char ***keyv)
    {
      return redis_send(RC_MULTIBULK, "KEYS %s\r\n", pattern); 
    }

    int SimpleRedisClient::randomkey( char **key)
    {
      return redis_send( RC_BULK, "RANDOMKEY\r\n"); 
    }

    int SimpleRedisClient::rename( const char *key, const char *new_key_name)
    {
      return redis_send( RC_INLINE, "RENAME %s %s\r\n",   key, new_key_name);
    }

    int SimpleRedisClient::renamenx( const char *key, const char *new_key_name)
    {
      return redis_send( RC_INT, "RENAMENX %s %s\r\n", key, new_key_name); 
    }

    int SimpleRedisClient::dbsize()
    {
      return redis_send( RC_INT, "DBSIZE\r\n"); 
    }

    int SimpleRedisClient::expire( const char *key, int secs)
    { 
      return redis_send( RC_INT, "EXPIRE %s %d\r\n", key, secs); 
    }

    int SimpleRedisClient::ttl( const char *key)
    {
      return redis_send( RC_INT, "TTL %s\r\n", key); 
    }

 
    
    /**
     * 
     * @param TimeOut
     */
    void SimpleRedisClient::setTimeout( int TimeOut)
    {
      timeout = TimeOut;
    }

    /**
     * Закрывает соединение
     */
    void SimpleRedisClient::redis_close()
    {
        if(debug >1) printf("close ok %d\n", fd);
        if(fd != 0 )
        {
            close(fd);
        } 
    }
    
    
    char* SimpleRedisClient::getData() const
    {
        return data;
    }
    
    int SimpleRedisClient::getDataSize() const
    {
        return data_size;
    }