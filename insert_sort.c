#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NAME_LEN 20

typedef struct {
    int active_count;      // кол-во активных
    int deleted_count;     // кол-во удалённых
    int first_active_ptr;  // указатель на первый активный
    int first_deleted_ptr; // указатель на первый удалённый
    int last_deleted_ptr;  // указатель на последний удалённый
} Header;

// Запись-элемент списка
typedef struct {
    unsigned char deleted_flag;
    char name[NAME_LEN];
    int next_ptr;
} Record;

// Чтение заголовка
int read_header(FILE *f, Header *h) {
    rewind(f);
    return fread(h, sizeof(Header), 1, f) == 1 ? 0 : -1;
}

// Запись заголовка
int write_header(FILE *f, Header *h) {
    rewind(f);
    return fwrite(h, sizeof(Header), 1, f) == 1 ? 0 : -1;
}

// Чтение записи по смещению
int read_record(FILE *f, int offset, Record *r) {
    if (fseek(f, offset, SEEK_SET) != 0) return -1;
    return fread(r, sizeof(Record), 1, f) == 1 ? 0 : -1;
}

// Запись записи по смещению
int write_record(FILE *f, int offset, Record *r) {
    if (fseek(f, offset, SEEK_SET) != 0) return -1;
    return fwrite(r, sizeof(Record), 1, f) == 1 ? 0 : -1;
}

// Проверка уникальности имени в активном списке
int name_exists(FILE *f, Header *h, const char *name) {
    int curr = h->first_active_ptr;
    Record rec;
    while (curr != 0) {
        if (read_record(f, curr, &rec) != 0) return -1;
        if (strncmp(name, rec.name, NAME_LEN) == 0) return 1;
        curr = rec.next_ptr;
    }
    return 0;
}

// Поиск позиции вставки для сохранения упорядоченности
int find_insert_position(FILE *f, Header *h, const char *name,
                         int *prev_off, int *next_off) {
    *prev_off = 0;
    *next_off = h->first_active_ptr;
    Record rec;
    while (*next_off != 0) {
        if (read_record(f, *next_off, &rec) != 0) return -1;
        if (strncmp(name, rec.name, NAME_LEN) < 0) break;
        *prev_off = *next_off;
        *next_off = rec.next_ptr;
    }
    return 0;
}

// Функция № 6
int add_sorted_element(const char *filename, const char *name) {
    FILE *f = fopen(filename, "rb+");
    if (!f) {
        f = fopen(filename, "wb+");
        if (!f) return -1;
        Header empty = {0, 0, 0, 0, 0};
        if (write_header(f, &empty) != 0) {
            fclose(f);
            return -1;
        }
    }

    Header h;
    if (read_header(f, &h) != 0) {
        fclose(f);
        return -1;
    }

    // Проверка уникальности
    int exists = name_exists(f, &h, name);
    if (exists < 0) { fclose(f); return -1; }
    if (exists) { fclose(f); return 0; }

    // Поиск места вставки
    int prev_off = 0, next_off = 0;
    if (find_insert_position(f, &h, name, &prev_off, &next_off) != 0) {
        fclose(f);
        return -1;
    }

    // Выделение физического места
    int new_off;
    if (h.deleted_count > 0) {
        new_off = h.first_deleted_ptr;
        Record del_rec;
        if (read_record(f, new_off, &del_rec) != 0) {
            fclose(f);
            return -1;
        }
        h.first_deleted_ptr = del_rec.next_ptr;
        if (h.first_deleted_ptr == 0) h.last_deleted_ptr = 0;
        h.deleted_count--;
    } else {
        fseek(f, 0, SEEK_END);
        new_off = ftell(f);
    }

    // Создание новой записи
    Record new_rec;
    new_rec.deleted_flag = 0;
    memset(new_rec.name, 0, NAME_LEN);
    strncpy(new_rec.name, name, NAME_LEN - 1);
    new_rec.name[NAME_LEN - 1] = '\0';
    new_rec.next_ptr = next_off;

    if (write_record(f, new_off, &new_rec) != 0) {
        fclose(f);
        return -1;
    }

    // Корректировка указателей
    if (prev_off == 0) {
        h.first_active_ptr = new_off;
    } else {
        Record prev_rec;
        if (read_record(f, prev_off, &prev_rec) != 0) {
            fclose(f);
            return -1;
        }
        prev_rec.next_ptr = new_off;
        if (write_record(f, prev_off, &prev_rec) != 0) {
            fclose(f);
            return -1;
        }
    }

    // Обновление заголовка
    h.active_count++;
    if (write_header(f, &h) != 0) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 1;
}

// Тестовый
int main() {
    const char *filename = "list.dat";

    // Добавляем элементы в произвольном порядке
    const char *names[] = {"Zebra", "Apple", "Banana", "Cherry", "Apple"};
    for (int i = 0; i < 5; ++i) {
        int res = add_sorted_element(filename, names[i]);
        if (res == 1)
            printf("Добавлен: %s\n", names[i]);
        else if (res == 0)
            printf("Дубликат: %s\n", names[i]);
        else
            printf("Ошибка при добавлении: %s\n", names[i]);
    }

    // Выводим активный список (отсортированный)
    FILE *f = fopen(filename, "rb");
    if (f) {
        Header h;
        fread(&h, sizeof(Header), 1, f);
        printf("\nАктивный список (по алфавиту):\n");
        int off = h.first_active_ptr;
        int idx = 1;
        while (off != 0) {
            Record r;
            fseek(f, off, SEEK_SET);
            fread(&r, sizeof(Record), 1, f);
            printf("%d. %s\n", idx++, r.name);
            off = r.next_ptr;
        }
        fclose(f);
    }
    return 0;
}