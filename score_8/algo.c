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
#include <sys/stat.h>

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
    if (argc != 2) {
        printf("Invalid number of params.\n");
        exit(-1);
    }

    char pipe_input[] = "pipe_input";
    char pipe_output[] = "pipe_output";

    char input_buffer[BUF_SIZE];
    int fd_input = open(pipe_input, O_RDONLY);
    size_t size = read(fd_input, input_buffer, BUF_SIZE);
    if (size < 0) {
        printf("Error while reading from pipe\n");
        exit(-1);
    }
    if (close(fd_input) < 0) {
        printf("Error while closing pipe\n");
        exit(-1);
    }
    size = strlen(input_buffer);

    // считаю ответ и привожу его к нужному формату
    int ans[size];
    size_t ans_size = find_all_matches_in_text(input_buffer, size, argv[1], strlen(argv[1]), ans);
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
    int fd_output = open(pipe_output, O_WRONLY);
    size = write(fd_output, output_result, BUF_SIZE);
    if (size < 0) {
        printf("Error while writing to pipe\n");
        exit(-1);
    }
    if (close(fd_output) < 0) {
        printf("Error while closing pipe\n");
        exit(-1);
    }

    // завершаю успешно процесс
    exit(0);

    return 0;
}