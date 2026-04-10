#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Заголовочная запись (20 байт: 5 чисел по 4 байта)
typedef struct {
    int activeCount;      // количество активных элементов
    int deletedCount;     // количество удалённых
    int firstActive;      // смещение на первый активный элемент
    int firstDeleted;     // смещение на первый удалённый элемент
    int lastDeleted;      // смещение на последний удалённый элемент
} Header;

// Запись-элемент (25 байт: 1 байт + 20 байт + 4 байта)
typedef struct {
    unsigned char deletedBit;  // 0 - активен, 1 - удалён
    char name[20];              // наименование (строка, 20 байт)
    int next;                   // указатель на следующий элемент (смещение)
} Record;

void printHelp(const char* programName) {
    printf("Использование: %s \n", programName);
    printf("Восстанавливает удалённый элемент по наименованию.\n");
    printf("Элемент исключается из очереди удалённых и включается в начало списка активных.\n");
    printf("Контролируется уникальность наименования среди активных элементов.\n");
    printf("При успешном восстановлении выводит сообщение об успехе.\n");
}

int isNameUnique(FILE* file, const Header* header, const char* name) {
    int current = header->firstActive;
    Record rec;
    
    while (current != -1) {
        fseek(file, current, SEEK_SET);
        fread(&rec, sizeof(Record), 1, file);
        
        if (rec.deletedBit == 0 && strncmp(rec.name, name, 20) == 0) {
            return 0;
        }
        current = rec.next;
    }
    return 1;
}

int findDeletedByName(FILE* file, const Header* header, const char* name, int* prevOffset) {
    int current = header->firstDeleted;
    *prevOffset = -1;
    Record rec;
    
    while (current != -1) {
        fseek(file, current, SEEK_SET);
        fread(&rec, sizeof(Record), 1, file);
        
        if (rec.deletedBit == 1 && strncmp(rec.name, name, 20) == 0) {
            return current;
        }
        *prevOffset = current;
        current = rec.next;
    }
    return -1;
}

int restoreElement(const char* filename, const char* targetName) {
    FILE* file;
    Header header;
    Record rec;
    int offsetToRestore;
    int prevDeletedOffset; 
    int result = 0;  // ← ЭТА ПЕРЕМЕННАЯ БЫЛА УТЕРЯНА

    file = fopen(filename, "rb+");
    if (file == NULL) {
        printf("Ошибка: не удалось открыть файл %s\n", filename);
        return 1;
    }
    
    // Важно: проверяем, что прочитали заголовок
    if (fread(&header, sizeof(Header), 1, file) != 1) {
        printf("Ошибка: не удалось прочитать заголовок\n");
        fclose(file);
        return 1;
    }
    
    if (header.activeCount < 0 || header.deletedCount < 0) {
        printf("Ошибка: файл имеет некорректную структуру\n");
        fclose(file);
        return 1;
    }
    
    if (!isNameUnique(file, &header, targetName)) {
        printf("Ошибка: элемент с наименованием '%s' уже существует в списке активных\n", targetName);
        fclose(file);
        return 1;
    }
    
    offsetToRestore = findDeletedByName(file, &header, targetName, &prevDeletedOffset);
    
    if (offsetToRestore == -1) {
        printf("Ошибка: удалённый элемент с наименованием '%s' не найден\n", targetName);
        fclose(file);
        return 1;
    }
    
    fseek(file, offsetToRestore, SEEK_SET);
    fread(&rec, sizeof(Record), 1, file);
    
    if (rec.deletedBit != 1) {
        printf("Ошибка: внутренняя ошибка структуры данных\n");
        fclose(file);
        return 1;
    }
    
    if (prevDeletedOffset == -1) {
        header.firstDeleted = rec.next;
    } else {
        Record prevRec;
        fseek(file, prevDeletedOffset, SEEK_SET);
        fread(&prevRec, sizeof(Record), 1, file);
        prevRec.next = rec.next;
        fseek(file, prevDeletedOffset, SEEK_SET);
        fwrite(&prevRec, sizeof(Record), 1, file);
    }
    
    if (header.lastDeleted == offsetToRestore) {
        header.lastDeleted = prevDeletedOffset;
    }
    
    rec.deletedBit = 0;
    rec.next = header.firstActive;   
    
    fseek(file, offsetToRestore, SEEK_SET);
    fwrite(&rec, sizeof(Record), 1, file);
    
    header.firstActive = offsetToRestore;
    header.activeCount++;
    header.deletedCount--;
    
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(Header), 1, file);
    
    printf("Успешно: элемент '%s' восстановлен и добавлен в начало списка активных\n", targetName);
    printf("Активных элементов: %d, Удалённых элементов: %d\n", header.activeCount, header.deletedCount);
    
    result = 0;
    
cleanup:  // ← ЭТА МЕТКА ТОЖЕ ВАЖНА (хотя здесь она не строго необходима, но сохраняем структуру)
    fclose(file);
    return result;
}

int main(int argc, char* argv[]) 
{
    if (argc != 3) {
        printHelp(argv[0]);
        return 1;
    }
    return restoreElement(argv[1], argv[2]);
}
