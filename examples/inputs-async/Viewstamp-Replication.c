#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1

enum NormalOperations {Prepare_ROUND, PrepareOk_ROUND};

enum ViewChange {DoViewChange_ROUND, StartView_ROUND};

enum Recovery {Recovery_ROUND, RecoveryResponse_ROUND};

enum Status {normal, view_change, recovering};

enum Labels {Prepare, PrepareOk, DoViewChange, StartView, Recovery,
                RecoveryResponse};

typedef struct {
    int view_nr, op_number, commited;

    /* This message includes client_id and client req nr */
    char *message;
} log_str;

typedef struct {
    /* Consider piggy backed in message */
    char *message;
    int request_nr, view_nr;
    int replica_id;
    enum Labels label;
    /* Data for piggy-backing */
    int commiting;
    int op_number;
} msg_NormalOp;

typedef struct {
    int view_nr, replica_id, log_size;
    enum Labels label;
    log_str **log;
} msg_ViewChange;

typedef struct {
    int op_number, commited;
    char *message;
} msg_Recovery;

typedef struct lista {
    int size;
    msg_NormalOp *info;
    struct lista *next;
} listA;

typedef struct listb {
    int size;
    msg_ViewChange *info;
    struct listb *next;
} listB;

typedef struct listc {
    int size;
    msg_Recovery *info;
    struct listb *next;
} listC;

/* size - actual length; current - max buffer length */
typedef struct {
    int size, current;

    log_str **array;
} arraylist;

typedef struct comm {
    int replica_id, op_number, view_nr;
    struct comm *next;
} commit_list;

void abort() {
    exit(1);
}

int get_primary(int view_nr, int n) {
    return (view_nr % n);
}

char *in() {
    char *m = malloc(sizeof(char));
    return m;
}

char *prepare_ping() {
    char *m = malloc(5 * sizeof(char));

    if (!m) {
        abort();
    }

    strcpy(m, "ping");

    return m;
}

void send(void *mess, int n) {

}

/* Return from algorithm function */
void out_internal() {

}

void out_external(char *mess) {

}

log_str *create_log_entry(int view_nr, int op_number, char *message) {
    log_str *entry = malloc(sizeof(log_str));
    if (!entry) {
        abort();
    }

    entry->view_nr = view_nr;
    entry->op_number = op_number;

    entry->message = malloc(strlen(message) * sizeof(char));
    if (!entry->message) {
        free(entry);
        abort();
    }

    strcpy(entry->message, message);

    return entry;
}

arraylist *init_log() {
    arraylist *log;
    log = malloc(sizeof(arraylist));
    if (!log) {
        abort();
    }

    log->size = 0;

    log->array = malloc(2 * sizeof(log_str));
    if (!log->array) {
        abort();
    }
    log->current = 2;
    return log;
}

void dispose_log(arraylist **log) {
    arraylist *aux_log = *log;

    int i;
    for (i = 0; i < aux_log->size; i++) {
        if (aux_log->array[i]) {
            if (aux_log->array[i]->message) {
                free(aux_log->array[i]->message);
            }

            free(aux_log->array[i]);
        }
    }

    free(log);
}

/* Adds an entry to a log, and resizes log if necessary */
void add_entry_log(log_str *entry, arraylist *log) {
    log_str **aux;
    if (log->size == log->current) {
        log->current *= 2;

        aux = realloc(log->array, log->current * sizeof(log_str*));
        if (!aux) {
            if (log) {
                dispose_log(&log);
            }
            abort();
        }
        log->array = aux;
    }

    log->array[log->size] = entry;
    log->size++;
}

int timeout() { return 0; }

void reset_timeout() {}

void *recv() {
    return NULL;
}

msg_NormalOp *check_maj(listA *mbox, int op_number) {
    listA *it = mbox;
    msg_NormalOp *res;
    int nr = 0;
    while (it) {
        if (it->info->request_nr == op_number) {
            nr++;
            res = it->info;
        }
        it = it->next;
    }

    if (nr == mbox->size)
        return res;

    return NULL;
}

void update_commit(arraylist *log, int op_number) {
    int i;
    for (i = 0; i < log->size; i++)
        if (log->array[i]->op_number == op_number) {
            log->array[i]->commited = 1;
            break;
        }
}

int nr_commited(int size, log_str **array) {
    int nr = 0, i;
    for (i = 0; i < size; i++) {
        if (array[i]->commited == 1)
            nr++;
    }

    return nr;
}

