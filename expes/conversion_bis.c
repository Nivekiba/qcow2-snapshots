/* declaration des bibliotheques utiles */
#include<stdlib.h>
#include<stdio.h>
#include<endian.h>
#include<glib.h>  //pour les appels a gnew
#include<stdint.h>  //pour les types unsigned int 
#include<assert.h>  //pour les appels a la fonction assert
#include<sys/stat.h> //pour la fonction fstat
#include<string.h> // pour la fonction strcpy

/*Declaration des masques utiles pour recuperer certaines informations*/
#define L1_ENTRY_OFFSET_MASK 0x00fffffffffffe00ULL //masque pour recuperer l'offset d'une entre L1
#define REFCOUNT_TABLE_ENTRY_OFFSET_MASK 0xfffffffffffffe00ULL  //masque pour recuperer l'offset d'une entree de la refcount table
#define REFCOUNT_BLOCK_ENTRY_MASK 0xffff  //recupere le refcount d'une entree d'un refcount block. WARNING: Valeur par defaut, 16bits
#define L1_ENTRY_REFCOUNT_MASK (1ULL<<63)  //recuperation du refcount d'une entree L1
#define L2_ENTRY_OFFSET_MASK 0x00fffffffffffe00ULL //recuperation de l'offset d'une entree L2
#define L2_ENTRY_ISZEROS_MASK (1ULL)  //recuperation du bit 0 d'une entree L2, signification confert documentation
#define L2_ENTRY_REFCOUNT_MASK (1ULL<<63)  //recuperation du refcount d'une entre L2
#define L2_COMPRESSED_MASK (1ULL<<62)  //recuperation du bit 62 d'une entree L2
#define L2_ENTRY_B63_MASK 0x8000000000000000ULL //permet de reuperer la valeur du bit 63 de l'entree L2
#define INCOMPATIBLE_FEATURES_MASK 0x000000000000001fULL //permet de recuperer les bits de 0-4 de incompatible features
#define BITS_POIDS_FAIBLE_MASK 0xff //permet de recupérer les bits 0-7 de l'index que l'on veut update dans l'entrée L2
#define BITS_POIDS_FORT_MASK 0x3f00 ////permet de recupérer les bits 8-13 de l'index que l'on veut update dans l'entrée L2
#define BITS_14_15_NULS_MASK 0xc000
#define OLD_L2_ENTRY_MASK 0xc0fffffffffffe01ULL
#define HIGH_BITS_L2_ENTRY_MASK 0x3f00000000000000ULL
#define LOW_BITS_L2_ENTRY_MASK 0x00000000000001feULL
#define SET_TO_ONE_BIT_63_MASK 0x8000000000000000ULL
#define QCOW2_EXTERN_SNAPSHOT_OFFSET_MASK 0xfffffffffffffffc
#define DEFAULT_OUTPUT_NAME "output.txt"  //fichier par defaut des resultats si non specifie en parametres

/* This structure is mostly taken from qemu code */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t backing_file_offset;
    uint32_t backing_file_size;
    uint32_t cluster_bits;
    uint64_t size;
    uint32_t crypt_method;
    uint32_t l1_size;
    uint64_t l1_table_offset;
    uint64_t refcount_table_offset;
    uint32_t refcount_table_clusters;
    uint32_t nb_snapshots;
    uint64_t snapshots_offset;

    /* The following fields are only valid for version >= 3 */
    uint64_t incompatible_features;
    uint64_t compatible_features;
    uint64_t autoclear_features;

    uint32_t refcount_order;
    uint32_t header_length;

    /* Additional fields */
    uint8_t compression_type;

    /* header must be a multiple of 8  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    uint8_t padding[7];
} QCowHeader;

/* Use an extra structure to store only useful or transformed header 
 * information that we will need */
typedef struct {

    uint64_t nb_guest_clusters;
    uint32_t l1_size;
    uint64_t l1_table_offset;
    uint64_t refcount_table_offset;  //offset dans le fichier a partir duquel est stocke la refcount table
    uint32_t refcount_table_clusters; //nombre de clusters occupes par la refcount table
    uint32_t cluster_size;
    uint32_t nb_l2_entries;
    uint32_t nb_refcount_table_entries; //nombre d'entrées par refcount table
    uint32_t nb_refcount_block_entries; //nombre d'entrees par refcount block
    uint64_t backing_file_offset;
    uint32_t backing_file_size;
    uint64_t incompatible_features; //we add this field which stores the position of the file in the chain
    uint64_t autoclear_features;
    uint16_t index_in_chain; //position of the file in the chain. Base file has the index 0
    uint64_t refcount_bits; // the number of bits of an refcount block entry
    uint64_t size; // virtual disk size in bytes

} UsefulHeader;

/* This is to collect the information stored in a L1 entry */
typedef struct {    
    uint64_t offset;
    uint8_t refcount;
} L1Entry ;

/* Same for a L2 entry */
typedef struct {
    uint8_t is_zeros;
    uint64_t offset;
    uint8_t is_compressed;
    uint8_t refcount;
    uint8_t b63_is_clear;  // ==1 if bit 63 is clear !!!!!!!!!!!!!!!!!!! clear == 0??
    uint8_t b0_is_clear;  // ==1 if bit 0 is clear !
    uint16_t last_snapshot_index;  //the index of the last snapshot where the cluster had been modified
} L2Entry ;

/* This is to collect the information stored in a Refcount table entry */
typedef struct 
{
    uint64_t offset;
}RefcountTableEntry;

/* This is to collect the information stored in a Refcount block entry */
typedef struct 
{
    uint16_t refcount;
}RefcountBlockEntry;

/* Get the length of a string */
uint32_t str_len(char *str){

    uint32_t result = 0;
    while(str[result] != '\0')
        ++ result;
    return result;
}

/* Take a full path to a file, split it and collect the path (everything until
 * the last '/') and the actual name (what is after). 
 * For instance, on "/home/username/code.c, this will store "/home/username/" 
 * in *path and "code.c" in *name . */
void split_path(char* full_path, char** path, char** name){
    
    uint32_t last_bs = 0;
    uint32_t length = 0;
    /* Get the position of the last backslash and the total length of full_path */
    while(full_path[length] != '\0'){
        if(full_path[length] == '/')
            last_bs = length;
        ++ length;
    }
    
    *path = (char *) malloc((last_bs + 2) * sizeof(char));
    *name = (char *) malloc((length - last_bs) * sizeof(char));
    
    uint32_t i = 0;
    while(i <= last_bs){
        (*path)[i] = full_path[i];
        ++ i;
    }
    (*path)[i] = '\0';
    while(i < length){
        (*name)[i - last_bs - 1] = full_path[i];
        ++ i;
    }
    (*name)[i - last_bs - 1] = '\0';

    return ;
}

/* Merge two strings. Used to build paths of manipulated files.
 * result should be already malloc'ed */
void merge_strings(char *first, char* second, char *result){
    
    uint32_t first_length = str_len(first);
    uint32_t second_length = str_len(second);

    for(uint32_t i = 0 ; i < first_length ; ++ i)
        result[i] = first[i];
    for(uint32_t i = 0 ; i < second_length ; ++ i)
        result[i + first_length] = second[i];
    result[first_length + second_length] = '\0';

    return ;
}

/* This reads the beginning of the header and collects 
 * all the information we need. There are plenty of verification
 * that could be done here to make sure that the file is not
 * corrupted. 
 * See the qcow2 format specification for details about what composes
 * this header, and what are the information that are in the header but
 * we don't read here.
 * Note that everything in the qcow2 file is in big-endian, thus we use
 * <endian.h> functions to do all the swapping. */
