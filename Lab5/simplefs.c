#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include "simplefs.h"

// Режим дебага
#define SFS_DEBUG
// Размеры и ограничения
#define DIR_SYS_RESERVED 4           // Зарезервированные записи в корневом каталоге (служебные)
#define CNT_BLOCKS 8                 // Общее количество блоков в корневом каталоге
#define SIZE_DIRECTORY_ENTRY 128     // Размер одной записи каталога в байтах
#define DIR_ENTRIES_PER_BLOCK (BLOCKSIZE / SIZE_DIRECTORY_ENTRY) // Количество записей каталога в одном блоке
#define DIR_ENTRIES_CNT (DIR_ENTRIES_PER_BLOCK * (CNT_BLOCKS - 1)) // Общее число записей каталога
#define MAX_FILENAME_LEN 32          // Максимальная длина имени файла
#define MAX_OPENED_FILES 10          // Максимальное количество одновременно открытых файлов

#define SIZE_FAT_ENTRY_BYTES 8       // Размер одной записи FAT (таблицы размещения файлов) в байтах
#define FAT_ENTRIES_CNT_BLOCKS 1024  // Количество записей FAT на блок
#define FAT_ENTRIES_CNT (1024 * 128) // Общее количество записей FAT для всех блоков

// Структура суперблока: метаинформация о файловой системе
typedef struct {
    char type[32];            // Тип файловой системы (например, "sfs")
    int block_size;           // Размер одного блока в байтах
    int total_blocks;         // Общее количество блоков в файловой системе
    int fat_blocks;           // Количество блоков, выделенных для FAT
    int root_dir_blocks;      // Количество блоков, выделенных для корневого каталога
    char reserved[976];       // Зарезервированное место для выравнивания до размера блока
} SuperBlock;

// Структура записи каталога: информация о файле
typedef struct {
    char filename[MAX_FILENAME_LEN]; // Имя файла
    int size;                        // Размер файла в байтах
    int first_block;                 // Номер первого блока файла в FAT
    char is_used;                    // Признак использования записи (1 — используется, 0 — свободна)
    char reserved[128 - MAX_FILENAME_LEN - 4 * 2 - 1]; // Зарезервированное место для выравнивания
} DirectoryEntry; // Размер: 128 байт

// Структура записи FAT (таблицы размещения файлов)
typedef struct {
    int next_block;  // Номер следующего блока в цепочке
    int reserved;    // Зарезервированное место для выравнивания
} FAT;  // Размер: 8 байт

// Структура для хранения информации об открытых файлах
typedef struct {
    int fd;               // Дескриптор файла
    int dir_ind;           // Индекс записи в каталоге
    char filename[32];    // Имя файла
    int mode;             // Режим доступа: чтение или запись
} OpenedFiles;

// Каталог (корневой каталог)
DirectoryEntry directory[DIR_ENTRIES_CNT] = {0};

// Таблица FAT (таблица размещения файлов)
FAT fat[FAT_ENTRIES_CNT] = {0};

// Суперблок: информация о файловой системе
SuperBlock superblock = {0};

// Таблица открытых файлов (максимум 10 записей)
OpenedFiles open_files[MAX_OPENED_FILES] = {0};

// Счетчик открытых файлов
char cnt_opened_files = 0; // Количество текущих открытых файлов

int vdisk_fd; // global virtual disk file descriptor
              // will be assigned with the sfs_mount call
              // any function in this file can use this.

int min(int a, int b) {
    if (a < b)
        return a;
    return b;
}

int abs(int a) {
    if (a < 0)
        a *= -1;
    return a;
}

int find_free_block() {
    // Ищем свободный блок в FAT
    // Пропускаем первые CNT_BLOCKS блоков (корневой каталог) и FAT_ENTRIES_CNT_BLOCKS блоков (для таблицы FAT)
    for (int i = 0; i < (superblock.total_blocks - CNT_BLOCKS - FAT_ENTRIES_CNT_BLOCKS); i++) {
        if (fat[i].next_block == 0) // Если блок свободен
            return i + CNT_BLOCKS + FAT_ENTRIES_CNT_BLOCKS; // Возвращаем индекс свободного блока
    }

    // Если свободных блоков нет, возвращаем -1
    return -1;
}

