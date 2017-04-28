/* 
Authors: Eyhao-mjoconnell2 
Strategy:
1) Parse command line arguments
2) Build Cache
3) Parse trace file
4) Operate on cache per line
5) Output final results
*/

#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


// line structure contains a flag indicating validity, set index, and  tag
typedef struct {
	int valid; // flag indicating if line is valid
	unsigned long setIndex; // index of set where this line is stored
	unsigned long tag; // modified address
	int lruIndex;
} line;

// cache_set structure contains an array of line and the LRU index
typedef struct {
	line** lines;
} cache_set;

// cache structure contains an array of cache_set
typedef struct {
	cache_set** sets;
} cache;

void printUsage(void);
cache buildCache(int setIndexBits, int associativity, int numSets);
cache readTrace(FILE *fpTrace, int isVerbose, int setIndexBits, int associativity, int blockBits);
void setLine(line *myline, unsigned long address, int setIndexBits, int blockBits, int *currentIndex);
void clearCache(cache mycache, int numSets);
line* emptyLine(cache mycache, int associativity, int numSets);
line* getLRU (cache mycache, int associativity, int numSets, int currentIndex);
unsigned long getTag(unsigned long address, int setIndexBits, int blockBits);
line* getLine(cache mycache, int associativity, int numSets, unsigned long tag);
void testcache (cache mycache, int associativity, int numSets);

//Todo run the trace simulation

// notes for operating on cache
// check for valid lines with given tag
// 

int main(int argc, char *argv[])
{
    FILE *fpTrace; // pointer to trace file
    int isVerbose = 0; // default value
    int setIndexBits, associativity, blockBits; // required program arguments

    int hit_count = 0; // default value
    int miss_count = 0; // default value
    int eviction_count = 0; // default value

	int currentIndex = 0; // current index used for tracking LRU

	int numSets;
	cache mycache;
	line *myline;

    if (argc < 9 || argc > 11) {
		printf("Invalid arguments passed too csim\n");
		return 1;
    }
    
    for(int i = 1; i < argc; i++) {
	
		char* argValue = argv[i];
		if (strcmp(argValue,"-h") == 0) {
			printUsage();
		} else if (strcmp(argValue,"-v") == 0) {
			// set flag for displaying trace info
			isVerbose = 1;
		} else if (strcmp(argValue, "-hv") == 0
				|| strcmp(argValue, "-vh") == 0
				|| strcmp(argValue, "[-hv]") == 0
				|| strcmp(argValue, "[-vh]") == 0) {
			printUsage();
			isVerbose = 1;
		} else if (strcmp(argValue, "-s") == 0) {
			if (i < argc) {
				i++;
				argValue = argv[i];
		        if (strcmp(argValue, "0") == 0) {
					setIndexBits = 0;
		            printf("The value given with -s is %d\n", setIndexBits);
		        } else {
					int num = atoi(argValue);
					if (num < 1 || num > 64) { // TODO write better checks! alpha strings result in 0
						printf("Invalid value given with option -s\n");
						return 1;
					} else {
						setIndexBits = num;
				                printf("The value given with -s is %d\n", setIndexBits);
					}
				}
			} else {
				printf("No value given with option -s\n");
				return 1;
			}
		} else if (strcmp(argValue, "-E") == 0) {
			if (i < argc) {
				i++;
				argValue = argv[i];
		        if (strcmp(argValue, "0") == 0) {
			   		associativity = 0;
		            printf("The value given with -E is %d\n", associativity);
		        } else {
					int num = atoi(argValue);
					if (num < 1 || num > 64) { // TODO write better checks! alpha strings result in 0
				    	printf("Invalid value given with option -E\n");
			   			return 1;
					} else {
			    		associativity = num;
	                    printf("The value given with -E is %d\n", associativity);
					}
				}
			} else { 
			  printf("No value given with option -E\n");
			  return 1; 
			}
		} else if (strcmp(argValue, "-b") == 0) {
			if (i < argc) {
				i++;
				argValue = argv[i];
           	 	if (strcmp(argValue, "0") == 0) {
					blockBits = 0;
                	printf("The value given with -b is %d\n", blockBits);
            	} else {
					int num = atoi(argValue);
					if (num < 1 || num > 64) { // TODO write better checks! alpha strings result in 0
						printf("Invalid value given with option -b\n");
						return 1;
					} else {
						blockBits = num;
                    	printf("The value given with -b is %d\n", blockBits);
					}
				}
			} else { 
			  	printf("No value given with option -b\n");
			  	return 1; 
			}
		} else if (strcmp(argValue, "-t") ==0) {
			if (i < argc) {
				i++; 
				argValue = argv[i];
				int len = strlen(argValue);
				if(len < 7) {
				    printf("Invalid file given with option -t\n");
					return 1;
				} else {
					fpTrace = fopen(argValue, "r"); // open trace file for reading
					if (fpTrace == NULL) {
						printf("Failed to open %s for reading.\n", argValue);
						return 1;
					}
				}
			} else {
				printf("No value given with option -t\n");
				return 1;
			}
		}
    }

	numSets = pow(2, setIndexBits); // compute number of sets to create
	mycache = buildCache(setIndexBits, associativity, numSets); // build cache
	
	char operation;
    unsigned long address;
    int size;
    while (fscanf(fpTrace," %c %lx, %d", &operation, &address, &size) > 0) {
		// TODO delete - for testing
		testcache(mycache, associativity, numSets);
		unsigned long tag = getTag(address, setIndexBits, blockBits);
		// TODO search for tag within cache
		/* assuming tag is stored correctly within cache */
		myline = getLine(mycache, associativity, numSets, tag);
		if (myline != NULL) { // check if line with tag was found in cache
			hit_count++;
		} else {
			miss_count++; // tag not found within cache
			// try to get empty line from cache
			myline = emptyLine(mycache, associativity, numSets);
			if (myline != NULL) { // check if empty line was found
				// no empty lines in cache, get least recently used line (evict)
				myline = getLRU (mycache, associativity, numSets, currentIndex);
				eviction_count++;
				if (myline == NULL) {
					printf("oops...\n");
				}
			}
		}
		// at this point myline contains a line from our cache
		// set line
		setLine(myline, address, setIndexBits, blockBits, &currentIndex);
		
		if (isVerbose) {
			printf("%c %lx,%d\n", operation, address, size);
		}
	}

    // readTrace(fpTrace, isVerbose, setIndexBits, associativity, blockBits);
	
	clearCache(mycache, numSets);
    printSummary(hit_count, miss_count, eviction_count);
    fclose(fpTrace);
    return 0;
}

