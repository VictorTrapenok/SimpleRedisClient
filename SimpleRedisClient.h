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
  
#define CR_ERROR '-'
#define CR_INLINE '+'
#define CR_BULK '$'
#define CR_MULTIBULK '*'
#define CR_INT ':'
#define CR_ANY '?'
#define CR_NONE ' '

class SimpleRedisClient
{

    int fd = 0;
    int yes = 1;
    int timeout = 1000;

    char* bufer;
    int bufer_size;

    int port = 6379;
    char host[64];
public:

    SimpleRedisClient();
    virtual ~SimpleRedisClient();

    int redis_conect();
    
    /**
     * Ни ключь ни значение не должны содержать "\r\n"
     * @param key
     * @param val
     * @return
     */
    int set(const char *key, const char *val);

    
    int setex(const char *key, const char *val, int seconds);
    

    int get(const char *key, char **val);

    int getset(const char *key, const char *set_val, char **get_val);


    int ping();

    int echo(const char *message, char **reply);

    int quit();

    int auth(const char *password);
 

    int credis_setnx(const char *key, const char *val);
    

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

    int credis_append(const char *key, const char *val);

    int credis_substr( const char *key, int start, int end, char **substr);

    int credis_exists( const char *key);

    int credis_del( const char *key);

    int credis_type( const char *key);

    int credis_keys(REDIS rhnd, const char *pattern, char ***keyv);

    int credis_randomkey( char **key);

    int credis_rename( const char *key, const char *new_key_name);

    int credis_renamenx( const char *key, const char *new_key_name);

    int credis_dbsize();

    int credis_expire( const char *key, int secs);

    int credis_ttl( const char *key);

 
    
    /**
     * 
     * @param TimeOut
     */
    void setTimeout( int TimeOut);

    /**
     * Закрывает соединение
     */
    void redis_close();
    
protected:
    
    int read_select(int fd, int timeout );
    
    int wright_select(int fd, int timeout );
    
    int redis_send(char recvtype, const char *format, ...);

    int receive_reply( char recvtype );

 
   /**
    * Принимает данные
    * @param buf
    * @param size
    * @return 
    */
   int receive_data( char *buf, int size);

    /**
     * Отправляет данные
     * @param buf
     * @return 
     */
    int send_data( char *buf );
 
};


#endif	/* SIMPLEREDISCLIENT_H */

