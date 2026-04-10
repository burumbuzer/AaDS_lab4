/*
pack_reorder.c
Бригада 5, функция 7: Упаковка файла
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define HEADER_SIZE 20
#define NULL_PTR -1

#pragma pack(push, 1)
typedef struct {
    unsigned char deleted;
    char name[20];
    int next;
} record_t;
#pragma pack(pop)

#define RECORD_SIZE sizeof(record_t)

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: pack_reorder <filename>\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "r+b");
    if (!f) {
        printf("Error: cannot open file\n");
        return 1;
    }

    // Чтение заголовка
    int n_active, n_deleted, first_active, first_deleted, last_deleted;
    fread(&n_active, 4, 1, f);
    fread(&n_deleted, 4, 1, f);
    fread(&first_active, 4, 1, f);
    fread(&first_deleted, 4, 1, f);
    fread(&last_deleted, 4, 1, f);

    if (n_active + n_deleted == 0) {
        printf("File is empty\n");
        fclose(f);
        return 0;
    }

    // Выделение памяти
    record_t *active = malloc(n_active * RECORD_SIZE);
    record_t *deleted = malloc(n_deleted * RECORD_SIZE);

    if ((n_active > 0 && !active) || (n_deleted > 0 && !deleted)) {
        printf("Error: out of memory\n");
        free(active); free(deleted);
        fclose(f);
        return 1;
    }

    // Сбор активных записей
    int i = 0, ptr = first_active;
    while (ptr != NULL_PTR && i < n_active) {
        fseek(f, ptr, SEEK_SET);
        fread(&active[i], RECORD_SIZE, 1, f);
        ptr = active[i].next;
        i++;
    }
    n_active = i;

    // Сбор удалённых записей
    int j = 0;
    ptr = first_deleted;
    while (ptr != NULL_PTR && j < n_deleted) {
        fseek(f, ptr, SEEK_SET);
        fread(&deleted[j], RECORD_SIZE, 1, f);
        ptr = deleted[j].next;
        j++;
    }
    n_deleted = j;

    // Пересчёт указателей для активных
    for (i = 0; i < n_active; i++) {
        active[i].next = (i + 1 < n_active) 
            ? HEADER_SIZE + (i + 1) * RECORD_SIZE 
            : NULL_PTR;
        active[i].deleted = 0;
    }

    // Пересчёт указателей для удалённых
    for (j = 0; j < n_deleted; j++) {
        deleted[j].next = (j + 1 < n_deleted) 
            ? HEADER_SIZE + (n_active + j + 1) * RECORD_SIZE 
            : NULL_PTR;
        deleted[j].deleted = 1;
    }

    // Обновление заголовка
    fseek(f, 0, SEEK_SET);
    first_active = (n_active > 0) ? HEADER_SIZE : NULL_PTR;
    first_deleted = (n_deleted > 0) ? HEADER_SIZE + n_active * RECORD_SIZE : NULL_PTR;
    last_deleted = (n_deleted > 0) ? HEADER_SIZE + (n_active + n_deleted - 1) * RECORD_SIZE : NULL_PTR;

    fwrite(&n_active, 4, 1, f);
    fwrite(&n_deleted, 4, 1, f);
    fwrite(&first_active, 4, 1, f);
    fwrite(&first_deleted, 4, 1, f);
    fwrite(&last_deleted, 4, 1, f);

    // Запись активных
    for (i = 0; i < n_active; i++) {
        int offset = HEADER_SIZE + i * RECORD_SIZE;
        fseek(f, offset, SEEK_SET);
        fwrite(&active[i], RECORD_SIZE, 1, f);
    }

    // Запись удалённых
    for (j = 0; j < n_deleted; j++) {
        int offset = HEADER_SIZE + (n_active + j) * RECORD_SIZE;
        fseek(f, offset, SEEK_SET);
        fwrite(&deleted[j], RECORD_SIZE, 1, f);
    }

    // Обрезка файла (кроссплатформенно)
    long new_size = HEADER_SIZE + (long)(n_active + n_deleted) * RECORD_SIZE;
    fflush(f);
#ifdef _WIN32
    _chsize(_fileno(f), new_size);
#else
    ftruncate(fileno(f), new_size);
#endif

    fclose(f);
    free(active);
    free(deleted);

    printf("Pack complete: %d active, %d deleted\n", n_active, n_deleted);
    return 0;
}
