#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#pragma pack(push, 1)

struct Header{
    int active_count;
    int deleted_count;
    int first_active;
    int first_deleted;
    int last_deleted;
};

struct  Record{
    unsigned char is_del;
    char name[20];
    int next;
};

#pragma pack(pop)

// Функция создания тестового файла с 5 записями
void create_test_file(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("Не удалось открыть файл\n");
        return;
    }
    
    Header h;
    h.active_count = 5;
    h.deleted_count = 0;
    h.first_active = sizeof(Header);
    h.first_deleted = -1;
    h.last_deleted = -1;
    
    fwrite(&h, sizeof(Header), 1, f);
    Record r1 = {0, "nstu", sizeof(Header) + sizeof(Record)};              
    Record r2 = {0, "house", sizeof(Header) + sizeof(Record) * 2};     
    Record r3 = {0, "world", sizeof(Header) + sizeof(Record) * 3};    
    Record r4 = {0, "hello", sizeof(Header) + sizeof(Record) * 4};
    Record r5 = {0, "cat", -1};
    
    fwrite(&r1, sizeof(Record), 1, f);
    fwrite(&r2, sizeof(Record), 1, f);
    fwrite(&r3, sizeof(Record), 1, f);
    fwrite(&r4, sizeof(Record), 1, f);
    fwrite(&r5, sizeof(Record), 1, f);

    fclose(f);
    printf("Файл '%s' создан\n", filename);
}

void info(const char *filename, int mode) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("Не удалость открыть файл\n");
        return;
    }
    
    Header h;
    fread(&h, sizeof(Header), 1, f);
    
    if (mode == 1) {
        // Вывод списка активных
        printf("\nАктивный список\n");
        if (h.first_active == -1) {
            printf("Пусто\n");
        } else {
            int offset = h.first_active;
            Record r;
            while (offset != -1) {
                fseek(f, offset, SEEK_SET);
                fread(&r, sizeof(Record), 1, f);
                printf("Смещение %d: is_del=%d, имя=%s, next=%d\n", offset, r.is_del, r.name, r.next);
                offset = r.next;
            }
        }
    }
    else if (mode == 2) {
        // Вывод списка удалённых
        printf("\nУдаленный список\n");
        if (h.first_deleted == -1) {
            printf("Пусто\n");
        } else {
            int offset = h.first_deleted;
            Record r;
            while (offset != -1) {
                fseek(f, offset, SEEK_SET);
                fread(&r, sizeof(Record), 1, f);
                printf("Смещение %d: is_del=%d, имя=%s, next=%d\n", offset, r.is_del, r.name, r.next);
                offset = r.next;
            }
        }
    }
    else if (mode == 3) {
        // Вывод служебной информации
        printf("\nВся информация\n");
        printf("\nЗаголовок\n");
        printf("Число активных: %d\n", h.active_count);
        printf("Число удаленных: %d\n", h.deleted_count);
        printf("Первый активный: %d\n", h.first_active);
        printf("Первый удаленный: %d\n", h.first_deleted);
        printf("Последний удаленный: %d\n\n", h.last_deleted);
        
        Record r;
        int offset = sizeof(Header);
        fseek(f, offset, SEEK_SET);
        
        while (fread(&r, sizeof(Record), 1, f) == 1) {
            printf("Смещение %d: is_del=%d, имя=%s, next=%d\n", offset, r.is_del, r.name, r.next);
            offset += sizeof(Record);
            fseek(f, offset, SEEK_SET);
        }
    }
    else {
        printf("Ошибка. Выберите режим: 1 - Активный спиок, 2 - Удаленный список, 3 - Вся информация\n");
    }
    
    fclose(f);
}

int main(int argc, char* argv[]) {
    // create_test_file("f.dat");
    if (argc < 3) {
        printf("Использование: %s <имя_файла> <режим>\n", argv[0]);  
        printf("Режимы:\n");
        printf("1 - Активный список\n");
        printf("2 - Удаленный список\n");
        printf("3 - Вся информация\n");
        return 1;
    }
    
    const char* filename = argv[1];
    int mode = atoi(argv[2]);  

    printf("Информация о файле %s:\n", filename);
    info(filename, mode);
    
    return 0;
}