void read_header(FILE *f, UsefulHeader *target_header){
    /* Reset the file stream at the beginning of the file */
    fseek(f, 0, SEEK_SET);

    QCowHeader *header = g_new(QCowHeader, 1);
    /* First, store in header the first 104 bytes of the file. The information
     * is correct for qcow2 version 2 and 3 for bytes 0-71, and only for version 3
     * for bytes 72-103 */
    int result = fread(header, 1, 104, f);
    if(!result){
        printf("Failed to fetch header data. Exiting...\n");
        return ;
    }

    /* Header values are stored in big endian. Change that to what is used
     * by the cpu */
    header->magic = be32toh(header->magic);
    header->version = be32toh(header->version);
    header->backing_file_offset = be64toh(header->backing_file_offset);
    header->backing_file_size = be32toh(header->backing_file_size);
    header->cluster_bits = be32toh(header->cluster_bits);
    header->size = be64toh(header->size);
    header->crypt_method = be32toh(header->crypt_method);
    header->l1_size = be32toh(header->l1_size);
    header->l1_table_offset = be64toh(header->l1_table_offset);
    header->refcount_table_offset = be64toh(header->refcount_table_offset);
    header->refcount_table_clusters =
        be32toh(header->refcount_table_clusters);
    header->nb_snapshots = be32toh(header->nb_snapshots);
    header->snapshots_offset = be64toh(header->snapshots_offset);

    /* If we use version 3, swap the remaining bytes as well.
     * Otherwise, fill these fields with default values. */
    if(header->version == 3){
        header->incompatible_features = be64toh(header->incompatible_features);
        header->compatible_features = be64toh(header->compatible_features);
        header->autoclear_features = be64toh(header->autoclear_features);
        header->refcount_order = be32toh(header->refcount_order);
        header->header_length = be32toh(header->header_length);
    }
    else{
        header->incompatible_features = 0;
        header->compatible_features = 0;
        header->autoclear_features = 0;
        header->refcount_order = 4;
        header->header_length = 72;
        
    }

    
    /* Now we fill our UsefulHeader structure with the information from
     * QCowHeader */
    
    target_header->cluster_size = 1 << header->cluster_bits;
    target_header->nb_guest_clusters = header->size / (uint64_t) target_header->cluster_size;
    target_header->l1_size = header->l1_size;
    target_header->l1_table_offset = header->l1_table_offset;
    target_header->refcount_table_offset = header->refcount_table_offset;
    target_header->refcount_table_clusters = header->refcount_table_clusters;
    target_header->nb_l2_entries = target_header->cluster_size / (uint32_t) (sizeof(uint64_t)); // XXX beware of extended l2 entries, see the documentation to get what I am talking about
    target_header->backing_file_offset = header->backing_file_offset;
    target_header->backing_file_size = header->backing_file_size;
    target_header->incompatible_features = header->incompatible_features;
    target_header->autoclear_features = header->autoclear_features;
    target_header->index_in_chain = (target_header->incompatible_features) >> 5;
    target_header->refcount_bits = 1 << header->refcount_order;
    target_header->size = header->size;
    target_header->nb_refcount_block_entries = (8*target_header->cluster_size)/target_header->refcount_bits;
    target_header->nb_refcount_table_entries = target_header->refcount_table_clusters * target_header->nb_l2_entries;

    /* Free the QCowHeader as we don't need it anymore */
    free(header);

    return ;
}



/* Read in the file the name of its backing file, and write it
 * in *name. Space for the name should already be allocated.
 * Returns 1 if the current file actually has a backing file, 0 if not
 * or if an error happened.
 * header is the header of the current file. */
int get_backing_file_name(FILE *f, UsefulHeader *header, char *name){
    
    uint64_t offset = header->backing_file_offset;

    /* If there is no backing file, offset is 0 */
    if(!offset){
        *name = '\0'; 
        return 0 ;
    }

    /* Place the cursor at the offset were we will find the backing file name */
    //printf("%s %lu\n", __func__, offset);
    if(fseek(f, offset, SEEK_SET)){
        printf("Could not set file pointer to backing file name offset.\n");
        return 0 ;
    }

    if(!fread(name, 1, header->backing_file_size, f)){
        printf("Could not read backing file name\n");
        return 0 ;
    }
    else
        name[header->backing_file_size] = '\0';

    return 1 ;       

}

/* cette fonction permet de compter le nombre de snapshots d'un disque
 * elle prend en parametre la derniere snapshot
 * et renvoie le nombre de snapshots
 * la position tient sur 16 bits dans le header
*/
uint16_t nb_snapshots(FILE *f, UsefulHeader *header, char *path_to_input_file){
    
    char *backing_file_name = malloc(1024*sizeof(char));
    char *full_path = malloc((str_len(path_to_input_file) + 1024)*sizeof(char));
    uint8_t status = get_backing_file_name(f, header, backing_file_name);
    //si f n'a pas de backing file
    if(!status){
        return 0;
    }else{
        merge_strings(path_to_input_file, backing_file_name, full_path);
        //open the backing file
        
        FILE *backfile = fopen(full_path, "rb");
        //read the header of the backing file
        UsefulHeader *header_backfile = g_new(UsefulHeader, 1);
        read_header(backfile, header_backfile);

        return 1+nb_snapshots(backfile, header_backfile, path_to_input_file);
    }
}

/* Load the l1 table in memory at the adress given by target.
 * target should be already malloc'ed to the correct size. */
void load_l1_table(FILE *f, UsefulHeader *header, uint64_t *target){
    
    //printf("%s %lu\n", __func__, header->l1_table_offset);
     fseek(f, header->l1_table_offset, SEEK_SET);
    fread(target, sizeof(uint64_t), header->l1_size, f);
    return ;
}

/* Load the entry of the L1 table stored at position index, get the information
 * thanks to masks defined at beginning of this file, and store the information 
 * in the L1Entry object pointed by l1_entry.
 * XXX Warning: there is an inconsistence between the qcow2 format specification
 * and what the Qemu code actually does. For this reason, while the 
 * documentation says the refcount bit is bit 63, we read bit 0. */
void load_l1_entry(uint64_t *l1_table, uint32_t index, L1Entry *l1_entry){
    
    uint64_t raw_entry = be64toh(l1_table[index]) ;
    //printf("l1_entry = %lx \n", raw_entry);
    l1_entry->offset = raw_entry & L1_ENTRY_OFFSET_MASK ;
    l1_entry->refcount = raw_entry & L1_ENTRY_REFCOUNT_MASK ? 1 : 0 ;

    return ;
}

/* Load a whole L2 table cluster in the allocated space pointed by target. */
void load_l2_table(FILE *f, uint64_t offset, UsefulHeader *header, uint64_t *target){
    
    //printf("%s %lu\n", __func__, offset);
    fseek(f, offset, SEEK_SET);
    fread(target, 1, header->cluster_size, f);

    return ;
}

/* Load the entry of the L2 table stored at position index and store it in 
 * the L2Entry object pointed by l2_entry.
 * XXX Warning: same than the one for L1 entries. */
