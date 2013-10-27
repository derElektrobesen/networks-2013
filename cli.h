#ifndef CLI_H
#define CLI_H

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef DONT_DO_SRAND
#   include <time.h>
#endif

#include "proto.h"
#include "macro.h"
#include "types.h"

/* Defines */
#define SRV_UNKN                0   /**< Сервер недоступен                      */
#define SRV_READY               1   /**< Сервер готов к передаче                */
#define SRV_BUSY                2   /**< Сервер занят                           */

#define TRM_WAITING_SERVERS     0   /**< Ожидание серверов                      */
#define TRM_OBTAINING           1   /**< Получение файла                        */
#define TRM_COMPLETED           2   /**< Передача завершена                     */
#define TRM_UNKN                3   /**< Ошибка при передаче                    */

#define TRME_TOO_MANY_TRM      -1   /**< Слишком много передач                  */
#define TRME_DECODE_ERR        -2   /**< Ошибка при создании сообщения          */
#define TRME_SOCKET_FAILURE    -3   /**< Возникла ошибка на сокете              */
#define TRME_NO_ACTIVE_SRVS    -4   /**< Ни один сервер не смог получить запрос */
                                    /**< на передачу                            */
#define TRME_ALLOC_FAILURE     -5   /**< Ошибка при выделении памяти            */

/* Typedefs */

/**
 * Структура содержит часть полученных данных
 * Адрес первого элемента списка не должен быть изменен
 * до окончания передачи
 */
struct file_data_t {
    piece_id_t start_piece_id;      /**< Начальный индекс куска файла           */
    piece_id_t end_piece_id;        /**< Конечный индекс куска файла            */
    ssize_t piece_len;              /**< Текущий размер куска                   */
    ssize_t space_left;             /**< Сколько выделенного места еще осталось */
    unsigned char *data;            /**< Кусок данных                           */
    struct file_data_t *next_piece; /**< Следующий кусок данных                 */
};

/**
 * Структура описывает одно активное соединение
 */
struct active_connection {
    int srv_sock;
    int timeout;
    int transmission_id;
    int status;
    piece_id_t piece_id;
    file_id_t file_id;                 /* TODO */
    pack_id_t pack_id;
    struct file_data_t *data;
    struct active_connection *next;
    struct active_connection *prev;
};

/**
 * Список всех активных соединений
 */
struct active_connections {
    struct active_connection *q_head;
    struct active_connection *q_tail;
};

/* Часть файла которую необходимо запросить */
struct pieces_queue {
    piece_id_t max_piece_num;                      /**< Максимальный номер куска                       */
    piece_id_t cur_piece;                          /**< Содержит минимальный незапрошенный номер куска */
    piece_id_t max_failed_piece_num;               /**< Максимальный номер куска в failed_pieces       */
    piece_id_t failed_pieces[MAX_PIECES_COUNT];    /**< Содержит неполученные куски файла              */
};

/**
 * Структура определяет одну активную передачу
 */
struct transmission {
    char filename[FILE_NAME_MAX_LEN];
    unsigned char filesum[MD5_DIGEST_LENGTH];
    unsigned short status;
    struct pieces_queue pieces;

    /* TODO: Добавить список активных для данной передачи серверов */
};

/**
 * Структура описывает список всех активных передач
 */
struct transmissions {
    struct transmission trm[MAX_TRANSMISSIONS];
    int count;
};


/* Functions prototypes */
/* Обрабатывает сообщение полученное от сервера */
int process_srv_message(int sock, const char *msg, ssize_t len);
/* Основной диспетчер */
void main_dispatcher();
/* Запускает посылку файла */
int recieve_file(const char *filename, const unsigned char *hsum,
        unsigned long fsize, const struct sockets_queue *q);

#endif
