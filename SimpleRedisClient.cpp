/*
 * File:   SimpleRedisClient.cpp
 * Author: victor
 *
 * Created on 10 Август 2013 г., 22:26
 */

#include <cstdlib>
#include <iostream>
#include <memory>

#include <pthread.h>

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 

#include "SimpleRedisClient.h"

/**
 * Читает целое число из строки, если ошибка то вернёт -1
 * @param bufer Строка
 * @param delimiter Конец для числа
 * @param delta Количество символов занятое числом и разделителем
 * @return 
 */
int read_int(const char* bufer,char delimiter,int* delta)
{
    const char* p = bufer;
    int len = 0;
    int d = 0;
    
    if(*p == '0' )
    {
        (*delta)++;
        return 0;
    }

    while(*p != delimiter)
    {
        if(*p > '9' || *p < '0')
        {
            return -1;
        }
        
        len = (len*10)+(*p - '0');
        p++;
        (*delta)++;
        d++;
        
        if(d > 7)
        {
            return -1;
        }
    }

    return len;
}

int read_int(const char* bufer,char delimiter)
{ 
    const char* p = bufer;
    int len = 0;
    int delta = 0;

    if(*p == '0' )
    {
        return 0;
    }
    
    while(*p != delimiter)
    {
        if(*p > '9' || *p < '0')
        {
            return -1;
        }

        len = (len*10)+(*p - '0');
        p++;
        delta++;
        if(delta > 7)
        {
            return -1;
        }
    }

    return len;
}

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
        setBuferSize(2048);
    }

    int SimpleRedisClient::getBuferSize()
    {
        return bufer_size;
    }

    void SimpleRedisClient::setBuferSize(int size)
    {
        if(bufer != 0)
        {
            delete bufer;
            delete buf;
        }

        bufer_size = size;
        bufer = new char[bufer_size];
        buf = new char[bufer_size];
    }

    SimpleRedisClient::~SimpleRedisClient()
    {
        printf("~SimpleRedisClient\n");

        redis_close();

        delete bufer;
        bufer_size = 0;

        if(host != 0)
        {
            delete host;
        }
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
    
    void SimpleRedisClient::LogLevel(int l)
    {
        debug = l;
    }

    int SimpleRedisClient::LogLevel(void)
    {
        return debug;
    }

    int SimpleRedisClient::redis_send(char recvtype, const char *format, ...)
    {
        if(fd == 0)
        {
            redis_conect();
        }
        
        data = 0;
        data_size = 0;
        
        if(answer_multibulk != 0)
        {
            delete answer_multibulk; 
        }
        multibulk_arg = -1;
        answer_multibulk = 0;

        va_list ap;
        va_start(ap, format);

        bzero(bufer,bufer_size);
        int  rc = vsnprintf(bufer, bufer_size, format, ap);
        va_end(ap);

        if( rc < 0 )
        {
            return RC_ERR_DATA_FORMAT;
        }

        if( rc >= bufer_size )
        {
            return RC_ERR_BUFER_OVERFLOW;; // Не хватило буфера
        }

        if(debug > 3  ) printf("SEND:%s",bufer);
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

            if(debug > 3 && 1) printf("REDIS BUF: recv:%d bufer[%s]",rc, bufer);

            char prefix = bufer[0];

            if (recvtype != RC_ANY && prefix != recvtype && prefix != RC_ERROR)
            {
                    printf("\x1b[31m[fd=%d]REDIS RC_ERR_PROTOCOL[%c]:%s\x1b[0m\n",fd, recvtype, bufer);  
                    return RC_ERR_PROTOCOL;
            }

            char *p;
            int len = 0;

            switch (prefix)
            {
                case RC_ERROR:
                    printf("\x1b[31mREDIS[fd=%d] RC_ERROR:%s\x1b[0m\n",fd,bufer);
                        data = bufer;
                        data_size = rc; 
                    return rc;
                case RC_INLINE:
                    if(debug) printf("\x1b[33mREDIS[fd=%d] RC_INLINE:%s\x1b[0m\n", fd,bufer);
                        data_size = strlen(bufer+1)-2;
                        data = bufer+1;
                        data[data_size] = 0;
                    return rc;
                case RC_INT:
                    if(debug) printf("\x1b[33mREDIS[fd=%d] RC_INT:%s\x1b[0m\n",fd, bufer);
                        data = bufer+1;
                        data_size = rc;
                    return rc;
                case RC_BULK:
                    if(debug) printf("\x1b[33mREDIS[fd=%d] RC_BULK:%s\x1b[0m\n",fd, bufer);

                        p = bufer;
                        p++;

                        if(*p == '-')
                        {
                            data = 0;
                            data_size = -1;
                            return rc;
                        }

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
                    if(debug) printf("\x1b[33mREDIS[fd=%d] RC_MULTIBULK[Len=%d]:%s\x1b[0m\n", fd, rc, bufer);
                        data = bufer;
                        data_size = rc;
                        
                        p = bufer;
                        p++;
                        int delta = 0;
                        multibulk_arg =  read_int(p, '\r', &delta);
                        
                        answer_multibulk = new char*[multibulk_arg];
                        
                        p+= delta + 3;
                        
                        for(int i =0; i< multibulk_arg; i++)
                        {
                            len = 0; 
                            while(*p != '\r') {
                                len = (len*10)+(*p - '0');
                                p++;
                            }
                            
                            p+=2;
                            answer_multibulk[i] = p; 
                            
                            p+= len;
                            *p = 0;
                            p+= 3; 
                        }
                          
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

    void SimpleRedisClient::setPort(int Port)
    {
        port = Port;
    }

    void SimpleRedisClient::setHost(const char* Host)
    {
        if(host != 0)
        {
            delete host;
        }

        host = new char[strlen(Host)];
        memcpy(host,Host,strlen(Host));
    }

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
                if(debug) printf("getaddrinfo error: %s\n", gai_strerror(err));
            }

            memcpy(&sa.sin_addr.s_addr, &(info->ai_addr->sa_data[2]), sizeof(in_addr_t));
            freeaddrinfo(info);
        }

        int flags = fcntl(fd, F_GETFL);
        if ((rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK)) < 0)
        {
          if(debug) printf("Setting socket non-blocking failed with: %d\n", rc);
        }

        if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0)
        {
            if (errno != EINPROGRESS)
            {
                return RC_ERR;
            }

            if (wright_select(fd, timeout) > 0)
            {
                int err;
                unsigned int len = sizeof(err);
                if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err)
                {
                    return RC_ERR;
                }
            }
            else /* timeout or select error */
            {
                return RC_ERR_TIMEOUT;
            }
        }
        if(debug >1) printf("open ok %d\n", fd);
          
        
        return fd;
    }

    /**
     * SMEMBERS key
     * Время выполнения: O(N).
     * Возвращает все члены множества, сохранённого в указанном ключе. Эта команда - просто упрощённый синтаксис для SINTER.
     * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-smembers
     * @see http://redis.io/commands/smembers
     * @param key
     * @return 
     */
    int SimpleRedisClient::smembers(const char *key)
    { 
      return redis_send(RC_MULTIBULK, "SMEMBERS %s\r\n", key);
    }

    
    char** SimpleRedisClient::getMultiBulkData()
    {
        return answer_multibulk;
    }
    
    /**
     * Вернёт количество ответов. Или -1 если последняя операция вернула что либо кроме множества ответов.
     * @return 
     */
    int SimpleRedisClient::getMultiBulkDataAmount() 
    {
        return multibulk_arg;
    }
    
    /**
     * Работает только после запроса данных которые возвращаются как множество ответов
     * @param i Номер ответа в множестве отвкетов
     * @return Вернёт ноль в случаи ошибки или указатель на данные
     */
    char* SimpleRedisClient::getData(int i) const
    {
        if(multibulk_arg > i)
        {
            return answer_multibulk[i];
        } 
        return 0;
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

    /*int SimpleRedisClient::set_printf(const char *key, const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);

        bzero(buf, bufer_size);
        int  rc = vsnprintf(buf, bufer_size, format, ap);
        va_end(ap);

        if( rc >= bufer_size )
        {
            return RC_ERR_BUFER_OVERFLOW;; // Не хватило буфера
        }

        if(rc <  0)
        {
            return RC_ERR_DATA_FORMAT;
        }

        rc = redis_send( RC_INLINE, "SET %s %s\r\n",   key, buf);
        return rc;
    }*/

    int SimpleRedisClient::set_printf(const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);

        bzero(buf, bufer_size);
        int  rc = vsnprintf(buf, bufer_size, format, ap);
        va_end(ap);
        
        printf("set_printf test-buf:%s\n", buf);

        if( rc >= bufer_size )
        {
            return RC_ERR_BUFER_OVERFLOW;; // Не хватило буфера
        }

        if(rc <  0)
        {
            return RC_ERR_DATA_FORMAT;
        }

        rc = redis_send( RC_INLINE, "SET %s\r\n", buf);
        return rc;
    }

    SimpleRedisClient& SimpleRedisClient::operator=(const char *key_val)
    {
        redis_send( RC_INLINE, "SET %s\r\n", key_val);
        return *this;
    }


    int SimpleRedisClient::setex(const char *key, const char *val, int seconds)
    {
        return redis_send(RC_INLINE, "SETEX %s %d %s\r\n",key, seconds, val);
    }

    int SimpleRedisClient::setex_printf(int seconds, const char *key, const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);

        bzero(buf, bufer_size);
        int  rc = vsnprintf(buf, bufer_size, format, ap);
        va_end(ap);

        if( rc >= bufer_size )
        {
            return RC_ERR_BUFER_OVERFLOW;; // Не хватило буфера
        }

        if(rc <  0)
        {
            return RC_ERR_DATA_FORMAT;
        }

        return redis_send(RC_INLINE, "SETEX %s %d %s\r\n",key, seconds, buf); 
    }


    int SimpleRedisClient::setex_printf(const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);

        bzero(buf, bufer_size);
        int  rc = vsnprintf(buf, bufer_size, format, ap);
        va_end(ap);

        if( rc >= bufer_size )
        {
            return RC_ERR_BUFER_OVERFLOW;; // Не хватило буфера
        }

        if(rc <  0)
        {
            return RC_ERR_DATA_FORMAT;
        }

        return redis_send(RC_INLINE, "SETEX %s\r\n", buf); 
    }


    int SimpleRedisClient::get(const char *key)
    {
      return redis_send( RC_BULK, "GET %s\r\n", key);
    }
    
    
    int SimpleRedisClient::get_printf( const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);

        bzero(buf, bufer_size);
        int  rc = vsnprintf(buf, bufer_size, format, ap);
        va_end(ap);

        if( rc >= bufer_size )
        {
            return RC_ERR_BUFER_OVERFLOW;; // Не хватило буфера
        }

        if(rc <  0)
        {
            return RC_ERR_DATA_FORMAT;
        }

        return redis_send(RC_BULK, "GET %s\r\n", buf); 
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
        printf("SimpleRedisClient::operator int (%d) \n", data_size);
        if(data_size < 1)
        {
             printf("SimpleRedisClient::operator int (%d) \n", data_size);
            return data_size;
        }
        
        if(getData() == 0)
        {
            return -1;
        }
        
        int r = read_int(getData(), '\r');
        
        
        printf("SimpleRedisClient::operator int (%d|res=%d) \n", data_size, r);
        
        return r;
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

    /**
     * Время выполнения: O(1)
     * Удаление указанных ключей. Если переданный ключ не существует, операция для него не выполняется. Команда возвращает количество удалённых ключей.
     * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-del
     * @see http://redis.io/commands/del
     * @param key
     * @return 
     */
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
    
    int SimpleRedisClient::flushall(void)
    {
        return redis_send( RC_INLINE, "FLUSHALL\r\n");
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

    int SimpleRedisClient::delta( int delta, const char *key)
    {
        if (delta == 1 || delta == -1)
        {
            return redis_send( RC_INT, "%s %s\r\n",  delta > 0 ? "INCR" : "DECR", key);
        }
        else
        {
            return redis_send( RC_INT, "%s %s %d\r\n", delta > 0 ? "INCRBY" : "DECRBY", key, abs(delta) );
        }
    }


    int SimpleRedisClient::operator +=( const char *key)
    {
        return redis_send( RC_INT, "%s %s\r\n",  "INCR" , key);
    }

    int SimpleRedisClient::operator -=( const char *key)
    {
        return redis_send( RC_INT, "%s %s\r\n",  "DECR" , key);
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


    SimpleRedisClient::operator bool () const
    {
        return fd != 0;
    }

    int SimpleRedisClient::operator == (bool d)
    {
        return (fd != 0) == d;
    }


    char* SimpleRedisClient::getData() const
    {
        return data;
    }

    int SimpleRedisClient::getDataSize() const
    {
        return data_size;
    }


    int SimpleRedisClient::sadd(const char *key, const char *member)
    {
      return redis_send(RC_INT, "SADD %s %s\r\n", key, member);
    }

    /**
     * Add the specified members to the set stored at key. Specified members that are already a member of this set are ignored. If key does not exist, a new set is created before adding the specified members.
     * An error is returned when the value stored at key is not a set.
     * @see http://redis.io/commands/sadd
     * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-sadd
     * @param format
     * @param ... Ключь Значение (через пробел, в значении нет пробелов)
     * @return 
     */
    int SimpleRedisClient::sadd_printf(const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);

        bzero(buf, bufer_size);
        int  rc = vsnprintf(buf, bufer_size, format, ap);
        va_end(ap);

        if( rc >= bufer_size )
        {
            return RC_ERR_BUFER_OVERFLOW;; // Не хватило буфера
        }

        if(rc <  0)
        {
            return RC_ERR_DATA_FORMAT;
        }

        return redis_send(RC_INT, "SADD %s\r\n", buf);
    }
    
    int SimpleRedisClient::srem(const char *key, const char *member)
    {
        return redis_send(RC_INT, "SREM %s %s\r\n", key, member);
    }

    /**
     * Remove the specified members from the set stored at key. Specified members that are not a member of this set are ignored. If key does not exist, it is treated as an empty set and this command returns 0.
     * An error is returned when the value stored at key is not a set.
     * @see http://redis.io/commands/srem
     * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-srem
     * @param format
     * @param ... Ключь Значение (через пробел, в значении нет пробелов)
     * @return 
     */
    int SimpleRedisClient::srem_printf(const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);

        bzero(buf, bufer_size);
        int  rc = vsnprintf(buf, bufer_size, format, ap);
        va_end(ap);

        if( rc >= bufer_size )
        {
            return RC_ERR_BUFER_OVERFLOW;; // Не хватило буфера
        }

        if(rc <  0)
        {
            return RC_ERR_DATA_FORMAT;
        }

        return redis_send(RC_INT, "SREM %s\r\n", buf);
    }
