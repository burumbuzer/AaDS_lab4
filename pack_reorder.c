/*
pack_reorder.c 
7. упаковка файла

gcc pack_reorder.c -o pack_reorder
./pack_reorder test.dat
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define HEADER_SIZE 20
#define RECORD_SIZE 25
#define NULL_PTR -1

typedef struct {
    char deleted;
    char name[20];
    int next;
} record_t;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: pack.exe <filename>\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "r+b");
    if (!f) {
        printf("Error: cannot open file\n");
        return 1;
    }

    // читаем заголовок
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

    // выделяем память
    record_t *active = malloc(n_active * sizeof(record_t));
    record_t *deleted = malloc(n_deleted * sizeof(record_t));
    
    if ((n_active > 0 && !active) || (n_deleted > 0 && !deleted)) {
        printf("Error: out of memory\n");
        free(active); free(deleted); fclose(f);
        return 1;
    }

    // собираем активные записи
    int i = 0, ptr = first_active;
    while (ptr != NULL_PTR && i < n_active) {
        fseek(f, ptr, SEEK_SET);
        fread(&active[i], sizeof(record_t), 1, f);
        ptr = active[i].next;
        i++;
    }
    n_active = i;

    // собираем удалённые записи
    int j = 0;
    ptr = first_deleted;
    while (ptr != NULL_PTR && j < n_deleted) {
        fseek(f, ptr, SEEK_SET);
        fread(&deleted[j], sizeof(record_t), 1, f);
        ptr = deleted[j].next;
        j++;
    }
    n_deleted = j;

    // пересчитываем указатели для активных
    for (i = 0; i < n_active; i++) {
        if (i + 1 < n_active)
            active[i].next = HEADER_SIZE + (i + 1) * RECORD_SIZE;
        else
            active[i].next = NULL_PTR;
        active[i].deleted = 0;
    }

    // пересчитываем указатели для удалённых
    for (j = 0; j < n_deleted; j++) {
        if (j + 1 < n_deleted)
            deleted[j].next = HEADER_SIZE + (n_active + j + 1) * RECORD_SIZE;
        else
            deleted[j].next = NULL_PTR;
        deleted[j].deleted = 1;
    }

    // обновляем заголовок
    rewind(f);
    first_active = (n_active > 0) ? HEADER_SIZE : NULL_PTR;
    first_deleted = (n_deleted > 0) ? HEADER_SIZE + n_active * RECORD_SIZE : NULL_PTR;
    last_deleted = (n_deleted > 0) ? HEADER_SIZE + (n_active + n_deleted - 1) * RECORD_SIZE : NULL_PTR;
    
    fwrite(&n_active, 4, 1, f);
    fwrite(&n_deleted, 4, 1, f);
    fwrite(&first_active, 4, 1, f);
    fwrite(&first_deleted, 4, 1, f);
    fwrite(&last_deleted, 4, 1, f);

    // пишем активные записи
    for (i = 0; i < n_active; i++) {
        int offset = HEADER_SIZE + i * RECORD_SIZE;
        fseek(f, offset, SEEK_SET);
        fwrite(&active[i], sizeof(record_t), 1, f);
    }

    // пишем удалённые записи
    for (j = 0; j < n_deleted; j++) {
        int offset = HEADER_SIZE + (n_active + j) * RECORD_SIZE;
        fseek(f, offset, SEEK_SET);
        fwrite(&deleted[j], sizeof(record_t), 1, f);
    }

    // обрезаем файл
    long new_size = HEADER_SIZE + (long)(n_active + n_deleted) * RECORD_SIZE;
    fflush(f);
    ftruncate(fileno(f), new_size);

    fclose(f);
    free(active);
    free(deleted);
    
    printf("Pack complete: %d active, %d deleted\n", n_active, n_deleted);
    return 0;
}
