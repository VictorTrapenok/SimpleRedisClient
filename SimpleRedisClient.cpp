/* 
 * File:   SimpleRedisClient.cpp
 * Author: victor
 * 
 * Created on 10 Август 2013 г., 22:26
 */

#include "SimpleRedisClient.h"
 
    SimpleRedisClient::SimpleRedisClient()
    { 
        sprintf(host,"127.0.0.1");
        bufer_size = 2048;
        bufer = new char[bufer_size];
    }

    int SimpleRedisClient::read_select(int fd, int timeout )
    {
      struct timeval tv;
        fd_set fds;

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000)*1000;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        return select(fd + 1, &fds, NULL, NULL, &tv); 
    }
    
    int SimpleRedisClient::wright_select(int fd, int timeout )
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

        printf("SEND:%s",bufer);
        rc = send_data(bufer);

        if (rc != (int)strlen(bufer))
        {
          if (rc < 0)
          {
              return CREDIS_ERR_SEND;
          }

          return CREDIS_ERR_TIMEOUT;
        }/**/


        rc = receive_reply(recvtype);

        return rc;
    }

    int SimpleRedisClient::receive_reply( char recvtype )
    {
        //char *line;
        //char *prefix = 0;
        bzero(bufer,bufer_size);
        int r = receive_data( bufer, bufer_size);
        
            printf("REDIS BUF: R:%d S:%s\n\n",r, bufer);
        if ( r > 0)
        {
            //printf("REDIS BUF:%s\n\n",bufer);
            /*prefix = *(line++);

            if (recvtype != CR_ANY && prefix != recvtype && prefix != CR_ERROR)
            {
                    return CREDIS_ERR_PROTOCOL;
            }

            switch (prefix)
            {
                case CR_ERROR:
                    return cr_receiveerror(rhnd, line);
                case CR_INLINE:
                    return cr_receiveinline(rhnd, line);
                case CR_INT:
                    return cr_receiveint(rhnd, line);
                case CR_BULK:
                    return cr_receivebulk(rhnd, line);
                case CR_MULTIBULK:
                    return cr_receivemultibulk(rhnd, line);
            }*/
        }

        return CREDIS_ERR_RECV;
    }

 
   /**
    * Принимает данные
    * @param buf
    * @param size
    * @return 
    */
   int SimpleRedisClient::receive_data( char *buf, int size)
   {
        /* Receives at most `size' bytes from socket `fd' to `buf'. Times out after 
         * `msecs' milliseconds if no data has yet arrived.
         * Returns:
         *  >0  number of read bytes on success
         *   0  server closed connection
         *  -1  on error
         *  -2  on timeout
         **/
        int rc = cr_selectreadable(fd, timeout);
        if (rc > 0)
        {
            return recv(fd, buf, size, 0);
        }
        else if (rc == 0)
        {
            return -2;
        }
        else
        {
            return -1;  
        }
   }

    /**
     * Отправляет данные
     * @param buf
     * @return 
     */
    int SimpleRedisClient::send_data( char *buf )
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
    
    
    int SimpleRedisClient::redis_conect()
    {
        int rc;
        struct sockaddr_in sa;


        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);

        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 || setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&yes, sizeof(yes)) == -1 || setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&yes, sizeof(yes)) == -1)
        {
            printf("open error %d\n", fd);
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

            if (cr_selectwritable(fd, timeout) > 0)
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
        printf("open ok %d\n", fd);
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
        return redis_send( CR_INLINE, "SET %s %s\r\n",   key, val);
    }

    
    int SimpleRedisClient::setex(const char *key, const char *val, int seconds)
    { 
        return redis_send(CR_INLINE, "SETEX %s %d %s\r\n",key, seconds, val); 
    }
    

    int SimpleRedisClient::get(const char *key, char **val)
    {
      int rc = redis_send( CR_BULK, "GET %s\r\n", key);

      //if (rc == 0 && (*val = rhnd->reply.bulk) == NULL)
      //  return -1;

      return rc;
    }

    int SimpleRedisClient::getset(const char *key, const char *set_val, char **get_val)
    { 
      int rc = redis_send( CR_BULK, "GETSET %s %s\r\n",   key, set_val);
      
      //if (rc == 0 && (*get_val = rhnd->reply.bulk) == NULL)
      //  return -1;

      return rc;
    }


    int SimpleRedisClient::ping() 
    {
      return redis_send( CR_INLINE, "PING\r\n");
    }

    int SimpleRedisClient::echo(const char *message, char **reply)
    { 
      return redis_send( CR_BULK, "ECHO %s\r\n", message);;
    }

    int SimpleRedisClient::quit() 
    {
      return redis_send( CR_INLINE, "QUIT\r\n");
    }

    int SimpleRedisClient::auth(const char *password)
    {
      int rc = redis_send( CR_INLINE, "AUTH %s\r\n", password);

      /* Request Redis server version once we have been authenticated */
      //if (rc == 0)
      //  return cr_getredisversion(rhnd);

      return rc;
    }
 

    int SimpleRedisClient::credis_setnx(const char *key, const char *val)
    {
      int rc = redis_send( CR_INT, "SETNX %s %s\r\n",  key, val);
      
      //if (rc == 0 && rhnd->reply.integer == 0)
      //  rc = -1;

      return rc;
    }
    

    /**
     * static int cr_incr(REDIS rhnd, int incr, int decr, const char *key, int *new_val)
     * credis_incr
     * credis_decr
     * credis_incrby
     * credis_decrby
     * 
     * 
     * cr_multikeybulkcommand
     * cr_multikeystorecommand
     * credis_mget
     * 
     */ 

    int SimpleRedisClient::credis_append(const char *key, const char *val)
    {
      int rc;
 
        rc = redis_send(CR_INT, "APPEND %s %s\r\n",  key, val);
       
      //if (rc == 0)
        //rc = rhnd->reply.integer;

      return rc;                            
    }

    int SimpleRedisClient::credis_substr( const char *key, int start, int end, char **substr)
    {
      int rc;

        rc = redis_send(CR_BULK, "SUBSTR %s %d %d\r\n",   key, start, end);
      
      //if (rc == 0 && substr) 
       // *substr = rhnd->reply.bulk;

      return rc;                            
    }

    int SimpleRedisClient::credis_exists( const char *key)
    {
      int rc = redis_send( CR_INT, "EXISTS %s\r\n", key);

      //if (rc == 0 && rhnd->reply.integer == 0)
        //rc = -1;

      return rc;
    }

    int SimpleRedisClient::credis_del( const char *key)
    {
      int rc = redis_send( CR_INT, "DEL %s\r\n", key);

      //if (rc == 0 && rhnd->reply.integer == 0)
       // rc = -1;

      return rc;
    }

    int SimpleRedisClient::credis_type( const char *key)
    {
      int rc = redis_send( CR_INLINE, "TYPE %s\r\n", key);

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

      return rc;
    }

    int SimpleRedisClient::credis_keys(REDIS rhnd, const char *pattern, char ***keyv)
    {
      int rc;

      /* with Redis 2.0.0 keys-command returns a multibulk instead of bulk */
        rc = redis_send(CR_MULTIBULK, "KEYS %s\r\n", pattern);
    

      //if (rc == 0) {
      //  *keyv = rhnd->reply.multibulk.bulks;
      //  rc = rhnd->reply.multibulk.len;
      //}

      return rc;
    }

    int SimpleRedisClient::credis_randomkey( char **key)
    {
      int rc;

        /* with Redis 2.0.0 randomkey-command returns a bulk instead of inline */
        rc = redis_send( CR_BULK, "RANDOMKEY\r\n");

        //if (rc == 0 && key) 
        //  *key = rhnd->reply.bulk;
      

      return rc;
    }

    int SimpleRedisClient::credis_rename( const char *key, const char *new_key_name)
    {
      return redis_send( CR_INLINE, "RENAME %s %s\r\n",   key, new_key_name);
    }

    int SimpleRedisClient::credis_renamenx( const char *key, const char *new_key_name)
    {
      int rc = redis_send( CR_INT, "RENAMENX %s %s\r\n", key, new_key_name);

      //if (rc == 0 && rhnd->reply.integer == 0)
      //  rc = -1;

      return rc;
    }

    int SimpleRedisClient::credis_dbsize()
    {
      int rc = redis_send( CR_INT, "DBSIZE\r\n");

      //if (rc == 0) 
      //  rc = rhnd->reply.integer;

      return rc;
    }

    int SimpleRedisClient::credis_expire( const char *key, int secs)
    { 
      int rc = redis_send( CR_INT, "EXPIRE %s %d\r\n", key, secs);

      //if (rc == 0 && rhnd->reply.integer == 0)
      //  rc = -1;

      return rc;
    }

    int SimpleRedisClient::credis_ttl( const char *key)
    {
      int rc = redis_send( CR_INT, "TTL %s\r\n", key);

      //if (rc == 0)
       // rc = rhnd->reply.integer;

      return rc;
    }

 
    
    /**
     * 
     * @param TimeOut
     */
    void setTimeout( int TimeOut)
    {
      timeout = TimeOut;
    }

    /**
     * Закрывает соединение
     */
    void redis_close()
    {
        printf("close ok %d\n", fd);
        if(fd != 0 )
        {
            close(fd);
        }

        delete bufer;
        bufer_size = 0;
    }