// This function is simply used to a create a virtual disk
// (a simple Linux file including all zeros) of the specified size.
// You can call this function from an app to create a virtual disk.
// There are other ways of creating a virtual disk (a Linux file)
// of certain size.
// size = 2^m Bytes
int create_vdisk (char *vdiskname, int m)
{
    char command[BLOCKSIZE];
    int size;
    int num = 1;
    int count;
    size  = num << m;
    count = size / BLOCKSIZE;
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d", vdiskname, BLOCKSIZE, count);
    printf ("executing command = %s\n", command);
    system (command);
    return 0;
}



// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk.
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = read (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf ("read error\n");
        return -1;
    }
    return 0;
}

// write block k into the virtual disk.
int write_block(void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = write(vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf("write error\n");
        return (-1);
    }
    return 0;
}


/**********************************************************************
   The following functions are to be called by applications directly.
***********************************************************************/

// Форматирование виртуального диска
// В функции sfs_format
int sfs_format(char *vdiskname) {
    // Открытие виртуального диска на чтение и запись
    vdisk_fd = open(vdiskname, O_RDWR);
    off_t file_size = lseek(vdisk_fd, 0, SEEK_END); // Определяем размер файла диска
    if (file_size == -1) {
        printf("lseek error\n");
        return -1; // Ошибка при получении размера виртуального диска
    }

    // Инициализация нового суперблока
    SuperBlock new_sb = {0};
    superblock = new_sb; // Обнуление текущего суперблока

    strcpy(superblock.type, "sfs\0");           // Установка типа файловой системы
    superblock.fat_blocks = FAT_ENTRIES_CNT_BLOCKS; // Количество блоков, выделенных для FAT
    superblock.root_dir_blocks = CNT_BLOCKS - 1; // Количество блоков для корневого каталога (7)
    superblock.block_size = BLOCKSIZE;         // Размер одного блока
    superblock.total_blocks = file_size / superblock.block_size; // Общее число блоков на диске

    // Запись суперблока в первый блок виртуального диска
    if (write_block(&superblock, 0) == -1) {
        return -1; // Ошибка записи суперблока
    }

    // Инициализация корневого каталога
    for (int i = 0; i < DIR_ENTRIES_CNT; ++i) {
        if (i >= DIR_ENTRIES_CNT - DIR_SYS_RESERVED) // Резервируем последние записи каталога
            directory[i].is_used = 1; // Системные записи помечены как используемые
        else
            directory[i].is_used = 0; // Остальные записи свободны

        directory[i].first_block = -1; // Указываем, что файл еще не имеет блоков данных
    }

    // Запись корневого каталога в соответствующие блоки виртуального диска
    for (int i = 0; i < 7; ++i) { // 7 блоков каталога
        int dir_ind = i * (BLOCKSIZE / sizeof(DirectoryEntry)); // Индекс первой записи в текущем блоке
        if (write_block(&directory[dir_ind], i + 1) == -1) { // Запись в блоки с 1 по 7
            return -1; // Ошибка записи корневого каталога
        }
    }

    // Инициализация FAT (таблицы размещения файлов)
    for (int i = 0; i < FAT_ENTRIES_CNT; ++i) {
        fat[i].next_block = 0; // 0 указывает на конец цепочки или свободный блок
        fat[i].reserved = 0;  // Резервируемое поле обнуляется
    }

    // Запись FAT в соответствующие блоки виртуального диска
    for (int i = 0; i < FAT_ENTRIES_CNT_BLOCKS; i++) { // Обработка всех блоков FAT
        int fat_ind = i * (BLOCKSIZE / sizeof(FAT)); // Индекс первой записи FAT в текущем блоке
        if (write_block(&fat[fat_ind], i + CNT_BLOCKS)) { // Запись в блоки, начиная с 8
            return -1; // Ошибка записи FAT
        }
    }

    // Обнуление таблицы открытых файлов
    for (int i = 0; i < MAX_OPENED_FILES; ++i) {
        memset(open_files[i].filename, '\0', MAX_FILENAME_LEN); // Очищаем имена файлов
    }

    return 0; // Успешное завершение форматирования
}

