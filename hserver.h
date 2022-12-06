#define RESPONSE_TEMPLATE "HTTP/%s %s %s\nContent-Type: %s\nContent-Length: %s\nConnection: close\n\n%s"

struct HTTP_Request
{
  char *HTTP_version;
  char *method;
  char *url;
};

struct HTTP_Response
{
  char *HTTP_version;
  char *status_code;
  char *status_text;
  char *content_type;
  char *content_length;
  char *body;
};

typedef struct HTTP_Request HTTP_Request;
typedef struct HTTP_Response HTTP_Response;

// Returns the string representation of the HTTP Response
char *get_string(HTTP_Response *);

// Get the response structure
HTTP_Response *handle_request(char *);

void cleanup(HTTP_Response *);