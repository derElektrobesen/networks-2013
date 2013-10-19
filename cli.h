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
#define SRV_UNKN                0   /**< Сервер недоступен      */
#define SRV_READY               1   /**< Сервер готов к передаче*/
#define SRV_BUSY                2   /**< Сервер занят           */

#define TRM_WAITING_SERVERS     0   /**< Ожидание серверов      */
#define TRM_OBTAINING           1   /**< Получение файла        */
#define TRM_COMPLETED           2   /**< Передача завуершена    */
#define TRM_UNKN                3   /**< Ошибка при передаче    */

#define TRME_TOO_MANY_TRM      -1   /**< Слишком много передач  */
#define TRME_DECODE_ERR        -2   /**< Ошибка при создании    */
                                    /**< сообщения              */
#define TRME_SOCKET_FAILURE    -3   /**< Возникла ошибка на     */
                                    /**< сокете                 */
#define TRME_NO_ACTIVE_SRVS    -4   /**< Ни один сервер не смог */
                                    /**< получить запрос на     */
                                    /**< передачу               */

/* Typedefs */

/**
 * Представляет собой элемент списка, хранящего
 * все ожидающие тикета события
 */
struct active_query {
    int srv_sock;
    int timeout;
    int transmission_id;
    int status;
    struct active_query *next;
};

/**
 * Список всех событий ожидающих тикета
 */
struct active_queries {
    struct active_query *q_head;
    struct active_query *q_tail;
};

/* Часть файла которую необходимо запросить */
struct pieces_queue {
    int max_piece_num;      /**< Максимальный номер куска */
    int cur_max_piece_num;  /**< Максимальный номер куска в pieces*/
    int cur_elem;           /**< Минимальный индекс в pieces*/
    int pieces[MAX_PIECES_COUNT];
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
void main_dispatcher(const struct sockets_queue *q);
/* Запускает посылку файла */
int recieve_file(const char *filename, const unsigned char *hsum,
        const struct sockets_queue *q);

#endif