int sfs_mount(char *vdiskname)
{
    // Открытие виртуального диска vdiskname на чтение и запись.
    vdisk_fd = open(vdiskname, O_RDWR);

    // Проверка успешности открытия диска
    if (vdisk_fd == -1) {
        printf("Ошибка открытия виртуального диска\n");
        return -1; // Ошибка при открытии файла диска
    }

    // Чтение суперблока из первого блока диска
    if (read_block(&superblock, 0) == -1) {
        printf("Ошибка чтения суперблока\n");
        return -1; // Ошибка при чтении суперблока
    }

    // Чтение корневого каталога из следующих 7 блоков диска
    for (int i = 0; i < 7; i++) { // Каждый блок соответствует части корневого каталога
        int dir_ind = i * (BLOCKSIZE / sizeof(DirectoryEntry)); // Индекс начальной записи текущего блока
        if (read_block(&directory[dir_ind], i + 1) == -1) { // Чтение блоков с 1 по 7
            printf("Ошибка чтения корневого каталога\n");
            return -1; // Ошибка при чтении корневого каталога
        }
    }

    // Чтение FAT (таблицы размещения файлов) из последующих 1024 блоков
    for (int i = 0; i < FAT_ENTRIES_CNT_BLOCKS; i++) {
        int fat_ind = i * (BLOCKSIZE / sizeof(FAT)); // Индекс начальной записи текущего блока
        if (read_block(&fat[fat_ind], i + CNT_BLOCKS)) { // Чтение блоков, начиная с 8
            printf("Ошибка чтения FAT\n");
            return -1;
        }
    }

    return 0; // Успешное монтирование файловой системы
}

int sfs_umount()
{
    // Синхронизация буферизированных данных с диском.
    // Эта операция гарантирует, что все изменения, выполненные во время работы с виртуальным диском,
    // будут записаны на физический носитель до его закрытия.
    fsync(vdisk_fd);

    // Закрытие файла виртуального диска.
    // После этого виртуальный диск становится недоступным для операций записи или чтения,
    // пока он не будет вновь смонтирован.
    close(vdisk_fd);

    // Очистка таблицы открытых файлов.
    // Для всех записей в таблице открытых файлов обнуляются имена файлов,
    // чтобы избежать остатков данных от предыдущей сессии.
    for (int i = 0; i < MAX_OPENED_FILES; ++i) {
        memset(open_files[i].filename, '\0', MAX_FILENAME_LEN); // Обнуляем имя файла
        open_files[i].fd = -1;    // Устанавливаем неиспользуемое значение дескриптора
        open_files[i].dir_ind = -1; // Сбрасываем индекс каталога
        open_files[i].mode = 0;   // Обнуляем режим доступа
    }

    // Сброс счетчика открытых файлов.
    // Это требуется, так как после размонтирования файловой системы все файлы автоматически закрываются.
    cnt_opened_files = 0;

    // Возврат значения 0 для обозначения успешного выполнения операции размонтирования.
    return 0;
}

int sfs_create(char *filename)
{
    // Проверка длины имени файла
    // Если имя файла слишком длинное, выводится ошибка, так как оно не может быть больше максимальной длины.
    if (strlen(filename) >= MAX_FILENAME_LEN) {
        printf("Error: Filename too long\n");
        return -1;
    }

    int free_ind = -1; // Переменная для хранения индекса свободной записи в каталоге

    // Поиск свободного места в каталоге для нового файла
    // Проходим по всем записям каталога, проверяя, существует ли уже файл с таким именем
    // и ищем первую свободную запись для нового файла.
    for (int i = 0; i < DIR_ENTRIES_CNT - DIR_SYS_RESERVED; ++i) {
        // Если файл с таким именем уже существует, выводим ошибку
        if (strcmp(filename, directory[i].filename) == 0) {
            printf("Error: %s already exists\n", filename);
            return -1;
        }

        // Если запись в каталоге не используется (свободная), сохраняем её индекс
        if (!directory[i].is_used) {
            free_ind = i;
        }
    }

    // Если не найдено свободного места в каталоге, выводим ошибку
    // Это означает, что все записи заняты.
    if (free_ind == -1) {
        printf("Error: No free space\n");
        return -1;
    }

    // Инициализация новой записи в каталоге
    // Помечаем запись как используемую, устанавливаем размер файла как 0 (файл только создан)
    // Очищаем имя файла и записываем новое имя.
    directory[free_ind].is_used = 1;
    directory[free_ind].size = 0; // Новый файл имеет размер 0
    memset(directory[free_ind].filename, '\0', MAX_FILENAME_LEN); // Очищаем старое имя (если было)
    strcpy(directory[free_ind].filename, filename); // Записываем новое имя файла

    // Определяем, в какой блок будет записана данная запись
    // Используем индекс свободной записи для вычисления блока и его начала.
    int free_block = free_ind / DIR_ENTRIES_PER_BLOCK; // Номер блока, в котором будет находиться запись
    int block_start_index = free_block * DIR_ENTRIES_PER_BLOCK; // Индекс начала этого блока

    // Записываем измененный каталог (с новой записью о файле) в нужный блок
    // Если запись не удается, функция возвращает ошибку.
    if (write_block(&directory[block_start_index], free_block + 1) == -1) {
        return -1; // Ошибка записи данных
    }

#ifdef SFS_DEBUG
    printf("File %s was successfully created\n", directory[free_ind].filename);
#endif

    // Возвращаем 0 для успешного завершения операции
    return 0;
}

