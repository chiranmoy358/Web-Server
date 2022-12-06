#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "hserver.h"

char *get_file_path(char *);
HTTP_Request *get_request_struct(char *);

HTTP_Response *handle_request(char *request_str)
{
    HTTP_Request *request = get_request_struct(request_str);

    HTTP_Response *response = (HTTP_Response *)malloc(sizeof(HTTP_Response));
    response->HTTP_version = "1.0";

    if (strcmp(request->method, "GET") != 0)
    {
        fprintf(stderr, "BAD REQUEST");
        response->status_code = "405";
        response->status_text = "Method Not Allowed";
        response->content_length = "0";
        response->content_type = "text/html";
        response->body = "";
        return response;
    }

    if (strstr(request->url, "favicon.ico"))
    {
        response->status_code = "404";
        response->status_text = "Not Found";
        response->content_length = "0";
        response->content_type = "image/jpeg";
        response->body = "";
        return response;
    }

    response->content_type = "text/html";

    char *file_path = get_file_path(request->url);
    struct stat file_stat;

    if (file_path != NULL)
    {
        stat(file_path, &file_stat);
        response->status_code = "200";
        response->status_text = "OK";
    }
    else
    {
        file_path = "html_files/404.html";
        stat(file_path, &file_stat);
        response->status_code = "404";
        response->status_text = "Not Found";
    }

    int content_length = (int)file_stat.st_size;

    sprintf(response->content_length = malloc(sizeof(char) * 16), "%d", content_length);

    char buffer[512], *content = (char *)malloc(content_length);
    bzero(content, content_length);

    FILE *file = fopen(file_path, "r");
    while (fgets(buffer, 512, file))
    {
        strcat(content, buffer);
    }

    response->body = content;

    return response;
}

HTTP_Request *get_request_struct(char *request_msg)
{
    HTTP_Request *request = (HTTP_Request *)malloc(sizeof(HTTP_Request));

    request->method = strtok(request_msg, " ");
    request->url = strtok(NULL, " ");
    request->HTTP_version = "1.0";

    printf("%s\n", request->url);

    return request;
}

char *get_file_path(char *url)
{
    char *path = (char *)malloc(sizeof(char) * 128);
    sprintf(path, "./html_files%s", url);

    struct stat path_stat;
    if (stat(path, &path_stat) != 0)
    {
        // URL is Invalid
        free(path);
        return NULL;
    }

    // URL is valid
    if (S_ISDIR(path_stat.st_mode))
        strcat(path, "/index.html");

    return path;
}

char *get_string(HTTP_Response *response)
{
    int total_length = strlen(RESPONSE_TEMPLATE) + strlen(response->HTTP_version) + strlen(response->status_code) + strlen(response->status_text) + strlen(response->content_length) + strlen(response->content_type) + strlen(response->body);

    char *response_str = (char *)malloc(total_length);

    sprintf(response_str, RESPONSE_TEMPLATE, response->HTTP_version, response->status_code, response->status_text, response->content_type, response->content_length, response->body);

    return response_str;
}

void cleanup(HTTP_Response *response)
{
    if (response->content_length[0] != '0')
    {
        free(response->content_length);
        free(response->body);
    }

    free(response);
}
