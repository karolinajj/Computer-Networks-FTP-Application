#include <stdio.h>
#include <string.h>
#include "ftp.h"
#include <stdlib.h>
#define MIN_LENGTH 8
#define MAX_URL_LENGTH 600
char *createURL()
{
    char *url = (char *)malloc(MAX_URL_LENGTH * sizeof(char));
    if (url == NULL)
    {
        printf("URL creation: Memory allocation failed! \n");
        return NULL;
    }
    char bufferU[100];
    bufferU[99] = '\0';
    char bufferP[100];
    bufferP[99] = '\0';
    char bufferH[100];
    bufferH[99] = '\0';
    char bufferF[200];
    bufferF[199] = '\0';

    printf("Username: ");
    if (scanf("%99s", bufferU) != 1)
    {
        printf("URL creation: Error reading username!\n");
        return NULL; 
    }
    printf("Password: ");
    if (scanf("%99s", bufferP) != 1)
    {
        printf("URL creation: Error reading password!\n");
        return NULL;
    }
    printf("Host: ");
    if (scanf("%99s", bufferH) != 1)
    {
        printf("URL creation: Error reading host!\n");
        return NULL;
    }

    printf("File: ");
    if (scanf("%199s", bufferF) != 1)
    {
        printf("URL creation: Error reading file!\n");
        return NULL;
    }

    sprintf(url, "ftp://%s:%s@%s/%s", bufferU, bufferP, bufferH, bufferF);
    return url;
}

int main(int argc, char *argv[])
{
    char *url;
    if (argc < 2)
    {
        url = createURL();
        if(!url){
            printf("Error creating URL!\n");
            return 1;
        }
        printf("Created url: %s.\n",url);
    }
    else
    {
        printf("Supplied URL found!\n");
        url = argv[1];
    }
    if (strlen(url) < MIN_LENGTH)
    {
        printf("Usage: ./<executable name> <URL> [<local filename>] or ./<executable name>\n");
        return -1;
    }

    printf("Argument verification successful!\n");

    if (ftp_init(url))
    {
        printf("Error: Couldn't init FTP layer!\n");
        return -1;
    }

    if (argc == 3)
    {
        printf("Local file name identified. Saving result to file %s",argv[2]);
        if (ftp_download_file(argv[2]))
        {
            printf("Error: couldn't download file!\n");
            return -1;
        }
    }
    else
    {
        printf("Saving result to file download");
        if (ftp_download_file("download"))
        {
            printf("Error: couldn't download file!\n");
            return -1;
        }
    }

    if (ftp_close())
    {
        printf("Error: couldn't close connection!\n");
        return -1;
    }
    if (argc < 2)
    {
        printf("Freeing URL memory!\n");
        free(url);
    }
    printf("Terminating successfully!\n");
    return 0;
}