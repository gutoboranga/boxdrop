#ifndef DROPBOX_SERVER_H_
#define DROPBOX_SERVER_H_

#include <list.h>
#include <process.h>

#define MISSING_PARAMETER_FOR_BACKUP_PROCESS "Processos de backup devem informar o ip do processo primário como terceiro parâmetro.\nExemplo de uso:\n\n\t$./dropboxServer backup 127.0.0.1 5000\n\n"
#define MISSING_PARAMETER "Parâmetro faltando.\nUso correto:\n\n\t$./dropboxServer primary\n\tou\n\t$./dropboxServer backup [ip do processo primário] [porta]\n\n"
#define INVALID_PARAMETER "Parâmetro não reconhecido.\nValores possíveis:\n\n\tprimary\n\tbackup\n\n"

#define BACKUP_CONNECTION_SUCCEDED "> Conexão com processo backup feita com sucesso!\nPid: %d\n\n"
#define PRIMARY_CONNECTION_SUCCEDED "> Conexão com processo primário feita com sucessso!\n\n"
#define BACKUP_ERROR_CONNECTING_WITH_PRIMARY "> Erro ao conectar com o processo primário.\nAbortando execução."
#define BACKUP_HAS_DETECTED_PRIMARY_FAILURE ">>>>>>>> PROCESSO PRIMÁRIO FALHOU <<<<<<<<\n\n"
#define NEW_PRIMARY_CONNECTION_SUCCEDED "> Conexão com novo processo primário feita com sucesso!\nPid: %d\n\n"
#define BACKUP_HAS_BEEN_WARNED_OF_PRIMARY_FAILURE "> Fui avisado de uma falha no primário, mayday!\n\n"
#define BACKUP_RECEIVED_ELECTION_MESSAGE "> Um processo com pid menor que o meu me disse para iniciar uma eleição!\n Vou repassar pros que têm pid maior que o meu!\n\n"
#define BACKUP_HAS_WARNED_ABOUT_PRIMARY_FAILURE "> Avisei %d processo(s) sobre a falha do primário.\n\n"
#define BACKUP_WITH_BIGGEST_PID_WILL_BECOME_LEADER "> Não havia nenhum processo com pid maior que o meu para fazer a eleição.\nVou virar primário então!\n"
#define WATER_DIVIDER "\n---------------------------------------------\n\n"
#define SERVER_UP_AND_RUNNING "Servidor %s rodando.\nPid: %d, Ip: %s, Porta: %d\n\n", self.role == 0 ? "primário" : "backup", self.pid, self.ip, self.port


void print_processes_list();
void receive_other_processes_data_from_primary(process_t *primary);
void remove_primary();
// int broadcast_message(message_t *m, int pid);
void handle_primary_failure();
void warn_leader_failure();
void create_election();
void become_leader();
void *primary_healthcheck();
process_t *get_primary(list_t *head);

#endif
