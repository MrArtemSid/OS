#include "main.h"

int fn_cd(char **args) {
    if (args[1] == NULL) {
        char path[256];
        getcwd(path,256);
        printf("%s\n", path);
        return 0;
    }

    if (chdir(args[1])) {
        perror("opendir");
        return 0;
    }

    return 0;
}
int fn_clr(char **args) {
    printf("\033c");
    return 0;
}

int fn_dir(char **args) {
    if (args[1] == NULL) {
        printf("Usage: dir <directory>\n");
        return 0;
    }

    DIR *dir;
    struct dirent *entry;

    dir = opendir(args[1]);
    if (dir == NULL) {
        perror("opendir");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return 0;
}

int fn_environ(char **args) {
    extern char **environ;

    int i = 0;
    while(environ[i]) {
        printf("%s\n", environ[i++]);
    }

    return 0;
}

int fn_echo(char **args) {
    if (args[1] == NULL) {
        printf("Usage: echo <something>\n");
        return 0;
    }
    printf("%s\n", args[1]);
    return 0;
}

int fn_help(char **args) {
    printf("Available commands:\n");
    for (int i = 0; i < fn_cnt(); i++) {
        printf("%s:\n %s\n\n", fn_list[i], fn_list_help[i]);
    }
    printf("Use 'help' to display this help message.\n");
    return 0;
}

int fn_pause(char **args) {
    printf("Press any key...\n");
    getchar();
    return 0;
}

int fn_quit(char **args) {
    return -1;
}

char * fn_list[] = {
    "cd",
    "clr",
    "dir",
    "environ",
    "echo",
    "help",
    "pause",
    "quit"
};

char * fn_list_help[] = {
    "cd <directory> - change directory to <directory>. If argument <directory> isn't present, then output current one.",
    "clr - clears the screen",
    "dir <directory> - shows all files in <directory>",
    "environ - shows all environmental variables",
    "echo <anything> - outputs <anything>",
    "help - helpbox",
    "pause - makes a pause, until u press any key",
    "quit - exits from shell"
};

int (*fns[]) (char **) = {
    &fn_cd,
    &fn_clr,
    &fn_dir,
    &fn_environ,
    &fn_echo,
    &fn_help,
    &fn_pause,
    &fn_quit,
};

int fn_cnt () {
    return sizeof (fn_list) / sizeof (char *);
}

int main(int argc, char **argv) {
    char * inp_ptr;
    size_t n;
    int status = 0;
    char path[256];

    getcwd(path, 256);
    setenv("shell", strcat(path, "/myshell"), 1);

    while (!status) {
        getcwd(path, 256);
        printf("(Shell) [%s] $ ", path);
        inp_ptr = NULL;
        getline(&inp_ptr, &n, stdin);
        char **args = malloc(2 * sizeof(char *));
        args[0] = strtok(inp_ptr, " \n");
        args[1] = strtok(NULL, " \n");

        int i;
        for (i = 0; i < fn_cnt(); ++i) {
            if (args[0] != NULL && strcmp(args[0], fn_list[i]) == 0) {
                status = (*fns[i])(args);
                break;
            }
        }

        if (i == fn_cnt()) {
            pid_t pid = fork();

            if (pid == 0) {
                // Child process
                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                perror("fork");
            } else {
                wait(NULL); // Wait for child process to finish
            }
        }


        n = 0;
        free(inp_ptr);
    }

    return 0;
}