int sfs_open(char *file, int mode)
{
    // Проверка на достижение лимита открытых файлов.
    // Если количество уже открытых файлов больше или равно 10, выводится ошибка,
    // так как в системе разрешено открывать не более 10 файлов одновременно.
    if (cnt_opened_files >= MAX_OPENED_FILES) {
        printf("Error: limit of opened files\n");
        return -1;
    }

    // Поиск файла в каталоге
    // Пробегаем все записи в каталоге, чтобы найти файл с соответствующим именем.
    // Если файл найден, сохраняем его индекс в переменной file_ind.
    int file_ind = -1;
    for (int i = 0; i < DIR_ENTRIES_CNT - DIR_SYS_RESERVED; ++i) {
        if (strcmp(file, directory[i].filename) == 0) {
            file_ind = i; // Файл найден, запоминаем его индекс
            break;
        }
    }

    // Если файл не найден в каталоге, выводим ошибку и возвращаем -1
    if (file_ind == -1) {
        printf("Error: File %s doesn't exist\n", file);
        return -1;
    }

    int i;

    // Поиск первого свободного слота в таблице открытых файлов
    // Проходим по таблице открытых файлов и ищем первый пустой слот,
    // где имя файла пустое (равно '\0'). Когда такой слот найден, записываем
    // информацию о файле (дескриптор, индекс каталога, режим) в этот слот.
    for (i = 0; i < MAX_OPENED_FILES; ++i) {
        if (open_files[i].filename[0] == '\0') { // Если слот пустой
            open_files[i].fd = i; // Присваиваем уникальный дескриптор
            open_files[i].dir_ind = file_ind; // Устанавливаем индекс файла в каталоге
            open_files[i].mode = mode; // Устанавливаем режим доступа (чтение/запись)
            memset(open_files[i].filename, '\0', MAX_FILENAME_LEN); // Очищаем старое имя файла
            strcpy(open_files[i].filename, file); // Записываем имя файла в таблицу открытых файлов
            break;
        }
    }

    // Увеличиваем счетчик открытых файлов, так как файл был успешно открыт
    cnt_opened_files += 1;

#ifdef SFS_DEBUG
    printf("File %s was successfully opened\n", file);
#endif

    // Возвращаем индекс в таблице открытых файлов (дескриптор), который используется для последующих операций с файлом
    return (i);
}

