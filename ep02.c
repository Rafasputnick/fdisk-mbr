// Principais referencias
// https://cpl.li/posts/2019-03-12-mbrfat/
// https://wiki.osdev.org/MBR_%28x86%29
// https://en.wikipedia.org/wiki/Partition_type#:~:text=The%20partition%20type%20(or%20partition,mappings%2C%20LBA%20access%2C%20logical%20mapped

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

typedef struct Partitions {
   uint8_t status;
   char chsStart[3];
   uint8_t type;
   char chsEnd[3];
   uint32_t lbaId;
   uint32_t qntSectors;
} partition;

#define READ_BINARY "rb"
#define BOOT_SIGNATURE_INDEX 510
#define DISK_SIGNATURE_INDEX 440
#define FIRST_PARTITION_INDEX 446
#define QUANTITY_OF_PARTITIONS 4
#define START_BOLD "\x1B[1m"
#define END_BOLD "\x1B[0m"

enum errors {
   OPENING_FILE,
   MALLOC,
   READING_FILE,
   BOOT_SIGNATURE
} error;

float_t sectors_to_gb(uint32_t num) {
   return num * (512 / pow(1024, 3));
}

char *get_type_desc(uint8_t typeId) {
   switch (typeId) {
   case 0x83:
      return "Linux";
   case 0x82:
      return "Linux swap";
   default:
      return "Desconhecido";
   }
}

void print_with_length_and_clear(char str[], int len) {
   int spaces = len - strlen(str);
   printf("%s", str);
   for (int i = 0; i < spaces; i++) {
      printf(" ");
   }
   memset(str, '\0', strlen(str));
}

int main() {
   FILE *file = fopen("mbr.bin", READ_BINARY);
   if (file == NULL) {
      printf("Error: Opening file");
      exit(OPENING_FILE);
   }

   fseek(file, 0, SEEK_END);
   uint16_t fileSize = ftell(file);
   rewind(file);

   char *mbrBuffer = (char *)malloc(sizeof(char) * fileSize);
   if (mbrBuffer == NULL) {
      printf("Error: Malloc return null");
      exit(MALLOC);
   }

   size_t resultSize = fread(mbrBuffer, 1, fileSize, file);
   if (resultSize != fileSize) {
      printf("Error: Reading file");
      exit(READING_FILE);
   }

   uint16_t bootSignature = *((uint16_t *)(mbrBuffer + BOOT_SIGNATURE_INDEX));

   if (bootSignature != 0xaa55) {
      printf("Error: File without a valid boot signature");
      exit(BOOT_SIGNATURE);
   }

   uint32_t diskSignature = *((uint32_t *)(mbrBuffer + DISK_SIGNATURE_INDEX));

   int partitionIndex = FIRST_PARTITION_INDEX;
   partition *partitions = malloc(QUANTITY_OF_PARTITIONS * sizeof(partition));
   for (int i = 0; i < QUANTITY_OF_PARTITIONS; i++) {
      partition *pAux = &partitions[i];

      uint8_t cursor = 0;
      pAux->status = *(mbrBuffer + partitionIndex);
      cursor += sizeof(pAux->status);

      strncat(pAux->chsStart, (mbrBuffer + partitionIndex + cursor), 3);
      cursor += sizeof(pAux->chsStart);

      pAux->type = *((uint8_t *)(mbrBuffer + partitionIndex + cursor));
      cursor += sizeof(pAux->type);

      strncat(pAux->chsEnd, (mbrBuffer + partitionIndex + cursor), 3);
      cursor += sizeof(pAux->chsEnd);

      pAux->lbaId = *((uint32_t *)(mbrBuffer + partitionIndex + cursor));
      cursor += sizeof(pAux->lbaId);

      pAux->qntSectors = *((uint32_t *)(mbrBuffer + partitionIndex + cursor));

      partitionIndex += sizeof(partition);
   }

   fclose(file);

   float_t memoryInDisk = 0;

   for (int i = 0; i < QUANTITY_OF_PARTITIONS; i++) {
      memoryInDisk += sectors_to_gb(partitions[i].qntSectors);
   }

   memoryInDisk = round(memoryInDisk);

   printf("%s", START_BOLD);
   printf("\nDisco /dev/sda: %.f GiB, %.f bytes, %.f setores\n", memoryInDisk, memoryInDisk * pow(1024, 3), (memoryInDisk * pow(1024, 3)) / 512);
   printf("%s", END_BOLD);

   printf("Modelo de disco: VBOX HARDDISK\n");
   printf("Unidades: setor de 1 * 512 = 512 bytes\n");
   printf("Tamanho E/S (mínimo/ótimo): 512 bytes / 512 bytes\n");
   printf("Tipo de rótulo do disco: dos\n");

   printf("Identificador do disco: 0x%08x\n", diskSignature);

   printf("%s", START_BOLD);
   printf("\nDispositivo Inicializar Início    Fim       Setores   Tamanho Id Tipo\n");
   printf("%s", END_BOLD);

   char *auxText = malloc(sizeof(char) * 20);
   for (int i = 0; i < QUANTITY_OF_PARTITIONS; i++) {
      if (partitions[i].lbaId) {
         printf("/dev/sda%d   ", i + 1);
         if (partitions[i].status == 0x80) {
            printf("*           ");
         } else {
            printf("            ");
         }

         sprintf(auxText, "%d", partitions[i].lbaId);
         print_with_length_and_clear(auxText, 10);

         sprintf(auxText, "%d", partitions[i].qntSectors + partitions[i].lbaId - 1);
         print_with_length_and_clear(auxText, 10);

         sprintf(auxText, "%d", partitions[i].qntSectors);
         print_with_length_and_clear(auxText, 10);

         sprintf(auxText, "%.1fG", sectors_to_gb(partitions[i].qntSectors));
         print_with_length_and_clear(auxText, 8);

         sprintf(auxText, "%x", partitions[i].type);
         print_with_length_and_clear(auxText, 3);

         printf("%s", get_type_desc(partitions[i].type));

         printf("\n");
      }
   }

   free(auxText);
   free(partitions);
   free(mbrBuffer);
   return 0;
}