void load_l2_entry(uint64_t *l2_table, uint32_t index, L2Entry *l2_entry){
    //TEST
    //uint64_t raw_test = l2_table[index];
    //printf("raw_test = %lu \t", raw_test);
    //
    uint64_t raw_entry = be64toh(l2_table[index]);
    //printf("raw_big_endian = %lu \n", raw_entry);
    l2_entry->is_zeros = raw_entry & L2_ENTRY_ISZEROS_MASK ? 1 : 0 ;
    l2_entry->offset = raw_entry & L2_ENTRY_OFFSET_MASK ;
    l2_entry->is_compressed = raw_entry & L2_COMPRESSED_MASK ? 1 : 0 ;
    l2_entry->refcount = raw_entry & L2_ENTRY_REFCOUNT_MASK ? 1 : 0 ;
    l2_entry->b63_is_clear = raw_entry & L2_ENTRY_B63_MASK ? 0 : 1;
    l2_entry->b0_is_clear = raw_entry & 1 ? 0 : 1;
    //chargement du last_snapshot_index
    uint64_t high_bits = raw_entry & HIGH_BITS_L2_ENTRY_MASK;
    uint64_t low_bits = raw_entry & LOW_BITS_L2_ENTRY_MASK;
    low_bits = low_bits >> 1;
    high_bits = high_bits >> 48;  // 48 = 56 - 8
    l2_entry->last_snapshot_index = (uint16_t)(high_bits+low_bits);

    return ;
}

/* charge la refcount table en memoire a l'adresse indiquee par target.
 * target doit etre correctement alloue au prealable */
void load_refcount_table(FILE *f, UsefulHeader *header, uint64_t *target){
    
    fseek(f, header->refcount_table_offset, SEEK_SET);
    fread(target, header->cluster_size, header->refcount_table_clusters, f);
    return ;
}

/* Charge l'entree de la refcount table stockee a la position index
 * recupere les informations grace au masque associe
 */
void load_refcount_table_entry(uint64_t *refcount_table, uint32_t index, RefcountTableEntry *refcount_table_entry){
    
    uint64_t raw_entry = be64toh(refcount_table[index]) ;

    refcount_table_entry->offset = raw_entry & REFCOUNT_TABLE_ENTRY_OFFSET_MASK;

    return ;
}

/* Load a whole refcount block in the allocated space pointed by target. */
void load_refcount_block(FILE *f, uint64_t offset, UsefulHeader *header, uint16_t *target){
    
    fseek(f, offset, SEEK_SET);
    fread(target, header->cluster_size, 1, f);

    return ;
}

/* Load the entry of the refcount block stored at position index and store it in 
 * the RefcountBlockEntry object pointed by refcount_block_entry.
 */
void load_refcount_block_entry(uint16_t *refcount_block, uint32_t index, RefcountBlockEntry *refcount_block_entry){
    
    uint16_t raw_entry = be64toh(refcount_block[index]);
    refcount_block_entry->refcount = raw_entry & REFCOUNT_BLOCK_ENTRY_MASK;
    return ;
}


void l1_entry_update_file(FILE *f, UsefulHeader *h, uint32_t l1_index, L1Entry *l1_entry){
    uint64_t result;

    //preparation de la nouvelle entree a ecrire
    result = l1_entry->offset;

    //on force le bit 63 à 1
    //result = result | SET_TO_ONE_BIT_63_MASK;
    //result = result << 9;
    result = htobe64(result);
    //positionnement au bon endroit dans le fichier 
    //printf("h->l1_table_offset = %lu et file_index = %u \n", h->l1_table_offset, h->index_in_chain);
    fseek(f, (8*l1_index) + h->l1_table_offset, SEEK_SET);
    
    //mise a jour du fichier
    uint64_t *result_p = &result;
    //printf("result=%lu     result_p=%lu index_in_chain=%u \n", result, *result_p, h->index_in_chain);
    if( fwrite(result_p, 8, 1, f) != 1 ){
        perror("ERROR while updating the file \n");
    }
    //synchronisation
    if ( fflush( f ) ) {
            printf( "Cannot flush the stream\n" );
            //break;
    }    
}

/*Cette fonction met a jour la nouvelle entree de la refcount table
 *apres allocation d'un nouveau refcount block
*/
void refcount_table_entry_update_file(FILE *f, UsefulHeader *h, uint32_t refcount_table_index, RefcountTableEntry *refc_table_entry){
    //preparation de l'entree
    uint64_t raw = refc_table_entry->offset;
    raw = raw << 9;
    raw = htobe64(raw);
    uint64_t *raw_p = &raw;
    //positionnement correct dans le fichier pour ecriture
    uint64_t final_file_offset = (refcount_table_index*8) + h->refcount_table_offset;
    assert(!(refc_table_entry->offset % h->cluster_size));
    fseek(f, final_file_offset, SEEK_SET);  
    //ecriture dans le fichier
    if( fwrite(raw_p, sizeof(uint64_t), 1, f ) != 1 ){
        perror("Error while updating file");
    }
    if ( fflush( f ) ) {
            printf( "Cannot flush the stream\n" );
            //break;
    }

}