int sfs_close(int fd){
    // Проверка на корректность дескриптора файла
    // Если дескриптор файла меньше 0 или больше или равен максимальному количеству открытых файлов,
    // то выводится ошибка, так как дескриптор недействителен.
    if (fd < 0 || fd >= (MAX_OPENED_FILES)) {
        printf("Error: file descriptor is invalid\n");
        return -1;
    }

    // Проверка на наличие открытых файлов
    // Если нет открытых файлов (cnt_opened_files == 0), выводится ошибка.
    if (!cnt_opened_files) {
        printf("Error: There is no opened files\n");
        return -1;
    }

    // Сохранение имени файла для отладочного вывода
    // Копируем имя файла, которое собираемся закрыть, чтобы вывести его в сообщении.
    char removed_name[32];
    strcpy(removed_name, open_files[fd].filename);

    // Очищение информации о файле в таблице открытых файлов
    // После закрытия файла мы очищаем запись о файле:
    // - обнуляем имя файла,
    // - сбрасываем дескриптор в -1,
    // - сбрасываем индекс каталога в -1.
    open_files[fd].filename[0] = '\0';  // Очищаем имя файла
    open_files[fd].fd = -1;              // Очищаем дескриптор файла
    open_files[fd].dir_ind = -1;          // Очищаем индекс в каталоге

    // Уменьшаем счетчик открытых файлов
    // Мы закрыли файл, поэтому уменьшаем количество открытых файлов.
    cnt_opened_files -= 1;

#ifdef SFS_DEBUG
    printf("File %s was successfully closed\n", removed_name);
#endif

    // Возвращаем 0, чтобы сигнализировать об успешном завершении операции.
    return 0;
}

int sfs_getsize(int fd)
{
    // Проверка на корректность дескриптора файла
    // Если дескриптор файла меньше 0 или больше или равен максимальному количеству открытых файлов,
    // то выводится ошибка, так как дескриптор недействителен.
    if (fd < 0 || fd >= (MAX_OPENED_FILES)) {
        printf("Error: file descriptor is invalid\n");
        return -1;
    }

    // Проверка на наличие открытых файлов
    // Если нет открытых файлов (cnt_opened_files == 0), выводится ошибка.
    if (!cnt_opened_files) {
        printf("Error: There is no opened files\n");
        return -1;
    }

    // Возвращаем размер файла
    // Получаем размер файла из записи каталога, используя индекс директории, который сохранен в таблице открытых файлов.
    return directory[open_files[fd].dir_ind].size;
}

int sfs_read(int fd, void *buf, int n) {
    // Проверка на корректность дескриптора файла
    // Если дескриптор меньше 0 или больше или равен максимальному количеству открытых файлов,
    // или если файл не открыт (open_files[fd].fd == -1), возвращаем ошибку.
    if (fd < 0 || fd >= MAX_OPENED_FILES || open_files[fd].fd == -1)
        return -1; // Неверный дескриптор или файл не открыт

    // Получаем индекс записи каталога для данного файла
    int dir_ind = open_files[fd].dir_ind;
    int file_size = directory[dir_ind].size; // Получаем размер файла
    int current_block = directory[dir_ind].first_block; // Получаем первый блок файла

    // Если размер файла равен 0 (файл пустой), возвращаем 0
    if (file_size == 0)
        return 0; // Файл пустой

    // Определяем количество байтов, которые нужно прочитать (не больше размера файла и запрашиваемого количества)
    int bytes_to_read = min(n, file_size);
    int total_read = 0; // Общее количество прочитанных байтов
    char block[BLOCKSIZE]; // Буфер для хранения данных из блока

    // Чтение блоков файла
    // Пока не прочитано нужное количество байтов или пока не дойдем до конца цепочки блоков
    while (total_read < bytes_to_read && current_block != 0) {
        // Чтение данных из текущего блока
        if (read_block(block, current_block) == -1)
            return -1; // Ошибка чтения блока

        // Определяем, сколько байтов нужно скопировать из текущего блока
        int bytes_in_block = min(BLOCKSIZE, bytes_to_read - total_read);
        memcpy((char *)buf + total_read, block, bytes_in_block); // Копируем данные в буфер
        total_read += bytes_in_block; // Увеличиваем количество прочитанных байтов

        // Переход к следующему блоку
        current_block = fat[current_block].next_block;
    }

    // Возвращаем количество прочитанных байтов
    return total_read;
}

