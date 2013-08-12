/* 
 * File:   SimpleRedisClient.h
 * Author: victor
 *
 * Created on 10 Август 2013 г., 22:26
 */

#ifndef SIMPLEREDISCLIENT_H
#define	SIMPLEREDISCLIENT_H
 

#include <list>

#define RC_ERR -1
  
#define RC_ERR_CONECTION_CLOSE -2
#define RC_ERR_SEND -101
#define RC_ERR_TIMEOUT -102
#define CR_ERR_RECV -103

#define RC_ERR_PROTOCOL -104
#define RC_ERR_BUFER_OVERFLOW -105
#define RC_ERR_DATA_FORMAT -106
 
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
    
    void setPort(int Port);
    
    void setHost(const char* Host);
    
    virtual ~SimpleRedisClient();

    int redis_conect();
    int redis_conect(const char* Host, int Port);
    int redis_conect(const char* Host,int Port, int TimeOut);
    
    /* 
      vector3 operator=(vector3 v);
      vector3 operator-=(float v); 
      bool    operator==(vector3 v);
     */
    
    
    /**
     * Ни ключь ни значение не должны содержать "\r\n"
     * @param key
     * @param val
     * @return
     */
    int set(const char *key, const char *val);
    
    /**
     * Операция set только с форматированием строки параметров.
     * Ключ от значения отделяется пробелом.
     * @param format
     * @param ... ключь пробел значение
     * @return 
     */
    int set_printf(const char *format, ...);
    
    /**
     * 
     * @param key Ключ
     * @param format
     * @param ... значение
     * @return 
     */
    int set_printf(const char *key, const char *format, ...);
    
    /**
     * Выполняет установку значения в редис
     * Ключ от значения отделяется пробелом.
     * @code rc = "MyKey MyValue";
     * 
     * 
     * @param key_val
     * @return 
     */
    SimpleRedisClient& operator=(const char *key_val);
     
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
    int setex_printf(int seconds, const char *key, const char *format, ...);
    
    /**
     * Формат должен иметь вид: key seconds data
     * Где 
     * key строковый ключ
     * seconds %d число секунд
     * data строковые данные
     * 
     * @param format
     * @param ...
     * @return 
     */
    int setex_printf(const char *format, ...);

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
    
    int delta( int delta, const char *key);
    
    /**
     * Инкриментирует значение ключа key
     * @param key ключь в редисе
     * @return 
     */
    int operator +=( const char *key);
    
    /**
     * Декриментирует значение ключа key
     * @param key ключь в редисе
     * @return 
     */ 
    int operator -=( const char *key); 

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
    
    void setBuferSize(int size);
    int getBuferSize();
    
    /**
     * Вернёт true если соединение установлено 
     */
    operator bool () const;
    
    
    /**
     *  rc == true  истино если соединение установлено
     *  rc == false  истино если соединение не установлено 
     */
    int operator == (bool); 
    
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



class RedisClientConnectionPool
{ 
    
    std::list <SimpleRedisClient*> * pool;
    pthread_mutex_t* request_mutex;
    
    int pool_index_size = 0; 
public:
    RedisClientConnectionPool()
    {
        setPoolIndexSize(100);
    }
    
    /**
     * Чем больше Pool_index_size тем меньше вероятность того что один поток будет ожидать завершения выполнения другого потока в этой секции.
     * @param Pool_index_size
     */
    RedisClientConnectionPool(int Pool_index_size)
    {
         setPoolIndexSize(Pool_index_size);
    }
    
    /**
     * Операция убъёт все откытые соединения с редисом и установит pool_index_size в новое значение 
     * @param Pool_index_size значение не должно быть меньше единицы
     */
    void setPoolIndexSize(int Pool_index_size)
    {
        if(Pool_index_size < 1)
        {
            Pool_index_size = 1;
        }
        
        if(pool != 0)
        {
             for(int i=0; i< pool_index_size; i++)
             {
                 pool[i].clear();
             }
             delete pool;
             delete request_mutex;
        }
        
        pool_index_size = Pool_index_size;
        pool = new std::list <SimpleRedisClient*>[pool_index_size];
        request_mutex = new pthread_mutex_t[pool_index_size];
        
        for(int i=0; i<pool_index_size+1; i++ )
        {
            pthread_mutex_init(&request_mutex[i],NULL);
        }
    }
    
    ~RedisClientConnectionPool()
    {
        if(pool != 0)
        {
            for(int i=0; i< pool_index_size; i++)
            {
                 pool[i].clear();
            }
            delete pool;
            delete request_mutex;
        }
    }
     
    
    SimpleRedisClient& grab(int random_id)
    {
         int id = random_id%pool_index_size;
         pthread_mutex_lock(&request_mutex[id]);
         
         SimpleRedisClient* rc = *pool[id].begin();
         pool[id].erase(pool[id].begin());
         
         pthread_mutex_unlock(&request_mutex[id]);
         
         return *rc;
    }
    
    void release(SimpleRedisClient& rc, int random_id)
    { 
         int id = random_id%pool_index_size;
         pthread_mutex_lock(&request_mutex[id]);
         
         pool[id].push_front( &rc);
         
         pthread_mutex_unlock(&request_mutex[id]);
    }
    
    
    
};

#endif	/* SIMPLEREDISCLIENT_H */