/* Read the whole chain and insert the correct index of each file in the appropriate zone in header
 * base file has the index 0
 * the zone concerned in the header is incompatible_features (Byte 72-79)
 * we use the bits 5-20 (16 bits)
*/
 void index_in_chain(char *input_file){
    
    char *path_to_input_file, *input_file_name; 
    split_path(input_file, &path_to_input_file, &input_file_name);

    
    /* We know from qcow2 format that the name won't exceed 1023 bytes */
    char *current_file_name = malloc(sizeof(char)*1024);
    /* Set current_file_name to the input file name */
    for(int i = 0 ; i < 1024 ; ++i){
        current_file_name[i] = input_file_name[i];
        if(current_file_name[i] == '\0')
            break;
    }
    
    char *full_path_to_current_file = malloc((str_len(path_to_input_file) + 1024)*sizeof(char));
    /* Set full_path_to_current_file to the input file path */
    int i = 0;
    while(input_file[i] != '\0'){
        full_path_to_current_file[i] = input_file[i];
        ++ i;
    }
    full_path_to_current_file[i] = '\0';


    /* Open the first datafile of the chain
     * 'r+' mode in order to read and write
     */
    FILE *current_file = fopen(full_path_to_current_file, "rb+");
    if(current_file == NULL){
        printf("\n Echec de l'ouverture du current_file \n");
    }

    /* Read the header of the input file */
    UsefulHeader *header = g_new(UsefulHeader, 1);
    read_header(current_file, header);



     //TEST FONCTION NB_SNAPSHOTS
    //printf("nombre de snapshots = %lu \n", nb_snapshots(current_file, header, path_to_input_file));
    int y = nb_snapshots(current_file, header, path_to_input_file);

    /* Iterate on the chain.
     * get_backing_file_name will return 0 when we reach the end of the chain, 
     * so the loop ends when no backing file is found. */
    //position du fichier dans la chaine
   
    uint64_t result_incompatible_features;

    while(y>=0){    
        result_incompatible_features = (header->incompatible_features) & INCOMPATIBLE_FEATURES_MASK;
        /*decalage a gauche de y 5 fois 
         *pour pouvoir ensuite inserer les bits 0-5 initiaux
        */
        uint16_t z = y << 5;
        //valeur finale a ecrire dans le fichier
        header->incompatible_features = z | result_incompatible_features;
        header->autoclear_features |= QCOW2_EXTERN_SNAPSHOT_OFFSET_MASK ;
        //conversion en big endian
        header->incompatible_features = htobe64(header->incompatible_features);
        header->autoclear_features = htobe64(header->autoclear_features);
        uint64_t *incompatible_features_p = &(header->incompatible_features);
        uint64_t *auto_features_p = &(header->autoclear_features);
      
        //on se positionne au bon endroit dans le fichier avant d'ecrire
        if(fseek(current_file, 72, SEEK_SET)){
            printf("Could not set the pointer to incompatible features zone");
            //break;
        }
        //on ecrit ensuite 

        if( 1 != fwrite(incompatible_features_p, sizeof(uint64_t), 1, current_file)){
            perror("\n Error: Cannot insert incompatible features in current file \n");
            //break;
        }
        if ( fflush( current_file ) ) {
            printf( "Cannot flush the stream\n" );
            //break;
        }

        if(fseek(current_file, 88, SEEK_SET)){
            printf("Could not set the pointer to incompatible features zone");
            //break;
        }
        //on ecrit ensuite 

        if( 1 != fwrite(auto_features_p, sizeof(uint64_t), 1, current_file)){
            perror("\n Error: Cannot insert auto features in current file \n");
            //break;
        }
        if ( fflush( current_file ) ) {
            printf( "Cannot flush the stream\n" );
            //break;
        }
      
        if(!get_backing_file_name(current_file, header, current_file_name)){
            printf("\n NO BACKING FILE FOUND \n");
            fclose(current_file);
            //break;
        }else{
            fclose(current_file);
            merge_strings(path_to_input_file, current_file_name, full_path_to_current_file);
            current_file = fopen(full_path_to_current_file, "rb+");
            read_header(current_file, header);
        }
        

        y--;
    }

    free(header);
    free(current_file_name);
    free(full_path_to_current_file);
}
/*Cette fonction permet de mettre a jour le last_index par rapport
 *a un offset donne (au niveau de l'entree L2). 
 *Dans cette configuration la table L2 existe
 *Si indication_bit_0=1 alors l'entrée L2 resultante finale doit avoir son bit_0 à 1
*/
void UpdateFile(FILE *f, uint64_t *l2_table, uint32_t l2_index, L1Entry *l1_entry, uint16_t index, uint8_t indication_bit_0){
    uint16_t b; //va contenir le last index   
    uint64_t a; //va contenir la valeur de l'entree L2 mise a jour

    //printf("%s %u \n", __func__, indication_bit_0);
    a = be64toh(l2_table[l2_index]);
    b = index;
    //on se rassure que les bits 14 et 15 sont nuls
    if( (b & BITS_14_15_NULS_MASK) != 0){
        printf("\n WARNING : l'index_in_chain tient sur plus de 14 bits \n");
    }
    uint64_t x = b & BITS_POIDS_FAIBLE_MASK;  //bits de poids faible de l'index
    uint64_t y = (b>>8) & BITS_POIDS_FORT_MASK;  //bits de poids fort de l'index
    a = a & OLD_L2_ENTRY_MASK;
    a = a | (x<<1) | (y<<56); //decalage des deux blocs de bits a leur bonne place
    //conversion
    //printf(" a_test = %lu \t", a);
    
    //on force le bit 63 à 1
    a = a | SET_TO_ONE_BIT_63_MASK;

    //on met éventuellement le bit 0 à 1
    if(indication_bit_0==1){
        a = a | 1;
    }

    a = htobe64(a);
    //printf("a_big_endian = %lu \t", a);
        //printf("%s %lu\n", __func__, (l1_entry->offset)+(8*l2_index));

    //positionnement dans le fichier
    fseek(f, (l1_entry->offset)+(8*l2_index), SEEK_SET);
    uint64_t *updateL2Entry = &a;
    //ecriture dans le fichier
    if(fwrite(updateL2Entry, sizeof(uint64_t), 1, f) != 1){
        perror("Error while updating file \n");
    }else{
        //printf("File %d updated successfully \n", index);
    }
    //printf("\n ici : offset = %lu; entree L2 = %lx \n", offset, a);
    if ( fflush( f ) ) {
            printf( "Cannot flush the stream\n" );
            //break;
        }
}

/*Cette fonction permet de mettre a jour une entree d'un refcount block
 *dans le fichier f
 *offset est l'offset dans le fichier du debut du refcount block correspondant
 *refcount_block_index est l'index dans le refcount block
 *refcount pointe sur la valeur a jour de l'entree du refcount_block
*/
void UpdateRefcountInFile(FILE *f, uint64_t offset, uint32_t refcount_block_index, uint16_t refcount){
    //on se positionne correctement
    fseek(f, offset + (2*refcount_block_index), SEEK_SET);
    //mise a jour dans le fichier
    uint16_t *refcount_p = &refcount;
    refcount = htobe16(refcount);
    if(fwrite(refcount_p, sizeof(uint16_t), 1, f) != 1){
        perror("Error while Updating file \n");
    }
    if ( fflush( f ) ) {
            printf( "Cannot flush the stream\n" );
            //break;
        }
}

/*Calculate the size of the file corresponding to the path file_path
 *Determine an valid offset 
 *Here, valid offset means that the offset is aligned to a cluster boundary
 *default value == 0
 *file_path must be allocated
*/
long long valid_offset(char *file_path, UsefulHeader *h){
    struct stat sb;
    long long result = 0;

    if(stat(file_path, &sb) != 0){
        perror("Error in stat function/ retrieving the file size \n");
        //printf("offset = %lld \n", result);
        return result;
    }else{
        //printf("AVANT= %ld ", sb.st_size);
        if(sb.st_size % h->cluster_size == 0){
            //printf("NO \n");
            result = sb.st_size + (2 * h->cluster_size);
        }else{
            //printf("other \n");
            result = ((sb.st_size / h->cluster_size) + 2) * h->cluster_size;
        }
        //printf("file %u size = %lld \n",h->index_in_chain, result);
        return result;
    }  
}

long int fsize(FILE *fh, UsefulHeader *h) {
    long int size;
  //FILE* fh;

  //fh = fopen(file, "rb"); //binary mode
  if(fh != NULL){
    if( fseek(fh, 0, SEEK_END) ){
      fclose(fh);
      printf("-1 \n");
      return -1;
    }

    size = ftell(fh);
    //printf("AVANT= %ld ", size);
    if(size % h->cluster_size != 0){
        size = ((size / h->cluster_size) + 1) * h->cluster_size;
    }
    //printf("file %u size = %ld \n",h->index_in_chain, size);
    return size;
  }
    printf("-1 \n");
  return -1; //error
}

uint8_t cluster_State(FILE *f, UsefulHeader *h, L1Entry *l1_entry, uint32_t l2_index){
        L2Entry *l2_entry = malloc(sizeof(L2Entry));
        if(l1_entry->offset == 0){
            free(l2_entry);
            //cluster non alloué
            return 0;
        }else{
            uint64_t *l2_table = malloc(h->cluster_size * sizeof(char));
            load_l2_table(f,l1_entry->offset,h,l2_table);
            
            load_l2_entry(l2_table, l2_index, l2_entry);
            if (l2_entry->offset == 0)
            {
                if(l2_entry->b63_is_clear){
                    //cluster non alloué
                    free(l2_entry);
                    free(l2_table);
                    return 1;
                }else{
                    free(l2_table);
                    free(l2_entry);
                    return 2;
                }
            }else{
                //cluster alloué
                free(l2_table);
                free(l2_entry);
                return 3;
            }
            
        }
    }


