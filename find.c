#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct file_header {
    int active;
    int deleted;
    int offset_active;
    int offset_first_deleted;
    int offset_last_deleted;
};

struct file_record {
    char is_deleted;
    char name[20];
    int offset_next;
};

int search_by_name_prefix(const char* filename, const char* prefix) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        perror("Ошибка открытия файла");
        return -1;
    }
    struct file_header header;
    // Читаем заголовочную запись
    if (fread(&header, sizeof(struct file_header), 1, fp) != 1) {
        perror("Ошибка чтения заголовочной записи");
        fclose(fp);
        return -1;
    }
    if (header.active == 0) {
        printf("Список активных элементов пуст.\n");
        fclose(fp);
        return 0;
    }

    int current_pos = header.offset_active;
    int found_number = 1;  //Нумерация найденных элементов
    int total_found = 0;  //Общее количество найденных элементов

    printf("Результаты поиска (наименование начинается с \"%s\"):\n", prefix);
    printf("№\tНаименование\n");
    printf("----------------\n");

    while (current_pos != -1) {
        //Перемещаемся к текущему элементу
        if (fseek(fp, current_pos, SEEK_SET) != 0) {
            break;
        }

        struct file_record element;

        if (fread(&element, sizeof(struct file_record), 1, fp) != 1) {
            perror("Ошибка чтения элемента");
            break;
        }

        //Проверяем что элемент не удалён
        if (element.is_deleted == 0) {
            //Проверяем начало наименования
            if (strncmp(element.name, prefix, strlen(prefix)) == 0) {
                printf("%d\t%s\n", found_number, element.name);
                found_number++;
                total_found++;
            }
        }

        //Переходим к следующему элементу
        current_pos = element.offset_next;
    }

    if (total_found == 0) {
        printf("Элементы, начинающиеся с \"%s\", не найдены.\n", prefix);
    } else {
        printf("\nВсего найдено: %d элементов\n", total_found);
    }

    fclose(fp);
    return 0;
}

int main(int argc, char* argv[]) {
    //Проверка параметров командной строки
    if (argc != 3) {
        printf("Usage: %s [FILENAME] [DATA]\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    const char* search_prefix = argv[2];

    //Выполняем поиск
    if (search_by_name_prefix(filename, search_prefix) != 0) {
        return 1;
    }

    return 0;
}
