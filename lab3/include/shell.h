#ifndef SHELL_H
#define SHELL_H

#define MAX_CMD_LEN 256

typedef struct command {
    const char *name;
    const char *help;
    void (*func)(int argc, char **argv);
} command_t;

// Command handler functions
void do_help(int argc, char **argv);
void do_hello(int argc, char **argv);
void do_reboot(int argc, char **argv);
void do_hw_info(int argc, char **argv);
void do_cpio_ls(int argc, char **argv);
void do_cpio_cat(int argc, char **argv);
void do_run_user_program(int argc, char **argv);
void do_timeout(int argc, char **argv);
// Shell functions
void shell_execute(char *cmd_buffer);
int shell(void);
void run_user_program(const char *program_name);

#endif  // SHELL_H