/* Write a new L2 Table in file (full zeros) at the offset specified
*/
void write_new_L2_table_in_file(FILE *f, UsefulHeader *h, uint64_t offset){
    //préparation de la table
    //printf("%s %lu \n", __func__, offset);
    uint64_t *new_l2_table = (uint64_t *)calloc(h->nb_l2_entries, 8);

    //printf("\n nbre d'entrées par table L2 = %u \n", h->nb_l2_entries);
    //raise(SIGINT);
    //positionnement correct dans le fichier
    fseek(f, offset, SEEK_SET);
    for(uint32_t s=0; s < h->nb_l2_entries; s++){
        new_l2_table[s] = htobe64(new_l2_table[s]);
    }
    if( fwrite(new_l2_table, h->cluster_size, 1, f) != 1){
        perror("echec write_new_L2_table_in_file");
    }
    if ( fflush( f ) ) {
        printf( "Cannot flush the stream \n" );
    }
    free(new_l2_table);
    //printf("L2_table offset=%lu\n", ftell(f));
}

/* This function sets the index good_index in files between first_p and second_p
 * referring to last_index of modification of a specific cluster
*/
void update_all_files_between(FILE *first_p, FILE *second_p, uint16_t good_index, uint32_t l1_index, uint32_t l2_index, char *path_to_input_file, char *full_path_second_p){
    /*Read the header of first_p*/
    UsefulHeader *first_h = g_new(UsefulHeader, 1);
    read_header(first_p, first_h);
    /*Read the header of second_p*/
    UsefulHeader *second_h = g_new(UsefulHeader, 1);
    read_header(second_p, second_h);
    uint8_t cluster_state;

    uint64_t *l1_table = malloc(first_h->l1_size * sizeof(uint64_t));
    L1Entry *l1_entry = malloc(sizeof(L1Entry));
    uint64_t *l2_table = malloc(first_h->cluster_size * sizeof(char));

    long long new_l2_table_offset = 0;
    uint8_t end_chain = 0;

    /* We know from qcow2 format that the name won't exceed 1023 bytes */
    char *current_file_name = malloc(sizeof(char)*1024);
    char *full_path_to_current_file = malloc((str_len(path_to_input_file) + 1024)*sizeof(char));

    uint8_t indic_bit_0 = 0;

    strcpy(full_path_to_current_file, full_path_second_p);

//TEST

    uint64_t *l1_table_bis = malloc(first_h->l1_size * sizeof(uint64_t));
    L1Entry *l1_entry_bis = malloc(sizeof(L1Entry));
    uint64_t *l2_table_bis = malloc(first_h->cluster_size * sizeof(char));
    L2Entry *l2_entry_bis = malloc(sizeof(L2Entry));
    load_l1_table(first_p, first_h, l1_table_bis);
    load_l1_entry(l1_table_bis, l1_index, l1_entry_bis);
    load_l2_table(first_p, l1_entry_bis->offset, first_h, l2_table_bis);
    load_l2_entry(l2_table_bis, l2_index, l2_entry_bis);

    if(!l2_entry_bis->b0_is_clear){
        indic_bit_0 = 1;
    }
    
    free(l1_entry_bis);
    free(l1_table_bis);
    free(l2_table_bis);
    free(l2_entry_bis);


    //printf("START=%u-%u \n",second_h->index_in_chain, first_h->index_in_chain);
    while( (end_chain==0) && (second_h->index_in_chain >= first_h->index_in_chain) ){
        load_l1_table(second_p, second_h, l1_table);
        load_l1_entry(l1_table, l1_index, l1_entry);
        //printf("offset=%lu file=%u \n", l1_entry->offset, second_h->index_in_chain);
        assert(!(l1_entry->offset % second_h->cluster_size));
    /*    if(second_h->index_in_chain == 4){
                        printf("avant 2 %lld state=%u \n", valid_offset(full_path_to_current_file, second_h), cluster_state);
                        if( valid_offset(full_path_to_current_file, second_h) == 4587520){
                            raise(SIGINT);
                        }
                    }  */
        cluster_state = cluster_State(second_p, second_h, l1_entry, l2_index);
        if( (cluster_state == 3) || (cluster_state == 1) ){ //cas 3 ou cas 1
            load_l2_table(second_p, l1_entry->offset, second_h, l2_table);
//TEST
            //printf("avant=%lx index=%u \n", be64toh(l2_table[l2_index]), good_index);

            UpdateFile(second_p, l2_table, l2_index, l1_entry, good_index, indic_bit_0);
//TEST
/*            load_l2_table(second_p, l1_entry->offset, second_h, l2_table);
                        L2Entry *l2_entry = malloc(sizeof(L2Entry));
                        load_l2_entry(l2_table, l2_index, l2_entry);
                        printf("apres=%lx index=%u \n", be64toh(l2_table[l2_index]), l2_entry->last_snapshot_index);
                        free(l2_entry);  */
        }else{
            if(cluster_state == 2){
                //cas 2
                printf("\n AMAZING CONFIGURATION CASE 2 \n");
            }else{
                if(cluster_state == 0){//cas 0
                    //nouvel offset pour la table L2
                    //new_l2_table_offset = fsize(second_p, second_h);
                    if(second_h->index_in_chain == 1){
                        printf("l1_index=%u  l2_index=%u \n", l1_index, l2_index);
                    }
                    new_l2_table_offset = valid_offset(full_path_to_current_file, second_h);
            
                    if(new_l2_table_offset == 0){
                        perror("\n Error while generating a new offset \n");
                    }else{
                        //écriture de la nouvelle table L2
                        //printf("file=%u offset=%llu \n", second_h->index_in_chain, new_l2_table_offset);
                        write_new_L2_table_in_file(second_p, second_h, new_l2_table_offset);
                        //mise a jour de l'entrée L1
                        l1_entry->offset = new_l2_table_offset;

                        //printf("avant=%lu \n", l1_entry->offset);

                        l1_entry_update_file(second_p, second_h, l1_index, l1_entry);

                        /*
                        uint64_t *l1_table_bis = malloc(first_h->l1_size * sizeof(uint64_t));
                        L1Entry *l1_entry_bis = malloc(sizeof(L1Entry));
                        load_l1_table(second_p, second_h, l1_table_bis);
                        load_l1_entry(l1_table_bis, l1_index, l1_entry_bis);
                        printf("apres=%lu \n",l1_entry_bis->offset);
                        free(l1_table_bis);
                        free(l1_entry_bis);
                        */

                        //mise a jour last_index
                        load_l2_table(second_p, new_l2_table_offset, second_h, l2_table);
        //                printf("avant=%lx index=%u \n", be64toh(l2_table[l2_index]), good_index);
                        UpdateFile(second_p, l2_table, l2_index, l1_entry, good_index, indic_bit_0);

        //TEST          
        /*                load_l2_table(second_p, new_l2_table_offset, second_h, l2_table);
                        L2Entry *l2_entry = malloc(sizeof(L2Entry));
                        load_l2_entry(l2_table, l2_index, l2_entry);
                        printf("apres=%lx index=%u \n", be64toh(l2_table[l2_index]), l2_entry->last_snapshot_index);
                        free(l2_entry);    */
                        //gestion des refcount
        /*                uint64_t refcount_block_entries = (second_h->cluster_size * 8)/second_h->refcount_bits;
                        uint32_t refcount_block_index = (new_l2_table_offset/second_h->cluster_size) % refcount_block_entries;
                        uint32_t refcount_table_index = (new_l2_table_offset/second_h->cluster_size) / refcount_block_entries;
                        uint64_t *refcount_table = malloc(second_h->cluster_size * second_h->refcount_table_clusters);
                        load_refcount_table(second_p, second_h, refcount_table);
                        RefcountTableEntry *refcount_table_entry = malloc(sizeof(RefcountBlockEntry));
                        load_refcount_table_entry(refcount_table, refcount_table_index, refcount_table_entry);
                        printf("%lu \n", refcount_table_entry->offset);
                        uint16_t *refcount_block = malloc(second_h->nb_refcount_block_entries * sizeof(uint16_t));
                        RefcountBlockEntry *refcount_block_entry = malloc(sizeof(RefcountBlockEntry));
                        if(refcount_table_entry->offset != 0){

                            load_refcount_block(second_p, refcount_table_entry->offset, second_h, refcount_block);
                            load_refcount_block_entry(refcount_block, refcount_block_index, refcount_block_entry);
                            printf("ref=%u \n", refcount_block_entry->refcount);
                        }
                        free(refcount_table);
                        free(refcount_table_entry);
                        free(refcount_block);
                        free(refcount_block_entry);   */

                    }
                }
            }
        }
        //on fait progresser second_p
        if(!get_backing_file_name(second_p, second_h, current_file_name)){
            end_chain = 1;
        }else{
            merge_strings(path_to_input_file, current_file_name, full_path_to_current_file);
            fclose(second_p);
            second_p = fopen(full_path_to_current_file, "rb+");
            if (second_p == NULL)
            {
                perror("Error");
            }
            read_header(second_p, second_h);
        }
    }
    //libération des allocations
    free(first_h);
    free(second_h);
    free(l1_table);
    free(l2_table);
    free(l1_entry);
    free(current_file_name);
    free(full_path_to_current_file);
    //printf("END \n\n");
}


