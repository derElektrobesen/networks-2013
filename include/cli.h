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
#include "network.h"
#include "json.h"

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
#define TRME_FILE_ERROR        -6   /**< Ошибка при работе с файлом             */
#define TRME_OUT_OF_MEMORY     -7   /**< Не хватает памяти для всего файла      */
#define TRME_OPTS_FAILURE      -8   /**< Ошибка при передаче аргуметов          */

/* Typedefs */

/**
 * Структура описывает кусок, который выбивается из структуры ниже
 */
struct file_udata_t {
    piece_id_t piece_id;            /**< Индекс куска файла                     */
    piece_len_t piece_len;          /**< Текущий размер куска                   */
    unsigned char data[DATA_BLOCK_LEN]; /**< Кусок данных                       */
};

/**
 * Структура описывает блок данных для сброса.
 */
struct file_data_t {
    piece_id_t s_piece;
    piece_id_t f_piece;
    int pieces_copied;
    size_t full_size;
    unsigned char data[CACHED_PIECES_COUNT * DATA_BLOCK_LEN];
};

/**
 * Структура объеденяет структуры выше
 * Чтобы не передавать лишние параметры
 */
struct file_full_data_t {
    struct file_data_t data;
    struct file_udata_t udata[CACHED_QUEUE_LEN];
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
    file_id_t file_id;
    pack_id_t pack_id;
    struct file_full_data_t *data;
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
    piece_id_t max_piece_num;                       /**< Максимальный номер куска                       */
    piece_id_t cur_piece;                           /**< Содержит минимальный незапрошенный номер куска */
    signed int max_failed_piece_num;                /**< Максимальный номер куска в failed_pieces       */
    piece_id_t failed_pieces[MAX_PIECES_COUNT];     /**< Содержит неполученные куски файла              */
    piece_id_t flushed_pieces_count;                /**< Число записанных на диск кусков                */
};

/**
 * Структура определяет одну активную передачу
 */
struct transmission {
    FILE *file;
    char filename[FILE_NAME_MAX_LEN];
    unsigned char filesum[MD5_DIGEST_LENGTH];
    unsigned short status;
    size_t filesize;
    struct pieces_queue pieces;

    /* TODO: Добавить список активных для данной передачи серверов */
};

/**
 * Структура описывает список всех активных передач
 */
struct transmissions {
    struct transmission trm[MAX_TRANSMISSIONS];
    unsigned char openned_trms[MAX_TRANSMISSIONS];  /**< 1 && 0                                         */
    unsigned int count;                             /**< Число закрытых передач                         */
};


/* Functions prototypes */
/* Обрабатывает сообщение полученное от сервера */
int process_srv_message(int sock, const char *msg, size_t len);
/* Основной диспетчер */
void main_dispatcher();
/* Запускает посылку файла */
int receive_file(const char *filename, const unsigned char *hsum,
        unsigned long fsize, const struct sockets_queue *q);
void setup_gui_acts(struct gui_actions *acts);

#endif
