#include "config-parse.h"

void parseConfig(char *filename){

	char type[256],
		name[8],
		value[256];

	FILE *fp;
	int n_read;

	if ( ( fp = fopen( filename, "r" ) ) == NULL ) {
		perror("fopen");
		
	 	return;
	 }

	printf("Parsing config file: %s\n", filename);
	int media_index = 0;
    int host_index = 0;
		
	while( !feof( fp ) && !ferror( fp ) ){

		n_read = fscanf( fp, "%s%s%s", type, name, value);

        if(strcmp(type, "host")){
            strcpy(hosts[host_index].name, name);
            strcpy(hosts[host_index++].value, value);
        }
        else if(strcmp(type, "media")){
            strcpy(media[media_index].type, name);
            strcpy(media[media_index++].value, value);
        }
	}


	number_of_conf_entries = media_index + host_index;
    number_of_host_entries = host_index;
    number_of_media_entries = media_index;

    printf("found %d number of entries in the config file \n\n", number_of_conf_entries);

    //TODO: only print if verbose mode is on...
	printConfigOptions();

}

void printConfigOptions(){
    //print media entries
	for(int i = 0; i < number_of_media_entries; i++){
		printf("%s = %s \n", media[i].type, media[i].value);
	}

    for(int i = 0; i < number_of_host_entries; i++){
        printf("%s = %s \n", hosts[i].name, hosts[i].value);
    }
}