/* Read the whole virtual disk. Le pas utilisé = cluster_size
 * for each cluster allocated, insert the right index of the last snapshot where it has been modified
*/
void update_last_indexes(char *input_file){
    
    char *path_to_input_file, *input_file_name; 
    split_path(input_file, &path_to_input_file, &input_file_name);

    
    /* We know from qcow2 format that the name won't exceed 1023 bytes */
    char *current_file_name = malloc(sizeof(char)*1024);
    /* Set current_file_name to the input file name */
    for(int i = 0 ; i < 1024 ; ++i){
        current_file_name[i] = input_file_name[i];
        if(current_file_name[i] == '\0')
            break;
    }
    
    char *full_path_to_current_file = malloc((str_len(path_to_input_file) + 1024)*sizeof(char));
    char *full_path_to_current_file_base = malloc((str_len(path_to_input_file) + 1024)*sizeof(char));


    /* Set full_path_to_current_file to the input file path */
    int i = 0;
    while(input_file[i] != '\0'){
        full_path_to_current_file[i] = input_file[i];
        ++ i;
    }
    full_path_to_current_file[i] = '\0';

    int k = 0;
    while(input_file[k] != '\0'){
        full_path_to_current_file_base[k] = input_file[k];
        ++ k;
    }
    full_path_to_current_file_base[k] = '\0';


    /* Open the first datafile of the chain
     * 'r+' mode in order to read and write
     */
    FILE *initial_file = fopen(full_path_to_current_file, "rb+");
    if(initial_file == NULL){
        printf("\n Echec de l'ouverture du current_file \n");
    }

    /* Read the header of the input file */
    UsefulHeader *header = g_new(UsefulHeader, 1);
    read_header(initial_file, header);

    uint64_t *current_l1_table = malloc(header->l1_size * sizeof(uint64_t));
    uint64_t *current_l2_table = malloc(header->cluster_size * sizeof(char));

    L1Entry *current_l1_entry = malloc(sizeof(L1Entry));
    L2Entry *current_l2_entry = malloc(sizeof(L2Entry));

  
    uint8_t status = 1;
    FILE *first_pointer;
    FILE *second_pointer = NULL;

    char *full_path_interim = malloc((str_len(path_to_input_file) + 1024)*sizeof(char));

    uint64_t offset;
    int g;
    uint32_t l2_index;
    uint32_t l1_index;
    uint8_t cluster_state;
   
    for (offset = 0; offset < header->size; offset = offset+header->cluster_size){
        
        strcpy(full_path_to_current_file, full_path_to_current_file_base);

        first_pointer = fopen(full_path_to_current_file, "rb+");
        status = 1;
        
        if(first_pointer == NULL){
            printf("\n Echec de l'ouverture de first_pointer \n");
        }
        read_header(first_pointer, header);
        //on parcourt toute la chaine
        l2_index = (offset/header->cluster_size)%(header->nb_l2_entries);
        l1_index = (offset/header->cluster_size)/(header->nb_l2_entries);
        
        while (status != 2){
            
            //on charge la table L1 et l'entrée L1 correspondante
            load_l1_table(first_pointer, header, current_l1_table);
            load_l1_entry(current_l1_table, l1_index, current_l1_entry);
            /* Quickly check that the offset is coherent, i.e. it is indeed
             * the beginning of a cluster */
            assert(!(current_l1_entry->offset % header->cluster_size));
            //cluster alloué ou pas?
            cluster_state = cluster_State(first_pointer, header, current_l1_entry, l2_index);
            
            if(cluster_state == 3){ 
                //le cluster est alloué
                if(second_pointer == NULL){
                    //mise a jour immédiate sur le fichier courant
                    

               }else{
                   //mise à jour sur un ensemble de fichiers
                   fclose(second_pointer);
                   second_pointer = NULL;
               }
            }else{
                if (second_pointer == NULL){
                    //positionnement repère (droite_p) pour la mise à jour sur un ensemble de fichiers
                    
                }
            }

            //bout de la chaine ou pas?
            if(!get_backing_file_name(first_pointer, header, current_file_name)){
                status = 2;
            }else{
                merge_strings(path_to_input_file, current_file_name, full_path_to_current_file);        
                fclose(first_pointer);
                first_pointer = fopen(full_path_to_current_file, "rb+");
                if(first_pointer == NULL){
                    perror("Error");
                }
                read_header(first_pointer, header);
            }
        }
        fclose(first_pointer);
        //printf("OFF %lu HSIZE %lu END \n",offset,header->size);

    }

    printf("\n FIN DE LA LECTURE DU DISQUE DE LA VM \n");

}

void three_first_base(char *file){

    FILE *f;
    UsefulHeader *h = g_new(UsefulHeader, 1);

    f = fopen(file, "rb+");
    read_header(f, h);
    uint64_t *l1_table = malloc(h->l1_size * sizeof(uint64_t));
    load_l1_table(f, h, l1_table);
    for(int index=0; index<3; index++){
        printf("%lu \n", l1_table[index]);
    }

    free(h);
    free(l1_table);
}

void l2_tables_offset(char *file){

    FILE *f;
    UsefulHeader *h = g_new(UsefulHeader, 1);

    printf("\n %s \n", __func__);

    f = fopen(file, "rb+");
    read_header(f,h);
    uint64_t *l1_table = malloc(h->l1_size * sizeof(uint64_t));
    load_l1_table(f, h, l1_table);
    L1Entry *l1_entry = malloc(sizeof(L1Entry));
    for(uint16_t index=0; index<h->l1_size; index++){
        load_l1_entry(l1_table, index, l1_entry);
        printf("index=%u %lu %lu \n", index, l1_entry->offset, l1_entry->offset + h->cluster_size);
    }

    free(h);
    free(l1_table);
    free(l1_entry);
}

