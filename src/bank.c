#include "../include/bank.h"

sig_atomic_t data_ready = 0;
sig_atomic_t exit_flag = 0;
sig_atomic_t server_pid = 0;

int main(){

    int account_amount = 0;
    int current_account = 0;
    int session_amount = 0;

    struct account *accounts = read_accounts(&account_amount);
    struct bank_server *bank_server = malloc(sizeof(struct bank_server));
    struct session *session = malloc(sizeof(struct session));

    sem_unlink(SEMKEYBANK);
    sem_unlink(SEMKEYSERVER);
    bank_server->sem_bank = semaphore_open(SEMKEYBANK, 0);
    bank_server->sem_server = semaphore_open(SEMKEYSERVER, 1);
    bank_server->server_pid = 0;

    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_sigaction = &sigusr1_handler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    if(sigaction(SIGUSR1, &sa, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_chld;
    bzero(&sa_chld, sizeof(sa_chld));
    sa_chld.sa_handler = &sigchld_handler;
    sa_chld.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa_chld, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_sigint;
    bzero(&sa_sigint, sizeof(sa_sigint));
    sa_sigint.sa_handler = &sigint_handler;
    if(sigaction(SIGINT, &sa_sigint, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    int shmid;
    pid_t server_proc = fork();
    if(server_proc < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(server_proc == 0){
        execl("./server", "./server", (char*)NULL);
    }
    else {
        while (exit_flag == 0) {
            if (data_ready == 0) {
                continue;
            } 
            else {
                if(session_amount == 0){
                    shmid = shared_mem_id(server_pid);
                    bank_server->shared_mem = shared_mem_ptr(shmid);
                    printf("Connection: %d Ptr:%p \n", server_pid, bank_server->shared_mem);
                }
                else{
                    //get session info
                    for(int i = 0; i < session_amount; i++){
                        if(session[i].connection_id == server_pid){
                            current_account = session[i].current_account;
                            break;
                        }
                    }
                    shmid = shared_mem_id(server_pid);
                    bank_server->shared_mem = shared_mem_ptr(shmid);
                    printf("Connection: %d Ptr:%p \n", server_pid, bank_server->shared_mem);
                }
                data_ready = 0;
                read_from_server(bank_server);
                char path[500] = {0};
                sscanf(bank_server->buffer, "%s", path);
                
                if(strstr(path, "/login/")){
                    printf("LOGIN\n");
                    int status = login(accounts, path, account_amount);
                    current_account = status;
                    push_session(session, current_account, server_pid, &session_amount);
                    if(status != 404){
                        char response[1024] = "200 OK/";
                        strcat(response, accounts[status].password);
                        strcpy(bank_server->buffer, response);
                        send_to_server(bank_server);
                    }
                    else{
                        strcpy(bank_server->buffer, "404 NOT FOUND");
                        send_to_server(bank_server);
                    }
                }
                else if(strstr(path, "/create/")){
                    printf("CREATE\n");
                    int status = create_acc(accounts, path, &account_amount);
                    current_account = status;
                    if(status == 201){
                        strcpy(bank_server->buffer, "201 CREATED");
                        send_to_server(bank_server);
                    }
                    else{
                        strcpy(bank_server->buffer, "404 NOT FOUND");
                        send_to_server(bank_server);
                    }
                }
                else if(strstr(path, "/deposit/")){
                    printf("DEPOSIT\n");
                    int status = deposit_money(accounts, path, current_account);
                    update_db(accounts, &account_amount);
                    if(status == 202){
                        strcpy(bank_server->buffer, "202 ACCEPTED");
                        send_to_server(bank_server);
                    }
                    else{
                        strcpy(bank_server->buffer, "404 NOT FOUND");
                        send_to_server(bank_server);
                    }
                }
                else if(strstr(path, "/withdraw/")){
                    printf("WITHDRAW\n");
                    int status = withdraw_money(accounts, path, current_account);
                    update_db(accounts, &account_amount);
                    if(status == 202){
                        strcpy(bank_server->buffer, "202 ACCEPTED");
                        send_to_server(bank_server);
                    }
                    else{
                        strcpy(bank_server->buffer, "400 BAD REQUEST");
                        send_to_server(bank_server);
                    }
                }
                else if(strstr(path, "/balance/")){
                    printf("BALANCE\n");
                    int status = fetch_balance(accounts, path, current_account);
                    if(status == 200){
                        char response[1024] = "200 OK/";
                        char balance[20];
                        snprintf(balance, sizeof(balance), "%ld", accounts[current_account].balance);
                        strcat(response, balance);
                        strcpy(bank_server->buffer, response);
                        send_to_server(bank_server);
                    }
                    else{
                        strcpy(bank_server->buffer, "404 NOT FOUND");
                        send_to_server(bank_server);
                    }
                }
                else if(strstr(path, "/exit/")){
                    printf("EXIT\n");
                    shmctl(shmid, IPC_RMID, NULL);
                    shmdt(bank_server->shared_mem);
                }
            }
        }
        printf("Exiting\n");
        sem_close(bank_server->sem_bank);
        sem_close(bank_server->sem_server);
        sem_destroy(bank_server->sem_bank);
        sem_destroy(bank_server->sem_server);
        free(bank_server);
        free(accounts);
        exit(EXIT_SUCCESS);
    }

    return 0;
}


int fetch_balance(struct account *accounts, char buffer[], int current_account){
    if(current_account >= 0){
        return 200;
    }
    else{
        return 404;
    }
}

int withdraw_money(struct account *accounts, char buffer[], int current_account){
    char amount[20];
    char *token = strtok(buffer, "/");
    token = strtok(NULL, "/");
    int64_t amount_to_withdraw = atoll(token);
    if(accounts[current_account].balance < amount_to_withdraw){
        return 400;
    }
    accounts[current_account].balance -= amount_to_withdraw;
    return 202;
}

int deposit_money(struct account *accounts, char buffer[], int current_account){
    char amount[20];
    char *token = strtok(buffer, "/");
    token = strtok(NULL, "/");
    int64_t amount_to_deposit = atoll(token);
    accounts[current_account].balance += amount_to_deposit;
    return 202;
}

int create_acc(struct account *accounts, char buffer[], int *accounts_amount){
    char nickname[20];
    char password[20];
    char *token = strtok(buffer, "/");
    token = strtok(NULL, "/");
    strcpy(nickname, token);
    token = strtok(NULL, "/");
    strcpy(password, token);
    accounts = push_account(accounts, nickname, password, 0, accounts_amount);
    update_db(accounts, accounts_amount);
    return 201;
}

int login(struct account *accounts, char path[], int accounts_amount){
    char nickname_temp[20];
    char password_temp[20];
    char *token = strtok(path, "/");
    token = strtok(NULL, "/");
    strcpy(nickname_temp, token);
    token = strtok(NULL, "/");
    strcpy(password_temp, token);
    int index = check_if_acc_exists(accounts, nickname_temp, accounts_amount);
    if(index >= 0){
        return index;
    }
    else{
        return 404;
    }
}

int check_if_acc_exists(struct account *accounts, char nickname[], int accounts_amount){
    int i;
    for (i = 0; i < accounts_amount; i++){
        if(strcmp(accounts[i].nickname, nickname) == 0){
            return i;
        }
    }
    return -1;
}

struct account *read_accounts(int *account_number){
    FILE *file;
    file = fopen(ACCOUNTS, "r");
    if(file == NULL){
        perror("Opening file");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    struct account *accounts = NULL;
    //read "database" into memory
    while(fgets(line, 1024, file) != NULL){
        char *nickname = strtok(line, ":");
        char *password = strtok(NULL, ":");
        char *balance = strtok(NULL, ":");
        accounts = push_account(accounts, nickname, password, atoll(balance), account_number);
    }
    fclose(file);
    return accounts;
}

struct session *push_session(struct session *sessions, int current_account, pid_t connection_id, int *session_amount){
    sessions = realloc(sessions, ((*session_amount) + 1) * sizeof(struct session));
    if(sessions == NULL){
        perror("Memory allocation");
        exit(EXIT_FAILURE);
    }
    sessions[(*session_amount)].current_account = current_account;
    sessions[(*session_amount)].connection_id = connection_id;
    (*session_amount)++;
    return sessions;
}

struct account *push_account(struct account *accounts, char nickname[], char password[], int balance, int *account_number){
    accounts = realloc(accounts, ((*account_number) + 1) * sizeof(struct account));
    if(accounts == NULL){
        perror("Memory allocation");
        exit(EXIT_FAILURE);
    }
    strcpy(accounts[*account_number].nickname, nickname);
    strcpy(accounts[*account_number].password, password);
    accounts[*account_number].balance = balance;
    (*account_number)++;
    return accounts;
}

void update_db(struct account *accounts, int *account_number){
    FILE *file;
    file = fopen(ACCOUNTS, "w");
    if(file == NULL){
        perror("Opening file");
        exit(EXIT_FAILURE);
    }
    int i;
    for(i = 0; i < *account_number; i++){
        fprintf(file, "%s:%s:%ld\n", accounts[i].nickname, accounts[i].password, accounts[i].balance);
    }
    fclose(file);
}   

void send_to_server(struct bank_server *bank_server){
    sem_wait(bank_server->sem_bank);
    strcpy(bank_server->shared_mem, bank_server->buffer);
    printf("SENDING TO SERVER: %s\n", bank_server->shared_mem);
    sem_post(bank_server->sem_server);
    sem_post(bank_server->sem_server);
    sigusr1_send(server_pid);
}

void read_from_server(struct bank_server *bank_server){
    sem_wait(bank_server->sem_bank);
    strcpy(bank_server->buffer, bank_server->shared_mem);
    printf("READING FROM SERVER: %s\n", bank_server->buffer);
}

semaphore *semaphore_open(char *name, int init_val){
    semaphore *sem = sem_open(name, O_CREAT, 0644, init_val);
    if(sem == SEM_FAILED){
        perror("sem_open_bank");
        exit(EXIT_FAILURE);
    }
    return sem;
}

int shared_mem_id(int key){
    int shmid = shmget(key, 1024, 0666|IPC_CREAT);
    if (shmid == -1){
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

char *shared_mem_ptr(int shmid){    
    char *str = (char*) shmat(shmid, NULL, 0);
    if (str == (char*)-1){
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return str;
}

void sigusr1_send(int pid){
    kill(pid, SIGUSR1);
}

void sigusr1_handler(int signum, siginfo_t *info, void *ucontext){
    data_ready = 1;
    server_pid = info->si_pid;
}
void sigint_handler(int signum){
    exit_flag = 1;
}
void sigchld_handler(int signum){
    while(waitpid(-1, NULL, WNOHANG) > 0); // figure how this works
}