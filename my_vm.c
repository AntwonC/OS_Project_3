// Authors: Anthony Chen, Mohammad Mahdi Emad
// NetID: ac1678, mme94

#include "my_vm.h"


void * physical_memory = NULL;
pde_t ** page_dir;

char * vp_bitmap;
int vp_bitmap_len;
char * pp_bitmap;
int pp_bitmap_len;

int totalBitLen;
int pageOffsetLen;
int pdeOffsetLen;
int pteOffsetLen;

int address_mat1; 
int address_mat2; 
int address_answer;

void* pa; 

pthread_mutex_t lock; 

struct tlb * tlb_stored;
int hits; 
int misses; 
int lookups; 




// TODO: Make a REAL bitmap
int check_bitmap(char * bitmap, unsigned int index){
    return bitmap[index];
}

void set_bitmap(char * bitmap, unsigned int index, int value){
    bitmap[index] = value;
    return;
}

 // TODO: Thread-safe implementation
/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {
    
    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating
    totalBitLen = (int) log2((double) MAX_MEMSIZE); // 32
    pageOffsetLen = (int) log2((double) PGSIZE); // 30
    pdeOffsetLen = (totalBitLen - pageOffsetLen) / 2; // (32 - 30) / 2 = 1
    pteOffsetLen = totalBitLen - pageOffsetLen - pdeOffsetLen; // 10
  //  printf("What is pteOffsetLen: %d\n", pteOffsetLen);

    // Setting the initial allocations of memory and bitmap global variables
    physical_memory = malloc(MEMSIZE);
    
    page_dir = (pde_t **) malloc((2 ^ pdeOffsetLen) * sizeof(pde_t)); // 2^29 * 4

    // Made page_dir 2D array, 1st level, is page_dir[outer_bits]
    // 2nd Level is page_dir[outer_bits][inner_bits];
    int j; 
    for(j = 0; j < (2^pdeOffsetLen); j++)   {
        page_dir[j] = (pde_t*) malloc((2^pdeOffsetLen) * sizeof(pde_t)); 
    } 


    vp_bitmap_len = (2 ^ (totalBitLen - pageOffsetLen)); // 2 ^ 
    vp_bitmap = (char *) malloc(vp_bitmap_len * sizeof(char));

    pp_bitmap_len = (2 ^ ((int) log2((double) MEMSIZE) - pageOffsetLen));
    pp_bitmap = (char *) malloc(pp_bitmap_len * sizeof(char));


    // Initializing bitmaps
    int i = 0;
    for(i; i < vp_bitmap_len; i++)
    {
        set_bitmap(vp_bitmap, i, 0);
    }

    i = 0;
    for(i; i < pp_bitmap_len; i++)
    {
        set_bitmap(pp_bitmap, i, 0);
    }

    tlb_stored = malloc(TLB_ENTRIES); // Set up our TLB. 

  

    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

}

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int
add_TLB(void *va, void *pa)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */
    
    // Calculating the bits we need, inner, outer, offset. 

    unsigned int offset = (2^pageOffsetLen - 1) | (unsigned int) va;
    unsigned int pdeIndex = (2^pdeOffsetLen - 1) | (unsigned int) ((unsigned int)va>>pdeOffsetLen);
    unsigned int pteIndex = (2^pteOffsetLen - 1) | (unsigned int) ((unsigned int)va>>(pdeOffsetLen + pdeOffsetLen));

    // We will only use the inner and outer bits and use that as the tag. 
    // To calculate use: ( tag % num_sets ) 
   unsigned int num = pdeIndex % TLB_ENTRIES; 
   pthread_mutex_lock(&lock);
   if (  tlb_stored[num].v_tag == NULL || tlb_stored[num].p_tag == NULL )   { // Cold miss
        tlb_stored[num].v_tag = (void*) pdeIndex;  
        tlb_stored[num].p_tag = (void*) pa;
       misses++; 

   }    else if ( (unsigned int) tlb_stored[num].v_tag == pdeIndex ) { // Checking if a hit
         hits++; 
         
   }    else if ( (unsigned int) tlb_stored[num].v_tag != pdeIndex )    { // Checking if its a miss
        misses++; 
         tlb_stored[num].v_tag = (void*) pdeIndex;  
         tlb_stored[num].p_tag = (void*) pa;
   }

    pthread_mutex_unlock(&lock);

    return 0;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va) {



    /* Part 2: TLB lookup code here */
    lookups++; 

     unsigned int offset = (2^pageOffsetLen - 1) | (unsigned int) va;
    unsigned int pdeIndex = (2^pdeOffsetLen - 1) | (unsigned int) ((unsigned int)va>>pdeOffsetLen);
    unsigned int pteIndex = (2^pteOffsetLen - 1) | (unsigned int) ((unsigned int)va>>(pdeOffsetLen + pdeOffsetLen));

    int i; 
    unsigned int index = pdeIndex % TLB_ENTRIES; 
    
    // Look up in our TLB if the virtual address is in the TLB, if so, then return the physical address. 
  
        if ( (unsigned int) tlb_stored[index].v_tag == pdeIndex)     {
            hits++; 
            return (pte_t*) tlb_stored[index].p_tag; 
        }   else    {
            misses++; 
            return NULL;
        }
    

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate()
{
    double miss_rate = 0;	

    /*Part 2 Code here to calculate and print the TLB miss rate*/
    miss_rate = misses / lookups; 

    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t *translate(pde_t *pgdir, void *va) {
    /* Part 1 HINT: Get the Page directory index (1st level) Then get the
    * 2nd-level-page table index using the virtual address.  Using the page
    * directory index and page table index get the physical address.
    *
    * Part 2 HINT: Check the TLB before performing the translation. If
    * translation exists, then you can return physical address from the TLB.
    */

    pte_t * check = check_TLB(va); 

    if ( check != NULL )  {

         unsigned int offset = (2^pageOffsetLen - 1) | (unsigned int) va;
    unsigned int pdeIndex = (2^pdeOffsetLen - 1) | (unsigned int) ((unsigned int)va>>pdeOffsetLen);
    unsigned int pteIndex = (2^pteOffsetLen - 1) | (unsigned int) ((unsigned int)va>>(pdeOffsetLen + pdeOffsetLen));

    if ( check_bitmap(vp_bitmap, (unsigned int)va>>pdeOffsetLen) )
       
       return (pte_t *) ( ((unsigned int) page_dir[pdeIndex][pteIndex]<<pdeOffsetLen) + offset);

     return NULL;   

    }   else    {

        return check;
    }

  
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
page_map(pde_t *pgdir, void *va, void *pa)
{

    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

    unsigned int offset = (2^pageOffsetLen - 1) | (unsigned int) va;
    unsigned int pdeIndex = (2^pdeOffsetLen - 1) | (unsigned int) ((unsigned int)va>>pdeOffsetLen);
    unsigned int pteIndex = (2^pteOffsetLen - 1) | (unsigned int) ((unsigned int)va>>(pdeOffsetLen + pdeOffsetLen));

    pte_t * check_mapping = page_dir[pdeIndex]; // Checks the first level?
    pte_t ** check_two = (pde_t**)page_dir[pdeIndex][pteIndex]; // Checks the second level?

        pthread_mutex_lock(&lock);

    if ( check_mapping == NULL )    { // Need to create a mapping, set physical bitmap, (This new entry being added? )
        // Allocate L2 page in that index 
        page_dir[pdeIndex] = malloc((2^pdeOffsetLen) * sizeof(pde_t));
        //page_dir[pdeIndex][pteIndex] = physical_memory[]; // Give the index physical address?
      //  page_dir[pdeIndex][pteIndex] = (unsigned int) va;
        page_dir[pdeIndex][pteIndex] = &page_dir[pdeIndex]; // Stores the address of L1.
        //physical_memory[offset] = pa;  // Map the physical memory to L2 of page table.
       
        // Set physical bitmap and virtual bitmap to allocated. 
        set_bitmap(pp_bitmap, (unsigned int)pa, 1); 
        set_bitmap(vp_bitmap, (unsigned int)va, 1);

        pthread_mutex_unlock(&lock);

        return 0; // Able to set a mapping

    }   else    {

        pthread_mutex_unlock(&lock);

        return -1; 
    }  
  
}


/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {
   //Use virtual address bitmap to find the next free page

    // walk virtual bitmap, pages must be contiguous 
    int counter = 0; 
    int index = 0; 
    
    int i; 
    if(num_pages < 1)
        return NULL;
    
    // Find the contiguous pages 
    for(i = 0; i < vp_bitmap_len; i++) {

         pthread_mutex_lock(&lock);

        if( (check_bitmap(vp_bitmap, i) == 0) )
            counter++;  
        else  // Not free and still looking so restart
            counter = 0;
        

        if(counter == num_pages) {
            index = (i + 1) - counter; // We have found x continguous pages  
            
            pthread_mutex_unlock(&lock);
            return (void*) index; 
        }
    }

    // If we didn't find any contiguous free pages 
    return  NULL;  
}

// TODO: Thread-safe implementation
/* Function responsible for allocating pages
and used by the benchmark
*/
void * a_malloc(unsigned int num_bytes) {
    /* 
     * HINT: If the physical memory is not yet initialized, then allocate and initialize.
     */
    void * va;
    if(physical_memory == NULL)
        set_physical_mem();
   /* 
    * HINT: If the page directory is not initialized, then initialize the
    * page directory. Next, using get_next_avail(), check if there are free pages. If
    * free pages are available, set the bitmaps and map a new page. Note, you will 
    * have to mark which physical pages are used. 
    */
    int pages_requested = num_bytes / PGSIZE;
    if(num_bytes % PGSIZE != 0)
        pages_requested++;
   
    void * vpa = get_next_avail(pages_requested); //virtual page address

    if( vpa == NULL )
        return NULL;


    int i = 0;
    for(i; i < pages_requested; i++)
    {
        int j = 0;
         va = (unsigned int)vpa << pageOffsetLen;// virtual address (with offset set to 0's)
        
        for(j; j < pp_bitmap_len; j++)

       page_map(*page_dir, va, pa);

      //  printf("page_map: %d\n", page_map(*page_dir, va, pa));
    }

    return (void*) va;
}


 // TODO: Thread-safe implementation
/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void a_free(void *va, int size) {

    /* Part 1: Free the page table entries starting from this virtual address
     * (va). Also mark the pages free in the bitmap. Perform free only if the 
     * memory from "va" to va+size is valid.
     *
     * Part 2: Also, remove the translation from the TLB
     */
    int pgs = size / PGSIZE;
    if (size % PGSIZE != 0)
        pgs++;

  

     unsigned int offset = (2^pageOffsetLen - 1) | (unsigned int) va;
    unsigned int pdeIndex = (2^pdeOffsetLen - 1) | (unsigned int) ((unsigned int)va>>pdeOffsetLen);
    unsigned int pteIndex = (2^pteOffsetLen - 1) | (unsigned int) ((unsigned int)va>>(pdeOffsetLen + pdeOffsetLen));

    void* pa = translate(*page_dir, va);
    // Just change physical bitmap? Set the bit to 0
    pde_t * mapped = page_dir[pdeIndex];

    pthread_mutex_lock(&lock);
    set_bitmap(pp_bitmap, (unsigned int) pa, 0); // Free page in the physical address. 
     pthread_mutex_unlock(&lock);
    int i; 
    int x;

    for(i = (unsigned int)va; i < pgs; i++)  { // Find where the address starts first. 
        if ( check_bitmap(vp_bitmap, i) == 1 )    {
            pthread_mutex_lock(&lock);
            set_bitmap(vp_bitmap, i, 0); // Frees the pages in the virtual bitmap.

            // Removing from TLB 
            for(x = 0; x < TLB_ENTRIES; x++)    {
                if ( (unsigned int) tlb_stored[x].v_tag == (unsigned int) va )    {
                        tlb_stored[x].v_tag = NULL; 
                        tlb_stored[x].p_tag = NULL;
                }
            }

             pthread_mutex_unlock(&lock);
        }   else if ( vp_bitmap[i] == 0 )   {
            return;
        }
    } 
    
    return;
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void put_value(void *va, void *val, int size) {

    /* HINT: Using the virtual address and translate(), find the physical page. Copy
     * the contents of "val" to a physical page. NOTE: The "size" value can be larger 
     * than one page. Therefore, you may have to find multiple pages using translate()
     * function.
     */
    pthread_mutex_lock(&lock); 
    int pageOffsetLen = (int) log2((double) PGSIZE);
    unsigned int offset = (2^pageOffsetLen - 1) | (unsigned int) va;

    pa = translate(*page_dir, va);
  //  printf("pa: %X\n", pa);
    while (size != 0)
    {
      //  &va = &val; // Currently an lvalue error, but makes sense. 
      // Taking the address of the value and giving it to the address of the virtual address
        size --;
        va++;
        offset = (2^pageOffsetLen - 1) | (unsigned int) va;
        val++;

        // If at a new page offset, translate next page
        if (offset == 0)
            pa = translate(*page_dir, va);
        else
            pa++; //Still in same page, can increment
    }
    pthread_mutex_unlock(&lock); 
    return;
}


/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    * "val" address. Assume you can access "val" directly by derefencing them.
    */

    pthread_mutex_lock(&lock); 
    int pageOffsetLen = (int) log2((double) PGSIZE);
    unsigned int offset = (2^pageOffsetLen - 1) | (unsigned int) va;
    
    pa = translate(*page_dir, va);

    while(size != 0)
    {
      //  &val = &va; // Currently an error, but makes sense. 
      // Taking the address of the virtual address and getting its value. 
        size --;
        va++;
        offset = (2^pageOffsetLen - 1) | (unsigned int) va;
        val++;

        // If at a new page offset, translate next page
        if (offset == 0)
            pa = translate(*page_dir, va);
        else
            pa++; //Still in same page, can increment
    }
    pthread_mutex_unlock(&lock); 
    return;
}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
     * matrix accessed. Similar to the code in test.c, you will use get_value() to
     * load each element and perform multiplication. Take a look at test.c! In addition to 
     * getting the values from two matrices, you will perform multiplication and 
     * store the result to the "answer array"
     */
    int y, z;
    int sum;
    int i; 
    int j;

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            address_mat1 = (unsigned int)mat1 + ((i * size * sizeof(int))) + (j * sizeof(int));
            address_mat2 = (unsigned int)mat2 + ((i * size * sizeof(int))) + (j * sizeof(int));
            address_answer = (unsigned int)answer + ((i * size * sizeof(int))) + (j * sizeof(int)); // size or PGSIZE?
            get_value((void *)address_mat1, &y, sizeof(int));
            get_value((void *)address_mat2, &z, sizeof(int));
            
            printf("%d ", y);
        }
        printf("\n");
    }

    print_TLB_missrate();
       
}