void data_clusters_offset(char *file){
    FILE *f;
    UsefulHeader *h = g_new(UsefulHeader, 1);

    printf("\n %s \n", __func__);

    f = fopen(file, "rb+");
    read_header(f, h);
    uint64_t *l1_table = malloc(h->l1_size * sizeof(uint64_t));
    load_l1_table(f, h, l1_table);
    L1Entry *l1_entry = malloc(sizeof(L1Entry));
    L2Entry *l2_entry = malloc(sizeof(L2Entry));
    uint64_t *l2_table = malloc(h->cluster_size * sizeof(char));
    for(uint16_t l1_index=0; l1_index<h->l1_size; l1_index++){
        load_l1_entry(l1_table, l1_index, l1_entry);
        if(l1_entry->offset != 0){
            load_l2_table(f, l1_entry->offset, h, l2_table);
            for(uint16_t l2_index=0; l2_index<h->nb_l2_entries; l2_index++){
                load_l2_entry(l2_table, l2_index, l2_entry);
                if(l2_entry->offset != 0){
                    printf("%lu %lu \n", l2_entry->offset, l2_entry->offset + h->cluster_size);
                }
            }
        }
    }
}

    void entrees_L2(char *file)
    {
        FILE *f;
        UsefulHeader *h = g_new(UsefulHeader, 1);

        printf("\n %s \n", __func__);

        f = fopen(file, "rb+");
        read_header(f, h);
        uint64_t *l1_table = malloc(h->l1_size * sizeof(uint64_t));
        load_l1_table(f, h, l1_table);
        L1Entry *l1_entry = malloc(sizeof(L1Entry));
        L2Entry *l2_entry = malloc(sizeof(L2Entry));
        uint64_t *l2_table = malloc(h->cluster_size * sizeof(char));
        for (uint16_t l1_index = 0; l1_index < h->l1_size; l1_index++)
        {
            load_l1_entry(l1_table, l1_index, l1_entry);
            if (l1_entry->offset != 0)
            {
                load_l2_table(f, l1_entry->offset, h, l2_table);
                for (uint16_t l2_index = 0; l2_index < h->nb_l2_entries; l2_index++)
                {
                    load_l2_entry(l2_table, l2_index, l2_entry);
                    /* if( (l2_entry->offset == 0) && (l2_entry->b63_is_clear==0) ){
                    if((l2_entry->b63_is_clear==0) ){
                        printf("YES 63\n");
                    }else{
                        printf("NO 63\n");
                    }
                    if((l2_entry->b63_is_clear==0) && (l2_entry->offset==0) ){
                        printf("TOTO\n");
                    }else{
                        printf("NO 0\n");
                    } */
                    printf("l1_index=%u l2_index=%u val=%lx offset=%lu b63_is_clear=%u b0_is_clear=%u\n", l1_index, l2_index, l2_table[l2_index], l2_entry->offset, l2_entry->b63_is_clear, l2_entry->b0_is_clear);
                }
            }
        }

        free(h);
        free(l1_entry);
        free(l1_table);
        free(l2_entry);
        free(l2_table);
    }


void entrees_L1(char *file){

    FILE *f;
    UsefulHeader *h = g_new(UsefulHeader, 1);

    f = fopen(file, "rb+");
    read_header(f, h);
    uint64_t *l1_table = malloc(h->l1_size * sizeof(uint64_t));
    load_l1_table(f, h, l1_table);
    L1Entry *l1_entry = malloc(sizeof(L1Entry));
    for(uint16_t index=0; index<h->l1_size; index++){
        load_l1_entry(l1_table, index, l1_entry);
        printf("index=%u entry=%lx bckg=%lx offset=%lu \n", index, be64toh(l1_table[index]), h->backing_file_offset, l1_entry->offset);
    }

    free(h);
    free(l1_table);
    free(l1_entry);
}

void refcount_table_entries(char *file){
    FILE *f;
    UsefulHeader *h = g_new(UsefulHeader, 1);

    //uint64_t refcount_block_entries = (h->cluster_size * 8)/h->refcount_bits;
    //uint32_t refcount_block_index = (new_l2_table_offset/second_h->cluster_size) % refcount_block_entries;
    //uint32_t refcount_table_index = (new_l2_table_offset/second_h->cluster_size) / refcount_block_entries;
    f = fopen(file, "rb+");
    read_header(f, h);
    printf("refcount_table_clusters = %u \n", h->refcount_table_clusters);

    uint64_t *refcount_table = malloc(h->nb_refcount_table_entries * sizeof(uint64_t));
    load_refcount_table(f, h, refcount_table);
    RefcountTableEntry *refcount_table_entry = malloc(sizeof(RefcountBlockEntry));
    for(uint32_t index=0; index<h->nb_refcount_table_entries; index++){
        load_refcount_table_entry(refcount_table, index, refcount_table_entry);
        if(refcount_table_entry->offset != 0){
            printf("index=%u %lu \n", index, refcount_table_entry->offset);
        }
    }
/*                        uint16_t *refcount_block = malloc(second_h->nb_refcount_block_entries * sizeof(uint16_t));
                        RefcountBlockEntry *refcount_block_entry = malloc(sizeof(RefcountBlockEntry));
                        if(refcount_table_entry->offset != 0){

                            load_refcount_block(second_p, refcount_table_entry->offset, second_h, refcount_block);
                            load_refcount_block_entry(refcount_block, refcount_block_index, refcount_block_entry);
                            printf("ref=%u \n", refcount_block_entry->refcount);
*/
    free(h);
    free(refcount_table);
    free(refcount_table_entry);
}

void refcount_block_entries(char *file){
    FILE *f;
    UsefulHeader *h = g_new(UsefulHeader, 1);

    //uint64_t refcount_block_entries = (h->cluster_size * 8)/h->refcount_bits;
    //uint32_t refcount_block_index = (new_l2_table_offset/second_h->cluster_size) % refcount_block_entries;
    //uint32_t refcount_table_index = (new_l2_table_offset/second_h->cluster_size) / refcount_block_entries;
    f = fopen(file, "rb+");
    read_header(f, h);
    printf("refcount_table_clusters = %u \n", h->refcount_table_clusters);
    uint16_t *refcount_block = malloc(h->nb_refcount_block_entries * sizeof(RefcountBlockEntry));
    RefcountBlockEntry *refcount_block_entry = malloc(sizeof(RefcountBlockEntry));
    uint64_t *refcount_table = malloc(h->nb_refcount_table_entries * sizeof(uint64_t));
    load_refcount_table(f, h, refcount_table);
    RefcountTableEntry *refcount_table_entry = malloc(sizeof(RefcountBlockEntry));
    for(uint32_t index=0; index<h->nb_refcount_table_entries; index++){
        load_refcount_table_entry(refcount_table, index, refcount_table_entry);
        if(refcount_table_entry->offset != 0){
            printf("index=%u %lu \n\n", index, refcount_table_entry->offset);
            load_refcount_block(f, refcount_table_entry->offset, h, refcount_block);
            for(uint32_t j=0; j<h->nb_refcount_block_entries; j++){
                load_refcount_block_entry(refcount_block, j, refcount_block_entry);
                if(refcount_block_entry->refcount != 0){
                    printf("YES index=%u refcount=%u \n", j, refcount_block_entry->refcount);
                }else{
                    printf("NO \n");
                }
            }
        }
    }
/*                        uint16_t *refcount_block = malloc(second_h->nb_refcount_block_entries * sizeof(uint16_t));
                        RefcountBlockEntry *refcount_block_entry = malloc(sizeof(RefcountBlockEntry));
                        if(refcount_table_entry->offset != 0){

                            load_refcount_block(second_p, refcount_table_entry->offset, second_h, refcount_block);
                            load_refcount_block_entry(refcount_block, refcount_block_index, refcount_block_entry);
                            printf("ref=%u \n", refcount_block_entry->refcount);
*/
    free(h);
    free(refcount_table);
    free(refcount_table_entry);
    free(refcount_block);
    free(refcount_block_entry);
}

