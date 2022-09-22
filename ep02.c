// https://cpl.li/posts/2019-03-12-mbrfat/ 
// https://wiki.osdev.org/MBR_%28x86%29

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <math.h>

typedef struct Partitions{
   uint8_t status;
   char chsStart[3]; // 3 bytes
   uint8_t type; 
   char chsEnd[3]; // 3 bytes
   uint32_t lba;
   uint32_t qntSectors;
} partition;

#define READ_BINARY "rb"
#define BOOT_SIGNATURE_INDEX 510
#define DISK_SIGNATURE_INDEX 440
#define FIRST_PARTITION_INDEX 446
#define QUANTITY_OF_PARTITIONS 4
#define START_BOLD "\x1B[1m"
#define END_BOLD "\x1B[0m"



enum errors {OPENING_FILE, MALLOC, READING_FILE, BOOT_SIGNATURE} error;

float_t sectors_to_gb(uint64_t num){
   return num * (512 / pow(1024, 3));
}

void print_with_length_and_clear(char str[], int len){
   int spaces = len - strlen(str);
   printf("%s", str);
   for(int i = 0; i < spaces; i++){
      printf(" ");
   }
   memset(str, '\0', sizeof(str));
}

int main () {
  FILE * file = fopen("mbr.bin", READ_BINARY);
  if(file == NULL) {
      printf("Error: Opening file");
      exit(OPENING_FILE);
   }

  fseek(file , 0 , SEEK_END);
  uint16_t fileSize = ftell(file);
  rewind(file);

  char * mbrBuffer = (char*) malloc(sizeof(char)*fileSize);
  if(mbrBuffer == NULL) {
      printf("Error: Malloc return null");
      exit(MALLOC);
   }

  size_t resultSize = fread(mbrBuffer, 1, fileSize, file);
  if(resultSize != fileSize) {
      printf("Error: Reading file");
      exit(READING_FILE);
   }

   uint16_t bootSignature = *( (uint16_t *) &mbrBuffer[BOOT_SIGNATURE_INDEX] ); 
   // printf("Boot signature: %04x\n", bootSignature);
   // printf("Patition type is %02X check in https://en.wikipedia.org/wiki/Partition_type\n", partitions[0].type);


   if(bootSignature != 0xaa55) {
      printf("Error: File without a valid boot signature");
      exit(BOOT_SIGNATURE);
   } 

   uint32_t diskSignature = *( (uint32_t *) &mbrBuffer[DISK_SIGNATURE_INDEX] ); 

   int partitionFileIndex = FIRST_PARTITION_INDEX;
   partition * partitions = malloc(QUANTITY_OF_PARTITIONS + sizeof(partition));
   for(int i = 0; i < QUANTITY_OF_PARTITIONS; i++) {
      partition * pAux = &partitions[i];
      
      uint8_t cursor = 0;
      pAux->status = *(mbrBuffer + partitionFileIndex);
      cursor += sizeof(pAux->status);

      strncat(pAux->chsStart, (mbrBuffer + partitionFileIndex + cursor), 3); 
      cursor += sizeof(pAux->chsStart);

      pAux->type = *( (uint8_t *) (mbrBuffer + partitionFileIndex + cursor)); 
      cursor += sizeof(pAux->type);

      strncat(pAux->chsEnd, (mbrBuffer + partitionFileIndex + cursor), 3); 
      cursor += sizeof(pAux->chsEnd);
      
      pAux->lba = *( (uint32_t *) (mbrBuffer + partitionFileIndex + cursor));
      cursor += sizeof(pAux->lba);
      
      pAux->qntSectors = *( (uint32_t *) (mbrBuffer + partitionFileIndex + cursor));

      partitionFileIndex += sizeof(partition);
   }

  fclose(file);
   
   float_t memoryInDisk = 0;

   for(int i = 0; i < QUANTITY_OF_PARTITIONS; i++) {
      uint64_t qntdSectors = (uint64_t) (partitions[i].qntSectors);
      memoryInDisk += sectors_to_gb(qntdSectors);
   }

   memoryInDisk = round(memoryInDisk);

   char mystr[10]; 
 
   sprintf(mystr, "Disco /dev/sda: %.f GiB", memoryInDisk);  

   printf("%s", START_BOLD);
   printf("\nDisco /dev/sda: %.f GiB, %.f bytes, %.f setores\n", memoryInDisk, memoryInDisk * pow(1024, 3), (memoryInDisk * pow(1024, 3)) / 512);
   printf("%s", END_BOLD);

   printf("Unidades: setor de 1 * 512 = 512 bytes\n");
   printf("Tamanho E/S (mínimo/ótimo): 512 bytes / 512 bytes\n");
   printf("Tipo de rótulo do disco: dos\n");
   printf("Identificador do disco: 0x%08x\n", diskSignature);
 
   printf("%s", START_BOLD);
   printf("\nDispositivo Inicializar Início    Fim       Setores   Tamanho Id \n");
   printf("%s", END_BOLD);
   
   char * auxText = malloc(sizeof(char) * 20);
   for(int i = 0; i < QUANTITY_OF_PARTITIONS; i++) {
      if (partitions[i].lba ) {
         printf("/dev/sda%d   ", i + 1);
         if (partitions[i].status == 0x80){
            printf("*           ");
         } else {
            printf("            ");
         }


         sprintf(auxText, "%d", partitions[i].lba);
         print_with_length_and_clear(auxText, 10);


         sprintf(auxText, "%d", partitions[i].qntSectors + partitions[i].lba - 1);
         print_with_length_and_clear(auxText, 10);

         
         sprintf(auxText, "%d", partitions[i].qntSectors);
         print_with_length_and_clear(auxText, 10);

         
         sprintf(auxText, "%.1fG", sectors_to_gb(partitions[i].qntSectors));
         print_with_length_and_clear(auxText, 8);

         sprintf(auxText, "%x", partitions[i].type);
         print_with_length_and_clear(auxText, 3);

         printf("\n");
      }
   }

   free(auxText);
  free(mbrBuffer);
  return 0;
}