int main(int argc, char **argv)
{
    /* Check for replica_id, IP and total nr of nodes as command line args */
    if (argc < 4) {
        printf("Not enough arguments!\n");
        return 0;
    }

    enum NormalOperations round;
    enum ViewChange bround;
    enum Recovery cround;

    msg_NormalOp *msgA = NULL;
    msg_ViewChange *msgB = NULL;
    msg_Recovery *msgC = NULL;
    listA *mboxA = NULL;
    listB *mboxB = NULL;

    commit_list *recovery_buffer = NULL;

    arraylist *log = init_log();

    char *client_req = NULL;

    /* view_nr - current view, op_number - most recent received request */
    int view_nr = -1, op_number = 0;

    /* pid it is actually replica id */
    int pid = atoi(argv[1]);

    /* Nr of nodes */
    int n = atoi(argv[3]);

    int to_all = n+1;

    /* Start ViewChange algorithm for primary election */
    enum Status status = view_change;
    while (true) {
        /* Used by primary to get the log with max size */
        log_str **mess_log;
        int Max = -1;

        bround = DoViewChange_ROUND;
        view_nr++;

        if (pid == get_primary(view_nr, n)) {
            mboxB = NULL;
            while (true) {
                msgB = (msg_ViewChange*)recv();

                if (msgB != NULL && msgB->view_nr == view_nr && msgB->label == DoViewChange) {
                    listB* mboxB_new = malloc(sizeof(listB));
                    if (!mboxB_new) {
                        abort();
                    }

                    mboxB_new->info = msgB;
                    if (mboxB) {
                        mboxB_new->size = mboxB->size + 1;
                    } else {
                        mboxB_new->size = 1;
                    }

                    mboxB_new->next = mboxB;
                    mboxB = mboxB_new;
                    /* Get log with most commited operations*/
                    if (Max < nr_commited(msgB->log_size, msgB->log)) {
                        Max = nr_commited(msgB->log_size, msgB->log);
                        mess_log = msgB->log;
                    }
                }

                if (timeout()) {
                    break;
                }

                if (mboxB != NULL && mboxB->size >= n / 2) {
                    log->array = mess_log;
                    log->size = log->current = Max;
                    break;
                }
            }

            if (mboxB != NULL && mboxB->size >= n / 2) {

                bround = StartView_ROUND;

                msgB = malloc(sizeof(msg_ViewChange));
                if (!msgB)
                    abort();

                msgB->view_nr = view_nr;
                msgB->label = StartView;
                msgB->log_size = log->size;
                msgB->log = log->array;

                send((void*)msgB, to_all);

                /* Start Normal Op algo */
                while (true) {
                    round = Prepare_ROUND;
                    recovery_buffer = NULL;
                    msgA = malloc(sizeof(msg_NormalOp));
                    if (!msgA) {
                        abort();
                    }

                    /* Check if a request has been received from client */
                    client_req = in();

                    if (!client_req) {
                        msgA->message = prepare_ping();
                    } else {
                        msgA->message = malloc(strlen(client_req) * sizeof(char));
                        if (!msgA->message) {
                            abort();
                        }

                        strcpy(msgA->message, client_req);
                    }

                    msgA->commiting = 0;
                    if (log->size > 0 && log->array[log->size - 1]->commited == 1) {
                        msgA->commiting = 1;
                        msgA->op_number = log->array[log->size - 1]->op_number;
                    }

                    msgA->view_nr = view_nr;
                    msgA->request_nr = op_number;
                    msgA->replica_id = -1;
                    msgA->label = Prepare;

                    if (client_req) {

                        add_entry_log(create_log_entry(view_nr, op_number, client_req),
                                        log);
                        op_number++;
                    }


                    send((void*)msgA, to_all);

                    free(msgA->message);
                    msgA->message = NULL;

                    free(msgA);
                    msgA = NULL;

                    round = PrepareOk_ROUND;

                    while (true) {
                        msgA = (msg_NormalOp*) recv();

                        if (msgA != NULL && msgA->view_nr == view_nr && msgA->label == PrepareOk
                            && msgA->request_nr == op_number) {
                            listA* mboxA_new = malloc(sizeof(listA));
                            if (!mboxA_new) {
                                abort();
                            }

                            mboxA_new->info = msgA;
                            if (mboxA) {
                                mboxA_new->size = mboxA->size + 1;
                            } else {
                                mboxA_new->size = 1;
                            }

                            mboxA_new->next = mboxA;
                            mboxA = mboxA_new;

                            /* Update the commit list */
                            commit_list *it = recovery_buffer;
                            while (it) {
                                if (it->replica_id == msgA->replica_id) {
                                    it->view_nr = msgA->view_nr;
                                    it->op_number = msgA->request_nr;
                                    break;
                                }
                                it = it->next;
                            }

                            if (!it) {
                                it = malloc(sizeof(commit_list));
                                if (!it) {
                                    abort();
                                }

                                it->replica_id = msgA->replica_id;
                                it->view_nr = msgA->view_nr;
                                it->op_number = msgA->request_nr;
                                it->next = recovery_buffer;
                                recovery_buffer = it;
                            }
                        } else if (msgA != NULL && msgA->view_nr == view_nr && msgA->label == PrepareOk
                            && msgA->request_nr < op_number) {

                            commit_list *it = recovery_buffer;
                            while (it) {
                                if (it->replica_id == msgA->replica_id) {
                                    break;
                                }
                                it = it->next;
                            }

                            /* Late receiver indeed */
                            if (it && it->op_number <= msgA->request_nr) {
                                int index, i;
                                if (!it) {
                                    index = 0;
                                } else {
                                    index = it->op_number + 1;
                                }

                                i = index;
                                while (i < log->size) {
                                    msgC = malloc(sizeof(msg_Recovery));
                                    if (!msgC) {
                                        abort();
                                    }

                                    msgC->op_number = log->array[index]->op_number;
                                    msgC->commited = log->array[index]->commited;
                                    strcpy(msgC->message, log->array[index]->message);

                                    send((void*) msgC, msgA->replica_id);
                                    i++;
                                }
                            }

                        }

                        if (timeout()) {
                            out_internal();
                        }

                        /* Check if I have a majority */
                        if (mboxA != NULL && mboxA->size >= n / 2) {
                            break;
                        }
                    }

                    if (mboxA != NULL && mboxA->size >= n / 2) {
                        log->array[log->size - 1]->commited = 1;
                    }

                    round = Prepare_ROUND;
                }
            }

        } else {
            msgB = malloc(sizeof(msg_ViewChange));
            if (!msgB) {
                abort();
            }

            msgB->view_nr = view_nr;
            msgB->replica_id = pid;
            msgB->label = StartView;
            msgB->log_size = log->size;
            msgB->log = log->array;

            send(msgB, get_primary(view_nr, pid));

            bround = StartView_ROUND;
            while (true) {
                msgB = (msg_ViewChange*) recv();

                if (msgB != NULL && msgB->view_nr == view_nr && msgB->label == StartView) {
                    listB* mboxB_new = malloc(sizeof(listB));
                    if (!mboxB_new) {
                        abort();
                    }

                    mboxB_new->info = msgB;
                    if (mboxB) {
                        mboxB_new->size = mboxB->size + 1;
                    } else {
                        mboxB_new->size = 1;
                    }

                    mboxB_new->next = mboxB;
                    mboxB = mboxB_new;
                }

                if (timeout()) {
                    break;
                }

                if (mboxB != NULL && mboxB->size == 1 && mboxB->next == NULL) {
                    log->size = mboxB->info->log_size;
                    log->array = mboxB->info->log;
                    break;
                }
            }

            if (mboxB != NULL && mboxB->size == 1 && mboxB->next == NULL) {
                /* Launch Normal Op algo */
                while (true) {
                    round = Prepare_ROUND;

                    while (true) {
                        msgA = (msg_NormalOp*)recv();

                        if (msgA != NULL && msgA->view_nr == view_nr &&
                            msgA->label == Prepare && msgA->request_nr == op_number &&
                            strcmp(msgA->message, "ping") != 0) {
                            listA *new_mboxA = malloc(sizeof(msg_NormalOp));

                            if (new_mboxA == NULL) {
                                abort();
                            }

                            new_mboxA->next = NULL;
                            if (mboxA) {
                                new_mboxA->size = mboxA->size + 1;
                            } else {
                                new_mboxA->size = 1;
                            }
                            new_mboxA->info = msgA;
                            new_mboxA->next = mboxA;
                            mboxA = new_mboxA;

                            if (msgA->commiting == 1) {
                                update_commit(log, msgA->op_number);
                            }

                        } else if (msgA != NULL && msgA->view_nr == view_nr &&
                                msgA->label == Prepare && msgA->request_nr == op_number &&
                                strcmp(msgA->message, "ping") == 0) {
                            reset_timeout();

                            if (msgA->commiting == 1)
                                update_commit(log, msgA->op_number);

                            free(msgA->message);
                            free(msgA);

                        } else if (msgA != NULL && msgA->view_nr == view_nr &&
                                msgA->label == Prepare && msgA->request_nr > op_number) {

                            /* Inner receive for recovery */
                            while (true) {
                                msgC = (msg_Recovery *)recv();

                                if (msgC) {
                                    add_entry_log(create_log_entry(msgA->view_nr,
                                            msgC->op_number,
                                            msgC->message), log);
                                    log->array[log->size - 1]->commited = msgC->commited;
                                }

                                if (timeout()) {
                                    break;
                                }
                            }
                        }

                        if (timeout()) {
                            out_internal();
                        }

                        if (mboxA != NULL && mboxA->size == 1 && mboxA->next == NULL) {
                            add_entry_log(create_log_entry(mboxA->info->view_nr,
                                        mboxA->info->request_nr,
                                        mboxA->info->message), log);
                            op_number++;
                            break;
                        }
                    }

                    if (mboxA != NULL && mboxA->size == 1 && mboxA->next == NULL) {
                        round = PrepareOk_ROUND;

                        msgA = malloc(sizeof(msg_NormalOp));

                        msgA->label = PrepareOk;
                        msgA->view_nr = view_nr;
                        msgA->replica_id = pid;

                        msgA->request_nr = mboxA->info->request_nr;

                        send(msgA, get_primary(view_nr, n));
                    }

                    round = Prepare_ROUND;
                }
            }
        }

        bround = DoViewChange_ROUND;
    }

    if (client_req) {
        free(client_req);
    }

    if (log) {
        dispose_log(&log);
    }

    if (msgA) {
        if (msgA->message) {
            free(msgA->message);
        }
        free(msgA);
    }

    if (msgB) {
        free(msgB);
    }
    return 0;
}