int sfs_append(int fd, void *buf, int n) {
    // Проверка на корректность дескриптора файла
    // Если дескриптор меньше 0 или больше или равен максимальному количеству открытых файлов,
    // или если файл не открыт (open_files[fd].fd == -1), возвращаем ошибку.
    if (fd < 0 || fd >= MAX_OPENED_FILES || open_files[fd].fd == -1)
        return -1; // Неверный дескриптор или файл не открыт

    // Получаем индекс записи каталога для данного файла
    int dir_ind = open_files[fd].dir_ind;
    int current_block = directory[dir_ind].first_block; // Получаем первый блок файла
    int offset = directory[dir_ind].size % BLOCKSIZE; // Вычисляем смещение в блоке (если файл не пустой)
    int total_written = 0; // Общее количество записанных байтов
    char block[BLOCKSIZE] = {0}; // Буфер для записи данных в блок

    // Если файл еще не имеет данных (first_block == -1), находим первый свободный блок
    if (current_block == -1) {
        current_block = find_free_block();
        if (current_block == -1)
            return -1; // Нет свободных блоков
        directory[dir_ind].first_block = current_block; // Записываем первый блок в директорию
    } else {
        // Если файл уже существует, читаем текущий блок для продолжения записи
        read_block(block, current_block);
    }

    // Цикл записи данных
    while (total_written < n) {
        // Определяем сколько байтов можно записать в текущий блок
        int to_copy = min(n - total_written, BLOCKSIZE - offset);
        memcpy(block + offset, (char *)buf + total_written, to_copy); // Копируем данные в блок
        total_written += to_copy; // Увеличиваем количество записанных байтов
        offset += to_copy; // Увеличиваем смещение в блоке

        // Если блок заполнен (offset == BLOCKSIZE), записываем его на диск и ищем новый блок
        if (offset == BLOCKSIZE) {
            write_block(block, current_block); // Записываем текущий блок
            offset = 0; // Сбрасываем смещение для следующего блока

            // Находим новый свободный блок для продолжения записи
            int new_block = find_free_block();
            if (new_block == -1)
                return total_written; // Нет свободных блоков, возвращаем количество записанных байтов

            // Обновляем таблицу FAT для текущего блока
            fat[current_block].next_block = new_block;
            current_block = new_block; // Переходим к новому блоку
            memset(block, 0, BLOCKSIZE); // Очищаем буфер перед записью в новый блок
        }
    }

    // Если остались данные, которые не заполнили весь блок, записываем неполный блок
    if (offset > 0)
        write_block(block, current_block); // Записываем неполный блок

    // Обновляем размер файла в директории
    directory[dir_ind].size += total_written;

    // Возвращаем количество записанных байтов
    return total_written;
}

int sfs_delete(char *filename)
{
    // Ищем открытые файлы, чтобы убедиться, что файл не открыт
    int clear_ind = -1;
    for (int i = 0; i < MAX_OPENED_FILES; ++i) {
        // Проверка на совпадение имени файла в открытых файлах
        if (strcmp(open_files[i].filename, filename) == 0) {
            // Если файл открыт, сбрасываем его запись в таблице open_files
            clear_ind = open_files[i].dir_ind;
            open_files[i].filename[0] = '\0';  // Очищаем имя файла
            open_files[i].fd = -1;  // Сбрасываем дескриптор
            open_files[i].dir_ind = -1;  // Сбрасываем индекс директории
            break;
        }
    }

    // Если файл не найден среди открытых файлов, ищем его в директории
    if (clear_ind == -1) {
        for (int i = 0; i < DIR_ENTRIES_CNT - DIR_SYS_RESERVED; ++i) {
            // Поиск файла в директории
            if (strcmp(filename, directory[i].filename) == 0) {
                clear_ind = i;
                break;
            }
        }

        // Если файл не найден в директории, возвращаем ошибку
        if (clear_ind == -1) {
            return -1; // Файл не найден
        }
    }

    // Очищаем запись о файле в директории
    directory[clear_ind].filename[0] = '\0';  // Очищаем имя файла
    directory[clear_ind].size = 0;  // Обнуляем размер файла
    directory[clear_ind].is_used = 0;  // Отмечаем файл как не используемый

    // Определяем номер блока в директории, где находится запись о файле
    int block = clear_ind / DIR_ENTRIES_PER_BLOCK; // Определяем номер блока
    int block_start_index = block * DIR_ENTRIES_PER_BLOCK; // Индекс начала блока

    // Записываем обновленную директорию обратно в блок
    if (write_block(&directory[block_start_index], block + 1) == -1) {
        return -1; // Ошибка записи
    }

#ifdef SFS_DEBUG
    printf("File %s was successfully deleted\n", filename);
#endif

    return 0;
}