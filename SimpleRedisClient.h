/* 
 * File:   SimpleRedisClient.h
 * Author: victor
 *
 * Created on 10 Август 2013 г., 22:26
 */

#ifndef SIMPLEREDISCLIENT_H
#define	SIMPLEREDISCLIENT_H
 

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

#define RC_ERR -1
  
#define RC_ERR_CONECTION_CLOSE -2
#define RC_ERR_SEND -101
#define RC_ERR_TIMEOUT -102
#define CR_ERR_RECV -103

#define RC_ERR_PROTOCOL -104
 
class SimpleRedisClient
{

    int fd = 0;
    int yes = 1;
    int timeout = 1000;

    char* bufer;
    int bufer_size;

    int port = 6379;
    char* host = 0;
    
    int version_major = -1;
    int version_minor = -1;
    int version_patch = -1;
    
    
    char* data = 0;
    unsigned int data_size = 0;
    
    int debug = 0;
public:

    SimpleRedisClient();
    
    void setPort(int Port)
    {
        port = Port;
    }
    
    void setHost(const char* Host)
    {
        if(host != 0)
        {
            delete host;
        }
        
        host = new char[strlen(Host)];
        memcpy(host,Host,strlen(Host));
    }
    
    virtual ~SimpleRedisClient();

    int redis_conect();
    int redis_conect(const char* Host, int Port);
    int redis_conect(const char* Host,int Port, int TimeOut);
    
    /* 
      vector3 operator=(vector3 v);         // Ïåðåãðóæåííûé îïåðàòîð = 
      
      vector3 operator-=(vector3 v);         // Ïåðåãðóæåííûé îïåðàòîð -= 
      
      vector3 operator=(float v);         // Ïåðåãðóæåííûé îïåðàòîð = 
      
      vector3 operator-=(float v);         // Ïåðåãðóæåííûé îïåðàòîð - 
      bool    operator==(vector3 v);       // Ïåðåãðóæåííûé îïåðàòîð ==
     */
    
    /**
     * Ни ключь ни значение не должны содержать "\r\n"
     * @param key
     * @param val
     * @return
     */
    int set(const char *key, const char *val);
    
     
    /**
     * Вернёт значение ключа или 0 в случаии ошибки.
     * Пример:
     * 
     * char* val = rc["key"];
     * if(val != 0)
     * {
     *    printf("VAL:%s\n", val);
     * }
     *  
     * Выдаётся указатьель на внутриний буфер чтения/записи ссылка потеряет актуальность при вызове любой функции на получение или запись данных.
     * Или по указаному адресу будут уже другие данные либо мусор.
     */
    char* operator[] (const char *key);
    
    /**
     * Равносильно методу getData()
     * @return 
     */
    operator char* () const;
    
    /**
     * Равносильно методу getDataSize()
     * @return 
     */
    operator int () const;
    
    int setex(const char *key, const char *val, int seconds);
    

    int get(const char *key);

    int getset(const char *key, const char *set_val, char **get_val);


    int ping();

    int echo(const char *message, char **reply);

    int quit();

    int auth(const char *password);
 

    int setnx(const char *key, const char *val);
    

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

    int append(const char *key, const char *val);

    int substr( const char *key, int start, int end, char **substr);

    int exists( const char *key);

    int del( const char *key);

    int type( const char *key);

    int keys(const char *pattern, char ***keyv);

    int randomkey( char **key);

    int rename( const char *key, const char *new_key_name);

    int renamenx( const char *key, const char *new_key_name);

    int dbsize();

    int expire( const char *key, int secs);

    /**
     * Returns the remaining time to live of a key that has a timeout.
     * @param key
     * @return 
     */
    int ttl( const char *key);

    int getRedisVersion();
    
    /**
     * 
     * @param TimeOut
     */
    void setTimeout( int TimeOut);

    /**
     * Закрывает соединение
     */
    void redis_close();
    
    /** 
     * Выдаётся указатьель на внутриний буфер чтения/записи ссылка потеряет актуальность при вызове любой функции на получение или запись данных.
     * Или по указаному адресу будут уже другие данные либо мусор. 
     */
    char* getData() const;
    
    int getDataSize() const;
    
    
protected:
      
    int read_select(int fd, int timeout )  const;
    
    int wright_select(int fd, int timeout )  const;
    
    int redis_send(char recvtype, const char *format, ...);
  
    /**
     * Отправляет данные
     * @param buf
     * @return 
     */
    int send_data( const char *buf ) const;
 
};


#endif	/* SIMPLEREDISCLIENT_H */

