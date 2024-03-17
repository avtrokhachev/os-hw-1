//
// Created by Andrey Trokhachev on 15.03.2024.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_SIZE 5000  // размер буфера
#define INDEX_MAX_SIZE 4 //  максимальный размер интекса ответа принимаю за 4 байта

// функция отвечающая за проверку вхождения шаблона в строку начиная с конкретной позиции
// возвращает true если подстрока может начинаться с этой позиции и false в противном случае
int check_start_position(char *text, size_t text_size, size_t pos, char *template, size_t template_size) {
    size_t i = 0;
    for (; pos + i < text_size && i < template_size; ++i) {
        if (text[pos + i] != template[i]) {
            return 0;
        }
    }

    return (i == template_size);
}


// функция отвечающая за поиск всевозможных вхождений шаблона в строку
// возвращает кол-во вхождений, а сами вхождения записывает в answer
int find_all_matches_in_text(char *text, size_t text_size, char *template, size_t template_size, int *answer) {
    int matches_cnt = 0;
    for (int i = 0; i < text_size; ++i) {
        if (check_start_position(text, text_size, i, template, template_size)) {
            answer[matches_cnt] = i;
            ++matches_cnt;
        }
    }

    return matches_cnt;
}


int main(int argc, char *argv[]) {
    // проверяю что задан файл откуда читать, файл куда писать и шаблон
    if (argc != 4) {
        printf("Invalid number of params.\n");
        exit(-1);
    }

    pid_t pid_input_output, pid_main;
    int fd_input[2], fd_output[2], p_status;

    // создаю каналы
    if (pipe(fd_input) < 0 || pipe(fd_output) < 0) {
        printf("Error while opening pipe\n");
        exit(-1);
    }

    // создаю процесс для ввода текста
    pid_input_output = fork();
    if (pid_input_output < 0) {
        printf("Error while forking the process\n");
        exit(-1);
    }

    if (pid_input_output == 0) {
        // открываю файл и читаю из него текст
        char file_buffer[BUF_SIZE];
        int input_file = open(argv[1], O_RDONLY);
        if (input_file < 0) {
            printf("Error while opening input file\n");
            exit(-1);
        }
        read(input_file, file_buffer, BUF_SIZE);
        if (close(input_file) < 0) {
            printf("Error while closing input file\n");
            exit(-1);
        }

        // передаю текст главному процессу через канал
        if (write(fd_input[1], file_buffer, BUF_SIZE) != BUF_SIZE) {
            printf("Error while writing to pipe\n");
            exit(-1);
        }
        if (close(fd_input[1]) < 0) {
            printf("Error while closing pipe\n");
            exit(-1);
        }

        pid_main = fork();
        if (pid_main < -1) {
            printf("Error while forking the process\n");
            exit(-1);
        }

        if (pid_main == 0) {
            // процесс для обработки текста
            char input_buffer[BUF_SIZE];
            size_t size = read(fd_input[0], input_buffer, BUF_SIZE);
            if (size < 0) {
                printf("Error while reading from pipe\n");
                exit(-1);
            }
            if (close(fd_input[0]) < 0) {
                printf("Error while closing pipe\n");
                exit(-1);
            }
            size = strlen(input_buffer);

            // считаю ответ и привожу его к нужному формату
            int ans[size];
            size_t ans_size = find_all_matches_in_text(input_buffer, size, argv[3], strlen(argv[3]), ans);
            char output_result[BUF_SIZE];
            output_result[0] = '\0';
            for (size_t i = 0; i < ans_size; ++i) {
                char buffer[INDEX_MAX_SIZE];
                sprintf(buffer, "%d", ans[i]);
                strcat(output_result, buffer);

                if (i < ans_size - 1) {
                    strcat(output_result, " ");
                }
            }

            // передаю данные процессу на вывод в файл
            size = write(fd_output[1], output_result, BUF_SIZE);
            if (size < 0) {
                printf("Error while writing to pipe\n");
                exit(-1);
            }
            if (close(fd_output[1]) < 0) {
                printf("Error while closing pipe\n");
                exit(-1);
            }

            // завершаю успешно процесс
            exit(0);
        }

        if (waitpid(pid_main, &p_status, 0) < 0) {
            perror("Error while waiting pid\n");
            exit(-1);
        }

        // получаю результат из второго процесса
        char output_result[BUF_SIZE];
        size_t size = read(fd_output[0], output_result, BUF_SIZE);
        if (size < 0) {
            printf("Error while reading from pipe\n");
            exit(-1);
        }

        // выводу результат в файл
        int output_file = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_file < 0) {
            printf("Error while opening file\n");
            exit(-1);
        }
        write(output_file, output_result, strlen(output_result));

        // закрываю канал и файл
        if (close(output_file) < 0) {
            printf("Error while closing file\n");
            exit(-1);
        }
        if (close(fd_output[0]) < 0) {
            printf("Error while closing pipe\n");
            exit(-1);
        }

        // завершаю процесс вывода данных в файл
        exit(0);
    }

    if (waitpid(pid_input_output, &p_status, 0) < 0) {
        perror("Error while waiting pid\n");
        exit(-1);
    }

    return 0;
}