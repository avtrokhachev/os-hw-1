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


int main(int argc, char *argv[]) {
    // проверяю что задан файл откуда читать, файл куда писать и шаблон
    if (argc != 3) {
        printf("Invalid number of params.\n");
        exit(-1);
    }

    char pipe_input[] = "pipe_input";
    char pipe_output[] = "pipe_output";

    // создаю каналы
    if (mkfifo(pipe_input, 0666) < 0 || mkfifo(pipe_output, 0666) < 0) {
        printf("Error while opening pipe\n");
        exit(-1);
    }

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
    int fd_input = open(pipe_input, O_WRONLY);
    if (write(fd_input, file_buffer, BUF_SIZE) != BUF_SIZE) {
        printf("Error while writing to pipe\n");
        exit(-1);
    }
    if (close(fd_input) < 0) {
        printf("Error while closing pipe\n");
        exit(-1);
    }

    int fd_output = open(pipe_output, O_RDONLY);
    char output_result[BUF_SIZE];
    size_t size = read(fd_output, output_result, BUF_SIZE);
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
    if (close(fd_output) < 0) {
        printf("Error while closing pipe\n");
        exit(-1);
    }

    unlink(pipe_input);
    unlink(pipe_output);

    return 0;
}