/* Print usage info */
void printUsage(void) {
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options: \n"); 
    printf("-h         Print this help message. \n"); 
    printf("-v         Optional verbose flag. \n"); 
    printf("-s <num>   Number of set index bits. \n"); 
    printf("-E <num>   Number of lines per set. \n"); 
    printf("-b <num>   Number of block offset bits. \n"); 
    printf(" -t <file>  Trace file. \n"); 
}
/*
cache readTrace(FILE *fpTrace, int isVerbose, int setIndexBits, int associativity, int blockBits) {
    char operation;
    unsigned long address;
    int size;

    while (fscanf(fpTrace," %c %lx, %d", &operation, &address, &size) > 0) {



	if (isVerbose) {
	    printf("%c %lx,%d\n", operation, address, size);
	}
	// create new line structure and initialize its fields
        

    }

    return mycache;
}*/
/*build a cache given s, E and n*/
cache buildCache(int setIndexBits, int associativity, int numSets) {
	cache new_cache; // new cache 
	new_cache.sets = (cache_set **) malloc (numSets * sizeof(cache_set*)); 

	for (int setIndex = 0; setIndex < numSets; setIndex++){
		new_cache.sets[setIndex]->lines = (line **) malloc(associativity * sizeof(line*)); 	
		for (int lineIndex = 0; lineIndex < associativity; lineIndex ++){
			line* myline = malloc(sizeof(line));  // lines in new cache 
			myline->valid =0; 
			myline->tag =0; 
			myline->setIndex =0;
			myline->lruIndex = -1; 
			new_cache.sets[setIndex]->lines[lineIndex]= myline; //initiaze all variables in line struct
		}
	}
	
	return new_cache; 

}

/* call free function to clean up cache after main is run*/
void clearCache(cache mycache, int numSets) {
	for (int setIndex = 0; setIndex < numSets; setIndex ++){
		if(mycache.sets[setIndex]->lines !=NULL){
			free(mycache.sets[setIndex]->lines); //free line 
		}

	}
	if(mycache.sets !=NULL){
		free(mycache.sets); //free sets 
	}

}

/* find an empty line in a set by checking the valid, 0 is empty, 1 is filled*/
line* emptyLine(cache mycache, int associativity, int numSets){
	line* myline = NULL; // line with lru index
	for (int setIndex = 0; setIndex < numSets; setIndex++){
		for (int lineIndex = 0; lineIndex < associativity; lineIndex ++){
			if (mycache.sets[setIndex]->lines[lineIndex]->valid==0) { // check if the line is empty
				myline = mycache.sets[setIndex]->lines[lineIndex];
			}
		}
	} 
	return myline;
}

line* getLRU (cache mycache, int associativity, int numSets, int currentIndex) {
	int lru = currentIndex; // initialized to most recently used index
	line* lruLine = NULL; // line with lru index
	for (int setIndex = 0; setIndex < numSets; setIndex++){
		for (int lineIndex = 0; lineIndex < associativity; lineIndex ++){
			line* currentline = mycache.sets[setIndex]->lines[lineIndex];
			if (currentline->lruIndex < lru) { // less recently used index found
				lru = currentline->lruIndex;
				lruLine = currentline;
			}
		}
	} 
	return lruLine;
}

/* 
Get line from cache if line with specified tag is present
Otherwise, return empty line
*/
line* getLine(cache mycache, int associativity, int numSets, unsigned long tag) {
	for (int setIndex = 0; setIndex < numSets; setIndex++) {
		for (int lineIndex = 0; lineIndex < associativity; lineIndex ++){
			line* currentline = mycache.sets[setIndex]->lines[lineIndex];
			if (currentline->tag == tag) {
				return currentline;
			}
		}
	} 
	return NULL; //return NULL if line with specified tag is not found
}

void setLine(line *myline, unsigned long address, int setIndexBits, int blockBits, int *currentIndex) {
	(*currentIndex)++;    
	myline->valid = 1;
	myline->tag = getTag(address, setIndexBits, blockBits);
	myline->setIndex = (address << myline->tag) >> (myline->tag + blockBits);    
	myline->lruIndex = *currentIndex;
}

unsigned long getTag(unsigned long address, int setIndexBits, int blockBits) {
	return (address >> (setIndexBits + blockBits));
}

void testcache (cache mycache, int associativity, int numSets) {
	printf("\t set index:\tline index\tvalid:\tset index\ttag\tlruIndex\n");
	for (int setIndex = 0; setIndex < numSets; setIndex++) {
		for (int lineIndex = 0; lineIndex < associativity; lineIndex ++) {
			line* currentline = mycache.sets[setIndex]->lines[lineIndex];
			printf("\t%d\t\t%d\t\t%d\t%lx\t\t%lx\t%d\n", setIndex, lineIndex,
				currentline->valid, currentline->setIndex, currentline->tag, currentline->lruIndex);
		}
	}
	return;
}















