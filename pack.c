#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_SIZE 20
#define RECORD_SIZE sizeof(Record) 

// Структура заголовка файла
typedef struct {
    int active_count;
    int deleted_count;
    int first_active;
    int first_deleted;
    int last_deleted;
} Header;

// Структура записи
typedef struct {
    unsigned char deleted; // 1 - удалена, 0 - активна
    char name[20];
    int next;
} Record;

void print_usage() {
    printf("Использование: pack.exe <файл> compress\n");
    printf("Сжатие файла - удаляет все помеченные записи\n");
}

// Чтение заголовка из файла
int read_header(FILE* f, Header* h) {
    fseek(f, 0, SEEK_SET);
    return fread(h, sizeof(Header), 1, f) == 1;
}

// Запись заголовка в файл
int write_header(FILE* f, Header* h) {
    fseek(f, 0, SEEK_SET);
    return fwrite(h, sizeof(Header), 1, f) == 1;
}

// Чтение записи по смещению
int read_record(FILE* f, int offset, Record* r) {
    if (offset <= 0) return 0;
    fseek(f, offset, SEEK_SET);
    return fread(r, RECORD_SIZE, 1, f) == 1;
}

// Запись записи по смещению
int write_record(FILE* f, int offset, Record* r) {
    fseek(f, offset, SEEK_SET);
    return fwrite(r, RECORD_SIZE, 1, f) == 1;
}

int main(int argc, char* argv[]) {
    FILE* file = fopen(argv[1], "rb+");
    if (!file) {
        printf("Не может открыть файл %s\n", argv[1]);
        return 1;
    }

    // Читаем заголовок
    Header h;
    if (!read_header(file, &h)) {
        printf("Не может прочитать заголовок\n");
        fclose(file);
        return 1;
    }

    printf("Активных: %d, Удалённых: %d\n", h.active_count, h.deleted_count);

    if (h.deleted_count == 0) {
        printf("Нет удалённых записей\n");
        fclose(file);
        return 0;
    }

    // Собираем все активные записи
    Record* active = NULL;
    if (h.active_count > 0) {
        active = (Record*)malloc(h.active_count * sizeof(Record));
        if (!active) {
            printf("Ошибка: не хватает памяти\n");
            fclose(file);
            return 1;
        }
    }

    int pos = h.first_active;
    int i = 0;
    while (pos != 0 && i < h.active_count) {
        if (!read_record(file, pos, &active[i])) {
            printf("Ошибка чтения записи на позиции %d\n", pos);
            free(active);
            fclose(file);
            return 1;
        }
        pos = active[i].next;
        i++;
    }

    // Проверка: все ли активные записи прочитаны
    if (i != h.active_count) {
        printf("Предупреждение: прочитано %d из %d активных записей\n", i, h.active_count);
    }

    // Создаём временный файл
    char temp_name[256];
    strcpy(temp_name, argv[1]);
    strcat(temp_name, ".tmp");

    FILE* temp = fopen(temp_name, "wb");
    if (!temp) {
        printf("Не может создать временный файл\n");
        free(active);
        fclose(file);
        return 1;
    }

    // Новый заголовок
    Header new_h = { 0 };
    new_h.active_count = h.active_count;
    new_h.first_active = sizeof(Header);
    new_h.deleted_count = 0;
    new_h.first_deleted = 0;
    new_h.last_deleted = 0;

    // Записываем заголовок
    if (!write_header(temp, &new_h)) {
        printf("Не может записать заголовок\n");
        free(active);
        fclose(file);
        fclose(temp);
        remove(temp_name);
        return 1;
    }

    // Записываем активные записи подряд
    int offset = sizeof(Header);
    for (i = 0; i < h.active_count; i++) {
        active[i].deleted = 0;
        active[i].next = (i < h.active_count - 1) ? offset + RECORD_SIZE : 0;

        if (!write_record(temp, offset, &active[i])) {
            printf("Не может записать запись %d\n", i);
            free(active);
            fclose(file);
            fclose(temp);
            remove(temp_name);
            return 1;
        }
        offset += RECORD_SIZE;
    }

    free(active);
    fclose(file);
    fclose(temp);

    // Заменяем файл
    if (remove(argv[1]) != 0) {
        printf("Не может удалить исходный файл\n");
        return 1;
    }

    if (rename(temp_name, argv[1]) != 0) {
        printf("Не может переименовать временный файл\n");
        return 1;
    }

    printf("Сжатие завершено!\n");
    printf("Удалено %d записей\n", h.deleted_count);

    return 0;
}
