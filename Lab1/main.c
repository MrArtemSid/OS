#include "main.h"

// Команда 'cd': изменяет текущую рабочую директорию на указанную
int fn_cd(char **args) {
    if (args[1] == NULL) { // при отсутствии аргумента вывести текущий путь
        char path[256];
        getcwd(path,256);
        printf("%s\n", path);
        return 0;
    }

    if (chdir(args[1])) { // попытка сменить директорию
        perror("cd");
        return 0;
    }

    return 0;
}

// Команда 'clr': очищает экран
int fn_clr(char **args) {
    printf("\033c");
    return 0;
}

// Команда 'dir': выводит список всех файлов в указанной директории
int fn_dir(char **args) {
    if (args[1] == NULL) {
        printf("Usage: dir <directory>\n");
        return 0;
    }

    DIR *dir;
    struct dirent *entry;

    dir = opendir(args[1]);
    if (dir == NULL) {
        perror("dir");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) { // вывод списка файлов и папок
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return 0;
}

// Команда 'environ': отображает все переменные среды
int fn_environ(char **args) {
    extern char **environ;

    int i = 0;
    while(environ[i]) {
        printf("%s\n", environ[i++]);
    }

    return 0;
}

// Команда 'echo': отображает указанный текст
int fn_echo(char **args) {
    if (args[1] == NULL) {
        printf("Usage: echo <something>\n");
        return 0;
    }
    printf("%s\n", args[1]);
    return 0;
}

// Команда 'help': выводит список доступных команд
int fn_help(char **args) {
    printf("Available commands:\n");
    for (int i = 0; i < fn_cnt(); i++) {
        printf("%s:\n %s\n\n", fn_list[i], fn_list_help[i]);
    }
    printf("Use 'help' to display this help message.\n");
    return 0;
}

// Команда 'pause': приостанавливает выполнение оболочки до нажатия клавиши
int fn_pause(char **args) {
    printf("Press any key...\n");
    getchar();
    return 0;
}

// Команда 'quit': завершает выполнение оболочки
int fn_quit(char **args) {
    return 1;
}

// Список команд
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

// Описание команд
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

// Массив указателей на функции для каждой команды
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

// Возвращает количество команд
int fn_cnt () {
    return sizeof (fn_list) / sizeof (char *);
}

// Проверяет существует ли функция
bool is_fn_exist (char **args) {
    int i;
    for (i = 0; i < fn_cnt(); ++i) { // цикл для поиска и запуска введеных команд
        if (args[0] != NULL && strcmp(args[0], fn_list[i]) == 0) {
            status = (*fns[i])(args);
            break;
        }
    }

    return i != fn_cnt();
}

// Запускает внешние программы
int run_cmd (char **args) {
    pid_t pid = fork(); // клонирование процесса

    if (pid == 0) {
        // Child процесс
        if (is_background) {
            // Перенаправляем stdout и stderr в /dev/null
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);
        }
        execvp(args[0], args);
        perror("execvp");
        return(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        if (!is_background)
            wait(NULL);
    }

    return 0;
}

int console_input;  // Стандартный консольный ввод
int console_output; // Стандартный консольный вывод

void restore_original_fd() {
    dup2(console_input, STDIN_FILENO);
    dup2(console_output, STDOUT_FILENO);
}

int handle_redirection(char **args) {
    int i = 0;
    char *input_file = NULL;
    char *output_file = NULL;
    int append_mode = 0;

    // Поиск символов
    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) {
            input_file = args[i + 1];
            args[i] = NULL; // Игнорируем текущий элемент
        } else if (strcmp(args[i], ">") == 0) {
            output_file = args[i + 1];
            args[i] = NULL; // Игнорируем текущий элемент
        } else if (strcmp(args[i], ">>") == 0) {
            output_file = args[i + 1];
            args[i] = NULL; // Игнорируем текущий элемент
            append_mode = 1;
        } else if (strcmp(args[i], "&") == 0) {
            args[i] = NULL;
            is_background = true;
        }
        ++i;
    }

    // Перенаправление ввода
    if (input_file) {
        FILE *file = fopen(input_file, "r");
        if (file) {
            dup2(fileno(file), STDIN_FILENO);
            fclose(file);
        } else {
            perror("Error opening input file");
            restore_original_fd();
            return 1;
        }
    }

    // Перенаправление вывода
    if (output_file) {
        const char *mode = append_mode ? "a" : "w"; // Устанавливаем режим
        FILE *file = fopen(output_file, mode);

        if (file) {
            dup2(fileno(file), STDOUT_FILENO);
            fclose(file);
        } else {
            perror("Error opening output file");
            restore_original_fd();
            return 1;
        }
    }
    return 0;
}


// Основная функция оболочки
int main(int argc, char **argv) {
    // Переключение на ввод с файла, если предоставлен аргумент или же вывод ошибки если файла не существует
    if (argc == 2 && freopen(argv[1], "r", stdin) == NULL) {
        printf("Error: input file\n");
        exit(1);
    }

    char * inp_ptr; // указатель на строку
    size_t n; // размер строки
    status = 0; // статус выполнения, если не 0, то программа завершается
    char *args[10];
    char path[256]; // массив для сохранения пути
    int i; // индекс для args

    console_input = dup(STDIN_FILENO); // Стандартный консольный ввод
    console_output = dup(STDOUT_FILENO); // Стандартный консольный вывод

    getcwd(path, 256);
    setenv("shell", strcat(path, "/myshell"), 1);

    while (!status) {
        getcwd(path, 256);
        printf("(Shell) [%s] $ ", path);

        inp_ptr = NULL;
        getline(&inp_ptr, &n, stdin); // ввод пользователя

        i = 0;
        args[i++] = strtok(inp_ptr, " \n");
        while (args[i++] = strtok(NULL, " \n"));

        if (handle_redirection(args)) // попытка редиректа, в случае ошибки - пропуск команды
            continue;

        if (!is_fn_exist(args)) { // если команды нет в списке существующих, то запуск программ
            run_cmd(args);
        }

        n = 0;
        free(inp_ptr);
        restore_original_fd();
        is_background = false;
        memset(args, 0, 10 * sizeof(char *));
    }

    return 0;
}
