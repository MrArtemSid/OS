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
        execvp(args[0], args);
        perror("execvp");
        return(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        wait(NULL); // Ожидание завершения child процесса
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
    char path[256]; // массив для сохранения пути

    getcwd(path, 256);
    setenv("shell", strcat(path, "/myshell"), 1);

    while (!status) {
        getcwd(path, 256);
        printf("(Shell) [%s] $ ", path);

        inp_ptr = NULL;
        getline(&inp_ptr, &n, stdin); // ввод пользователя
        char **args = malloc(2 * sizeof(char *));
        args[0] = strtok(inp_ptr, " \n"); // сохранение команды
        args[1] = strtok(NULL, " \n"); // сохранение аргумента команды

        if (!is_fn_exist(args)) { // если команды нет в списке существующих, то запуск программ
            run_cmd(args);
        }

        n = 0;
        free(inp_ptr);
    }

    return 0;
}