/*Cette fonction permet de récupérer une entrée L2 dans un fichier donné
 *et d'insérer ensuite good_last_index dans l'entrée L2.
 *Elle retourne l'entrée L2 version big endian, prete a etre stockée dans le fichier
*/
uint64_t insert_index(FILE *f, UsefulHeader *h, uint16_t l1_index, uint16_t l2_index, uint16_t good_last_index){
    uint64_t result=0;
    uint64_t *l1_table = malloc(h->l1_size * sizeof(uint64_t));
    uint64_t *l2_table = malloc(h->cluster_size * sizeof(char));
    L1Entry *l1_entry = malloc(sizeof(L1Entry));

    load_l1_table(f, h, l1_table);
    load_l1_entry(l1_table, l1_index, l1_entry);
    load_l2_table(f, l1_entry->offset, h, l2_table);

    //récupération de l'entrée L2 initiale
    result = be64toh(l2_table[l2_index]);
    //bits de poids fort de good index bit 8-13
    uint64_t a = good_last_index & 0x3f00;
    a = a >> 8;
    a = a << 56;
    //bits de poids faible de good index bit 0-7
    uint64_t b = good_last_index & 0xff;
    b = b << 1;
    //préparation de l'entrée L2 finale
    result = result | a | b;
    result = htobe64(result);

    free(l1_table);
    free(l2_table);
    free(l1_entry);

    return result;
}


/*Cette fonction permet de mettre à jour une entrée L2 dans un fichier
 *à partir de l'entrée L2 passée en paramètre
 *Ici updated_l2_entry est déjà sous forme big endian
*/
void update_L2_entry(FILE *f, UsefulHeader *h, uint16_t l1_index, uint16_t l2_index, uint64_t *updated_l2_entry){

    uint64_t *l1_table = malloc(h->l1_size * sizeof(uint64_t));
    L1Entry *l1_entry = malloc(sizeof(L1Entry));

    load_l1_table(f, h, l1_table);
    load_l1_entry(l1_table, l1_index, l1_entry);
    assert(!(l1_entry->offset % h->cluster_size));
    //on se positionne au bon endroit dans le fichier
    if( fseek(f, l1_entry->offset + (8*l2_index), SEEK_SET) ){
        perror("Erreur fseek %s \n", __func__);
    }
    //on écrit la nouvelle entrée L2
    if(fwrite(updated_l2_entry, sizeof(uint64_t), 1, f) != 1){
        perror("Erreur fwrite %s \n", __func__);
    }
    //synchronisation
    if ( fflush( f ) ) {
            printf( "Cannot flush the stream\n" );
    }
    free(l1_table);
    free(l1_entry);  
}

/* cette fonction permet d'insérer une nouvelle table L2 par rapport à l'index l1_index
 *Elle met à jour l'entrée L1 au passage. file_path permet de calculer la taille courante
 *du fichier via valid_offset
*/
void insert_new_l2_table(FILE *f, UsefulHeader *h, char *file_path, uint16_t l1_index){
    //calcul de l'offset de la nouvelle table L2
    uint64_t new_offset = valid_offset(file_path, h);
    uint64_t result;

    //preparation de la nouvelle entree a ecrire
    result = new_offset;
    result = htobe64(result);
    //positionnement au bon endroit dans le fichier 
    fseek(f, (8*l1_index) + h->l1_table_offset, SEEK_SET);
    
    //mise a jour du fichier
   
    if( fwrite(&result, sizeof(uint64_t), 1, f) != 1 ){
        perror("ERROR while updating the file %s\n", __func__);
    }
    //synchronisation
    if ( fflush( f ) ) {
            printf( "Cannot flush the stream\n" );
            //break;
    }
    //écriture de la table L2 remplie de 0
    //préparation de la table
    uint64_t *new_l2_table = (uint64_t *)calloc(h->nb_l2_entries, sizeof(uint64_t));
    //positionnement correct dans le fichier
    fseek(f, new_offset, SEEK_SET);
    if( fwrite(new_l2_table, h->cluster_size, 1, f) != 1){
        perror("echec write_new_L2_table_in_file");
    }
    if ( fflush( f ) ) {
        printf( "Cannot flush the stream \n" );
    }
    free(new_l2_table);

}

/*Cette fonction permet de créer les tables L2 sur les fichiers compris entre gauche_p et droite_p ou 
 *l'index l1_index n'indique aucune table L2
*/
void insert_l2_tables_between(FILE *gauche_p,char *full_path_to_droite_p, char *path_to_input_file, uint16_t l1_index){
     /*Read the header of gauche_p*/
    UsefulHeader *gauche_h = g_new(UsefulHeader, 1);
    read_header(gauche_p, gauche_h);
    /*Read the header of interim_p*/
    FILE *interim_p = fopen(full_path_to_droite_p, "rb+");
    UsefulHeader *interim_h = g_new(UsefulHeader, 1);
    read_header(interim_p, interim_h);

    uint8_t end_chain = 0;
    char *current_file_name = malloc(sizeof(char)*1024);
    char *full_path_to_current_file = malloc((str_len(path_to_input_file) + 1024)*sizeof(char));
    uint64_t *l1_table = malloc(interim_h->l1_size * sizeof(uint64_t));
    L1Entry *l1_entry = malloc(sizeof(L1Entry));

    full_path_to_current_file = full_path_to_droite_p;
    while( (end_chain==0) && (interim_h->index_in_chain >= gauche_h->index_in_chain) ){
        
        load_l1_table(interim_p, interim_h, l1_table);
        load_l1_entry(l1_table, l1_index, l1_entry);
        if(l1_entry->offset == 0){
            insert_new_l2_table(interim_p, interim_h, full_path_to_current_file, l1_index);
        }
        
        //on fait progresser interim_p 
        if(!get_backing_file_name(interim_p, interim_h, current_file_name)){
            end_chain = 1;
        }else{
            merge_strings(path_to_input_file, current_file_name, full_path_to_current_file);
            fclose(interim_p);
            interim_p = fopen(full_path_to_current_file, "rb+");
            if (interim_p == NULL)
            {
                perror("Error \n");
            }
            read_header(interim_p, interim_h);
        }
    }

    free(gauche_h);
    free(interim_h);
    free(current_file_name);
    free(full_path_to_current_file);
    free(l1_table);
    free(l1_entry);
}


int main(int argc, char* argv[]){
       
    if(argc < 2){
        printf("Arguments missing. How to use:\n\
                ./conversion_a_froid qcow2_file \n\
                Exiting...\n");
        return 1;
    } 
    index_in_chain(argv[1]);
    update_last_indexes(argv[1]);


    //entrees_L2(argv[1]);

    //l2_tables_offset(argv[1]);
    //data_clusters_offset(argv[1]);

    //three_first_base(argv[1]);
    //printf("size_of_char = %ld \n", sizeof(char));

    return 0;    
}





