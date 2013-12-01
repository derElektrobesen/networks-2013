#include "../include/srv.h"

static struct files_queue f_descr = {
    .count = 0,
    .positions = { [0 ... (MAX_TRANSMISSIONS - 1)] = 0 },
};
static const struct gui_actions *g_acts;

static void _get_cache(int id, const unsigned char *data,
        unsigned long data_len) {
    struct file_cache *c = f_descr.cache + id;
    size_t start, len, nlen, count;

    start = DATA_BLOCK_LEN * c->end_piece;
    len = st_arr_len(c->data);
    count = CACHED_PIECES_COUNT;
    log(SERVER, "getting cache: start: %lu, len: %lu, count: %lu", 
            start, len, count);
    if (!data) {
        fseek(c->file, start, SEEK_SET);
        nlen = fread(c->data, sizeof(*(c->data)), len, c->file);
        if (nlen != len) {
            count = nlen / DATA_BLOCK_LEN;
            if (count * DATA_BLOCK_LEN != nlen)
                count++;
        }
    } else {
        if (data_len > len)
            data_len = len;
        memcpy(c->data, data, data_len);
        if (!(count = data_len / DATA_BLOCK_LEN))
            count++;
    }
    c->start_piece = c->end_piece;
    c->end_piece += count;
}
inline static void get_cache(int id) {
    _get_cache(id, NULL, 0);
}

/**
 * Ф-ия рассчитывает хэш-сумму файла и сравнивает ее с пришедшей
 * в структуре. В случае, если суммы совпадают, возвращает 0
 */
static int cmp_file_hash(const struct cli_fields *f, int id) {
    MD5_CTX md5;
    FILE *file = f_descr.cache[id].file;
    unsigned char data[BUF_MAX_LEN];
    unsigned char digest[MD5_DIGEST_LENGTH];
    int i = 0;
    unsigned long count;

    MD5_Init(&md5);
    while ((count = fread(data, sizeof(*data), st_arr_len(data), file))) {
        if (i++ == 0) {
            f_descr.cache[id].end_piece = 0;
            _get_cache(id, data, count);
        }
        MD5_Update(&md5, data, count);
    }
    MD5_Final(digest, &md5);

    return bcmp(digest, f->hsumm, st_arr_len(digest));
}

/**
 * Ф-ия ищет среди раздач подходящую для fname
 * В случае успеха возвращает 1, иначе 0
 */
static int search_file(const char *fname, char *full_name,
        int full_name_max_len) {
    /* TODO */
    int r = 0;
    if (full_name)
        snprintf(full_name, full_name_max_len, "%s/%s", HOME_DIR_PATH, fname);
    log(SERVER, "full file name: %s", full_name);
    if (access(full_name, F_OK) != 0)
        r = set_bit(r, PE_FILE_NOT_EXISTS);
    else if (access(full_name, R_OK) != 0)
        r = set_bit(r, PE_READ_ACCESS_DENIED);
    return r;
}

/**
 * Ф-ия добавляет в список передаваемых файлов
 * новый элемент
 * Возвращает номер передачи (индекс в массиве names)
 */
static file_id_t add_transmission(struct cli_fields *f) {
    int i, flag = 0, stat;
    file_id_t r = -1;
    char buf[255];
    char filename[FULL_FILE_NAME_MAX_LEN];
    for (i = 0; (r == -1) && (i < f_descr.count); i++) {
        if (f_descr.positions[i] == 0)
            r = i;
    }
    if (r < 0) {
        r = f_descr.count;
        flag = 1;
    }
    f->error = 0;
    if (!(stat = search_file(f->file_name, filename, st_arr_len(filename)))) {
        strncpy(f_descr.cache[r].name, f->file_name, st_arr_len(f->file_name));
        f_descr.cache[r].file = fopen(filename, "rb");
        if (!f_descr.cache[r].file) {
            r = -1;
            err_n(SERVER, "%s: fopen failure", f->file_name);
            f->error = set_bit(f->error, PE_FOPEN_FAILURE);
        } else {
            i = cmp_file_hash(f, r);
            if (i) {
                log(SERVER, "File hash comparing failure");
                f->error = set_bit(f->error, PE_HASH_CMP_FAILURE);
                r = -1;
            } else {
                f_descr.positions[r] = 1;
                if (flag)
                    f_descr.count++;
                f->file_id = r;
            }
        }
    } else {
        r = -1;
        f->error |= stat;
        decode_proto_error(stat, buf, st_arr_len(buf));
        log(SERVER, "%s: %s", f->file_name, buf);
    }
    return r;
}

static void read_file_piece(struct srv_fields *f) {
    int piece_id = f->cli_field.piece_id,
        file_id = f->cli_field.file_id;
    struct file_cache *c = &(f_descr.cache[file_id]);
    if (c->start_piece > piece_id || c->end_piece <= piece_id) {
        if (c->start_piece > piece_id)
            c->end_piece = piece_id;
        get_cache(file_id);
    }
    memcpy(f->piece, c->data + 
        (piece_id - c->start_piece) * DATA_BLOCK_LEN, DATA_BLOCK_LEN);
    f->piece_len = DATA_BLOCK_LEN;
}

/**
 * Ф-ия вызывается при завершении передачи
 */
static void remove_transmission(struct cli_fields *f) {
    int i;

    fclose(f_descr.cache[f->file_id].file);
    f_descr.positions[f->file_id] = 0;

    for (i = f_descr.count; i >= 0; i--) {
        if (f_descr.positions[i] == 0)
            f_descr.count--;
        else
            i = -1;
    }
}

static void process_cli_msg(struct srv_fields *f) {
    struct cli_fields *cf = &(f->cli_field);
    file_id_t fid = cf->file_id;

    if (get_bit(cf->error, PE_TRNMS_CMPL))
        remove_transmission(cf);
    else {
        if (fid == -1) {
            fid = add_transmission(cf);
            if (fid == -1) {
                log(SERVER, "Transmission adding failure");
                return;
            }
            cf->file_id = fid;
        }
        read_file_piece(f);
    }
}

static void send_answer(const struct srv_fields *f, int sock) {
    size_t msg_len;
    char msg[BUF_MAX_LEN];

    char buf[255];
    char *g_opts[] = {"id"};
    char *g_vals[] = {buf};
    int count = 1;

    msg_len = decode_srv_msg(f, msg);
    log_srv_fields(f);
    send_data(sock, msg, msg_len, 0);

    g_acts->package_sent(g_opts, g_vals, count);
}

int process_client_message(int sender_sock, const char *msg, size_t count) {
    struct srv_fields f;
    int r;

    r = encode_cli_msg(&(f.cli_field), msg, count);
    log_cli_fields(&(f.cli_field));
    if (r)
        err(SERVER, "Encoding cli message failure");
    else {
        process_cli_msg(&f);
        send_answer(&f, sender_sock);
    }
    return r;
}

static void on_terminate_gui_act(char **opts_names,
        char **opts_vals, unsigned int count, const struct sockets_queue *q) {
    char *r_opts_names[] = {"result"};
    char *r_opts_vals[] = {"0"};
    int i;

    for (i = 0; i < q->count; i++)
        close(q->sockets[i]);

    g_acts->answer(r_opts_names, r_opts_vals, 1);

    exit(0);    /* terminate */
}

void setup_gui_acts(struct gui_actions *acts) {
    g_acts = acts;
    acts->terminate = &on_terminate_gui_act;
}
