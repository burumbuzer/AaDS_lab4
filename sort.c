#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#pragma pack(push, 1)
typedef struct {
    int32_t active_count;
    int32_t deleted_count;
    int32_t head_active;
    int32_t head_deleted;
    int32_t tail_deleted;
} Header;

typedef struct {
    uint8_t is_deleted;
    char name[20];
    int32_t next_ptr;
} Node;
#pragma pack(pop)

typedef struct {
    int32_t offset;
    char name[20];
} NodeInfo;

int compare_nodes(const void* a, const void* b) {
    return strncmp(((NodeInfo*)a)->name, ((NodeInfo*)b)->name, 20);
}

void print_list(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return;
    Header hdr;
    fread(&hdr, sizeof(Header), 1, f);
    printf("\nТекущий список (head: %d)\n", hdr.head_active);
    int32_t curr = hdr.head_active;
    //роверка на -1
    while (curr != -1) {
        Node n;
        fseek(f, curr, SEEK_SET);
        if (fread(&n, sizeof(Node), 1, f) != 1) break;
        printf("Offset: %3d  Name: %-20.20s  Next: %d\n", curr, n.name, n.next_ptr);
        curr = n.next_ptr;
    }
    fclose(f);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Программа логической сортировки списка в бинарном файле.\n");
        printf("Использование: sort.exe <имя_файла>\n");
        printf("Параметры: имя_файла - путь к файлу БД.\n");
        return 1;
    }

    printf("До сортировки:");
    print_list(argv[1]);

    FILE* f = fopen(argv[1], "rb+");
    if (!f) {
        printf("Ошибка открытия файла\n");
        return 1;
    }

    Header hdr;
    fread(&hdr, sizeof(Header), 1, f);

    if (hdr.active_count > 1) {
        NodeInfo* list = malloc(sizeof(NodeInfo) * hdr.active_count);
        int32_t current_ptr = hdr.head_active;
        
        //цикл до -1
        for (int i = 0; i < hdr.active_count && current_ptr != -1; i++) {
            Node temp;
            fseek(f, current_ptr, SEEK_SET);
            fread(&temp, sizeof(Node), 1, f);
            list[i].offset = current_ptr;
            memcpy(list[i].name, temp.name, 20);
            current_ptr = temp.next_ptr;
        }

        qsort(list, hdr.active_count, sizeof(NodeInfo), compare_nodes);

        hdr.head_active = list[0].offset;
        for (int i = 0; i < hdr.active_count; i++) {
            int32_t next = (i == hdr.active_count - 1) ? -1 : list[i + 1].offset;
            fseek(f, list[i].offset + offsetof(Node, next_ptr), SEEK_SET);
            fwrite(&next, sizeof(int32_t), 1, f);
        }
        fseek(f, 0, SEEK_SET);
        fwrite(&hdr, sizeof(Header), 1, f);
        free(list);
    }
    fclose(f);

    printf("\nПосле сортировки:");
    print_list(argv[1]);

    return 0;
}